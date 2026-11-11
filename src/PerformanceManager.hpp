#pragma once
#include <atomic>
#include <string>
#include <vector>
#include "uevr/API.hpp"

// Console variables forced by the plugin to lighten the GPU load in VR. The game re-applies its
// own render settings on each stage load and cutscene (LevelScalabilityBP), so the forced values
// are checked regularly and re-applied when the game changed them.
class PerformanceManager {
public:
	struct CVarOverride {
		const char* group;
		const wchar_t* name;
		const char* tooltip;
		bool isInt;
		float min;
		float max;
		float value; // forced while enabled
		bool enabled = false;
		// cvar value before the plugin changed it, restored when the override is disabled
		bool hasOriginal = false;
		float original = 0.0f;
		uevr::API::IConsoleVariable* cvar = nullptr;
		bool missing = false; // not found in the game (some cvars are stripped from shipping builds)
	};

	PerformanceManager();

	std::vector<CVarOverride> Overrides;

	void Init();
	// Game thread only (cvars aren't safe to change from the render thread)
	void Update(float deltaTime);
	// Menu side: applied on the next game tick
	void RequestApply();
	void Save();

private:
	std::string configFilePath;
	float checkTimer = 0.0f;
	std::atomic<bool> applyNow = true;

	void apply(CVarOverride& o);
	void load();
};
