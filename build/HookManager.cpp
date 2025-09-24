#include "HookManager.hpp"

WeaponController* HookManager::s_weaponController = nullptr;
CameraController* HookManager::s_cameraController = nullptr;

void HookManager::Initialize()
{
	s_weaponController = weaponController;
	s_cameraController = cameraController;
}

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

void HookManager::HookFireFunction() {
	m_fire_hook_id = hook_fn(
		L"Class /Script/Gsr.GsrEquipController", 
		L"Fire", 
		OnFire, 
		(void**)&m_fire_hook_fn, 
		true);
}

void* HookManager::OnFireModded(uevr::API::UObject* equipController, Utilities::FGsrFireRequest* FireRequest) {

	Logger::DebugPrint("OnFireHook");
	//Logger::DebugPrint("%d", s_weaponController->IsShooting);
	s_weaponController->IsShooting = true;
	//s_cameraController->ToggleCameraWobblingController(false);
	//Logger::DebugPrint("%f", s_weaponController->AimPosition.x);
	FireRequest->FirePosition.x = s_weaponController->FirePosition.x;
	FireRequest->FirePosition.y = s_weaponController->FirePosition.y;
	FireRequest->FirePosition.z = s_weaponController->FirePosition.z;

	//Logger::DebugPrint("s_weaponController->AimPosition.x.x : %f, s_weaponController->AimPosition.x.y : %f, s_weaponController->AimPosition.x.z : %f", 
	//	s_weaponController->AimPosition.x, s_weaponController->AimPosition.y, s_weaponController->AimPosition.z);
	Logger::DebugPrint("Old Aim Position: %f, %f, %f", FireRequest->AimPosition.x, FireRequest->AimPosition.y, FireRequest->AimPosition.z);
	FireRequest->AimPosition.x = s_weaponController->AimPosition.x;
	FireRequest->AimPosition.y = s_weaponController->AimPosition.y;
	FireRequest->AimPosition.z = s_weaponController->AimPosition.z;
	Logger::DebugPrint("New Aim Position: %f, %f, %f", FireRequest->AimPosition.x, FireRequest->AimPosition.y, FireRequest->AimPosition.z);


	//Call original function with our replaced parameters
	auto result = m_fire_hook_fn(equipController, FireRequest);
	//PLUGIN_LOG_ONCE("Result: 0x%p", result);
	return result;
}

//void HookManager::hook_ISetSmoothedAimPosition_fn() {
//	m_ISetSmoothedAimPosition_hook_id = hook_fn(
//		L"Class /Script/Gsr.GsrPlayerCameraInfoInterface", 
//		L"ISetCameraRotation", 
//		on_ISetSmoothedAimPosition, 
//		(void**)&m_ISetSmoothedAimPosition_hook_fn, 
//		false);
//}
//
//void* HookManager::on_ISetSmoothedAimPosition_internal(uevr::API::UObject* self, UEVR_Rotatord* rotation) {
//	//Logger::DebugPrint("Function hooked\n");
//	//position->x = 100.0f;
//	//position->y = 1800.0f;
//	//position->z = 400.0f;
//	Logger::DebugPrint("%f", rotation->pitch);
//	auto result = m_ISetSmoothedAimPosition_hook_fn(self, rotation);
//	return result;
//}

//bool HookManager::mod_ISetCameraRotation_pre(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
//{
//	Logger::DebugPrint("fuck ye");
//
//	if (result != nullptr)
//	{
//		struct rotatortest {
//			UEVR_Rotatord* Rotation;
//		};
//		rotatortest* params = (rotatortest*)result;
//		Logger::DebugPrint("Result : %f", params->Rotation->pitch);
//		//params->Rotation->pitch = 180.0f;
//	}
//
//	if (locals != nullptr)
//	{
//		struct rotatortest {
//			UEVR_Rotatord* Rotation;
//		};
//		rotatortest* params = (rotatortest*)locals;
//		Logger::DebugPrint("Locals : %p", locals);
//		//params->Rotation->pitch = 180.0f;
//	}
//	else
//	{
//		uevr::API::get()->log_info("mod_GetCurrentAimPitch_post - locals is null");
//	}
//	return false;
//	//API::get()->log_info("mod_GetCurrentAimPitch_post returning false");
//}
//
//bool HookManager::hook_bp_fn(std::wstring_view class_name, std::wstring_view fn_name, UEVR_UFunction_NativePreFn pre, UEVR_UFunction_NativePostFn post, bool use_native)
//{
//	Logger::DebugPrint("Entering hook_bp_fn");
//	auto obj = (uevr::API::UClass*)uevr::API::get()->find_uobject(class_name);
//
//	if (obj == nullptr) {
//		Logger::DebugPrint("Failed to find %ls", class_name.data());
//		return false;
//	}
//
//	auto fn = obj->find_function(fn_name);
//
//	if (fn == nullptr) {
//		Logger::DebugPrint("Failed to find %ls", fn_name.data());
//		return false;
//	}
//
//	Logger::DebugPrint("getting function flags");
//	uint32_t flags = fn->get_function_flags();
//
//	if (use_native)
//	{
//		flags = flags | 0x400;
//	}
//	else
//	{
//		flags = flags & ~0x400;
//	}
//
//	Logger::DebugPrint("Setting function flags to 0x%08x", flags);
//	fn->set_function_flags(flags);
//
//	Logger::DebugPrint("Calling hook_ptr for %ls", fn_name.data());
//	return uevr::API::get()->param()->sdk->ufunction->hook_ptr((UEVR_UFunctionHandle)fn, pre, post);
//}





//void HookManager::SetGunAdjustValuesFromConfig(const std::filesystem::path& ConfigFile) {
//	// Check if the file exists
//	if (!std::filesystem::exists(ConfigFile)) {
//		g_AimOffsetValueVertical = 0.0f;
//		g_AimOffsetValueHorizontal = 0.0f;
//		return;
//	}
//
//	std::ifstream file(ConfigFile);
//	std::string line;
//
//	while (std::getline(file, line)) {
//		// Trim leading and trailing spaces
//		line.erase(0, line.find_first_not_of(" \t"));
//		line.erase(line.find_last_not_of(" \t") + 1);
//
//		// Skip comments and blank lines
//		if (line.empty() || line[0] == '#') {
//			continue;
//		}
//
//		// Look for the "GunAdjustV=" prefix
//		if (line.rfind("GunAdjustV=", 0) == 0) {
//			std::string value_str = line.substr(11); // Extract the value as a string
//			char* end_ptr = nullptr;
//			float value = std::strtof(value_str.c_str(), &end_ptr);
//
//			// Validate conversion
//			if (end_ptr != value_str.c_str() && *end_ptr == '\0') {
//				g_AimOffsetValueVertical = value; // Successfully parsed float
//			}
//			else {
//				g_AimOffsetValueVertical = 0.0f; // Invalid float
//			}
//		}
//
//		// Look for the "GunAdjustH=" prefix
//		if (line.rfind("GunAdjustH=", 0) == 0) {
//			std::string value_str = line.substr(11); // Extract the value as a string
//			char* end_ptr = nullptr;
//			float value = std::strtof(value_str.c_str(), &end_ptr);
//
//			// Validate conversion
//			if (end_ptr != value_str.c_str() && *end_ptr == '\0') {
//				g_AimOffsetValueHorizontal = value; // Successfully parsed float
//			}
//			else {
//				g_AimOffsetValueHorizontal = 0.0f; // Invalid float
//			}
//		}
//	}
//}