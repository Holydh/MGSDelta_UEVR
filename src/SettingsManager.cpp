#include "SettingsManager.hpp"

#include <algorithm>
#include <charconv>
#include <cstring>
#include <variant>

std::unordered_map<std::string, WeaponConfig> SettingsManager::g_weaponConfigs{};

bool SettingsManager::LoadWeaponConfigFromFile(const fs::path& path, WeaponConfig& outCfg)
{
	std::ifstream ifs(path);
    if (!ifs.is_open())
    {
        Logger::DebugPrint("Failed to open %s", path.string().c_str());
        return false;
    }

    json j;
    try
    {
        ifs >> j;
    }
    catch (const std::exception& e)
    {
        Logger::DebugPrint("JSON parse error in %s : %s", path.string().c_str(), e.what());
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

void SettingsManager::LoadWeaponConfigs(const std::filesystem::path& dir)
{
    g_weaponConfigs.clear();

    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir))
    {
		Logger::DebugPrint("Config folder not found: %s", dir.string().c_str());
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
            Logger::DebugPrint("Loaded config for %s", key.c_str());
        }
    }
}

namespace {
using SettingMember = std::variant<bool PluginSettings::*, int PluginSettings::*, float PluginSettings::*>;

// Saved as "category.key=value", sorted by category: a new setting can go anywhere in the table
struct SettingField
{
	const char* category;
	const char* key;
	SettingMember member;
};

const SettingField settingFields[] = {
	{ "Camera", "attachCameraToSnakeHead", &PluginSettings::attachCameraToSnakeHead },
	{ "Camera", "cameraRotationSpeed", &PluginSettings::cameraRotationSpeed },
	{ "Camera", "cameraRotationDeadZone", &PluginSettings::cameraRotationDeadZone },
	{ "Camera", "smoothCameraRotation", &PluginSettings::smoothCameraRotation },
	{ "Camera", "cameraYawLerp", &PluginSettings::cameraYawLerp },
	{ "Camera", "smoothCameraPosition", &PluginSettings::smoothCameraPosition },
	{ "Camera", "cameraPositionXYLerp", &PluginSettings::cameraPositionXYLerp },
	{ "Camera", "cameraPositionZLerp", &PluginSettings::cameraPositionZLerp },

	{ "Gameplay", "enableRoomscaleProneAndCrouch", &PluginSettings::enableRoomscaleProneAndCrouch },
	{ "Gameplay", "linearLocomotion", &PluginSettings::linearLocomotion },
	{ "Gameplay", "gameTakesMovementControl", &PluginSettings::gameTakesMovementControl },
	{ "Gameplay", "squatUnderThisValue", &PluginSettings::squatUnderThisValue },
	{ "Gameplay", "crawlUnderThisValue", &PluginSettings::crawlUnderThisValue },

	{ "Weapons", "enableWeaponsScopes", &PluginSettings::enableWeaponsScopes },
	{ "Weapons", "scopeResolution", &PluginSettings::scopeResolution },
	{ "Weapons", "disableAllRecoil", &PluginSettings::disableAllRecoil },
	{ "Weapons", "enableTwoHandsPivot", &PluginSettings::enableTwoHandsPivot },
	{ "Weapons", "enableTwoHandsRealisticRecoil", &PluginSettings::enableTwoHandsRealisticRecoil },

	{ "Items", "disableThermalVisionScanline", &PluginSettings::disableThermalVisionScanline },

	{ "Cutscenes", "enableImmersiveMode", &PluginSettings::enableImmersiveMode },
	{ "Cutscenes", "blackBarsSize", &PluginSettings::blackBarsSize },
	{ "Cutscenes", "smoothCameraMovementsInImmersiveCutscenes", &PluginSettings::smoothCameraMovementsInImmersiveCutscenes },
	{ "Cutscenes", "cutsceneCameraPositionLerp", &PluginSettings::cutsceneCameraPositionLerp },
	{ "Cutscenes", "cutsceneCameraRotationLerp", &PluginSettings::cutsceneCameraRotationLerp },

	{ "Rendering", "enable3DCutscenes", &PluginSettings::enable3DCutscenes },
	{ "Rendering", "enable3DRadio", &PluginSettings::enable3DRadio },
	{ "Rendering", "enable3DSurvivalView", &PluginSettings::enable3DSurvivalView },
};

std::string trim(const std::string& text)
{
	const size_t first = text.find_first_not_of(" \t");
	if (first == std::string::npos)
		return {};
	return text.substr(first, text.find_last_not_of(" \t") - first + 1);
}

bool parseValue(const std::string& text, bool& out)
{
	if (_stricmp(text.c_str(), "true") == 0) { out = true; return true; }
	if (_stricmp(text.c_str(), "false") == 0) { out = false; return true; }
	return false;
}

// int and float: the whole text must be a number
template <typename T>
bool parseValue(const std::string& text, T& out)
{
	const char* end = text.data() + text.size();
	const auto [ptr, error] = std::from_chars(text.data(), end, out);
	return error == std::errc{} && ptr == end;
}
}

void SettingsManager::Load()
{
	configFilePath = uevr::API::get()->get_persistent_dir() / "MGSDelta_config.txt";
	Logger::DebugPrint("%s", configFilePath.string().c_str());

	// key -> value text
	std::unordered_map<std::string, std::string> values;
	std::ifstream file(configFilePath);
	std::string line;
	while (std::getline(file, line))
	{
		const size_t equalPos = line.find('=');
		if (line.empty() || line[0] == '#' || equalPos == std::string::npos)
			continue;
		values[trim(line.substr(0, equalPos))] = trim(line.substr(equalPos + 1));
	}

	bool complete = true;
	for (const auto& field : settingFields)
	{
		auto it = values.find(std::string{ field.category } + '.' + field.key);
		if (it == values.end())
		{
			complete = false;
			// Files written before the keys had their category
			it = values.find(field.key);
			if (it == values.end())
				continue;
		}
		const bool valid = std::visit([&](auto member) { return parseValue(it->second, Settings.*member); }, field.member);
		if (!valid)
			Logger::DebugPrint("Invalid value for %s.%s in MGSDelta_config.txt: %s", field.category, field.key, it->second.c_str());
	}

	savedSettings = Settings;
	// First start, settings added by a plugin update (written with their defaults), or old key format
	if (!complete)
		Save();
}

void SettingsManager::Save()
{
	savedSettings = Settings; // not retried if the file can't be written
	std::ofstream file(configFilePath, std::ios::trunc);
	if (!file.is_open())
	{
		Logger::DebugPrint("Failed to write %s", configFilePath.string().c_str());
		return;
	}

	// Sorted by category, settings of a category in table order
	std::vector<const SettingField*> fields;
	for (const auto& field : settingFields)
		fields.push_back(&field);
	std::stable_sort(fields.begin(), fields.end(),
		[](const SettingField* a, const SettingField* b) { return std::strcmp(a->category, b->category) < 0; });

	file << "# MGSDelta Plugin Configuration File\n"
		"# This file is auto-generated. It saves the mgs plugin menu settings you set ingame.\n"
		<< std::boolalpha;
	const char* category = "";
	for (const auto* field : fields)
	{
		if (std::strcmp(field->category, category) != 0)
		{
			category = field->category;
			file << '\n';
		}
		file << field->category << '.' << field->key << '=';
		std::visit([&](auto member) { file << Settings.*member; }, field->member);
		file << '\n';
	}
	Logger::DebugPrint("Config change saved in MGSDelta_config.txt");
}
