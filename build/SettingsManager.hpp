#pragma once
#include <iostream>
#include <windows.h>
#include <string>
#include "uevr/API.hpp"
#include "Logger.hpp"

class SettingsManager {
public:
	bool debugMod = false;

	void InitSettingsManager();
	void GetAllConfigFilePaths();
	void UpdateSettingsIfModifiedByPlayer();

private:
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

	bool uevr_DecoupledPitch = false;
};