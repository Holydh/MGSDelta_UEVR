#include "PerformanceMonitor.hpp"

#include <pdh.h>
#include <pdhmsg.h>
#include <tlhelp32.h>

#include <algorithm>
#include <optional>
#include <thread>
#include <unordered_map>

#include "Globals.hpp"
#include "uevr/API.hpp"
#include "ue/Types.hpp"

#pragma comment(lib, "pdh.lib")

namespace {

int64_t qpc()
{
	LARGE_INTEGER value;
	QueryPerformanceCounter(&value);
	return value.QuadPart;
}

// GPU Engine and GPU Process Memory counters, per process and per GPU
class GpuCounters {
public:
	explicit GpuCounters(DWORD pid) : pidTag(L"pid_" + std::to_wstring(pid) + L"_")
	{
		if (PdhOpenQueryW(nullptr, 0, &query) != ERROR_SUCCESS)
		{
			query = nullptr;
			return;
		}
		PdhAddEnglishCounterW(query, L"\\GPU Engine(*)\\Utilization Percentage", 0, &engine);
		PdhAddEnglishCounterW(query, L"\\GPU Process Memory(*)\\Dedicated Usage", 0, &memory);
		PdhCollectQueryData(query); // utilization is computed between two collections
	}

	void Collect(PerformanceMonitor::Snapshot& s)
	{
		if (query == nullptr || PdhCollectQueryData(query) != ERROR_SUCCESS)
			return;

		// Instances look like pid_1234_luid_0x..._0x..._phys_0_eng_0_engtype_3D. Like Task Manager, a
		// GPU's usage is its busiest engine (3D, Graphics_1, Compute_0...), summed over all processes.
		struct Engine {
			std::wstring type;
			double total = 0.0;
			double game = 0.0;
		};
		std::unordered_map<std::wstring, std::unordered_map<std::wstring, Engine>> engines; // luid -> engine -> usage
		std::unordered_map<std::wstring, double> gameByGpu;
		forEachInstance(engine, [&](const wchar_t* instance, double value) {
			const wchar_t* luid = wcsstr(instance, L"luid_");
			const wchar_t* luidEnd = luid ? wcsstr(luid, L"_phys") : nullptr;
			const wchar_t* eng = luidEnd ? wcsstr(luidEnd, L"_eng_") : nullptr;
			const wchar_t* type = eng ? wcsstr(eng, L"_engtype_") : nullptr;
			if (type == nullptr)
				return;
			const std::wstring gpu{ luid, luidEnd };
			auto& e = engines[gpu][std::wstring{ eng, type }];
			e.type = type + wcslen(L"_engtype_");
			e.total += value;
			if (wcsstr(instance, pidTag.c_str()) != nullptr)
			{
				e.game += value;
				gameByGpu[gpu] += value;
			}
		});
		// The GPU the game uses most
		const auto gpu = std::max_element(gameByGpu.begin(), gameByGpu.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
		if (gpu == gameByGpu.end())
			return;
		std::vector<const Engine*> sorted;
		for (const auto& [id, e] : engines[gpu->first])
		{
			s.gpuPercent = (std::max)(s.gpuPercent, (float)(std::min)(100.0, e.total));
			s.gpuGamePercent = (std::max)(s.gpuGamePercent, (float)(std::min)(100.0, e.game));
			if (e.game >= 1.0)
				sorted.push_back(&e);
		}
		std::sort(sorted.begin(), sorted.end(), [](const Engine* a, const Engine* b) { return a->game > b->game; });
		for (size_t i = 0; i < sorted.size() && i < 4; ++i)
		{
			char text[64];
			snprintf(text, sizeof(text), "%s%s %.0f%%", i > 0 ? ", " : "", ue::narrow(sorted[i]->type).c_str(), sorted[i]->game);
			s.gpuGameEngines += text;
		}

		double vramBytes = -1.0;
		forEachInstance(memory, [&](const wchar_t* instance, double value) {
			if (wcsstr(instance, pidTag.c_str()) != nullptr)
				vramBytes = (vramBytes < 0.0 ? 0.0 : vramBytes) + value;
		});
		if (vramBytes >= 0.0)
			s.vramMB = (float)(vramBytes / (1024.0 * 1024.0));
	}

private:
	PDH_HQUERY query = nullptr;
	PDH_HCOUNTER engine = nullptr;
	PDH_HCOUNTER memory = nullptr;
	std::wstring pidTag;

	template <typename F>
	static void forEachInstance(PDH_HCOUNTER counter, F&& callback)
	{
		if (counter == nullptr)
			return;
		DWORD size = 0;
		DWORD count = 0;
		if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &size, &count, nullptr) != PDH_MORE_DATA)
			return;
		std::vector<uint8_t> buffer(size);
		auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
		if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &size, &count, items) != ERROR_SUCCESS)
			return;
		for (DWORD i = 0; i < count; ++i)
		{
			const auto status = items[i].FmtValue.CStatus;
			if (status == PDH_CSTATUS_VALID_DATA || status == PDH_CSTATUS_NEW_DATA)
				callback(items[i].szName, items[i].FmtValue.doubleValue);
		}
	}
};

