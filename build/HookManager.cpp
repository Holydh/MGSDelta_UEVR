#include "HookManager.hpp"

WeaponController* HookManager::s_weaponController = nullptr;
CameraController* HookManager::s_cameraController = nullptr;

int32_t hook_fn(std::wstring_view class_name, std::wstring_view fn_name, void* destination, void** original, bool findRealCall) {
	auto obj = (uevr::API::UClass*)uevr::API::get()->find_uobject(class_name);

	if (obj == nullptr) {
		PLUGIN_LOG_ONCE_ERROR("Failed to find %ls", class_name.data());
		return -1;
	}

	auto fn = obj->find_function(fn_name);

	if (fn == nullptr) {
		PLUGIN_LOG_ONCE_ERROR("Failed to find %ls", fn_name.data());
		return -1;
	}

	auto native = fn->get_native_function();

	if (native == nullptr) {
		PLUGIN_LOG_ONCE_ERROR("Failed to get native function");
		return -1;
	}

	Logger::DebugPrint("%ls native: 0x%p", fn_name.data(), native);

	if (!findRealCall){
		return uevr::API::get()->param()->functions->register_inline_hook((void*)native, (void*)destination, original);
	}

	auto insn = utility::scan_disasm((uintptr_t)native, 0x1000, "E8 ? ? ? ?");

	if (!insn) {
		PLUGIN_LOG_ONCE_ERROR("Failed to find the instruction");
		Logger::DebugPrint("Not found");
		return -1;
	}

	uintptr_t insn_val = insn.value();

	auto relative_offset = *(int32_t*)(insn_val + 1);

	uintptr_t function_address = (insn_val + 5) + relative_offset;

	Logger::DebugPrint("Function address: 0x%p\n", (void*)function_address);
	return uevr::API::get()->param()->functions->register_inline_hook((void*)function_address, (void*)destination, original);
}

void HookManager::hook_fire_fn() {
	m_fire_hook_id = hook_fn(
		L"Class /Script/Gsr.GsrEquipController", 
		L"Fire", 
		on_get_fire, 
		(void**)&m_fire_hook_fn, 
		true);
}

void* HookManager::on_get_fire_internal(uevr::API::UObject* equipController, FGsrFireRequest* FireRequest) {

	Logger::DebugPrint("Function hooked\n");

	if (equipController != s_weaponController->EquipController)
	{
		Logger::DebugPrint("Not the player equip controller. Do nothing");
	}
	else
	{
		s_weaponController->IsShooting = true;
		s_cameraController->ToggleCameraWobblingController(false);

		struct {
			uevr::API::UObject* value{ nullptr };
		} obj;

		equipController->call_function(L"GetCurrentWeapon", &obj);

		auto weapon = obj.value;

		if (weapon != nullptr) {
			const auto weapon_name = weapon->get_full_name();
			//DebugPrint("Weapon: %ls\n", weapon_name.c_str());

			auto root_component = weapon->get_property<uevr::API::UObject*>(L"RootComponent");
			s_weaponController->CurrentWeapon = root_component;
			if (root_component != nullptr) {
				UEVR_Vector3d forward_params;

				root_component->call_function(L"GetForwardVector", &forward_params);
				//DebugPrint("Forward params: %f, %f, %f\n", forward_params.x, forward_params.y, forward_params.z);

				FTransform muzzle_transform;

				weapon->call_function(L"GetMuzzleTransform", &muzzle_transform);
				auto location_params = muzzle_transform.Translation;
				//DebugPrint("Location params: %f, %f, %f\n", location_params.x, location_params.y, location_params.z);

				FireRequest->FirePosition.x = location_params.x;
				FireRequest->FirePosition.y = location_params.y;
				FireRequest->FirePosition.z = location_params.z;

				float scaler = 8192.0;

				RotateAngleAxis rotate_params;
				static auto kismet_math_library_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Engine.KismetMathLibrary");
				static auto kismet_math_library = kismet_math_library_c->get_class_default_object();

				rotate_params.InVect = forward_params;
				rotate_params.AngleDeg = 90.0f;
				root_component->call_function(L"GetUpVector", &rotate_params.Axis);
				//DebugPrint("Up Vector: %f, %f, %f\n", rotate_params.Axis.x, rotate_params.Axis.y, rotate_params.Axis.z);

				kismet_math_library->call_function(L"RotateAngleAxis", &rotate_params);
				forward_params = rotate_params.Result;
				//DebugPrint("Forward Params: %f, %f, %f\n", forward_params.x, forward_params.y, forward_params.z);

				Logger::DebugPrint("Old Aim Position: %f, %f, %f\n", FireRequest->AimPosition.x, FireRequest->AimPosition.y, FireRequest->AimPosition.z);
				FireRequest->AimPosition.x = location_params.x + (forward_params.x * scaler);
				FireRequest->AimPosition.y = location_params.y + (forward_params.y * scaler) - g_AimOffsetValueHorizontal;
				FireRequest->AimPosition.z = location_params.z + (forward_params.z * scaler) - g_AimOffsetValueVertical;
				Logger::DebugPrint("New Aim Position: %f, %f, %f\n", FireRequest->AimPosition.x, FireRequest->AimPosition.y, FireRequest->AimPosition.z);
			}
		}
	}



	//Call original function with our replaced parameters
	auto result = m_fire_hook_fn(equipController, FireRequest);
	//PLUGIN_LOG_ONCE("Result: 0x%p", result);

	return result;
}

void HookManager::Init()
{
	s_weaponController = weaponController;
	s_cameraController = cameraController;
}

void HookManager::SetGunAdjustValuesFromConfig(const std::filesystem::path& ConfigFile) {
	// Check if the file exists
	if (!std::filesystem::exists(ConfigFile)) {
		g_AimOffsetValueVertical = 0.0f;
		g_AimOffsetValueHorizontal = 0.0f;
		return;
	}

	std::ifstream file(ConfigFile);
	std::string line;

	while (std::getline(file, line)) {
		// Trim leading and trailing spaces
		line.erase(0, line.find_first_not_of(" \t"));
		line.erase(line.find_last_not_of(" \t") + 1);

		// Skip comments and blank lines
		if (line.empty() || line[0] == '#') {
			continue;
		}

		// Look for the "GunAdjustV=" prefix
		if (line.rfind("GunAdjustV=", 0) == 0) {
			std::string value_str = line.substr(11); // Extract the value as a string
			char* end_ptr = nullptr;
			float value = std::strtof(value_str.c_str(), &end_ptr);

			// Validate conversion
			if (end_ptr != value_str.c_str() && *end_ptr == '\0') {
				g_AimOffsetValueVertical = value; // Successfully parsed float
			}
			else {
				g_AimOffsetValueVertical = 0.0f; // Invalid float
			}
		}

		// Look for the "GunAdjustH=" prefix
		if (line.rfind("GunAdjustH=", 0) == 0) {
			std::string value_str = line.substr(11); // Extract the value as a string
			char* end_ptr = nullptr;
			float value = std::strtof(value_str.c_str(), &end_ptr);

			// Validate conversion
			if (end_ptr != value_str.c_str() && *end_ptr == '\0') {
				g_AimOffsetValueHorizontal = value; // Successfully parsed float
			}
			else {
				g_AimOffsetValueHorizontal = 0.0f; // Invalid float
			}
		}
	}
}