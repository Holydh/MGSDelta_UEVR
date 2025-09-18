#include "SettingsManager.hpp"

bool debugMod = true;
std::unordered_map<std::string, WeaponConfig> SettingsManager::g_weaponConfigs{};

bool SettingsManager::LoadWeaponConfigFromFile(const fs::path& path, WeaponConfig& outCfg)
{
	 std::ifstream ifs(path);
    if (!ifs.is_open())
    {
        Logger::DebugPrint("Failed to open %s", path);
        return false;
    }

    json j;
    try
    {
        ifs >> j;
    }
    catch (const std::exception& e)
    {
        Logger::DebugPrint("JSON parse error in %s : %s",path ,e.what());
        return false;
    }

    if (!j.contains("state")) return false;
    auto& state = j["state"];

    if (state.contains("location_offset"))
    {
        auto& loc = state["location_offset"];
        outCfg.location_offset.x = loc.value("x", 0.0);
        outCfg.location_offset.y = loc.value("y", 0.0);
        outCfg.location_offset.z = loc.value("z", 0.0);
    }

    if (state.contains("rotation_offset"))
    {
        auto& rot = state["rotation_offset"];
        outCfg.rotation_offset.w = rot.value("w", 1.0);
        outCfg.rotation_offset.x = rot.value("x", 0.0);
        outCfg.rotation_offset.y = rot.value("y", 0.0);
        outCfg.rotation_offset.z = rot.value("z", 0.0);
    }

    return true;
}

void SettingsManager::LoadAllWeaponConfigs(const std::filesystem::path& dir)
{
    g_weaponConfigs.clear();

    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir))
    {
		Logger::DebugPrint("Config folder not found: %s", dir);
        return;
    }

    for (auto& entry : std::filesystem::directory_iterator(dir))
    {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".json") continue;

        WeaponConfig cfg;
        if (LoadWeaponConfigFromFile(entry.path(), cfg))
        {
            std::string key = entry.path().stem().string(); // e.g. "WP_Mk22"
            g_weaponConfigs[key] = cfg;
            Logger::DebugPrint("Loaded config for %s", key);
        }
    }
}

void SettingsManager::InitSettingsManager()
{
	GetAllConfigFilePaths();
	Logger::DebugPrint("%s", uevrConfigFilePath.c_str());
	Logger::DebugPrint("%s", pluginConfigFilePath.c_str());
	FetchPluginSettings();
}

void SettingsManager::FetchUevrSettings(bool writeToPlugin)
{
	if (debugMod) Logger::DebugPrint("UpdateUevrSettings()");
	
	//xAxisSensitivity = SettingsManager::GetFloatValueFromFile(uevrConfigFilePath, "VR_AimSpeed", 125.0f) * 10; //*10 because the base UEVR setting is too low as is 
	//joystickDeadzone = SettingsManager::GetFloatValueFromFile(uevrConfigFilePath, "VR_JoystickDeadzone", 0.1f);
	//uevr_DecoupledPitch = SettingsManager::GetBoolValueFromFile(uevrConfigFilePath, "VR_DecoupledPitch", true);
	//uevr_LerpPitch = SettingsManager::GetBoolValueFromFile(uevrConfigFilePath, "VR_LerpCameraPitch", true);
	//uevr_LerpRoll = SettingsManager::GetBoolValueFromFile(uevrConfigFilePath, "VR_LerpCameraRoll", true);
	//uevr_LerpYaw = SettingsManager::GetBoolValueFromFile(uevrConfigFilePath, "VR_LerpCameraYaw", false);

	if (writeToPlugin)
		WriteChangedSettingsToPluginConfigFile();
	if (debugMod) Logger::DebugPrint("UEVR Settings Updated");
}


void SettingsManager::FetchPluginSettings()
{
	if (debugMod) Logger::DebugPrint("UpdatePluginSettings()");

	if (debugMod) Logger::DebugPrint("Plugin Settings Updated");
}

