#include "HookManager.hpp"

WeaponManager* HookManager::s_weaponManager = nullptr;
CameraController* HookManager::s_cameraController = nullptr;
PlayerManager* HookManager::s_playerManager = nullptr;
VRHands* HookManager::s_vrHands = nullptr;
Locomotion* HookManager::s_locomotion = nullptr;

void HookManager::Initialize()
{
	s_weaponManager = weaponManager;
	s_cameraController = cameraController;
	s_playerManager = playerManager;
	s_vrHands = vrHands;
	s_locomotion = locomotion;
}


// findRealCall: hooks the function called by the exec thunk instead, at callOffset from the first instruction
// matching realCallPattern (one instruction: scan_disasm compares the pattern within each instruction)
int32_t hook_fn(std::wstring_view class_name, std::wstring_view fn_name, void* destination, void** original, bool findRealCall,
	const char* realCallPattern = "E8 ? ? ? ?", uintptr_t callOffset = 0) {
	auto obj = (uevr::API::UClass*)uevr::API::get()->find_uobject(class_name);

	if (obj == nullptr) {
		uevr::API::get()->log_error("[MGSDelta] Hook: class %ls not found", class_name.data());
		return -1;
	}

	auto fn = obj->find_function(fn_name);

	if (fn == nullptr) {
		uevr::API::get()->log_error("[MGSDelta] Hook: function %ls not found", fn_name.data());
		return -1;
	}

	auto native = fn->get_native_function();

	if (native == nullptr) {
		uevr::API::get()->log_error("[MGSDelta] Hook: no native function for %ls", fn_name.data());
		return -1;
	}

	Logger::DebugPrint("%ls native: 0x%p", fn_name.data(), native);

	if (!findRealCall) {
		return uevr::API::get()->param()->functions->register_inline_hook((void*)native, (void*)destination, original);
	}

	auto insn = utility::scan_disasm((uintptr_t)native, 0x1000, realCallPattern);

	if (!insn) {
		uevr::API::get()->log_error("[MGSDelta] Hook: instruction pattern not found in %ls", fn_name.data());
		return -1;
	}

	uintptr_t insn_val = insn.value() + callOffset;

	if (*(uint8_t*)insn_val != 0xE8) {
		uevr::API::get()->log_error("[MGSDelta] Hook: no call after the instruction found for %ls", fn_name.data());
		return -1;
	}

	auto relative_offset = *(int32_t*)(insn_val + 1);

	uintptr_t function_address = (insn_val + 5) + relative_offset;

	Logger::DebugPrint("Function address: 0x%p\n", (void*)function_address);
	return uevr::API::get()->param()->functions->register_inline_hook((void*)function_address, (void*)destination, original);
}

void HookManager::HookFireFunction() {
	hook_fn(
		L"Class /Script/Gsr.GsrEquipController",
		L"Fire",
		OnFire,
		(void**)&m_fire_hook_fn,
		true);
}

void HookManager::HookSetEnableWallCollision() {
	// The thunk's first call is the parameter reading: the real setter is the call after "setnz dl"
	hook_fn(
		L"Class /Script/Ics.IcsCharacter",
		L"SetEnableWallCollision",
		OnSetEnableWallCollision,
		(void**)&m_set_enable_wall_collision_fn,
		true,
		"0F 95 C2",
		3);
}

void HookManager::OnSetEnableWallCollision(uevr::API::UObject* character, bool enable) {
	m_set_enable_wall_collision_fn(character, s_locomotion->FilterWallCollision(character, enable));
}

void* HookManager::OnFireModded(uevr::API::UObject* equipController, void* fireRequest) {

	s_weaponManager->Recoil.IsShooting = true;

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
	return result;
}

void HookManager::ForwardEnteredState(uevr::API::UFunction* fn, void* locals)
{
	if (locals == nullptr)
		return;

	// `locals` is the call's FFrame (UEVR passes the frame), not the parameters. The parameters were found
	// 0x50 bytes before it, empirically: most likely ProcessEvent's stack copy of them (what FFrame::Locals points to).
	constexpr ptrdiff_t paramsOffset = 0x50;
	auto params = (char*)locals - paramsOffset;
	auto nextStateName = ue::param<ue::FName>(fn, params, L"NextStateName");
	auto stateMachineName = ue::param<ue::FName>(fn, params, L"StateMachineName");
	if (nextStateName && stateMachineName)
		s_playerManager->ProcessPlayerCharacterState(*nextStateName, *stateMachineName);
}