// CPU time of every thread of the game, named with the names Unreal gives them (RenderThread, RHIThread...)
class ThreadCounters {
public:
	std::vector<PerformanceMonitor::ThreadUsage> Sample(double windowSeconds, DWORD gameThreadId)
	{
		std::vector<PerformanceMonitor::ThreadUsage> usages;
		const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if (snapshot == INVALID_HANDLE_VALUE)
			return usages;

		for (auto& [id, thread] : threads)
			thread.seen = false;

		const DWORD pid = GetCurrentProcessId();
		THREADENTRY32 entry{ sizeof(entry) };
		for (BOOL ok = Thread32First(snapshot, &entry); ok; ok = Thread32Next(snapshot, &entry))
		{
			if (entry.th32OwnerProcessID != pid)
				continue;
			auto [it, inserted] = threads.try_emplace(entry.th32ThreadID);
			auto& thread = it->second;
			thread.seen = true;
			if (inserted)
			{
				thread.handle = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ThreadID);
				thread.lastCpu = cpuTime(thread.handle);
				continue; // no usage until the next sample
			}
			if (thread.name.empty())
				thread.name = description(thread.handle);

			const uint64_t cpu = cpuTime(thread.handle);
			const float percent = (float)((cpu - thread.lastCpu) / (windowSeconds * 1e7) * 100.0); // 100 ns units
			thread.lastCpu = cpu;

			std::string name = thread.name.empty() ? "Thread " + std::to_string(entry.th32ThreadID) : thread.name;
			if (entry.th32ThreadID == gameThreadId && name.find("GameThread") == std::string::npos)
				name = "GameThread (" + name + ")";
			usages.push_back({ std::move(name), percent });
		}
		CloseHandle(snapshot);

		for (auto it = threads.begin(); it != threads.end();)
		{
			if (it->second.seen)
			{
				++it;
				continue;
			}
			if (it->second.handle != nullptr)
				CloseHandle(it->second.handle);
			it = threads.erase(it);
		}

		std::sort(usages.begin(), usages.end(), [](const auto& a, const auto& b) { return a.cpuPercent > b.cpuPercent; });
		if (usages.size() > 6)
			usages.resize(6);
		return usages;
	}

private:
	struct Thread {
		HANDLE handle = nullptr;
		std::string name;
		uint64_t lastCpu = 0;
		bool seen = false;
	};
	std::unordered_map<DWORD, Thread> threads;

	static uint64_t cpuTime(HANDLE handle)
	{
		FILETIME creation, exit, kernel, user;
		if (handle == nullptr || !GetThreadTimes(handle, &creation, &exit, &kernel, &user))
			return 0;
		return ((uint64_t)kernel.dwHighDateTime << 32 | kernel.dwLowDateTime) + ((uint64_t)user.dwHighDateTime << 32 | user.dwLowDateTime);
	}

	static std::string description(HANDLE handle)
	{
		using GetThreadDescriptionFn = HRESULT(WINAPI*)(HANDLE, PWSTR*);
		static const auto getThreadDescription = reinterpret_cast<GetThreadDescriptionFn>(
			GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetThreadDescription"));
		PWSTR text = nullptr;
		if (handle == nullptr || getThreadDescription == nullptr || FAILED(getThreadDescription(handle, &text)))
			return {};
		std::string name = ue::narrow(text);
		LocalFree(text);
		return name;
	}
};

void appendValue(std::string& line, const char* format, float value)
{
	char text[32];
	if (value < 0.0f)
		snprintf(text, sizeof(text), "n/a");
	else
		snprintf(text, sizeof(text), format, value);
	line += text;
}

} // namespace

void PerformanceMonitor::Start()
{
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	qpcToMs = 1000.0 / (double)frequency.QuadPart;
	LogToFile = g_DebugEnabled;
	// Detached: Windows ends it with the process (the plugin is never unloaded before that)
	std::thread(&PerformanceMonitor::samplingLoop, this).detach();
}

