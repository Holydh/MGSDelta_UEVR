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

	if (state.contains("right_hand_rotation_offset"))
    {
        auto& rightHandRot = state["right_hand_rotation_offset"];
        outCfg.right_hand_rotation_offset.Pitch = rightHandRot.value("Pitch", 0.0);
        outCfg.right_hand_rotation_offset.Yaw = rightHandRot.value("Yaw", 0.0);
        outCfg.right_hand_rotation_offset.Roll = rightHandRot.value("Roll", 0.0);
    }

    if (state.contains("left_hand_location_offset"))
    {
        auto& leftHandLoc = state["left_hand_location_offset"];
        outCfg.left_hand_location_offset.X = leftHandLoc.value("x", 0.0);
        outCfg.left_hand_location_offset.Y = leftHandLoc.value("y", 0.0);
        outCfg.left_hand_location_offset.Z = leftHandLoc.value("z", 0.0);
    }

	if (state.contains("left_hand_rotation_offset"))
    {
        auto& leftHandRot = state["left_hand_rotation_offset"];
        outCfg.left_hand_rotation_offset.Pitch = leftHandRot.value("Pitch", 0.0);
        outCfg.left_hand_rotation_offset.Yaw = leftHandRot.value("Yaw", 0.0);
        outCfg.left_hand_rotation_offset.Roll = leftHandRot.value("Roll", 0.0);
    }

    return true;
}

void SettingsManager::LoadAllConfigs(const std::filesystem::path& dir)
{
	FetchPluginSettings();
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
	//FetchUevrSettings(false);

	AttachCameraToSnakeHeadPtr = &attachCameraToSnakeHead;
	CameraRotationSpeedPtr = &cameraRotationSpeed;
	CameraRotationDeadZonePtr = &cameraRotationDeadZone;
	SmoothCameraRotationPtr = &smoothCameraRotation;
	CameraYawLerpPtr = &cameraYawLerp;
	SmoothCameraPositionPtr = &smoothCameraPosition;
	CameraPositionXYLerpPtr = &cameraPositionXYLerp;
	CameraPositionZLerpPtr = &cameraPositionZLerp;

	EnableRoomscaleProneAndCrouchPtr = &enableRoomscaleProneAndCrouch;
	SquatUnderThisValuePtr = &squatUnderThisValue;
	CrawlUnderThisValuePtr = &crawlUnderThisValue;

	EnableWeaponsScopesPtr = &enableWeaponsScopes;
	ScopeResolutionSliderIndexPtr = &scopeResolutionSliderIndex;
	ScopeResolutionPtr = &scopeResolution;
	DisableAllRecoilPtr = &disableAllRecoil;
	EnableTwoHandsPivotPtr = &enableTwoHandsPivot;
	EnableTwoHandsRealisticRecoilPtr = &enableTwoHandsRealisticRecoil;

	DisableThermalVisionScanlinePtr = &disableThermalVisionScanline;

	EnableImmersiveModePtr = &enableImmersiveMode;
	SmoothCameraMovementsInImmersiveCutscenesPtr = &smoothCameraMovementsInImmersiveCutscenes;
	CutsceneCameraPositionLerpPtr = &cutsceneCameraPositionLerp;
	CutsceneCameraRotationLerpPtr = &cutsceneCameraRotationLerp;
	//EnableCutoutScreenForCutscenesPtr = &enableCutoutScreenForCutscenes;

	Enable3DCutscenesPtr = &enable3DCutscenes;
	Enable3DRadioPtr = &enable3DRadio;
	Enable3DSurvivalViewPtr = &enable3DSurvivalView;
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
	attachCameraToSnakeHead = SettingsManager::GetBoolValueFromFile(pluginConfigFilePath, "attachCameraToSnakeHead", false);
	cameraRotationSpeed = SettingsManager::GetFloatValueFromFile(pluginConfigFilePath, "cameraRotationSpeed", 10.0f);
	cameraRotationDeadZone = SettingsManager::GetFloatValueFromFile(pluginConfigFilePath, "cameraRotationDeadZone", 0.2f);
	smoothCameraRotation = SettingsManager::GetBoolValueFromFile(pluginConfigFilePath, "smoothCameraRotation", true);
	cameraYawLerp = SettingsManager::GetFloatValueFromFile(pluginConfigFilePath, "cameraYawLerp", 5.0f);
	smoothCameraPosition = SettingsManager::GetBoolValueFromFile(pluginConfigFilePath, "smoothCameraPosition", true);
	cameraPositionXYLerp = SettingsManager::GetFloatValueFromFile(pluginConfigFilePath, "cameraPositionXYLerp", 5.0f);
	cameraPositionZLerp = SettingsManager::GetFloatValueFromFile(pluginConfigFilePath, "cameraPositionZLerp", 5.0f);

	enableRoomscaleProneAndCrouch = SettingsManager::GetBoolValueFromFile(pluginConfigFilePath, "enableRoomscaleProneAndCrouch", false);
	squatUnderThisValue = SettingsManager::GetFloatValueFromFile(pluginConfigFilePath, "squatUnderThisValue", -0.4f);
	crawlUnderThisValue = SettingsManager::GetFloatValueFromFile(pluginConfigFilePath, "crawlUnderThisValue", -0.8f);

	enableWeaponsScopes = SettingsManager::GetBoolValueFromFile(pluginConfigFilePath, "enableWeaponsScopes", true);
	scopeResolution = SettingsManager::GetIntValueFromFile(pluginConfigFilePath, "scopeResolution", 1024);
	SyncScopeSliderWithResolution();
	
	disableAllRecoil = SettingsManager::GetBoolValueFromFile(pluginConfigFilePath, "disableAllRecoil", false);
	enableTwoHandsPivot = SettingsManager::GetBoolValueFromFile(pluginConfigFilePath, "enableTwoHandsPivot", true);
	enableTwoHandsRealisticRecoil = SettingsManager::GetBoolValueFromFile(pluginConfigFilePath, "enableTwoHandsRealisticRecoil", true);

	disableThermalVisionScanline = SettingsManager::GetBoolValueFromFile(pluginConfigFilePath, "disableThermalVisionScanline", true);

	enableImmersiveMode = SettingsManager::GetBoolValueFromFile(pluginConfigFilePath, "enableImmersiveMode", true);
	smoothCameraMovementsInImmersiveCutscenes = SettingsManager::GetBoolValueFromFile(pluginConfigFilePath, "smoothCameraMovementsInImmersiveCutscenes", true);
	cutsceneCameraPositionLerp = SettingsManager::GetFloatValueFromFile(pluginConfigFilePath, "cutsceneCameraPositionLerp", 5.0f);
	cutsceneCameraRotationLerp = SettingsManager::GetFloatValueFromFile(pluginConfigFilePath, "cutsceneCameraRotationLerp", 8.0f);
	//enableCutoutScreenForCutscenes = SettingsManager::GetBoolValueFromFile(pluginConfigFilePath, "enableCutoutScreenForCutscenes", true);
	
	enable3DCutscenes = SettingsManager::GetBoolValueFromFile(pluginConfigFilePath, "enable3DCutscenes", false);
	enable3DRadio = SettingsManager::GetBoolValueFromFile(pluginConfigFilePath, "enable3DRadio", false);
	enable3DSurvivalView = SettingsManager::GetBoolValueFromFile(pluginConfigFilePath, "enable3DSurvivalView", true);


	if (debugMod) Logger::DebugPrint("Plugin Settings Updated");
}

