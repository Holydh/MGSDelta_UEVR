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

void* HookManager::OnFireModded(uevr::API::UObject* equipController, SDK::FGsrFireRequest* fireRequest) {

	//Logger::DebugPrint("OnFireHook");
	//Logger::DebugPrint("%d", s_weaponController->IsShooting);
	s_weaponManager->IsShooting = true;
	//s_cameraController->ToggleCameraWobblingController(false);
	//Logger::DebugPrint("%f", s_weaponController->AimPosition.x);
	//Logger::DebugPrint("FireRequest->FirePosition.X : %f, FireRequest->FirePosition.Y : %f, FireRequest->FirePosition.Z : %f", FireRequest->FirePosition.X, FireRequest->FirePosition.Y, FireRequest->FirePosition.Z);

	fireRequest->FirePosition.X = s_weaponManager->FirePosition.x;
	fireRequest->FirePosition.Y = s_weaponManager->FirePosition.y;
	fireRequest->FirePosition.Z = s_weaponManager->FirePosition.z;

	//Logger::DebugPrint("s_weaponController->AimPosition.x.x : %f, s_weaponController->AimPosition.x.y : %f, s_weaponController->AimPosition.x.z : %f", 
	//	s_weaponController->AimPosition.x, s_weaponController->AimPosition.y, s_weaponController->AimPosition.z);
	//Logger::DebugPrint("Old Aim Position: %f, %f, %f", FireRequest->AimPosition.X, FireRequest->AimPosition.Y, FireRequest->AimPosition.Z);
	fireRequest->AimPosition.X = s_weaponManager->AimPosition.x;
	fireRequest->AimPosition.Y = s_weaponManager->AimPosition.y;
	fireRequest->AimPosition.Z = s_weaponManager->AimPosition.z;
	//Logger::DebugPrint("New Aim Position: %f, %f, %f", FireRequest->AimPosition.X, FireRequest->AimPosition.Y, FireRequest->AimPosition.Z);


	//Call original function with our replaced parameters
	auto result = m_fire_hook_fn(equipController, fireRequest);
	//PLUGIN_LOG_ONCE("Result: 0x%p", result);
	return result;
}

//void HookManager::HookCameraModifyFunction() {
//	m_camera_modify_hook_id = hook_fn(
//		L"Class /Script/GameplayCameras.CameraAnimationCameraModifier",
//		L"BlueprintModifyCamera",
//		OnCameraModify,
//		(void**)&m_camera_modify_hook_fn,
//		false);
//}
//
//void* HookManager::OnCameraModifyModded(uevr::API::UObject* cameraModifier, float* deltaTime, SDK::FVector* viewLocation, SDK::FRotator* viewRotation, float* FOV, SDK::FVector* newViewLocation, SDK::FRotator* newViewRotation, float* newFOV) {
//
//	//Logger::DebugPrint("newFOV : 0x%p", newFOV);
//	/*if (cameraModifier->is_a(Utilities::ABP_SnakeBasicAction_C))*/
//
//	//Call original function with our replaced parameters
//	auto result = m_camera_modify_hook_fn(cameraModifier, deltaTime, viewLocation, viewRotation, FOV, newViewLocation, newViewRotation, newFOV);
//	//PLUGIN_LOG_ONCE("Result: 0x%p", result);
//	return result;
//}

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
	//Logger::DebugPrint(obj->get_class()->get_full_name());
	if (obj->is_a(Utilities::ABP_SnakeBasicAction_C) || 
		obj->is_a(Utilities::ABP_PlayerRollingAction_C) || 
		obj->is_a(Utilities::ABP_PlayerBehindAction_C) || 
		obj->is_a(Utilities::ABP_PlayerMeleeAction_C) ||
		obj->is_a(Utilities::ABP_PlayerCarryAction_C))
	{
		if (locals != nullptr)
		{
			auto true_locals = (char*)locals - 0x50;
			SDK::Params::IcsAnimInstance_OnEnteredState* params = (SDK::Params::IcsAnimInstance_OnEnteredState*)true_locals;
			//SDK::FName nextState = params->NextStateName;
			//SDK::FName StateMachine = params->StateMachineName;
			//Logger::DebugPrint("State Machine : %s", StateMachine.ToString().c_str());
			//Logger::DebugPrint("Next state : %s", nextState.ToString().c_str());
			s_playerManager->ProcessPlayerCharacterState(params->NextStateName, params->StateMachineName);
		}
	}

	return true;
}

bool HookManager::mod_PlayerKnife_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{	
	if (!obj->is_a(Utilities::ABP_PlayerKnifeReadyAction_C))
		return true;

	if (locals != nullptr)
	{
		auto true_locals = (char*)locals - 0x50;
		SDK::Params::IcsAnimInstance_OnEnteredState* params = (SDK::Params::IcsAnimInstance_OnEnteredState*)true_locals;
		//SDK::FName nextState = params->NextStateName;
		//SDK::FName StateMachine = params->StateMachineName;
		//Logger::DebugPrint("State Machine : %s", StateMachine.ToString().c_str());
		//Logger::DebugPrint("Next state : %s",nextState.ToString().c_str());
		s_playerManager->ProcessPlayerCharacterState(params->NextStateName, params->StateMachineName);
	}

	return true;
}

