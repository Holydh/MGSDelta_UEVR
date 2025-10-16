#include "CameraController.hpp"

uevr::API::UObject* CameraController::CameraAttachementActor = nullptr;

void CameraController::ToggleCameraWobblingController(bool enabled) {
	//static auto pawn = uevr::API::get()->get_local_pawn(0);
	//Logger::DebugPrint("Camera Recoil : " + enabled);
	auto wobblingController_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Gsr.GsrWobblingController");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
	auto wobblingController = wobblingController_c->get_first_object_matching(false);
	//Logger::DebugPrint(wobblingController->get_full_name());
	bool setEnabled = enabled ? true : false;
	wobblingController->call_function(L"SetEnabled", &setEnabled);
}



//void CameraController::SetCameraRotation()
//{
//	//Snake follows the POV camera cache rotation when walking forward
//	auto cameraManager_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Engine.PlayerCameraManager");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
//	auto cameraManager = cameraManager_c->get_first_object_matching(false);
//
//	auto test = (reinterpret_cast<SDK::FCameraCacheEntry*>(uevr::API::get()->sdk()->uobject->get_property_data(cameraManager->to_handle(), L"CameraCachePrivate")));
//	if (test != NULL)
//		test->POV.Rotation.Yaw = ActualYaw;
//
//	//works to rotate camera :
//	auto cameraController_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Gsr.GsrPlayerCameraController");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
//	auto cameraController = cameraController_c->get_first_object_matching(false);
//	auto cameraActor = cameraController->get_property<uevr::API::UObject*>(L"m_cameraActor");
//	//Logger::DebugPrint(cameraActor->get_full_name());
//	//auto cameraComponent = cameraActor->get_property<uevr::API::UObject*>(L"RootComponent");
//
//	SDK::Params::Actor_K2_SetActorRelativeRotation params;
//	params.NewRelativeRotation = { 0.0f,  ActualYaw, 0.0f };
//	params.bSweep = false;
//	params.bTeleport = false;
//
//	cameraActor->call_function(L"K2_SetActorRelativeRotation", &params);
//	ActualYaw = params.NewRelativeRotation.Yaw;
//
//	UEVR_Vector3d forwardVector;
//	CameraAttachementActor->call_function(L"GetActorForwardVector", &forwardVector);
//	//Logger::DebugPrint("Forward Vector : %f, %f, %f", forwardVector.x, forwardVector.y, forwardVector.z);
//
//	glm::dvec3 position = { CameraPosition.x, CameraPosition.y, CameraPosition.z };
//	glm::dvec3 forward = { forwardVector.x, forwardVector.y, forwardVector.z };
//	CameraForwardVector = { (float)forward.x, (float)forward.y, (float)forward.z };
//	forward *= 400.0f; // We need to move the camera 400 units back to prevent weapon mesh culling
//	glm::dvec3 newPosition = position - forward;
//	
//	SDK::Params::Actor_K2_SetActorRelativeLocation params1;
//	params1.NewRelativeLocation = {newPosition.x, newPosition.y, newPosition.z};
//	params1.bSweep = false;
//	params1.bTeleport = false;
//
//	cameraActor->call_function(L"K2_SetActorRelativeLocation", &params1);
//	//CameraAttachementActor->call_function(L"K2_GetActorLocation", &newPosition);
//	//Logger::DebugPrint("Camera Attachement Actor Position : %f, %f, %f", newPosition.x, newPosition.y, newPosition.z);
//}




void CameraController::SpawnCameraAttachementActor()
{
	CameraController::CameraAttachementActor = Utilities::SpawnActor(UEVR_Vector3d{ LastCameraPosition.x, LastCameraPosition.y, LastCameraPosition.z }, 1, nullptr);
	CameraAttachementActorAvailable = true;
}

void CameraController::DestroyCameraAttachementActor()
{
	if (CameraController::CameraAttachementActor != nullptr) {
		CameraController::CameraAttachementActor->call_function(L"K2_DestroyActor", nullptr);
		CameraController::CameraAttachementActor = nullptr;
		Logger::DebugPrint("Camera Attachement Actor Destroyed");
	}
	CameraAttachementActorAvailable = false;
}