void SettingsManager::SyncScopeSliderWithResolution()
{
    if (ScopeResolutionSliderIndexPtr && ScopeResolutionPtr)
    {
        int resolution = *ScopeResolutionPtr;
        int newIndex = resolution / 64; // Assuming step is 64
        *ScopeResolutionSliderIndexPtr = newIndex;
    }
}


void SettingsManager::WriteSettingsToPluginConfigFile()
{
	if (debugMod) Logger::DebugPrint("WriteSettingsToPluginConfigFile()");
	SettingsManager::SetBoolValueToFile(pluginConfigFilePath, "attachCameraToSnakeHead", attachCameraToSnakeHead);
	SettingsManager::SetFloatValueToFile(pluginConfigFilePath, "cameraRotationSpeed", cameraRotationSpeed);
	SettingsManager::SetFloatValueToFile(pluginConfigFilePath, "cameraRotationDeadZone", cameraRotationDeadZone);
	SettingsManager::SetBoolValueToFile(pluginConfigFilePath, "smoothCameraRotation", smoothCameraRotation);
	SettingsManager::SetFloatValueToFile(pluginConfigFilePath, "cameraYawLerp", cameraYawLerp);
	SettingsManager::SetBoolValueToFile(pluginConfigFilePath, "smoothCameraPosition", smoothCameraPosition);
	SettingsManager::SetFloatValueToFile(pluginConfigFilePath, "cameraPositionXYLerp", cameraPositionXYLerp);
	SettingsManager::SetFloatValueToFile(pluginConfigFilePath, "cameraPositionZLerp", cameraPositionZLerp);

	SettingsManager::SetBoolValueToFile(pluginConfigFilePath, "enableRoomscaleProneAndCrouch", enableRoomscaleProneAndCrouch);
	SettingsManager::SetFloatValueToFile(pluginConfigFilePath, "squatUnderThisValue", squatUnderThisValue);
	SettingsManager::SetFloatValueToFile(pluginConfigFilePath, "crawlUnderThisValue", crawlUnderThisValue);

	SettingsManager::SetBoolValueToFile(pluginConfigFilePath, "enableWeaponsScopes", enableWeaponsScopes);
	SettingsManager::SetIntValueToFile(pluginConfigFilePath, "scopeResolution", scopeResolution);
	SettingsManager::SetBoolValueToFile(pluginConfigFilePath, "disableAllRecoil", disableAllRecoil);
	SettingsManager::SetBoolValueToFile(pluginConfigFilePath, "enableTwoHandsPivot", enableTwoHandsPivot);
	SettingsManager::SetBoolValueToFile(pluginConfigFilePath, "enableTwoHandsRealisticRecoil", enableTwoHandsRealisticRecoil);

	SettingsManager::SetBoolValueToFile(pluginConfigFilePath, "disableThermalVisionScanline", disableThermalVisionScanline);
	
	SettingsManager::SetBoolValueToFile(pluginConfigFilePath, "enableImmersiveMode", enableImmersiveMode);
	SettingsManager::SetBoolValueToFile(pluginConfigFilePath, "smoothCameraMovementsInImmersiveCutscenes", smoothCameraMovementsInImmersiveCutscenes);
	SettingsManager::SetFloatValueToFile(pluginConfigFilePath, "cutsceneCameraPositionLerp", cutsceneCameraPositionLerp);
	SettingsManager::SetFloatValueToFile(pluginConfigFilePath, "cutsceneCameraRotationLerp", cutsceneCameraRotationLerp);
	//SettingsManager::SetBoolValueToFile(pluginConfigFilePath, "enableCutoutScreenForCutscenes", enableCutoutScreenForCutscenes);
	
	SettingsManager::SetBoolValueToFile(pluginConfigFilePath, "enable3DCutscenes", enable3DCutscenes);
	SettingsManager::SetBoolValueToFile(pluginConfigFilePath, "enable3DRadio", enable3DRadio);
	SettingsManager::SetBoolValueToFile(pluginConfigFilePath, "enable3DSurvivalView", enable3DSurvivalView);

	Logger::DebugPrint("Config change saved in MGSDelta_config.txt");
}