bool HookManager::mod_PlayerCqcAction_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{	
	if (!obj->is_a(Utilities::ABP_EnemyCqc_C))
		return true;

	if (locals != nullptr)
	{
		auto true_locals = (char*)locals - 0x50;
		SDK::Params::IcsAnimInstance_OnEnteredState* params = (SDK::Params::IcsAnimInstance_OnEnteredState*)true_locals;
		//SDK::FName nextState = params->NextStateName;
		//SDK::FName StateMachine = params->StateMachineName;
		//Logger::DebugPrint("State Machine : %s", StateMachine.ToString().c_str());
		//Logger::DebugPrint("Next state : %s",nextState.ToString().c_str());
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

bool HookManager::mod_GsrPlayer_OnChangeCamouflageApply_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{
	uevr::API::get()->dispatch_lua_event("HandsReset", "0");
	return true;
}

bool HookManager::mod_GsrPlayer_OnStartUIMenu_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{
	s_cameraController->SmallUIOpen = true;
	return true;
}

bool HookManager::mod_GsrPlayer_OnEndUIMenu_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{
	s_cameraController->SmallUIOpen = false;
	return true;
}

bool HookManager::mod_BP_GsrPlayerDamageFilterController_OnStartUIMenu_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{
	s_cameraController->IsMenuOpen = true;
	Logger::DebugPrint("Menu Opened");
	return true;
}

bool HookManager::mod_BP_GsrPlayerDamageFilterController_OnEndUIMenu_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{
	s_cameraController->IsMenuOpen = false;
	Logger::DebugPrint("Menu Closed");
	return true;
}

//bool HookManager::mod_CameraModifier_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
//{
//	//if (locals != nullptr)
//	//{
//	//	//auto true_locals = (char*)locals - 0x50;
//	//	//SDK::Params::CameraModifier_BlueprintModifyCamera *params = (SDK::Params::CameraModifier_BlueprintModifyCamera*)true_locals;
//	//	//auto test = params->NewFOV;
//	//	/*Logger::DebugPrint("FUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU");*/
//	//	/*Logger::DebugPrint("New FOV : 0x%p", locals);*/
//	//	//s_playerManager->debugParamsTest.NextStateName = params->NextStateName;
//	//	//s_playerManager->debugParamsTest.StateMachineName = params->StateMachineName;
//	//	//Logger::DebugPrint("PlayerEludeAction StateMachineName: %s", s_playerManager->debugParamsTest.StateMachineName.ToString().c_str());
//	//	//Logger::DebugPrint("PlayerEludeAction NextStateName: %s", s_playerManager->debugParamsTest.NextStateName.ToString().c_str());
//	//	
//	//}
//
//	return true;
//}

//bool HookManager::mod_SetCameraRotation_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
//{
//	if (locals != nullptr)
//	{
//		auto true_locals = (char*)locals - 0x50;
//		SDK::Params::GsrPlayerCameraController_SetCameraRotation* params = (SDK::Params::GsrPlayerCameraController_SetCameraRotation*)true_locals;
//		auto cameraAttachementActor = (SDK::AActor*)s_cameraController->CameraAttachementActor;
//		params->EndRotation = cameraAttachementActor->K2_GetActorRotation();
//		Logger::DebugPrint("%f, %f, %f", params->EndRotation.Pitch, params->EndRotation.Yaw, params->EndRotation.Roll);
//		//s_playerManager->debugParamsTest.NextStateName = params->NextStateName;
//		//s_playerManager->debugParamsTest.StateMachineName = params->StateMachineName;
//		//s_playerManager->ProcessPlayerCharacterState(params->NextStateName, params->StateMachineName);
//	}
//	return true;
//}

//bool HookManager::mod_ISetAimPosition_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
//{
//	if (locals != nullptr)
//	{
//		auto true_locals = (char*)locals - 0x50;
//		SDK::Params::GsrPlayerCameraInfoInterface_ISetAimPosition* params = (SDK::Params::GsrPlayerCameraInfoInterface_ISetAimPosition*)true_locals;
//		s_cameraController->test = params->position;
//		
//		params->position.X = s_weaponController->AimPosition.x;
//		params->position.Y = s_weaponController->AimPosition.y;
//		params->position.Z = s_weaponController->AimPosition.z;
//
//		Logger::DebugPrint("%f, %f, %f", params->position.X, params->position.Y, params->position.Z);
//		//s_playerManager->debugParamsTest.NextStateName = params->NextStateName;
//		//s_playerManager->debugParamsTest.StateMachineName = params->StateMachineName;
//		//s_playerManager->ProcessPlayerCharacterState(params->NextStateName, params->StateMachineName);
//	}
//	return true;
//}

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