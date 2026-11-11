#pragma once
#include <iostream>
#include <windows.h>
#include <string>
#include "uevr/API.hpp"
#include "Utilities.hpp"
#include "ue/Types.hpp"
#include "nlohmann/json.hpp"
#include "Logger.hpp"

#include <unordered_map>
#include <vector>
#include <filesystem>
#include <fstream>
#include "glm/glm.hpp"
#include <glm/gtc/type_ptr.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

struct WeaponConfig
{
    glm::fvec3 location_offset{};
    glm::fquat rotation_offset{ 1.0f, 0.0f, 0.0f, 0.0f }; // identity (w, x, y, z): {} would be a zero quaternion
	ue::FRotator right_hand_rotation_offset{};
	ue::FVector left_hand_location_offset{};
	ue::FRotator left_hand_rotation_offset{};
};

class SettingsManager {
public:
	bool debugMod = false;

	bool SettingsModifiedThisFrame = false;

	void InitSettingsManager();
	void GetOrCreateConfigFile();
	void GetAllConfigFilePaths();
	void WriteSettingsToPluginConfigFile();

	static std::unordered_map<std::string, WeaponConfig> g_weaponConfigs;
	bool LoadWeaponConfigFromFile(const fs::path& path, WeaponConfig& outCfg);
	void LoadAllConfigs(const std::filesystem::path& baseDir);

	bool* AttachCameraToSnakeHeadPtr;
	float* CameraRotationSpeedPtr;
	float* CameraRotationDeadZonePtr;
	bool* SmoothCameraRotationPtr;
	float* CameraYawLerpPtr;
	bool* SmoothCameraPositionPtr;
	float* CameraPositionXYLerpPtr;
	float* CameraPositionZLerpPtr;

	bool* EnableRoomscaleProneAndCrouchPtr;
	float* SquatUnderThisValuePtr;
	float* CrawlUnderThisValuePtr;

	bool* EnableWeaponsScopesPtr;
	int* ScopeResolutionSliderIndexPtr;
	int* ScopeResolutionPtr;
	bool* DisableAllRecoilPtr;
	bool* EnableTwoHandsPivotPtr;
	bool* EnableTwoHandsRealisticRecoilPtr;

	bool* DisableThermalVisionScanlinePtr;

	bool* EnableImmersiveModePtr;
	float* BlackBarsSizePtr;
	bool* SmoothCameraMovementsInImmersiveCutscenesPtr;
	float* CutsceneCameraPositionLerpPtr;
	float* CutsceneCameraRotationLerpPtr;
	//bool* EnableCutoutScreenForCutscenesPtr;

	bool* Enable3DCutscenesPtr;
	bool* Enable3DRadioPtr;
	bool* Enable3DSurvivalViewPtr;
	
private:
	bool attachCameraToSnakeHead = false;
	float cameraRotationSpeed = 10.0f;
	float cameraRotationDeadZone = 0.2f;
	bool smoothCameraRotation = true;
	float cameraYawLerp = 5.0f;
	bool smoothCameraPosition = true;
	float cameraPositionXYLerp = 5.0f;
	float cameraPositionZLerp = 5.0f;

	bool enableRoomscaleProneAndCrouch = false;
	float squatUnderThisValue = -0.4f;
	float crawlUnderThisValue = -0.8f;

	bool enableWeaponsScopes = true;
	int scopeResolutionSliderIndex = 0;
	int scopeResolution = 1024;
	bool disableAllRecoil = false;
	bool enableTwoHandsPivot = true;
	bool enableTwoHandsRealisticRecoil = true;

	bool disableThermalVisionScanline = true;

	bool enableImmersiveMode = false;
	float blackBarsSize = 1.0f;
	bool smoothCameraMovementsInImmersiveCutscenes = true;
	float cutsceneCameraPositionLerp = 5.0f;
	float cutsceneCameraRotationLerp = 8.0f;
	//bool enableCutoutScreenForCutscenes = true;

	bool enable3DCutscenes = true;
	bool enable3DRadio = false;
	bool enable3DSurvivalView = true;

	std::string GetConfigFilePath(bool uevr);
	std::string uevrSettingsFileName = "config.txt";
	std::string pluginSettingsFileName = "MGSDelta_config.txt";

	std::string uevrConfigFilePath;
	FILETIME uevrLastWriteTime;
	bool uevrConfigWroteByPlugin = false;
	std::string pluginConfigFilePath;
	FILETIME pluginLastWriteTime;

	//Would need some rework if lots of config values to read. Now it opens the config.txt file each time we call these :
	void SetFloatValueToFile(const std::string& filePath, const std::string& key, float value);
	float GetFloatValueFromFile(const std::string& filePath, const std::string& key, float defaultValue);
	bool GetBoolValueFromFile(const std::string& filePath, const std::string& key, bool defaultValue);
	void SetBoolValueToFile(const std::string& filePath, const std::string& key, bool value);
	int GetIntValueFromFile(const std::string& filePath, const std::string& key, int defaultValue);
	void SetIntValueToFile(const std::string& filePath, const std::string& key, int value);

	void FetchUevrSettings(bool writeToPlugin);
	void FetchPluginSettings();
	void SyncScopeSliderWithResolution();
	void WriteChangedSettingsToPluginConfigFile();
};