void PerformanceMonitor::OnPreEngineTickStart()
{
	if (!Enabled)
	{
		lastFrameStart = 0;
		tickStart = 0;
		return;
	}
	gameThreadId = GetCurrentThreadId();
	const int64_t now = qpc();
	if (lastFrameStart != 0)
	{
		const double frameMs = (now - lastFrameStart) * qpcToMs;
		std::scoped_lock lock{ totalsMutex };
		totals.frames++;
		totals.frameSum += frameMs;
		totals.frameMax = (std::max)(totals.frameMax, frameMs);
	}
	lastFrameStart = now;
	tickStart = now;
}

void PerformanceMonitor::OnPreEngineTickEnd()
{
	if (tickStart != 0)
		pluginTicks = qpc() - tickStart;
}

void PerformanceMonitor::OnPostEngineTick()
{
	if (tickStart == 0)
		return;
	const double tickMs = (qpc() - tickStart) * qpcToMs;
	std::scoped_lock lock{ totalsMutex };
	totals.ticks++;
	totals.tickSum += tickMs;
	totals.tickMax = (std::max)(totals.tickMax, tickMs);
	totals.pluginSum += pluginTicks * qpcToMs;
}

PerformanceMonitor::Snapshot PerformanceMonitor::GetSnapshot()
{
	std::scoped_lock lock{ snapshotMutex };
	return latest;
}

void PerformanceMonitor::samplingLoop()
{
	std::optional<GpuCounters> gpu; // opened when first enabled
	ThreadCounters threads;
	bool wasEnabled = false;
	int64_t windowStart = 0;

	while (true)
	{
		Sleep(wasEnabled ? 2000 : 500);
		if (!Enabled)
		{
			wasEnabled = false;
			continue;
		}
		if (!wasEnabled)
		{
			// Baseline sample: usages are measured between two samples
			wasEnabled = true;
			if (gpu)
			{
				Snapshot ignored;
				gpu->Collect(ignored);
			}
			else
				gpu.emplace(GetCurrentProcessId());
			threads.Sample(1.0, 0);
			windowStart = qpc();
			{
				std::scoped_lock lock{ totalsMutex };
				totals = {};
			}
			std::scoped_lock lock{ snapshotMutex };
			latest = {};
			continue;
		}

		const int64_t now = qpc();
		const double windowSeconds = (now - windowStart) * qpcToMs / 1000.0;
		windowStart = now;

		Snapshot s;
		FrameTotals frame;
		{
			std::scoped_lock lock{ totalsMutex };
			frame = totals;
			totals = {};
		}
		if (frame.frames > 0)
		{
			s.fps = (float)(frame.frames / windowSeconds);
			s.frameMs = (float)(frame.frameSum / frame.frames);
			s.frameMaxMs = (float)frame.frameMax;
		}
		if (frame.ticks > 0)
		{
			s.gameTickMs = (float)(frame.tickSum / frame.ticks);
			s.gameTickMaxMs = (float)frame.tickMax;
			s.pluginMs = (float)(frame.pluginSum / frame.ticks);
		}
		gpu->Collect(s);
		s.busiestThreads = threads.Sample(windowSeconds, gameThreadId);
		s.valid = true;

		if (LogToFile)
		{
			char text[256];
			snprintf(text, sizeof(text), "[MGSDelta] Perf: %.0f fps | frame %.1f ms (max %.1f) | game thread tick %.1f ms (max %.1f, plugin %.2f) | GPU ",
				s.fps, s.frameMs, s.frameMaxMs, s.gameTickMs, s.gameTickMaxMs, s.pluginMs);
			std::string line = text;
			appendValue(line, "%.0f%%", s.gpuPercent);
			line += " (game ";
			appendValue(line, "%.0f%%", s.gpuGamePercent);
			if (!s.gpuGameEngines.empty())
				line += ": " + s.gpuGameEngines;
			line += ") | VRAM ";
			appendValue(line, "%.0f MB", s.vramMB);
			line += " | threads (% of one core):";
			for (const auto& t : s.busiestThreads)
			{
				snprintf(text, sizeof(text), " %s %.0f%%,", t.name.c_str(), t.cpuPercent);
				line += text;
			}
			uevr::API::get()->log_info("%s", line.c_str());
		}

		std::scoped_lock lock{ snapshotMutex };
		latest = std::move(s);
	}
}
