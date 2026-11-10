#include "HookManager.hpp"

#ifndef CPF_OutParm
#define CPF_OutParm 0x0000000000000002
#endif

WeaponManager* HookManager::s_weaponManager = nullptr;
CameraController* HookManager::s_cameraController = nullptr;
PlayerManager* HookManager::s_playerManager = nullptr;

void HookManager::Initialize()
{
	s_weaponManager = weaponManager;
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

void* HookManager::OnFireModded(uevr::API::UObject* equipController, void* fireRequest) {

	s_weaponManager->IsShooting = true;

	static uevr::API::UScriptStruct* fireRequestStruct = nullptr;
	if (fireRequestStruct == nullptr)
		fireRequestStruct = ue::find_struct(L"ScriptStruct /Script/Gsr.GsrFireRequest");

	// Replace the fire and aim positions with the ones computed from the motion controllers
	if (auto firePosition = ue::struct_member<ue::FVector>(fireRequest, fireRequestStruct, L"FirePosition"))
		*firePosition = { s_weaponManager->FirePosition.x, s_weaponManager->FirePosition.y, s_weaponManager->FirePosition.z };

	if (auto aimPosition = ue::struct_member<ue::FVector>(fireRequest, fireRequestStruct, L"AimPosition"))
		*aimPosition = { s_weaponManager->AimPosition.x, s_weaponManager->AimPosition.y, s_weaponManager->AimPosition.z };

	//Call original function with our replaced parameters
	auto result = m_fire_hook_fn(equipController, fireRequest);
	//PLUGIN_LOG_ONCE("Result: 0x%p", result);
	return result;
}

void HookManager::ForwardEnteredState(uevr::API::UFunction* fn, void* locals)
{
	if (locals == nullptr)
		return;

	auto true_locals = (char*)locals - 0x50;
	auto nextStateName = ue::param<ue::FName>(fn, true_locals, L"NextStateName");
	auto stateMachineName = ue::param<ue::FName>(fn, true_locals, L"StateMachineName");
	if (nextStateName && stateMachineName)
		s_playerManager->ProcessPlayerCharacterState(*nextStateName, *stateMachineName);
}

bool HookManager::mod_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{
	//Logger::DebugPrint(obj->get_class()->get_full_name());
	if (obj->is_a(Utilities::ABP_SnakeBasicAction_C) || 
		obj->is_a(Utilities::ABP_PlayerRollingAction_C) || 
		obj->is_a(Utilities::ABP_PlayerBehindAction_C) || 
		obj->is_a(Utilities::ABP_PlayerMeleeAction_C) ||
		obj->is_a(Utilities::ABP_PlayerCarryAction_C))
	{
		ForwardEnteredState(fn, locals);
	}

	return true;
}

bool HookManager::mod_PlayerKnife_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{	
	if (!obj->is_a(Utilities::ABP_PlayerKnifeReadyAction_C))
		return true;

	ForwardEnteredState(fn, locals);

	return true;
}

bool HookManager::mod_PlayerCqcAction_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{	
	if (!obj->is_a(Utilities::ABP_EnemyCqc_C))
		return true;

	ForwardEnteredState(fn, locals);

	return true;
}

bool HookManager::mod_PlayerFalling_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{
	if (!obj->is_a(Utilities::ABP_PlayerEludeAction_C))
		return true;

	ForwardEnteredState(fn, locals);

	return true;
}

bool HookManager::mod_GsrPlayer_OnChangeCamouflageApply_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{
	uevr::API::get()->dispatch_lua_event("HandsReset", "0");
	return true;
}

bool HookManager::mod_CobraLevelSequence_OnCameraCut_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{
	s_cameraController->InstantCameraPositionTimer = 0.05f;
	s_cameraController->InstantCameraPositionNeeded = true;
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