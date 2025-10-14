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
};

class SettingsManager {
public:
	bool debugMod = false;

	void InitSettingsManager();
	void GetAllConfigFilePaths();
	void UpdateSettingsIfModifiedByPlayer();

	static std::unordered_map<std::string, WeaponConfig> g_weaponConfigs;
	bool LoadWeaponConfigFromFile(const fs::path& path, WeaponConfig& outCfg);
	void LoadAllWeaponConfigs(const std::filesystem::path& baseDir);

	bool* SmoothCameraRotationPtr;
	float* CameraYawLerpPtr;
	

private:
	bool smoothCameraRotation = true;
	float cameraYawLerp = 2.25f;

	bool CheckSettingsModificationAndUpdate(const std::string& filePath, bool uevr);
	std::string GetConfigFilePath(bool uevr);
	std::string uevrSettingsFileName = "config.txt";
	std::string pluginSettingsFileName = "MGSDelta_config.txt";

	std::string uevrConfigFilePath;
	FILETIME uevrLastWriteTime;
	bool uevrConfigWroteByPlugin = false;
	std::string pluginConfigFilePath;
	FILETIME pluginLastWriteTime;

	//Would need some rework if lots of config values to read. Now it opens the config.txt file each time we call these :
	float GetFloatValueFromFile(const std::string& filePath, const std::string& key, float defaultValue);
	bool GetBoolValueFromFile(const std::string& filePath, const std::string& key, bool defaultValue);
	void SetBoolValueToFile(const std::string& filePath, const std::string& key, bool value);
	int GetIntValueFromFile(const std::string& filePath, const std::string& key, int defaultValue);
	void SetIntValueToFile(const std::string& filePath, const std::string& key, int value);

	void FetchUevrSettings(bool writeToPlugin);
	void FetchPluginSettings();
	void WriteChangedSettingsToPluginConfigFile();
};