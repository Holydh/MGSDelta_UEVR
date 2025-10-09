#include "HookManager.hpp"

#ifndef CPF_OutParm
#define CPF_OutParm 0x0000000000000002
#endif

WeaponController* HookManager::s_weaponController = nullptr;
CameraController* HookManager::s_cameraController = nullptr;
PlayerManager* HookManager::s_playerManager = nullptr;

void HookManager::Initialize()
{
	s_weaponController = weaponController;
	s_cameraController = cameraController;
	s_playerManager = playerManager;
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

	if (!findRealCall) {
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

void* HookManager::OnFireModded(uevr::API::UObject* equipController, SDK::FGsrFireRequest* FireRequest) {

	Logger::DebugPrint("OnFireHook");
	//Logger::DebugPrint("%d", s_weaponController->IsShooting);
	s_weaponController->IsShooting = true;
	//s_cameraController->ToggleCameraWobblingController(false);
	//Logger::DebugPrint("%f", s_weaponController->AimPosition.x);
	FireRequest->FirePosition.X = s_weaponController->FirePosition.x;
	FireRequest->FirePosition.Y = s_weaponController->FirePosition.y;
	FireRequest->FirePosition.Z = s_weaponController->FirePosition.z;

	//Logger::DebugPrint("s_weaponController->AimPosition.x.x : %f, s_weaponController->AimPosition.x.y : %f, s_weaponController->AimPosition.x.z : %f", 
	//	s_weaponController->AimPosition.x, s_weaponController->AimPosition.y, s_weaponController->AimPosition.z);
	Logger::DebugPrint("Old Aim Position: %f, %f, %f", FireRequest->AimPosition.X, FireRequest->AimPosition.Y, FireRequest->AimPosition.Z);
	FireRequest->AimPosition.X = s_weaponController->AimPosition.x;
	FireRequest->AimPosition.Y = s_weaponController->AimPosition.y;
	FireRequest->AimPosition.Z = s_weaponController->AimPosition.z;
	Logger::DebugPrint("New Aim Position: %f, %f, %f", FireRequest->AimPosition.X, FireRequest->AimPosition.Y, FireRequest->AimPosition.Z);


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

//void* HookManager::on_ISetSmoothedAimPosition_internal(uevr::API::UObject* self, UEVR_Rotatord* rotation) {
//	//Logger::DebugPrint("Function hooked\n");
//	//position->x = 100.0f;
//	//position->y = 1800.0f;
//	//position->z = 400.0f;
//	Logger::DebugPrint("%f", rotation->pitch);
//	auto result = m_ISetSmoothedAimPosition_hook_fn(self, rotation);
//	return result;
//}

bool HookManager::mod_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{
	if (!obj->is_a(Utilities::ABP_SnakeBasicAction_C))
		return true;


	if (locals != nullptr)
	{
		auto true_locals = (char*)locals - 0x50;
		SDK::Params::IcsAnimInstance_OnEnteredState* params = (SDK::Params::IcsAnimInstance_OnEnteredState*)true_locals;
		//s_playerManager->debugParamsTest.NextStateName = params->NextStateName;
		//s_playerManager->debugParamsTest.StateMachineName = params->StateMachineName;
		s_playerManager->ProcessPlayerCharacterState(params->NextStateName, params->StateMachineName);
	}
	return true;
}

bool HookManager::mod_PlayerFalling_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{
	if (!obj->is_a(Utilities::ABP_PlayerEludeAction_C))
		return true;

	if (locals != nullptr)
	{
		auto true_locals = (char*)locals - 0x50;
		SDK::Params::IcsAnimInstance_OnEnteredState* params = (SDK::Params::IcsAnimInstance_OnEnteredState*)true_locals;
		//s_playerManager->debugParamsTest.NextStateName = params->NextStateName;
		//s_playerManager->debugParamsTest.StateMachineName = params->StateMachineName;
		//Logger::DebugPrint("PlayerEludeAction StateMachineName: %s", s_playerManager->debugParamsTest.StateMachineName.ToString().c_str());
		//Logger::DebugPrint("PlayerEludeAction NextStateName: %s", s_playerManager->debugParamsTest.NextStateName.ToString().c_str());
		s_playerManager->ProcessPlayerCharacterState(params->NextStateName, params->StateMachineName);
	}
	return true;
}

bool HookManager::hook_bp_fn(std::wstring_view class_name, std::wstring_view fn_name, UEVR_UFunction_NativePreFn pre, UEVR_UFunction_NativePostFn post, bool use_native)
{
	Logger::DebugPrint("Entering hook_bp_fn");
	auto obj = (uevr::API::UClass*)uevr::API::get()->find_uobject(class_name);

	if (obj == nullptr) {
		Logger::DebugPrint("Failed to find %ls", class_name.data());
		return false;
	}

	auto fn = obj->find_function(fn_name);

	if (fn == nullptr) {
		Logger::DebugPrint("Failed to find %ls", fn_name.data());
		return false;
	}

	Logger::DebugPrint("getting function flags");
	uint32_t flags = fn->get_function_flags();

	if (use_native)
	{
		flags = flags | 0x400;
	}
	else
	{
		flags = flags & ~0x400;
	}

	Logger::DebugPrint("Setting function flags to 0x%08x", flags);
	fn->set_function_flags(flags);
	
	Logger::DebugPrint("Calling hook_ptr for %ls", fn_name.data());
	return uevr::API::get()->param()->sdk->ufunction->hook_ptr((UEVR_UFunctionHandle)fn, pre, post);
}