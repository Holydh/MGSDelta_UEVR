#pragma once
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

// Frame timings measured from the engine tick callbacks, plus GPU usage (the Windows counters
// Task Manager uses) and per-thread CPU usage, sampled on a background thread every 2 seconds.
class PerformanceMonitor {
public:
	struct ThreadUsage {
		std::string name;
		float cpuPercent; // of one core
	};
	struct Snapshot {
		bool valid = false;
		float fps = 0.0f;
		float frameMs = 0.0f;
		float frameMaxMs = 0.0f;
		float gameTickMs = 0.0f; // plugin pre engine tick + engine tick, i.e. game thread work
		float gameTickMaxMs = 0.0f;
		float pluginMs = 0.0f; // plugin's own pre engine tick work
		float gpuPercent = -1.0f; // busiest engine of the game's GPU, all processes (-1 = unavailable)
		float gpuGamePercent = -1.0f; // the game's busiest engine
		std::string gpuGameEngines; // the game's busiest engines, e.g. "graphics_1 85%, compute_0 20%"
		float vramMB = -1.0f; // dedicated GPU memory used by the game
		std::vector<ThreadUsage> busiestThreads;
	};

	void Start();
	// Game thread
	void OnPreEngineTickStart();
	void OnPreEngineTickEnd();
	void OnPostEngineTick();

	Snapshot GetSnapshot();

	std::atomic<bool> LogToFile = false;

private:
	struct FrameTotals {
		int frames = 0;
		double frameSum = 0.0;
		double frameMax = 0.0;
		int ticks = 0;
		double tickSum = 0.0;
		double tickMax = 0.0;
		double pluginSum = 0.0;
	};

	double qpcToMs = 0.0;
	int64_t lastFrameStart = 0;
	int64_t tickStart = 0;
	int64_t pluginTicks = 0;
	std::atomic<DWORD> gameThreadId = 0;

	std::mutex totalsMutex;
	FrameTotals totals;

	std::mutex snapshotMutex;
	Snapshot latest;

	void samplingLoop();
};
