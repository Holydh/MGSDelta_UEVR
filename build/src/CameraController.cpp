#include "CameraController.hpp"


void CameraController::ToggleGameCameraController(bool enabled)
{
	auto cameraController_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Gsr.GsrPlayerCameraController");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
	auto cameraController = cameraController_c->get_first_object_matching(false);
	bool setActive = enabled ? true : false;
	cameraController->call_function(L"SetActive", &setActive);
}

void CameraController::ToggleCameraWobblingController(bool enabled) {
	//static auto pawn = uevr::API::get()->get_local_pawn(0);
	//Logger::DebugPrint("Camera Recoil : " + enabled);
	auto wobblingController_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Gsr.GsrWobblingController");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
	auto wobblingController = wobblingController_c->get_first_object_matching(false);
	//Logger::DebugPrint(wobblingController->get_full_name());
	bool setEnabled = enabled ? true : false;
	wobblingController->call_function(L"SetEnabled", &setEnabled);
}



void CameraController::SetCameraRotation()
{
	//Snake follows the POV camera cache rotation when walking forward
	auto cameraManager_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Engine.PlayerCameraManager");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
	auto cameraManager = cameraManager_c->get_first_object_matching(false);

	auto test = (reinterpret_cast<Utilities::FCameraCacheEntry*>(uevr::API::get()->sdk()->uobject->get_property_data(cameraManager->to_handle(), L"CameraCachePrivate")));
	if (test != NULL)
		test->POV.Rotation.yaw = actualYaw;

	//works to rotate camera :
	auto cameraController_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Gsr.GsrPlayerCameraController");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
	auto cameraController = cameraController_c->get_first_object_matching(false);
	auto cameraActor = cameraController->get_property<uevr::API::UObject*>(L"m_cameraActor");
	//Logger::DebugPrint(cameraActor->get_full_name());
	//auto cameraComponent = cameraActor->get_property<uevr::API::UObject*>(L"RootComponent");
	Utilities::Parameter_K2_SetWorldOrRelativeRotation params;
	params.newRotation = UEVR_Rotatord(0.0f, actualYaw + YawInput * YawSpeed, 0.0f);
	params.bSweep = false;
	params.bTeleport = true;
	
	cameraActor->call_function(L"K2_SetActorRelativeRotation", &params);
	actualYaw = params.newRotation.yaw;

	UEVR_Vector3d forwardVector;
	CameraAttachementActor->call_function(L"GetActorForwardVector", &forwardVector);
	//Logger::DebugPrint("Forward Vector : %f, %f, %f", forwardVector.x, forwardVector.y, forwardVector.z);

	glm::dvec3 position = { playerPosition.x, playerPosition.y, playerPosition.z };
	glm::dvec3 forward = { forwardVector.x, forwardVector.y, forwardVector.z };
	forward *= 50.0f;
	glm::dvec3 newPosTemp = position - forward;
	UEVR_Vector3d newPosition = { newPosTemp.x, newPosTemp.y, newPosTemp.z };
	
	Utilities::Parameter_K2_SetWorldOrRelativeLocation params1;
	params1.newLocation = newPosition;
	params1.bSweep = false;
	params1.bTeleport = false;

	cameraActor->call_function(L"K2_SetActorRelativeLocation", &params1);
	//CameraAttachementActor->call_function(L"K2_GetActorLocation", &newPosition);
	//Logger::DebugPrint("Camera Attachement Actor Position : %f, %f, %f", newPosition.x, newPosition.y, newPosition.z);
}




void CameraController::SpawnCameraAttachementActor()
{
	auto gameEngine = uevr::API::get()->get_engine();
	auto world = gameEngine->get_property<uevr::API::UObject*>(L"GameViewport")->get_property<uevr::API::UObject*>(L"World");
	Logger::DebugPrint(world->get_full_name());
	CameraAttachementActor = Utilities::SpawnActor(world, playerPosition, 1, nullptr);
	Logger::DebugPrint(CameraAttachementActor->get_full_name());
	CameraAttachementActorAvailable = true;
}

void CameraController::DestroyCameraAttachementActor()
{
	if (CameraAttachementActor != nullptr) {
		CameraAttachementActor->call_function(L"K2_DestroyActor", nullptr);
		CameraAttachementActor = nullptr;
		Logger::DebugPrint("Camera Attachement Actor Destroyed");
	}
	CameraAttachementActorAvailable = false;
}

void CameraController::AttachCameraToAttachementActor()
{
	auto rootComponent = CameraAttachementActor->get_property<uevr::API::UObject*>(L"RootComponent");
	auto motionState = uevr::API::UObjectHook::get_or_add_motion_controller_state(rootComponent);
	//UEVR_Vector3f test = { 0.0, 0.0, 0.0 };
	//UEVR_Quaternionf testQ = { 0.0, 0.0, 0.0, 1.0 };
	//motionState->set_location_offset(&test);
	//motionState->set_rotation_offset(&testQ);
	motionState->set_hand(2);
	motionState->set_permanent(true);
	Logger::DebugPrint("Actor attached to VR Camera");
}

void CameraController::DetachCameraFromAttachementActor()
{
	uevr::API::UObjectHook::remove_motion_controller_state(CameraAttachementActor);
}

void CameraController::FetchAnimationState()
{
	//Class /Script/Ics.IcsAnimInstance
	//auto icsAnimInstance_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerBase.ABP_PlayerBase_C");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
	//auto icsAnimInstance = icsAnimInstance_c->get_first_object_matching(false);

	//Utilities::IcsAnimInstance_OnEnteredState param;
	//icsAnimInstance->call_function(L"OnEnteredState", &param);
	//auto stateName = param.NextStateName;
	
	//if (AnimationState.NextStateName)
	//	return;

	if (!start)
		return;
	Logger::DebugPrint(AnimationState.NextStateName.to_string().c_str());
	//Utilities::KismetStringLibrary_Conv_NameToString params;
	//params.InName = AnimationState.NextStateName;

	//Utilities::KismetStringLibrary->call_function(L"Conv_NameToString", &params);
	//const wchar_t* stateName = params.ReturnValue.CStr();
	//if (stateName && *stateName)
	//{
	//	Logger::DebugPrint("Animation State: %ws", stateName);
	//}
	//else
	//{
	//	Logger::DebugPrint("Animation State: <empty>");
	//}
	//auto icsAnimInstance = cameraController->get_property<uevr::API::UObject*>(L"m_cameraActor");
}


