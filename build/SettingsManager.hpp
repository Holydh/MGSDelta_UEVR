#pragma once
#include <iostream>
#include <windows.h>
#include <string>
#include "uevr/API.hpp"
#include "Utilities.hpp"
#include "nlohmann/json.hpp"
#include "Logger.hpp"

#include <unordered_map>
#include <vector>
#include <filesystem>
#include <fstream>
#include "glm/glm.hpp"
#include <glm/gtc/type_ptr.hpp>
#define GLM_FORCE_QUAT_DATA_XYZW

using json = nlohmann::json;
namespace fs = std::filesystem;

struct WeaponConfig
{
    glm::fvec3 location_offset{};
    glm::fquat rotation_offset{};
	SDK::FVector hand_location_offset{};
	SDK::FRotator hand_rotation_offset{};
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
	void LoadAllWeaponConfigs(const std::filesystem::path& baseDir);

	bool* AttachCameraToSnakeHeadPtr;
	float* CameraRotationSpeedPtr;
	float* CameraRotationDeadZonePtr;
	bool* SmoothCameraRotationPtr;
	float* CameraYawLerpPtr;
	bool* SmoothCameraPositionPtr;
	float* CameraPositionXYLerpPtr;
	float* CameraPositionZLerpPtr;

	bool* SniperScopeEnabledPtr;
	int* ScopeResolutionSliderIndexPtr;
	int* ScopeResolutionPtr;

	bool* DisableThermalVisionScanlinePtr;

	bool* EnableCutoutScreenForCutscenesAndMenusPtr;
	bool* Enable3DCutscenesAndMenusPtr;
	
private:
	bool attachCameraToSnakeHead = false;
	float cameraRotationSpeed = 10.0f;
	float cameraRotationDeadZone = 0.2f;
	bool smoothCameraRotation = true;
	float cameraYawLerp = 5.0f;
	bool smoothCameraPosition = true;
	float cameraPositionXYLerp = 5.0f;
	float cameraPositionZLerp = 5.0f;

	bool sniperScopeEnabled = true;
	int scopeResolutionSliderIndex = 0;
	int scopeResolution = 1024;

	bool disableThermalVisionScanline = true;

	bool enableCutoutScreenForCutscenesAndMenus = true;
	bool enable3DCutscenesAndMenus = false;


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
	void WriteChangedSettingsToPluginConfigFile();
};