void SettingsManager::GetOrCreateConfigFile()
{
	if (debugMod) Logger::DebugPrint("CheckSettingsModificationAndUpdate");

	HANDLE hFile = CreateFileA(pluginConfigFilePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		// File does not exist, create it with default values
		Logger::DebugPrint("File not found: %s. Creating default config.", pluginConfigFilePath.c_str());

		std::string defaultContent = "# MGSDelta Plugin Configuration File\n"
			"# This file is auto-generated. It saves the mgs plugin menu settings you set ingame.\n"
			"------ Camera Settings ------\n"
			"attachCameraToSnakeHead=false\n"
			"cameraRotationSpeed=25.0\n"
			"cameraRotationDeadZone=0.2\n"
			"smoothCameraRotation=false\n"
			"cameraYawLerp=5.0\n"
			"smoothCameraPosition=true\n"
			"cameraPositionXYLerp=3.5\n"
			"cameraPositionZLerp=5.0\n"
			"------ Gameplay Settings ------\n"
			"enableRoomscaleProneAndCrouch=false\n"
			"squatUnderThisValue=-0.4\n"
			"crawlUnderThisValue=-0.8\n"
			"------ Weapons Settings ------\n"
			"enableWeaponsScopes=true\n"
			"scopeResolution=1024\n"
			"disableAllRecoil=false\n"
			"enableTwoHandsPivot=true\n"
			"enableTwoHandsRealisticRecoil=true\n"
			"------ Items Settings ------\n"
			"disableThermalVisionScanline=true\n"
			"------ Cutscenes Settings ------\n"
			"enableImmersiveMode=true\n"
			"smoothCameraMovementsInImmersiveCutscenes=true\n"
			"cutsceneCameraPositionLerp=5.0\n"
			"cutsceneCameraRotationLerp=8.0\n"
			"smoothCameraMovementsInImmersiveCutscenes=true"
			//"enableCutoutScreenForCutscenes=true\n"
			"------ Rendering Settings ------\n"
			"enable3DCutscenes=false\n"
			"enable3DRadio=false\n"
			"enable3DSurvivalView=true\n";

		HANDLE hCreateFile = CreateFileA(pluginConfigFilePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hCreateFile != INVALID_HANDLE_VALUE)
		{
			DWORD bytesWritten;
			WriteFile(hCreateFile, defaultContent.c_str(), static_cast<DWORD>(defaultContent.size()), &bytesWritten, NULL);
			CloseHandle(hCreateFile);
			Logger::DebugPrint("Default config created at: %s", pluginConfigFilePath.c_str());
		}
		else
		{
			Logger::DebugPrint("Failed to create default config at: %s", pluginConfigFilePath.c_str());
		}
		return;
	}

	CloseHandle(hFile);
	return;
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
				if (debugMod) (Logger::DebugPrint("Updated %s to %s", key.c_str(), value ? "true" : "false"));
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

void SettingsManager::SetFloatValueToFile(const std::string& filePath, const std::string& key, float value)
{
	if (debugMod) Logger::DebugPrint("SetFloatValueToFile()");

	// Open file for reading first
	HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		Logger::DebugPrint("Failed to open %s for reading", filePath.c_str());
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

	// Find key in the file
	size_t pos = fileContents.find(key);
	if (pos != std::string::npos)
	{
		size_t equalPos = fileContents.find('=', pos);
		if (equalPos != std::string::npos)
		{
			size_t endOfLine = fileContents.find_first_of("\r\n", equalPos);
			std::string before = fileContents.substr(0, equalPos + 1);
			std::string after = (endOfLine != std::string::npos) ? fileContents.substr(endOfLine) : "";

			// Replace value (use limited precision to keep float clean)
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(6) << value;
			std::string newContents = before + oss.str() + after;

			// Write back
			HANDLE hWriteFile = CreateFileA(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hWriteFile != INVALID_HANDLE_VALUE)
			{
				DWORD bytesWritten;
				WriteFile(hWriteFile, newContents.c_str(), static_cast<DWORD>(newContents.size()), &bytesWritten, NULL);
				CloseHandle(hWriteFile);
				if (debugMod)(Logger::DebugPrint("Updated %s to %s", key.c_str(), oss.str().c_str()));
			}
			else
			{
				Logger::DebugPrint("Failed to open %s for writing", filePath.c_str());
			}
			return;
		}
	}

	Logger::DebugPrint("Key %s not found in %s", key.c_str(), filePath.c_str());
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
		Logger::DebugPrint("Failed to open %s for reading", filePath.c_str());
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

			std::string newContents = before + std::to_string(value) + after;

			HANDLE hWriteFile = CreateFileA(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hWriteFile != INVALID_HANDLE_VALUE)
			{
				DWORD bytesWritten;
				WriteFile(hWriteFile, newContents.c_str(), static_cast<DWORD>(newContents.size()), &bytesWritten, NULL);
				CloseHandle(hWriteFile);
				if (debugMod) Logger::DebugPrint("Updated %s to %d", key.c_str(), value);
			}
			else
			{
				Logger::DebugPrint("Failed to open %s for writing", filePath.c_str());
			}
			return;
		}
	}

	Logger::DebugPrint("Key %s not found in %s", key.c_str(), filePath.c_str());
}

int SettingsManager::GetIntValueFromFile(const std::string& filePath, const std::string& key, int defaultValue)
{
	if (debugMod) Logger::DebugPrint("GetIntValueFromFile()");

	HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		Logger::DebugPrint("Failed to open %s", filePath.c_str());
		return defaultValue;
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
			std::string valueStr = (endOfLine != std::string::npos)
				? fileContents.substr(equalPos + 1, endOfLine - equalPos - 1)
				: fileContents.substr(equalPos + 1);

			try
			{
				return std::stoi(valueStr);
			}
			catch (const std::invalid_argument&)
			{
				Logger::DebugPrint("Error: Invalid int value for key: %s", key.c_str());
				return defaultValue;
			}
		}
	}

	return defaultValue;
}

std::string GetDLLDirectory()
{
	char path[MAX_PATH];
	HMODULE hModule = GetModuleHandleA("MGSDelta_Plugin.dll"); // Get handle to the loaded DLL

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
		Logger::DebugPrint("Failed to get module handle for MGSDelta_Plugin.dll");

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