void SettingsManager::UpdateSettingsIfModifiedByPlayer()
{
	if (debugMod) Logger::DebugPrint("UpdateSettingsIfModified");

	CheckSettingsModificationAndUpdate(pluginConfigFilePath, false);
	if (uevrConfigWroteByPlugin) //Skip uevr config check if plugin write this frame
	{
		uevrConfigWroteByPlugin = false;
		return;
	}
	CheckSettingsModificationAndUpdate(uevrConfigFilePath, true);
}

bool SettingsManager::CheckSettingsModificationAndUpdate(const std::string& filePath, bool uevr)
{
	if (debugMod) Logger::DebugPrint("CheckSettingsModificationAndUpdate");

	HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		if (uevr)
		{
			Logger::DebugPrint("File not found: %s", filePath.c_str());
			return false;
		}
			
		// File does not exist, create it with default values
		Logger::DebugPrint("File not found: %s. Creating default config.", filePath.c_str());

		std::string defaultContent = "";
			//"# MGSDelta Plugin Configuration File\n"
			//"# This file is auto-generated. Modify the settings below as needed.\n"
			//"------ Weapon Settings ------"
			//"[Weapon Aim Offset : Vertical, Horizontal]"
			//"WP_CigarPistol = 150.0, 0.0"
			// 
			//"[Left Handed Mode :] -- Must be configured here. 0 = disabled, 1 = Triggers Swap, 2 = Full inputs Swap\n"
			//"LeftHandedMode=0\n"
			//"LeftHandedOnlyWhileOnFoot=true\n"
			//"\n"
			//"[Camera Settings :] -- Can be set directly ingame from uevr menu. The plugin will auto save each camera configuration for each vehicle type here\n"
			//"OnFoot_DecoupledPitch=true\n"
			//"OnFoot_LerpPitch=false\n"
			//"OnFoot_LerpRoll=false\n"
			//"OnFoot_LerpYaw=false\n"
			//"DrivingCar_DecoupledPitch=false\n"
			//"DrivingCar_LerpPitch=true\n"
			//"DrivingCar_LerpRoll=true\n"
			//"DrivingCar_LerpYaw=false\n"
			//"DrivingBike_DecoupledPitch=true\n"
			//"DrivingBike_LerpPitch=false\n"
			//"DrivingBike_LerpRoll=false\n"
			//"DrivingBike_LerpYaw=false\n"
			//"Flying_DecoupledPitch=false\n"
			//"Flying_LerpPitch=true\n"
			//"Flying_LerpRoll=true\n"
			//"Flying_LerpYaw=true\n";

		HANDLE hCreateFile = CreateFileA(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hCreateFile != INVALID_HANDLE_VALUE)
		{
			DWORD bytesWritten;
			WriteFile(hCreateFile, defaultContent.c_str(), static_cast<DWORD>(defaultContent.size()), &bytesWritten, NULL);
			CloseHandle(hCreateFile);
			Logger::DebugPrint("Default config created at: %s", filePath.c_str());
		}
		else
		{
			Logger::DebugPrint("Failed to create default config at: %s", filePath.c_str());
		}
		return false;
	}
	FILETIME currentWriteTime;
	if (GetFileTime(hFile, NULL, NULL, &currentWriteTime))
	{
		if (CompareFileTime(uevr ? &uevrLastWriteTime : &pluginLastWriteTime, &currentWriteTime) != 0)
		{
			if (uevr)
			{
				uevrLastWriteTime = currentWriteTime;  // Update last write time
				FetchUevrSettings(true);
			}
			else
			{
				pluginLastWriteTime = currentWriteTime; 
				FetchPluginSettings();
			}
			CloseHandle(hFile);
			uevr::API::get()->log_error("setting file has been modified");
			return true;  // File has been modified
		}
	}

	CloseHandle(hFile);
	return false;  // No change
}

