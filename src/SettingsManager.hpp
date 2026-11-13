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

// Plugin menu settings, saved in MGSDelta_config.txt. A new setting goes here (its default is the
// initializer) and in the field table of SettingsManager.cpp.
struct PluginSettings
{
	// Camera
	bool attachCameraToSnakeHead = false;
	float cameraRotationSpeed = 25.0f;
	float cameraRotationDeadZone = 0.2f;
	bool smoothCameraRotation = false;
	float cameraYawLerp = 5.0f;
	bool smoothCameraPosition = true;
	float cameraPositionXYLerp = 3.5f;
	float cameraPositionZLerp = 5.0f;

	// Gameplay
	bool enableRoomscaleProneAndCrouch = false;
	bool linearLocomotion = true;
	bool gameTakesMovementControl = true;
	float squatUnderThisValue = -0.4f;
	float crawlUnderThisValue = -0.8f;

	// Weapons
	bool enableWeaponsScopes = true;
	int scopeResolution = 1024;
	bool disableAllRecoil = false;
	bool enableTwoHandsPivot = true;
	bool enableTwoHandsRealisticRecoil = true;

	// Items
	bool disableThermalVisionScanline = true;

	// Cutscenes
	bool enableImmersiveMode = true;
	float blackBarsSize = 6.0f;
	bool smoothCameraMovementsInImmersiveCutscenes = true;
	float cutsceneCameraPositionLerp = 6.0f;
	float cutsceneCameraRotationLerp = 7.0f;

	// Rendering
	bool enable3DCutscenes = true;
	bool enable3DRadio = false;
	bool enable3DSurvivalView = true;
};

class SettingsManager {
public:
	PluginSettings Settings;
	// Set by the menu when a setting changes, saved once the edit ends
	bool SettingsModified = false;

	// Reads MGSDelta_config.txt from the profile folder. Written with the defaults if it's missing,
	// or lacks settings added since it was created.
	void Load();
	void Save();

	static std::unordered_map<std::string, WeaponConfig> g_weaponConfigs;
	bool LoadWeaponConfigFromFile(const fs::path& path, WeaponConfig& outCfg);
	void LoadWeaponConfigs(const std::filesystem::path& dir);

private:
	std::filesystem::path configFilePath;
};
