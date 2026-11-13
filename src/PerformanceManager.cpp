#include "PerformanceManager.hpp"

#include <cmath>
#include <fstream>

#include "Logger.hpp"
#include "ue/Types.hpp"

PerformanceManager::PerformanceManager()
{
	// group, cvar, tooltip, isInt, min, max, value forced when enabled, enabled by default
	Overrides = {
		{ "Anti-aliasing", L"r.AntiAliasingMethod",
			"0 = none, 1 = FXAA, 2 = TAA, 4 = TSR (3 = MSAA, unsupported here).\nTSR works at the headset's full output resolution, which is huge in VR", true, 0.0f, 4.0f, 4.0f, false },

		{ "Level of detail", L"r.Nanite.MaxPixelsPerEdge",
			"Most of the world is made of Nanite meshes, which ignore the LOD settings below.\nHigher = bigger triangles = less GPU work. Game default: 1", false, 1.0f, 8.0f, 3.0f, false },
		{ "Level of detail", L"r.StaticMeshLODDistanceScale",
			"Non-Nanite static meshes switch to their lower LODs sooner.\n2 = at half the usual distance. Game default: 1", false, 1.0f, 4.0f, 2.0f, true },
		{ "Level of detail", L"r.SkeletalMeshLODBias",
			"Characters and other animated meshes skip their most detailed LODs.\nAlso affects the VR hands and held weapons. Game default: 0", true, 0.0f, 3.0f, 0.0f, false },
		{ "Level of detail", L"foliage.LODDistanceScale",
			"Foliage switches to its lower LODs sooner when lower. Game default: 1", false, 0.1f, 2.0f, 1.0f, false },
		{ "Level of detail", L"r.ViewDistanceScale",
			"Objects with a cull distance disappear sooner when lower. Game default: 1", false, 0.2f, 1.5f, 0.5f, false },

		{ "Global illumination (Lumen)", L"r.Lumen.DiffuseIndirect.Allow",
			"0 turns Lumen global illumination off: biggest GPU saving,\nbut changes the lighting a lot (darker interiors)", true, 0.0f, 1.0f, 1.0f, true },
		{ "Global illumination (Lumen)", L"r.Lumen.ScreenProbeGather.DownsampleFactor",
			"Lumen computes lighting at fewer points on screen.\nHigher = cheaper, blurrier indirect lighting. Game default: 16", true, 16.0f, 64.0f, 16.0f, true },
		{ "Global illumination (Lumen)", L"r.Lumen.ScreenProbeGather.ShortRangeAO",
			"Lumen short range ambient occlusion. 0 = off. The game also changes it per level", true, 0.0f, 1.0f, 0.0f, true },
		{ "Global illumination (Lumen)", L"r.Lumen.ScreenProbeGather.ScreenTraces",
			"Lumen screen-space traces. Each eye traces its own image, so foliage\ncan be lit differently per eye. 0 = off (eyes match better). Game default: 1", true, 0.0f, 1.0f, 0.0f, true },

		{ "Shadows", L"r.Shadow.CSM.MaxCascades",
			"Number of sun shadow cascades. Fewer = cheaper, blurrier distant shadows", true, 1.0f, 10.0f, 1.0f, true },
		{ "Shadows", L"r.Shadow.DistanceScale",
			"Sun shadows end sooner when lower. Default: 1", false, 0.1f, 2.0f, 2.0f, false },
		{ "Shadows", L"r.Shadow.MaxResolution",
			"Maximum shadow map resolution", true, 256.0f, 4096.0f, 256.0f, true },

		{ "Effects", L"r.SeparateTranslucencyScreenPercentage",
			"Resolution of translucent effects (smoke, fire, water).\nThe game also changes it per level", false, 25.0f, 100.0f, 25.0f, false },
		{ "Effects", L"r.VolumetricFog",
			"0 turns volumetric fog off", true, 0.0f, 1.0f, 0.0f, false },
	};
}

void PerformanceManager::Init()
{
	configFilePath = (uevr::API::get()->get_persistent_dir() / "MGSDelta_performance.txt").string();
	load();
}

void PerformanceManager::Update(float deltaTime)
{
	checkTimer -= deltaTime;
	if (!applyNow && checkTimer > 0.0f)
		return;
	applyNow = false;
	checkTimer = 1.0f;
	for (auto& o : Overrides)
		apply(o);
}

void PerformanceManager::RequestApply()
{
	applyNow = true;
}

void PerformanceManager::apply(CVarOverride& o)
{
	if (o.missing || (!o.enabled && !o.hasOriginal))
		return;
	if (o.cvar == nullptr)
	{
		const auto consoleManager = uevr::API::get()->get_console_manager();
		if (consoleManager == nullptr)
			return;
		o.cvar = consoleManager->find_variable(o.name);
		if (o.cvar == nullptr)
		{
			o.missing = true;
			uevr::API::get()->log_error("[MGSDelta] Console variable not found: %ls", o.name);
			return;
		}
	}

	// UEVR's set() uses SetByHotfix (0x08000000) in this engine build: UE silently ignores it on cvars
	// the game set with Code/Console priority (e.g. ScreenTraces). SetByConsole is the highest.
	constexpr uint32_t SetByConsole = 0x0B000000;

	const float current = o.isInt ? (float)o.cvar->get_int() : o.cvar->get_float();
	if (!o.enabled)
	{
		if (o.isInt) o.cvar->set_ex(std::to_wstring((int)std::lround(o.original)), SetByConsole);
		else o.cvar->set_ex(std::to_wstring(o.original), SetByConsole);
		o.hasOriginal = false;
		Logger::DebugPrint("%ls restored to %g", o.name, o.original);
		return;
	}

	if (!o.hasOriginal)
	{
		o.original = current;
		o.hasOriginal = true;
	}
	const float target = o.isInt ? std::round(o.value) : o.value;
	if (std::fabs(current - target) > 0.001f)
	{
		if (o.isInt) o.cvar->set_ex(std::to_wstring((int)target), SetByConsole);
		else o.cvar->set_ex(std::to_wstring(target), SetByConsole);
		Logger::DebugPrint("%ls set to %g (was %g)", o.name, target, current);
	}
}

void PerformanceManager::load()
{
	std::ifstream file(configFilePath);
	if (!file.is_open())
		return; // created on the first change made in the menu

	std::string line;
	while (std::getline(file, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		const size_t equalPos = line.find('=');
		const size_t commaPos = line.find(',', equalPos);
		if (line.empty() || line[0] == '#' || equalPos == std::string::npos || commaPos == std::string::npos)
			continue;
		const std::string name = line.substr(0, equalPos);
		for (auto& o : Overrides)
		{
			if (ue::narrow(o.name) != name)
				continue;
			o.enabled = line.substr(equalPos + 1, commaPos - equalPos - 1) == "true";
			const float value = std::strtof(line.c_str() + commaPos + 1, nullptr);
			o.value = value < o.min ? o.min : (value > o.max ? o.max : value);
		}
	}
}

void PerformanceManager::Save()
{
	std::ofstream file(configFilePath, std::ios::trunc);
	if (!file.is_open())
	{
		Logger::DebugPrint("Failed to write %s", configFilePath.c_str());
		return;
	}
	file << "# MGSDelta Plugin performance settings, saved from the plugin menu (Performance section).\n"
		"# cvar=enabled,value\n";
	for (const auto& o : Overrides)
		file << ue::narrow(o.name) << '=' << (o.enabled ? "true" : "false") << ',' << o.value << '\n';
}
