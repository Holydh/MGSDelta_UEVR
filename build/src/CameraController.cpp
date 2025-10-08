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

	auto test = (reinterpret_cast<SDK::FCameraCacheEntry*>(uevr::API::get()->sdk()->uobject->get_property_data(cameraManager->to_handle(), L"CameraCachePrivate")));
	if (test != NULL)
		test->POV.Rotation.Yaw = actualYaw;

	//works to rotate camera :
	auto cameraController_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Gsr.GsrPlayerCameraController");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
	auto cameraController = cameraController_c->get_first_object_matching(false);
	auto cameraActor = cameraController->get_property<uevr::API::UObject*>(L"m_cameraActor");
	//Logger::DebugPrint(cameraActor->get_full_name());
	//auto cameraComponent = cameraActor->get_property<uevr::API::UObject*>(L"RootComponent");

	SDK::Params::Actor_K2_SetActorRelativeRotation params;
	params.NewRelativeRotation = { 0.0f,  actualYaw, 0.0f };
	params.bSweep = false;
	params.bTeleport = false;

	cameraActor->call_function(L"K2_SetActorRelativeRotation", &params);
	actualYaw = params.NewRelativeRotation.Yaw;


	UEVR_Vector3d forwardVector;
	CameraAttachementActor->call_function(L"GetActorForwardVector", &forwardVector);
	//Logger::DebugPrint("Forward Vector : %f, %f, %f", forwardVector.x, forwardVector.y, forwardVector.z);

	glm::dvec3 position = { CameraPosition.x, CameraPosition.y, CameraPosition.z };
	glm::dvec3 forward = { forwardVector.x, forwardVector.y, forwardVector.z };
	forward *= 100.0f;
	glm::dvec3 newPosition = position - forward;
	
	SDK::Params::Actor_K2_SetActorRelativeLocation params1;
	params1.NewRelativeLocation = {newPosition.x, newPosition.y, newPosition.z};
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
	CameraAttachementActor = Utilities::SpawnActor(world, UEVR_Vector3d{ CameraPosition.x, CameraPosition.y, CameraPosition.z }, 1, nullptr);
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

void CameraController::ProcessCamera(UEVR_Vector3d& cameraPosition, UEVR_Rotatord& cameraRotation)
{
	auto pawn = uevr::API::get()->get_local_pawn(0);
	UEVR_Vector3d pawnPosition;
	pawn->call_function(L"K2_GetActorLocation", &pawnPosition);
	//Logger::DebugPrint("Pawn Position : %f, %f, %f", pawnPosition.x, pawnPosition.y, pawnPosition.z);
	auto grassVolumeCollisionHead = pawn->get_property<uevr::API::UObject*>(L"GrassVolumeCollisionHead");
	UEVR_Vector3d headColliderPosition;
	grassVolumeCollisionHead->call_function(L"K2_GetComponentLocation", &headColliderPosition);
	UEVR_Vector3d tempPosition;
	glm::fvec3 newPosition;
	//Logger::DebugPrint("Current Player Stance : %i", playerManager.CurrentStance);
	//Logger::DebugPrint("Player Is Peeping: %i", playerManager.IsPeeping);

	//Logger::DebugPrint

	if (InstantCameraPositionNeeded)
	{
		CameraPosition = { pawnPosition.x, pawnPosition.y, (float)headColliderPosition.z };
		//CameraHeightOffset = pawnPosition.z - headColliderPosition.z;
		cameraPosition = { pawnPosition.x, pawnPosition.y, (float)headColliderPosition.z };

		auto pawn = uevr::API::get()->get_local_pawn(0);
		UEVR_Rotatord pawnRotation;
		pawn->call_function(L"K2_GetActorRotation", &pawnRotation);
		actualYaw = pawnRotation.yaw;
		cameraRotation.yaw = actualYaw;
		return;
	}
	
	//if (playerManager->IsChangingStance || playerManager->IsHanging)
	//{
	//	CameraHeightOffset = pawnPosition.z - headColliderPosition.z;
	//}

	if (playerManager->IsPeeping)
	{
		newPosition = glm::mix(CameraPosition, glm::fvec3(headColliderPosition.x, headColliderPosition.y, headColliderPosition.z), 0.1f);
	}
	else
	{
		tempPosition = UEVR_Vector3d(pawnPosition.x, pawnPosition.y, headColliderPosition.z);
		newPosition = glm::fvec3(
			glm::mix(CameraPosition.x, (float)tempPosition.x, 0.25f),
			glm::mix(CameraPosition.y, (float)tempPosition.y, 0.25f),
			glm::mix(CameraPosition.z, (float)tempPosition.z, 0.04f));
	}




	//Logger::DebugPrint("Camera Height Offset : %f", cameraController.CameraHeightOffset);
	CameraPosition = { newPosition.x, newPosition.y, newPosition.z };
	cameraPosition.x = newPosition.x;
	cameraPosition.y = newPosition.y;
	cameraPosition.z = newPosition.z;

	if (YawInput != 0.0)
		actualYaw += YawInput * YawSpeed;
	/*Logger::DebugPrint("rotation->yaw: %f", rotation->yaw);*/
	cameraRotation.yaw = actualYaw;
	SetCameraRotation();
}

void CameraController::ProcessCameraResetTimer(float delta)
{
	if (playerManager->IsPlayerControlled && !playerManager->WasPlayerControlled)
	{
		InstantCameraPositionTimer = 1.0f;
		InstantCameraPositionNeeded = true;
	}
	if (InstantCameraPositionTimer <= 0.0f)
	{
		InstantCameraPositionNeeded = false;
	}
	InstantCameraPositionTimer -= delta;
}