void SettingsManager::SetBoolValueToFile(const std::string& filePath, const std::string& key, bool value)
{
	if (debugMod) Logger::DebugPrint("SetBoolValueToFile()");

	HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		Logger::DebugPrint("Failed to open %s for reading", filePath);
		return;
	}

	DWORD bytesRead;
	char buffer[1024];
	std::string fileContents;

	while (ReadFile(hFile, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
	{
		buffer[bytesRead] = '\0';
		fileContents.append(buffer);
	}
	CloseHandle(hFile);

	size_t pos = fileContents.find(key);
	if (pos != std::string::npos)
	{
		size_t equalPos = fileContents.find('=', pos);
		if (equalPos != std::string::npos)
		{
			size_t endOfLine = fileContents.find_first_of("\r\n", equalPos);
			std::string before = fileContents.substr(0, equalPos + 1);
			std::string after = (endOfLine != std::string::npos) ? fileContents.substr(endOfLine) : "";

			// Replace value
			std::string newContents = before + (value ? "true" : "false") + after;
			
			// Write it back
			HANDLE hWriteFile = CreateFileA(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hWriteFile != INVALID_HANDLE_VALUE)
			{
				DWORD bytesWritten;
				WriteFile(hWriteFile, newContents.c_str(), static_cast<DWORD>(newContents.size()), &bytesWritten, NULL);
				CloseHandle(hWriteFile);
				Logger::DebugPrint("Updated %s to %s", key.c_str(), value ? "true" : "false");
			}
			else
			{
				Logger::DebugPrint("Failed to open %s for writing", filePath);
			}
			return;
		}
	}
}

bool SettingsManager::GetBoolValueFromFile(const std::string& filePath, const std::string& key, bool defaultValue)
{
	if (debugMod) Logger::DebugPrint("GetBoolValueFromFile()");

	HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		Logger::DebugPrint("Failed to open %s", filePath);
		return defaultValue;
	}

	DWORD bytesRead;
	char buffer[1024];  // Buffer to read the file content
	std::string fileContents;

	// Read the file into memory
	while (ReadFile(hFile, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
	{
		buffer[bytesRead] = '\0'; // Null terminate the string
		fileContents.append(buffer);
	}
	CloseHandle(hFile);

	// Look for the key in the file contents
	size_t pos = fileContents.find(key);
	if (pos != std::string::npos)
	{
		size_t equalPos = fileContents.find('=', pos);
		if (equalPos != std::string::npos)
		{
			// Find the end of the line after the '='
			size_t endOfLine = fileContents.find_first_of("\r\n", equalPos);
			std::string valueStr = fileContents.substr(equalPos + 1, endOfLine - (equalPos + 1));

			// Trim whitespace
			valueStr.erase(0, valueStr.find_first_not_of(" \t\n\r"));
			valueStr.erase(valueStr.find_last_not_of(" \t\n\r") + 1);

			// Convert to lowercase
			std::transform(valueStr.begin(), valueStr.end(), valueStr.begin(), ::tolower);

			if (debugMod) Logger::DebugPrint("Extracted value: %s", valueStr.c_str());

			if (valueStr == "true") return true;
			if (valueStr == "false") return false;

			Logger::DebugPrint("Error: Invalid bool value for key: %s", key.c_str());
		}
	}

	return defaultValue;  // Return default if the key is not found or invalid
}

float SettingsManager::GetFloatValueFromFile(const std::string& filePath, const std::string& key, float defaultValue)
{
	if (debugMod) Logger::DebugPrint("GetFloatValueFromFile()");

	HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		Logger::DebugPrint("Failed to open %s", filePath);
		return defaultValue;
	}

	DWORD bytesRead;
	char buffer[1024];  // Buffer to read the file content
	std::string fileContents;

	// Read the file into memory
	while (ReadFile(hFile, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
	{
		buffer[bytesRead] = '\0'; // Null terminate the string
		fileContents.append(buffer);
	}
	CloseHandle(hFile);

	// Look for the key in the file contents
	size_t pos = fileContents.find(key);
	if (pos != std::string::npos)
	{
		size_t equalPos = fileContents.find('=', pos);
		if (equalPos != std::string::npos)
		{
			std::string valueStr = fileContents.substr(equalPos + 1);
			try
			{
				return std::stof(valueStr); // Convert the string to float
			}
			catch (const std::invalid_argument&)
			{
				Logger::DebugPrint("Error: Invalid float value for key: %s", key.c_str());
				return defaultValue;
			}
		}
	}

	return defaultValue;  // Return default if the key is not found
}

void SettingsManager::SetIntValueToFile(const std::string& filePath, const std::string& key, int value)
{
	if (debugMod) Logger::DebugPrint("SetIntValueToFile()");

	HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		Logger::DebugPrint("Failed to open %s for reading", filePath);
		return;
	}

	DWORD bytesRead;
	char buffer[1024];
	std::string fileContents;

	while (ReadFile(hFile, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
	{
		buffer[bytesRead] = '\0';
		fileContents.append(buffer);
	}
	CloseHandle(hFile);

	size_t pos = fileContents.find(key);
	if (pos != std::string::npos)
	{
		size_t equalPos = fileContents.find('=', pos);
		if (equalPos != std::string::npos)
		{
			size_t endOfLine = fileContents.find_first_of("\r\n", equalPos);
			std::string before = fileContents.substr(0, equalPos + 1);
			std::string after = (endOfLine != std::string::npos) ? fileContents.substr(endOfLine) : "";

			// Replace value
			std::string newContents = before + std::to_string(value) + after;
			
			// Write it back
			HANDLE hWriteFile = CreateFileA(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hWriteFile != INVALID_HANDLE_VALUE)
			{
				DWORD bytesWritten;
				WriteFile(hWriteFile, newContents.c_str(), static_cast<DWORD>(newContents.size()), &bytesWritten, NULL);
				CloseHandle(hWriteFile);
				Logger::DebugPrint("Updated %s to %s", key.c_str(), std::to_string(value));
			}
			else
			{
				Logger::DebugPrint("Failed to open %s for writing", filePath);
			}
			return;
		}
	}
}

int SettingsManager::GetIntValueFromFile(const std::string& filePath, const std::string& key, int defaultValue)
{
	if (debugMod) Logger::DebugPrint("GetFloatValueFromFile()");

	HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		Logger::DebugPrint("Failed to open %s", filePath);
		return defaultValue;
	}

	DWORD bytesRead;
	char buffer[1024];  // Buffer to read the file content
	std::string fileContents;

	// Read the file into memory
	while (ReadFile(hFile, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
	{
		buffer[bytesRead] = '\0'; // Null terminate the string
		fileContents.append(buffer);
	}
	CloseHandle(hFile);

	// Look for the key in the file contents
	size_t pos = fileContents.find(key);
	if (pos != std::string::npos)
	{
		size_t equalPos = fileContents.find('=', pos);
		if (equalPos != std::string::npos)
		{
			std::string valueStr = fileContents.substr(equalPos + 1);
			try
			{
				return std::stoi(valueStr); // Convert the string to float
			}
			catch (const std::invalid_argument&)
			{
				Logger::DebugPrint("Error: Invalid int value for key: %s", key.c_str());
				return defaultValue;
			}
		}
	}

	return defaultValue;  // Return default if the key is not found
}

std::string GetDLLDirectory()
{
	char path[MAX_PATH];
	HMODULE hModule = GetModuleHandleA("UEVR_GTASADE.dll"); // Get handle to the loaded DLL

	if (hModule)
	{
		GetModuleFileNameA(hModule, path, MAX_PATH); // Get full DLL path
		std::string fullPath = path;

		// Remove the DLL filename to get the directory
		size_t pos = fullPath.find_last_of("\\/");
		if (pos != std::string::npos)
		{
			return fullPath.substr(0, pos + 1); // Keep the trailing slash
		}
	}
	else
		Logger::DebugPrint("Failed to get module handle for UEVR_GTASADE.dll");

	return "Unknown";
}

void SettingsManager::GetAllConfigFilePaths()
{
	if (debugMod) Logger::DebugPrint("GetAllConfigFilePaths");
	uevrConfigFilePath = GetConfigFilePath(true);
	pluginConfigFilePath = GetConfigFilePath(false);
}

std::string SettingsManager::GetConfigFilePath(bool uevr)
{
	if (debugMod) Logger::DebugPrint("GetConfigFilePath()");

	std::string fullPath = GetDLLDirectory();

	// Remove "SanAndreas\plugins\UEVR_GTASADE.dll" part, leaving "SanAndreas"
	size_t pos = fullPath.find_last_of("\\/");
	if (pos != std::string::npos)
	{
		fullPath = fullPath.substr(0, pos); // Remove "\plugins"
		pos = fullPath.find_last_of("\\/");
		if (pos != std::string::npos)
		{
			fullPath = fullPath.substr(0, pos + 1); // Keep "SanAndreas\"
		}
	}

	return fullPath + (uevr ? uevrSettingsFileName : pluginSettingsFileName); // Append "config.txt" or "UEVR_GTASADE_config.txt"
}

void SettingsManager::WriteChangedSettingsToPluginConfigFile()
{
	//switch (cameraModeSettings)
	//{
	//case OnFoot:
	//	SetBoolValueToFile(pluginConfigFilePath, "OnFoot_DecoupledPitch", uevr_DecoupledPitch);
	//	SetBoolValueToFile(pluginConfigFilePath, "OnFoot_LerpPitch", uevr_LerpPitch);
	//	SetBoolValueToFile(pluginConfigFilePath, "OnFoot_LerpRoll", uevr_LerpRoll);
	//	SetBoolValueToFile(pluginConfigFilePath, "OnFoot_LerpYaw", uevr_LerpYaw);
	//	break;
	//case DrivingCar:
	//	SetBoolValueToFile(pluginConfigFilePath, "DrivingCar_DecoupledPitch", uevr_DecoupledPitch);
	//	SetBoolValueToFile(pluginConfigFilePath, "DrivingCar_LerpPitch", uevr_LerpPitch);
	//	SetBoolValueToFile(pluginConfigFilePath, "DrivingCar_LerpRoll", uevr_LerpRoll);
	//	SetBoolValueToFile(pluginConfigFilePath, "DrivingCar_LerpYaw", uevr_LerpYaw);
	//	break;
	//case DrivingBike:
	//	SetBoolValueToFile(pluginConfigFilePath, "DrivingBike_DecoupledPitch", uevr_DecoupledPitch);
	//	SetBoolValueToFile(pluginConfigFilePath, "DrivingBike_LerpPitch", uevr_LerpPitch);
	//	SetBoolValueToFile(pluginConfigFilePath, "DrivingBike_LerpRoll", uevr_LerpRoll);
	//	SetBoolValueToFile(pluginConfigFilePath, "DrivingBike_LerpYaw", uevr_LerpYaw);
	//	break;
	//case Flying:
	//	SetBoolValueToFile(pluginConfigFilePath, "Flying_DecoupledPitch", uevr_DecoupledPitch);
	//	SetBoolValueToFile(pluginConfigFilePath, "Flying_LerpPitch", uevr_LerpPitch);
	//	SetBoolValueToFile(pluginConfigFilePath, "Flying_LerpRoll", uevr_LerpRoll);
	//	SetBoolValueToFile(pluginConfigFilePath, "Flying_LerpYaw", uevr_LerpYaw);
	//	break;
	//}
}