bool HookManager::mod_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{
	if (ue::is_a(obj, Utilities::ABP_SnakeBasicAction_C) || 
		ue::is_a(obj, Utilities::ABP_PlayerRollingAction_C) || 
		ue::is_a(obj, Utilities::ABP_PlayerBehindAction_C) || 
		ue::is_a(obj, Utilities::ABP_PlayerMeleeAction_C) ||
		ue::is_a(obj, Utilities::ABP_PlayerCarryAction_C))
	{
		ForwardEnteredState(fn, locals);
	}

	return true;
}

bool HookManager::mod_PlayerKnife_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{	
	if (!ue::is_a(obj, Utilities::ABP_PlayerKnifeReadyAction_C))
		return true;

	ForwardEnteredState(fn, locals);

	return true;
}

bool HookManager::mod_PlayerCqcAction_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{	
	if (!ue::is_a(obj, Utilities::ABP_EnemyCqc_C))
		return true;

	ForwardEnteredState(fn, locals);

	return true;
}

bool HookManager::mod_PlayerFalling_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{
	if (!ue::is_a(obj, Utilities::ABP_PlayerEludeAction_C))
		return true;

	ForwardEnteredState(fn, locals);

	return true;
}

bool HookManager::mod_GsrPlayer_OnChangeCamouflageApply_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{
	// The Lua script destroys the VR hands and creates new ones: forget the current ones now
	s_vrHands->Cleanup();
	uevr::API::get()->dispatch_lua_event("HandsReset", "0");
	return true;
}

bool HookManager::mod_CobraLevelSequence_OnCameraCut_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{
	s_cameraController->InstantCameraPositionTimer = 0.05f;
	s_cameraController->InstantCameraPositionNeeded = true;
	return true;
}

bool HookManager::mod_IcsActorActionModule_OnOverrideRootMotionVelocity_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{
	if (result == nullptr || obj == nullptr || obj != s_locomotion->BasicAction.get() || !s_locomotion->LinearLocomotionActive)
		return true;

	// Z kept from the game (root motion is horizontal while walking)
	auto velocity = static_cast<ue::FVector*>(result);
	velocity->X = s_locomotion->LinearLocomotionVelocity.X;
	velocity->Y = s_locomotion->LinearLocomotionVelocity.Y;
	return true;
}

bool HookManager::hook_bp_fn(std::wstring_view class_name, std::wstring_view fn_name, UEVR_UFunction_NativePreFn pre, UEVR_UFunction_NativePostFn post, bool use_native)
{
	auto& hook = m_bp_hooks.emplace_back(BlueprintHook{ std::wstring{ class_name }, std::wstring{ fn_name }, pre, post, use_native });
	return apply_bp_hook(hook);
}

void HookManager::RetryFailedHooks()
{
	for (auto& hook : m_bp_hooks)
	{
		if (!hook.hooked_function)
			apply_bp_hook(hook);
	}
}

bool HookManager::apply_bp_hook(BlueprintHook& hook)
{
	Logger::DebugPrint("Entering hook_bp_fn");
	auto obj = (uevr::API::UClass*)uevr::API::get()->find_uobject(hook.class_name);

	if (obj == nullptr) {
		Logger::DebugPrint("Failed to find %ls", hook.class_name.c_str());
		return false;
	}

	auto fn = obj->find_function(hook.fn_name);

	if (fn == nullptr) {
		Logger::DebugPrint("Failed to find %ls", hook.fn_name.c_str());
		return false;
	}

	Logger::DebugPrint("getting function flags");
	uint32_t flags = fn->get_function_flags();

	if (hook.use_native)
	{
		flags = flags | 0x400;
	}
	else
	{
		flags = flags & ~0x400;
	}

	Logger::DebugPrint("Setting function flags to 0x%08x", flags);
	fn->set_function_flags(flags);
	
	Logger::DebugPrint("Calling hook_ptr for %ls", hook.fn_name.c_str());
	const bool hooked = uevr::API::get()->param()->sdk->ufunction->hook_ptr((UEVR_UFunctionHandle)fn, hook.pre, hook.post);
	if (hooked)
		hook.hooked_function = fn;
	return hooked;
}