void CameraController::AttachCameraToAttachementActor()
{
	auto rootComponent = CameraController::CameraAttachementActor->get_property<uevr::API::UObject*>(L"RootComponent");
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

	// Reset camera position and rotation when player respawn or teleport
	if (InstantCameraPositionNeeded)
	{
		LastCameraPosition = { pawnPosition.x, pawnPosition.y, (float)headColliderPosition.z };
		//CameraHeightOffset = pawnPosition.z - headColliderPosition.z;
		cameraPosition = { pawnPosition.x, pawnPosition.y, (float)headColliderPosition.z };

		auto pawn = uevr::API::get()->get_local_pawn(0);
		UEVR_Rotatord pawnRotation;
		pawn->call_function(L"K2_GetActorRotation", &pawnRotation);
		ActualYaw = pawnRotation.yaw;
		cameraRotation.yaw = ActualYaw;
		return;
	}
	
	//Logger::DebugPrint("State Machine Name : %s ", playerManager->debugParamsTest.StateMachineName.ToString().c_str());
	//Logger::DebugPrint("Next State Name : %s ", playerManager->debugParamsTest.NextStateName.ToString().c_str());
	//Logger::DebugPrint("Prev State Name : %s ", playerManager->debugParamsTest.PrevStateName.ToString().c_str());
	//CameraHeightOffset = pawnPosition.z - headColliderPosition.z;
	//Logger::DebugPrint("Camera Height Offset : %f", CameraHeightOffset);

	switch (playerManager->CurrentStance)
	{
	case SDK::EGsrPlayerStance::Stand:
		CameraHeightOffset = -81.0f;
		break;
	case SDK::EGsrPlayerStance::Squat:
		CameraHeightOffset = -8.0f;
		break;
	case SDK::EGsrPlayerStance::Crawl:
		CameraHeightOffset = -19.0f;
		break;
	default:
		break;
	}

	if (playerManager->IsChangingStance || playerManager->IsRolling || playerManager->IsHanging)
	{
		CameraHeightOffset = pawnPosition.z - headColliderPosition.z;
	}
	
	//if (playerManager->IsHanging)
	//{
	//	tempPosition = UEVR_Vector3d(pawnPosition.x, pawnPosition.y, headColliderPosition.z);
	//	newPosition = glm::fvec3(
	//		glm::mix(CameraPosition.x, (float)tempPosition.x, 0.25f),
	//		glm::mix(CameraPosition.y, (float)tempPosition.y, 0.25f),
	//		glm::mix(CameraPosition.z, (float)tempPosition.z, 0.05f));
	//}
	if (playerManager->IsPeepingInternal)
	{
		newPosition = glm::mix(LastCameraPosition, glm::fvec3(headColliderPosition.x, headColliderPosition.y, pawnPosition.z - CameraHeightOffset), 0.1f);
	}
	else if (playerManager->IsPeeping)
	{
		tempPosition = UEVR_Vector3d(pawnPosition.x, pawnPosition.y, pawnPosition.z - CameraHeightOffset);
		newPosition = glm::fvec3(
			glm::mix(LastCameraPosition.x, (float)tempPosition.x, 0.1f),
			glm::mix(LastCameraPosition.y, (float)tempPosition.y, 0.1f),
			glm::mix(LastCameraPosition.z, (float)tempPosition.z, 0.05f));
	}
	else
	{
		tempPosition = UEVR_Vector3d(pawnPosition.x, pawnPosition.y, pawnPosition.z - CameraHeightOffset);
		newPosition = glm::fvec3(
			glm::mix(LastCameraPosition.x, (float)tempPosition.x, 0.25f),
			glm::mix(LastCameraPosition.y, (float)tempPosition.y, 0.25f),
			glm::mix(LastCameraPosition.z, (float)tempPosition.z, 0.05f));
	}




	//Logger::DebugPrint("Camera Height Offset : %f", cameraController.CameraHeightOffset);
	LastCameraPosition = { newPosition.x, newPosition.y, newPosition.z };
	cameraPosition.x = newPosition.x;
	cameraPosition.y = newPosition.y;
	cameraPosition.z = newPosition.z;

	if (YawInput != 0.0)
		ActualYaw += YawInput * YawSpeed;
	/*Logger::DebugPrint("rotation->yaw: %f", rotation->yaw);*/

	//The Imgui menu shows the smoothness "quantity", not the lerp factor. We need the inverse for the calculation. Makes more sense for users.
	float remappedLerpFactor = Utilities::Remap(*settingsManager->CameraYawLerpPtr, 1.0f, 10.0f, 10.0f, 1.0f);
	cameraRotation.yaw = *settingsManager->SmoothCameraRotationPtr ? 
		glm::mix(LastCameraRotation.yaw, ActualYaw, remappedLerpFactor * DeltaTime) :
		ActualYaw;
	LastCameraRotation = cameraRotation;
	/*SetCameraRotation();*/
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


