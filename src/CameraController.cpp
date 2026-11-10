#include "CameraController.hpp"

void CameraController::ToggleCameraWobblingController(bool enabled) {
	//static auto pawn = uevr::API::get()->get_local_pawn(0);
	//Logger::DebugPrint("Camera Recoil : " + enabled);
	auto static wobblingController_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Gsr.GsrWobblingController");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
	auto wobblingController = wobblingController_c->get_first_object_matching(false);
	if (!wobblingController)
		return;
	//Logger::DebugPrint(wobblingController->get_full_name());
	bool setEnabled = enabled ? true : false;
	wobblingController->call_function(L"SetEnabled", &setEnabled);
}

bool CameraController::SpawnCameraAttachementActor()
{
	CameraAttachementActor = Utilities::SpawnActor(UEVR_Vector3d{ LastCameraPosition.x, LastCameraPosition.y, LastCameraPosition.z }, nullptr);
	if (CameraAttachementActor != nullptr)
	{
		CameraAttachementActorAvailable = true;
		return true;
	}
	else
	{
		CameraAttachementActorAvailable = false;
		return false;
	}
}

void CameraController::DestroyCameraAttachementActor()
{
	if (CameraAttachementActor != nullptr) {
		CameraAttachementActor->call_function(L"K2_DestroyActor", nullptr);
		CameraAttachementActor = nullptr;
		IsCameraAttachedToAttachementActor = false;
		Logger::DebugPrint("Camera Attachement Actor Destroyed");
	}
	CameraAttachementActorAvailable = false;
}

void CameraController::AttachCameraToAttachementActor()
{
	if (!CameraAttachementActor) return;
	auto rootComponent = CameraAttachementActor->get_property<uevr::API::UObject*>(L"RootComponent");
	if (rootComponent) {
		auto motionState = uevr::API::UObjectHook::get_or_add_motion_controller_state(rootComponent);
		//UEVR_Vector3f test = { 0.0, 0.0, 0.0 };
		//UEVR_Quaternionf testQ = { 0.0, 0.0, 0.0, 1.0 };
		//motionState->set_location_offset(&test);
		//motionState->set_rotation_offset(&testQ);
		motionState->set_hand(2);
		motionState->set_permanent(true);
		Logger::DebugPrint("Actor attached to VR Camera");
		IsCameraAttachedToAttachementActor = true;
	}
	else
	{
		Logger::DebugPrint("AttachCameraToAttachementActor: RootComponent is null");
	}
}

void CameraController::DetachCameraFromAttachementActor()
{
	if (!CameraAttachementActor) return;
	auto rootComponent = CameraAttachementActor->get_property<uevr::API::UObject*>(L"RootComponent");
    if (rootComponent) {
        uevr::API::UObjectHook::remove_motion_controller_state(rootComponent);
		IsCameraAttachedToAttachementActor = false;
    } else {
        Logger::DebugPrint("DetachCameraFromAttachementActor: RootComponent is null");
    }
}

// not needed anymore after delta time capped in camera controller ?
//bool CameraController::CameraAttachementActorPositionSafeguard()
//{
//	if (!CameraAttachementActor) return false;
//	UEVR_Vector3d actorPosition;
//	CameraAttachementActor->call_function(L"K2_GetActorLocation", &actorPosition);
//	float distance = glm::distance(glm::fvec3{ (float)actorPosition.x, (float)actorPosition.y, (float)actorPosition.z }, LastCameraPosition);
//	//Logger::DebugPrint("Camera Attachement Actor Position Safeguard Check. Distance: %f", distance);
//	if (distance > 200.0f && IsCameraAttachedToAttachementActor)
//	{
//		Logger::DebugPrint("Camera Attachement Actor Position Safeguard Activated. Distance: %f", distance);
//		return true;
//	}
//	else
//	{
//		return false;
//	}
//}

void CameraController::CalculateForwardOffset(SDK::FVector location, SDK::FRotator rotation, float fov, float &forwardOffset, glm::fvec3 &forward)
{
	// Convert rotation to radians
	float pitchRad = glm::radians(rotation.Pitch);
	float yawRad = glm::radians(rotation.Yaw);

	// Calculate forward vector from pitch/yaw
	forward = {
		cos(pitchRad) * cos(yawRad),
		cos(pitchRad) * sin(yawRad),
		sin(pitchRad)
	};

	// Compute forward offset: smaller FOV = more zoom = push forward
	// tan(fov/2) ratio relative to a baseline FOV (e.g. 90°)
	float baseFOV = 68.0f;
	float zoomFactor = tan(glm::radians(baseFOV * 0.5f)) / tan(glm::radians(fov * 0.5f));
	float scale = ActualCutsceneState == CutsceneState::Playing3D ? 60.0f : 120.0f;
	forwardOffset = (zoomFactor - 1.0f) * scale; // 100.0f = scale, tune this
}

void CameraController::ProcessCamera(float deltaTime)
{
	// Cap deltaTime to prevent camera from yeeting into orbit when the game freezes or low fps.
	deltaTime = glm::min(deltaTime, 0.05f);
	glm::fvec3 tempPosition;
	glm::fvec3 newPosition;
	glm::fvec3 newRotation;
	float remappedLerpXYMultiplier = Utilities::Remap(*settingsManager->CameraPositionXYLerpPtr, 0.0f, 10.0f, 4.0f, 0.1f);
	float remappedLerpZMultiplier = Utilities::Remap(*settingsManager->CameraPositionZLerpPtr, 0.0f, 10.0f, 4.0f, 0.1f);

	auto static cobraCutsceneCameraCtrlMngr_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/CobraCutScene.CobraCutsceneCameraControlManagerComponent");
	auto cobraCutsceneCameraCtrlMngr = cobraCutsceneCameraCtrlMngr_c->get_first_object_matching(false);
	if (!cobraCutsceneCameraCtrlMngr)
	{
		Logger::DebugPrint("CobraCutsceneCameraCtrlMngr not found");
		return;
	}
	auto cc = (SDK::UCobraCutsceneCameraControlManagerComponent*)cobraCutsceneCameraCtrlMngr;

	auto static playerCameraManager_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Cobra.CobraPlayerCameraManager");
	auto playerCameraManager = playerCameraManager_c->get_first_object_matching(false);
	if (!playerCameraManager)
	{
		Logger::DebugPrint("PlayerCameraManager not found");
		return;
	}
	auto pcm = (SDK::ACobraPlayerCameraManager*)playerCameraManager;
	auto ueLocation = pcm->CameraCachePrivate.POV.Location;
	auto ueRotation = pcm->CameraCachePrivate.POV.Rotation;
	glm::fvec3 originalCamLocation = { ueLocation.X, ueLocation.Y, ueLocation.Z };
	glm::fvec3 originalCamRotation = { ueRotation.Pitch, ueRotation.Yaw, ueRotation.Roll };

	float actualFOV = pcm->CameraCachePrivate.POV.FOV;

	if (ActualCutsceneState == CutsceneState::Playing3D)
	{
		if (cc->bIsFpv)
		{
			InstantCameraPositionNeeded = true;
			InstantCameraPositionTimer = 0.25f;
			float forwardOffset = 0.0f;
			glm::fvec3 forward{};
			CalculateForwardOffset(ueLocation, ueRotation, actualFOV, forwardOffset, forward);
			// if fpv is enabled, we need to push the camera forward based on the FOV to prevent it from being too zoomed out
			originalCamLocation = {
				ueLocation.X + forward.x * forwardOffset,
				ueLocation.Y + forward.y * forwardOffset,
				ueLocation.Z + forward.z * forwardOffset
			};
		}

		if (InstantCameraPositionNeeded)
		{
			
			LastCameraPosition = originalCamLocation;
			NewCameraPosition = { originalCamLocation.x, originalCamLocation.y, originalCamLocation.z };
			LastCameraRotation = { ueRotation.Pitch, ueRotation.Yaw, ueRotation.Roll };
			NewCameraRotation = LastCameraRotation;
			return;
		}

		if (*settingsManager->EnableImmersiveModePtr)
		{
			if (*settingsManager->SmoothCameraMovementsInImmersiveCutscenesPtr)
			{
				// prevent camera yeeting into orbit on low fps
				float speedXY = 11.25f * remappedLerpXYMultiplier;
				float alphaXY = 1.0f - expf(-speedXY * deltaTime);
				float speedZ = 2.25f * remappedLerpZMultiplier;
				float alphaZ = 1.0f - expf(-speedZ * deltaTime);
				newPosition = *settingsManager->SmoothCameraMovementsInImmersiveCutscenesPtr ?
					glm::fvec3(
						glm::mix(LastCameraPosition.x, originalCamLocation.x, alphaXY),
						glm::mix(LastCameraPosition.y, originalCamLocation.y, alphaXY),
						glm::mix(LastCameraPosition.z, originalCamLocation.z, alphaZ)) :
					originalCamLocation;

				NewCameraPosition = { newPosition.x, newPosition.y, newPosition.z };
				LastCameraPosition = newPosition;

				float remappedRotationLerpMultiplier = Utilities::Remap(*settingsManager->CameraYawLerpPtr, 0.0f, 10.0f, 2.0f, 0.1f);
				float speed = 10.25f * remappedRotationLerpMultiplier;
				float alpha = 1.0f - expf(-speed * deltaTime);


				// 1. Calculate Deltas
				float deltaPitch = originalCamRotation.x - (float)LastCameraRotation.pitch;
				float deltaYaw = originalCamRotation.y - (float)LastCameraRotation.yaw;
				float deltaRoll = originalCamRotation.z - (float)LastCameraRotation.roll;

				// 2. Normalize Deltas to [-180, 180] (The "Shortest Path" trick)
				auto normalize = [](float delta) {
					while (delta > 180.0f)  delta -= 360.0f;
					while (delta < -180.0f) delta += 360.0f;
					return delta;
					};

				deltaPitch = normalize(deltaPitch);
				deltaYaw = normalize(deltaYaw);
				deltaRoll = normalize(deltaRoll);

				// --- CAMERA CUT DETECTION ---
				// If the yaw jump is huge (e.g. > 100 degrees), snap instantly
				if (std::abs(deltaYaw) > 45.0f) {
					newRotation = originalCamRotation;
				}
				else {
					// 3. Smoothly add the delta to the last known position
					newRotation.x = (float)LastCameraRotation.pitch + (deltaPitch * alpha);
					newRotation.y = (float)LastCameraRotation.yaw + (deltaYaw * alpha);
					newRotation.z = (float)LastCameraRotation.roll + (deltaRoll * alpha);
				}

				// 4. Final Wrap-around (Keeping the values in 0-360 or -180/180 for the Engine)
				auto wrapAngle = [](float angle) {
					while (angle >= 360.0f) angle -= 360.0f;
					while (angle < 0.0f)    angle += 360.0f;
					return angle;
					};

				NewCameraRotation = { wrapAngle(newRotation.x), wrapAngle(newRotation.y), wrapAngle(newRotation.z) };
				LastCameraRotation = NewCameraRotation;
			}
			else
			{
				LastCameraPosition = originalCamLocation;
				NewCameraPosition = { originalCamLocation.x, originalCamLocation.y, originalCamLocation.z };
				NewCameraRotation = { ueRotation.Pitch, ueRotation.Yaw, ueRotation.Roll };
				LastCameraRotation = NewCameraRotation;
			}
		}
	}
	else
	{
		auto pawn = uevr::API::get()->get_local_pawn(0);
		//UEVR_Vector3d floorPosition;
		//pawn->call_function(L"GetFloorPosition", &floorPosition);
		//Logger::DebugPrint("GetFloorPosition : %f, %f, %f", floorPosition.x, floorPosition.y, floorPosition.z);
		UEVR_Vector3d pawnPosition;
		pawn->call_function(L"K2_GetActorLocation", &pawnPosition);
		//Logger::DebugPrint("Pawn Position : %f, %f, %f", pawnPosition.x, pawnPosition.y, pawnPosition.z);
		auto grassVolumeCollisionHead = pawn->get_property<uevr::API::UObject*>(L"GrassVolumeCollisionHead");

		grassVolumeCollisionHead->call_function(L"K2_GetComponentLocation", &headColliderPosition);
		//Logger::DebugPrint("headColliderPosition : %f, %f, %f", headColliderPosition.x, headColliderPosition.y, headColliderPosition.z);

		// Reset camera position and rotation when player respawn or teleport
		if (InstantCameraPositionNeeded)
		{
			LastCameraPosition = { pawnPosition.x, pawnPosition.y, (float)headColliderPosition.z };
			//CameraHeightOffset = pawnPosition.z - headColliderPosition.z;
			NewCameraPosition = { pawnPosition.x, pawnPosition.y, (float)headColliderPosition.z };

			auto pawn = uevr::API::get()->get_local_pawn(0);
			UEVR_Rotatord pawnRotation;
			pawn->call_function(L"K2_GetActorRotation", &pawnRotation);
			ActualYaw = pawnRotation.yaw;
			LastCameraRotation.yaw = ActualYaw;
			NewCameraRotation.yaw = ActualYaw;
			return;
		}
		//Logger::DebugPrint("Actual Yaw : %f", ActualYaw);
		//Logger::DebugPrint("State Machine Name : %s ", playerManager->debugParamsTest.StateMachineName.ToString().c_str());
		//Logger::DebugPrint("Next State Name : %s ", playerManager->debugParamsTest.NextStateName.ToString().c_str());
		//Logger::DebugPrint("Prev State Name : %s ", playerManager->debugParamsTest.PrevStateName.ToString().c_str());
		//CameraHeightOffset = headColliderPosition.z - pawnPosition.z;
		//Logger::DebugPrint("stance : %i", playerManager->CurrentStance);
		//Logger::DebugPrint("pawnPosition.z : %f", pawnPosition.z);

		if (!*settingsManager->EnableRoomscaleProneAndCrouchPtr)
		{
			switch (playerManager->CurrentStance)
			{
			case SDK::EGsrPlayerStance::Stand:
				CameraHeightOffset = 83.0f; //-85.0f
				break;
			case SDK::EGsrPlayerStance::Squat:
				CameraHeightOffset = 6.7f;
				break;
			case SDK::EGsrPlayerStance::Crawl:
				CameraHeightOffset = 10.0f;
				break;
			default:
				break;
			}
			if (playerManager->IsChangingStance || playerManager->IsRolling || playerManager->IsHanging)
			{
				CameraHeightOffset = headColliderPosition.z - pawnPosition.z;
			}
		}
		else
		{
			switch (playerManager->CurrentStance)
			{
			case SDK::EGsrPlayerStance::Stand:
				CameraHeightOffset = 83.0f; //-85.0f
				break;
			case SDK::EGsrPlayerStance::Squat:
				CameraHeightOffset = 83.0f;
				break;
			case SDK::EGsrPlayerStance::Crawl:
				// playerPosition.Z actualise one frame after change stance, we need to compensate for that or it results in a sudden camera jump
				CameraHeightOffset = playerManager->PreviousStance == SDK::EGsrPlayerStance::Squat ? 83.0f : 83.0f + crawlOffset;
				break;
			default:
				break;
			}
		}



		//if (playerManager->IsHanging)
		//{
		//	tempPosition = UEVR_Vector3d(pawnPosition.x, pawnPosition.y, headColliderPosition.z);
		//	newPosition = glm::fvec3(
		//		glm::mix(CameraPosition.x, (float)tempPosition.x, 0.25f),
		//		glm::mix(CameraPosition.y, (float)tempPosition.y, 0.25f),
		//		glm::mix(CameraPosition.z, (float)tempPosition.z, 0.05f));
		//}



		if (playerManager->IsPeepingInternal) // initial peep movement
		{
			tempPosition = *settingsManager->AttachCameraToSnakeHeadPtr ?
				glm::fvec3(headColliderPosition.x, headColliderPosition.y, headColliderPosition.z) :
				glm::fvec3(headColliderPosition.x, headColliderPosition.y, pawnPosition.z + CameraHeightOffset);
			// prevent camera yeeting into orbit on low fps
			constexpr float speed = 3.0f;
			float alpha = 1.0f - expf(-speed * deltaTime);
			newPosition = *settingsManager->SmoothCameraPositionPtr ?
				glm::mix(LastCameraPosition, tempPosition, alpha) :
				tempPosition;
		}
		else if (playerManager->IsPeeping) // stops peeping
		{
			tempPosition = *settingsManager->AttachCameraToSnakeHeadPtr ?
				glm::fvec3(headColliderPosition.x, headColliderPosition.y, headColliderPosition.z) :
				glm::fvec3(pawnPosition.x, pawnPosition.y, pawnPosition.z + CameraHeightOffset);
			// prevent camera yeeting into orbit on low fps
			float speedXY = 4.5f * remappedLerpXYMultiplier;
			float alphaXY = 1.0f - expf(-speedXY * deltaTime);
			float speedZ = 2.25f * remappedLerpZMultiplier;
			float alphaZ = 1.0f - expf(-speedZ * deltaTime);
			newPosition = *settingsManager->SmoothCameraPositionPtr ?
				glm::fvec3(
					glm::mix(LastCameraPosition.x, tempPosition.x, alphaXY),
					glm::mix(LastCameraPosition.y, tempPosition.y, alphaXY),
					glm::mix(LastCameraPosition.z, tempPosition.z, alphaZ)) :
				tempPosition;
		}

		else
		{
			tempPosition = *settingsManager->AttachCameraToSnakeHeadPtr || CardBoardEquipped || (playerManager->IsFirstPersonView && !*settingsManager->EnableRoomscaleProneAndCrouchPtr) ?
				glm::fvec3(headColliderPosition.x, headColliderPosition.y, headColliderPosition.z) :
				glm::fvec3(pawnPosition.x, pawnPosition.y, pawnPosition.z + CameraHeightOffset);
			// prevent camera yeeting into orbit on low fps
			float speedXY = 11.25f * remappedLerpXYMultiplier;
			float alphaXY = 1.0f - expf(-speedXY * deltaTime);
			float speedZ = (CardBoardEquipped ? 5.0f : 2.25f) * remappedLerpZMultiplier;
			float alphaZ = 1.0f - expf(-speedZ * deltaTime);
			newPosition = *settingsManager->SmoothCameraPositionPtr ?
				glm::fvec3(
					glm::mix(LastCameraPosition.x, tempPosition.x, alphaXY),
					glm::mix(LastCameraPosition.y, tempPosition.y, alphaXY),
					glm::mix(LastCameraPosition.z, tempPosition.z, alphaZ)) :
				tempPosition;
		}

		//Logger::DebugPrint("%f", pawnPosition.z + CameraHeightOffset);
		//Logger::DebugPrint("%f, %f", remappedLerpZMultiplier, 2.25f * remappedLerpZMultiplier * DeltaTime);
		//Logger::DebugPrint("%f", DeltaTime);

		//Logger::DebugPrint("Camera Height Offset : %f", cameraController.CameraHeightOffset);
		LastCameraPosition = newPosition;
		NewCameraPosition.x = newPosition.x;
		NewCameraPosition.y = newPosition.y;
		NewCameraPosition.z = newPosition.z;

		if (playerManager->CurrentMenuType == SDK::EGsrUIMenuType::None)
		{
			YawProcessed = YawInput * 3.0f * *settingsManager->CameraRotationSpeedPtr * deltaTime;
			if (YawInput != 0.0)
				ActualYaw += YawProcessed;
		}

		//if (ActualYaw >= 360.0f)
		//	ActualYaw -= 360.0f;
		//else if (ActualYaw < 0.0f)
		//	ActualYaw += 360.0f;
		/*Logger::DebugPrint("rotation->yaw: %f", rotation->yaw);*/

		//The Imgui menu shows the smoothness "quantity", not the lerp factor. We need the inverse for the calculation. Makes more sense for users.
		float remappedRotationLerpMultiplier = Utilities::Remap(*settingsManager->CameraYawLerpPtr, 0.0f, 10.0f, 2.0f, 0.1f);
		// prevent camera yeeting into orbit on low fps
		float speed = 10.25f * remappedRotationLerpMultiplier;
		float alpha = 1.0f - expf(-speed * deltaTime);
		NewCameraRotation.yaw = *settingsManager->SmoothCameraRotationPtr ?
			glm::mix(LastCameraRotation.yaw, ActualYaw, alpha) :
			ActualYaw;

		if (CurrentItemID == SDK::EGsrItemId::IT_Scope)
		{
			// if fpv is enabled, we need to push the camera forward based on the FOV to prevent it from being too zoomed out
			float forwardOffset = 0.0f;
			glm::fvec3 forward{};
			auto sdkActor = (SDK::AActor*)CameraAttachementActor;
			SDK::FRotator hmdRotation;
			hmdRotation = sdkActor->K2_GetActorRotation();
			CalculateForwardOffset({newPosition.x, newPosition.y, newPosition.z}, { hmdRotation.Pitch, hmdRotation.Yaw, hmdRotation.Roll }, actualFOV, forwardOffset, forward);
			// if fpv is enabled, we need to push the camera forward based on the FOV to prevent it from being too zoomed out
			NewCameraPosition = {
				ueLocation.X + forward.x * forwardOffset,
				ueLocation.Y + forward.y * forwardOffset,
				ueLocation.Z + forward.z * forwardOffset
			};
			LastCameraPosition = { NewCameraPosition.x, NewCameraPosition.y, NewCameraPosition.z };
		}

		LastCameraRotation = NewCameraRotation;
		previousPawnPositionZ = pawnPosition.z;
		/*SetCameraRotation();*/
	}
}

void CameraController::ProcessCameraResetTimer(float delta)
{
	if (playerManager->IsPlayerControlled && !playerManager->WasPlayerControlled)
	{
		InstantCameraPositionTimer = 0.25f;
		InstantCameraPositionNeeded = true;
	}
	if (InstantCameraPositionTimer <= 0.0f)
	{
		InstantCameraPositionNeeded = false;
	}
	InstantCameraPositionTimer -= delta;
}

//Need more tests
//void CameraController::DisableGrassFading()
//{
//	Logger::DebugPrint("DisableGrassFading");
//	auto uClass = (uevr::API::UClass*)SDK::UCameraFadingComponent::StaticClass();
//	auto uCameraFadingComponents = uClass->get_objects_matching();
//	for (size_t i = 0; i < uCameraFadingComponents.size(); i++)
//	{
//		Logger::DebugPrint(uCameraFadingComponents[i]->get_full_name());
//		auto sdkComponent = (SDK::UCameraFadingComponent*)uCameraFadingComponents[i];
//		sdkComponent->bAutoActivate = false;
//		sdkComponent->GrassFadingRadius = 0.0f;
//		sdkComponent->ClearCache();
//		sdkComponent->Deactivate();
//	}
//}

void CameraController::ProcessRoomscaleStance()
{
	if (!*settingsManager->EnableRoomscaleProneAndCrouchPtr)
		return;

	if (!CameraAttachementActor)
	{
		Logger::DebugPrint("ProcessRoomscaleStance: CameraAttachementActor is null");
		return;
	}
	auto sdkActor = (SDK::AActor*)CameraAttachementActor;
	auto cameraAttachementPosition = sdkActor->K2_GetActorLocation();
	//Logger::DebugPrint("cameraAttachementPosition : %f", cameraAttachementPosition.Z);
	//Logger::DebugPrint("headColliderPosition : %f", headColliderPosition.z);
	auto difference = cameraAttachementPosition.Z - headColliderPosition.z;
	//Logger::DebugPrint("Head Collider - cameraAttachementPosition Difference : %f", difference);

	UEVR_Matrix4x4f hmdTransform{};
	uevr::API::get()->param()->vr->get_transform(uevr::API::VR::get_hmd_index(), &hmdTransform);
	auto hmdHeight = hmdTransform.m[3][1];
	//Logger::DebugPrint("HMD Transform Z Position : %f", hmdTransform.m[3][1]);

	if (playerManager->CurrentStance == SDK::EGsrPlayerStance::Stand && hmdHeight <= *settingsManager->SquatUnderThisValuePtr)
		playerManager->SetPlayerStance(SDK::EGsrPlayerStance::Squat);

	if (playerManager->CurrentStance == SDK::EGsrPlayerStance::Squat && hmdHeight <= *settingsManager->CrawlUnderThisValuePtr)
		playerManager->SetPlayerStance(SDK::EGsrPlayerStance::Crawl);

	if (playerManager->CurrentStance == SDK::EGsrPlayerStance::Squat && hmdHeight >= *settingsManager->SquatUnderThisValuePtr)
		playerManager->SetPlayerStance(SDK::EGsrPlayerStance::Stand);

	if (playerManager->CurrentStance == SDK::EGsrPlayerStance::Crawl && hmdHeight >= *settingsManager->CrawlUnderThisValuePtr)
		playerManager->SetPlayerStance(SDK::EGsrPlayerStance::Squat);
}

//void CameraController::SetNearClipPlane(float nearClip)
//{
//	auto playerCameraActor_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Gsr.GsrPlayerCameraActor");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
//	auto playerCamera = playerCameraActor_c->get_first_object_matching(false);
//	auto sdkPlayerCamera = (SDK::AGsrPlayerCameraActor*)playerCamera;
//	sdkPlayerCamera->CameraComponent->SetOrthoNearClipPlane(nearClip);
//	Logger::DebugPrint("%f", sdkPlayerCamera->CameraComponent->OrthoNearClipPlane);
//}

void CameraController::FixSurvivalViewerMenuRendering()
{
	auto static sv_main_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"WidgetBlueprintGeneratedClass /CobraUI/Blueprint/sv_main.sv_main_C");
	auto sv_main = sv_main_c->get_first_object_matching(false);

	SDK::Usv_main_C* mainWidget = (SDK::Usv_main_C*)sv_main;
	mainWidget->ModelCaptureCanvas->SetVisibility(SDK::ESlateVisibility::Hidden);
	//mainWidget->menuBLIcon->SetVisibility(SDK::ESlateVisibility::Visible);
	//mainWidget->Mask->SetVisibility(SDK::ESlateVisibility::Hidden);
	// 
	// Get sub menu type : mainWidget->GetSvStep();

	auto rootWidgetCanvasPanel = (SDK::UPanelWidget*)mainWidget->WidgetTree->RootWidget;
	Logger::DebugPrint(rootWidgetCanvasPanel->GetFullName().c_str());

	// Loop through slots of the root panel
	Logger::DebugPrint("Root widget slots:");
	for (int i = 0; i < rootWidgetCanvasPanel->Slots.Num(); i++)
	{
		auto slot = rootWidgetCanvasPanel->Slots[i];
		Logger::DebugPrint(slot->GetFullName().c_str());

		// If you need to cast to CanvasPanelSlot specifically
		auto canvasSlot = (SDK::UCanvasPanelSlot*)slot;

		// Access the widget contained in this slot
		auto containedWidget = canvasSlot->Content;
		if (containedWidget) {
			Logger::DebugPrint(("  Contains: " + std::string(containedWidget->GetFullName())).c_str());
		}
	}

	// If you want to access a specific child panel's slots
	// Find a specific child widget by name or index
	if (rootWidgetCanvasPanel->Slots.Num() > 0) {
		// Get the first slot's content (which might be another panel)
		auto firstSlot = (SDK::UCanvasPanelSlot*)rootWidgetCanvasPanel->Slots[0];
		auto firstChildWidget = firstSlot->Content;

		// If it's a panel, you can access its slots
		if (firstChildWidget && firstChildWidget->IsA(SDK::UPanelWidget::StaticClass())) {
			auto childPanel = (SDK::UPanelWidget*)firstChildWidget;

			for (int i = 0; i < childPanel->Slots.Num(); i++) {
				auto slot = (SDK::UPanelSlot*)childPanel->Slots[i];
				std::string slotName = slot->GetName(); // Get just the name without path

				if (slotName == "CanvasPanelSlot_10") {
					slot->Content->SetVisibility(SDK::ESlateVisibility::Hidden);
					Logger::DebugPrint("Found target slot at index: %i", i);
				}
				if (slotName == "CanvasPanelSlot_1")
				{
					slot->Content->SetVisibility(SDK::ESlateVisibility::Hidden);
					Logger::DebugPrint("Found target slot at index: %i", i);
				}
			}
		}
	}
}

void CameraController::FixRadioMenuRendering()
{
	auto static Radio_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"WidgetBlueprintGeneratedClass /CobraUI/Blueprint/Radio/Radio.Radio_C");
	auto radio_main = Radio_c->get_first_object_matching(false);

	SDK::URadio_C* mainWidget = (SDK::URadio_C*)radio_main;
	mainWidget->ModelBg->SetVisibility(SDK::ESlateVisibility::Hidden);
}

void CameraController::CheckCutscenePlaying()
{
	auto static gameStateSubsystem_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Gsr.GsrGameStateSubsystem");
	auto gameStateSubsystem = gameStateSubsystem_c->get_first_object_matching(false);
	bool cutscene = false;
	bool movie = false;

	if (gameStateSubsystem)
	{
		auto gameState = (SDK::UGsrGameStateSubsystem*)gameStateSubsystem;
		cutscene = gameState->CheckGameStatus(SDK::EGsrGameStatus::Cutscene);
		movie = gameState->CheckGameStatus(SDK::EGsrGameStatus::Movie);
	}

	if (cutscene || movie)
	{
		ActualCutsceneState = cutscene ? CutsceneState::Playing3D : CutsceneState::Playing2D;
	}
	else
	{
		ActualCutsceneState = CutsceneState::NotPlaying;
	}
}

bool CameraController::CutsceneStarts()
{
	return PreviousCutsceneState == CutsceneState::NotPlaying &&
		(ActualCutsceneState == CutsceneState::Playing3D || ActualCutsceneState == CutsceneState::Playing2D);
}

bool CameraController::CutscenePlaying()
{
	return ActualCutsceneState == CutsceneState::Playing3D || ActualCutsceneState == CutsceneState::Playing2D;
}

bool CameraController::CutsceneStops()
{
	return ActualCutsceneState == CutsceneState::NotPlaying &&
		(PreviousCutsceneState == CutsceneState::Playing3D || PreviousCutsceneState == CutsceneState::Playing2D);
}

void CameraController::ApplyCutsceneSettings()
{
	if (CutsceneStarts())
	{
		uevr::API::VR::recenter_view();
		if (ActualCutsceneState == CameraController::CutsceneState::Playing3D && !*settingsManager->EnableImmersiveModePtr)
		{
			SpawnCustom2DScreen();
		}
		Logger::DebugPrint("Cutscene State changed to : %s", std::string(magic_enum::enum_name(ActualCutsceneState)));
		uevr::API::VR::set_mod_value("VR_2DScreenMode", ActualCutsceneState == CameraController::CutsceneState::Playing2D ?	true : false);
		uevr::API::VR::set_mod_value("VR_WorldScale", *settingsManager->Enable3DCutscenesPtr ? 1.0f : 0.0f);
		uevr::API::VR::set_mod_value("VR_DecoupledPitch", *settingsManager->EnableImmersiveModePtr);
	}
	if (CutsceneStops())
	{
		DestroyCustom2DScreen();
		Logger::DebugPrint("Cutscene State changed to : %s", std::string(magic_enum::enum_name(ActualCutsceneState)));
		uevr::API::VR::set_mod_value("VR_DecoupledPitch", true);
		uevr::API::VR::set_mod_value("VR_2DScreenMode", false);
		uevr::API::VR::set_mod_value("VR_WorldScale", 1.0f);
	}

	if (CutscenePlaying())
	{
		auto static cobraCutsceneCameraCtrlMngr_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/CobraCutScene.CobraCutsceneCameraControlManagerComponent");
		auto cobraCutsceneCameraCtrlMngr = cobraCutsceneCameraCtrlMngr_c->get_first_object_matching(false);
		if (!cobraCutsceneCameraCtrlMngr)
		{
			Logger::DebugPrint("CobraCutsceneCameraCtrlMngr not found");
			return;
		}
		auto cc = (SDK::UCobraCutsceneCameraControlManagerComponent*)cobraCutsceneCameraCtrlMngr;

		if (!*settingsManager->EnableImmersiveModePtr)
		{
			if (cc->bIsFpv)
				DestroyCustom2DScreen();
			else
				SpawnCustom2DScreen();
		}
	}
}

void CameraController::SpawnCustom2DScreen()
{
	if (!CutscenePlaying() || Custom2DScreen != nullptr)
		return;

	UEVR_Vector3d uevrVector = { 0.0f, 0.0f, 0.0f };
	auto uevrActor = Utilities::SpawnActor(uevrVector, nullptr);
	auto actor = (SDK::AActor*)uevrActor;
	if (!actor)
	{
		return;
	}

    // 1. Find the mesh asset already loaded in memory
    static SDK::UStaticMesh* staticMesh = nullptr;
	auto meshUEVR = uevr::API::get()->find_uobject<uevr::API::UObject>(L"StaticMesh /Engine/BasicShapes/Plane.Plane");
	auto sdkMesh = (SDK::UObject*)meshUEVR;
    if (sdkMesh->IsA(SDK::UStaticMesh::StaticClass()))
    {
		staticMesh = (SDK::UStaticMesh*)meshUEVR;

        if (!staticMesh)
        {
            Logger::DebugPrint("Failed to find static mesh");
            return;
		}
		Logger::DebugPrint("Found static mesh %s : ", staticMesh->GetFullName().c_str());
	}
	else
	{
		Logger::DebugPrint("Asset found is not a StaticMesh or SkeletalMesh");
		return;
	}

	auto translucentMat = uevr::API::get()->find_uobject<uevr::API::UObject>(L"Material /Engine/EngineDebugMaterials/DebugMeshMaterial.DebugMeshMaterial");
	if (!translucentMat)
	{
		Logger::DebugPrint("Failed to find material for 2D screen");
		return;
	}

	auto sdkMat = (SDK::UMaterial*)translucentMat;
	sdkMat->TwoSided = true;
	sdkMat->bDisableDepthTest = true;
	sdkMat->BlendMode = SDK::EBlendMode::BLEND_Translucent;
	//sdkMat->MaterialDomain = SDK::EMaterialDomain::MD_Surface;
	sdkMat->ShadingModel = SDK::EMaterialShadingModel::MSM_Unlit;
	////Material /Engine/EngineMaterials/DefaultMaterial.DefaultMaterial

	//auto textureUEVR = uevr::API::get()->find_uobject<uevr::API::UObject>(L"Texture2D /CobraUI/Material/HUD/T_GoggleVignette.T_GoggleVignette");
	//auto texture = (SDK::UTexture2D*)textureUEVR;

	if (staticMesh)
	{
		// create 5 planes for the box
		for (int i = 0; i < 5; i++)
		{
			auto* staticMeshComponent = static_cast<SDK::UStaticMeshComponent*>(actor->AddComponentByClass(SDK::UStaticMeshComponent::StaticClass(),
				false, // bManualAttachment
				SDK::FTransform{}, // RelativeTransform
				false // bDeferConstruction
			));

			if (!staticMeshComponent)
			{
				Logger::DebugPrint("Failed to create UStaticMeshComponent");
				return;
			}
			Logger::DebugPrint("Created StaticMeshComponent: %s", staticMeshComponent->GetFullName().c_str());
			// 3. Assign the mesh asset to our new component instance
			staticMeshComponent->SetStaticMesh(staticMesh);
			Logger::DebugPrint("Assigned mesh to StaticMeshComponent");

			// 4. Attach the new component to the Actor's root component
			staticMeshComponent->K2_AttachToComponent(
				actor->K2_GetRootComponent(),                      // Parent
				SDK::FName(),                                      // SocketName (none)
				SDK::EAttachmentRule::KeepRelative,             // Use the relative location we just set
				SDK::EAttachmentRule::KeepRelative,             // Use the relative rotation we just set
				SDK::EAttachmentRule::KeepRelative,             // Use the relative scale we just set
				false                                            // bWeldSimulatedBodies
			);
			Logger::DebugPrint("Attached StaticMeshComponent to Actor's root component");

			// 6. Register the component to make it "live" and renderable in the world
			staticMeshComponent->Activate(true);
			Logger::DebugPrint("Activated StaticMeshComponent");

			// Optional: Disable collision if it's just for visuals
			staticMeshComponent->SetCollisionEnabled(SDK::ECollisionEnabled::NoCollision);
			staticMeshComponent->Activate(true);
			staticMeshComponent->SetVisibility(true, false);
			staticMeshComponent->SetHiddenInGame(false, false);
			staticMeshComponent->SetWorldScale3D({ 1, 1, 1 });
			staticMeshComponent->SetMobility(SDK::EComponentMobility::Movable);

			staticMeshComponent->SetRenderCustomDepth(true);

			staticMeshComponent->TranslucencySortPriority = 1000;
			staticMeshComponent->SetTranslucencySortDistanceOffset(1000);

			auto dynamicMaterial = staticMeshComponent->CreateAndSetMaterialInstanceDynamicFromMaterial(0, sdkMat);
			dynamicMaterial->SetVectorParameterValue(
				SDK::UKismetStringLibrary::Conv_StringToName(L"Color"),
				SDK::FLinearColor{ 0.0f, 0.0f, 0.0f, 9999.0f }
			);

			//					dynamicMaterial->SetVectorParameterValue(
			//		SDK::UKismetStringLibrary::Conv_StringToName(L"TintColorAndOpacity"),
			//		SDK::FLinearColor{ 0.0f, 0.0f, 0.0f, 99999.0f }
			//	);
			//	dynamicMaterial->SetVectorParameterValue(
			//		SDK::UKismetStringLibrary::Conv_StringToName(L"BackColor"),
			//		SDK::FLinearColor{ 0.0f, 0.0f, 0.0f, 99999.0f }
			//	);
			//	dynamicMaterial->SetScalarParameterValue(
			//		SDK::UKismetStringLibrary::Conv_StringToName(L"OpacityFromTexture"),
			//		9999.0f
			//	);
			//	dynamicMaterial->SetTextureParameterValue(
			//		SDK::UKismetStringLibrary::Conv_StringToName(L"SlateUI"),
			//		texture
			//	);
			//}
		}
	}

	Custom2DScreen = actor;

	ResizeBlackBars();

	auto static cameraManager_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"BlueprintGeneratedClass /Game/Blueprints/Camera/BP_PlayerCameraManager.BP_PlayerCameraManager_C");
	auto cameraManagerActor = (SDK::AActor*)cameraManager_c->get_first_object_matching(false);

	Custom2DScreen->K2_AttachToActor(cameraManagerActor, 		
				SDK::FName(),                                      // SocketName (none)
				SDK::EAttachmentRule::KeepRelative,             // Use the relative location we just set
				SDK::EAttachmentRule::KeepRelative,             // Use the relative rotation we just set
				SDK::EAttachmentRule::KeepRelative,             // Use the relative scale we just set
				false                                            // bWeldSimulatedBodies
			);
	Logger::DebugPrint("custom box parent : %ls", actor->GetParentActor()->GetFullName().c_str());
}

void CameraController::ResizeBlackBars()
{
	if (Custom2DScreen == nullptr)
		return;

	// Dimensions (Half-extents)
    float H = 250.0f; 
    float W = H * (16.0f / 9.0f);
    float D = 2000.0f; // How "deep" the cinema box is
    
    float planeUnit = 100.0f;
	auto root = Custom2DScreen->K2_GetRootComponent();
	root->K2_SetRelativeLocation({*settingsManager->BlackBarsSizePtr * 100.0f -1000.0f , 0.0f, 0.0f }, false, nullptr, true);

    auto children = root->AttachChildren;
    for (int i = 0; i < children.Num(); i++)
    {
        auto smc = (SDK::UStaticMeshComponent*)children[i];
        switch (i)
        {
            case 0: // TOP (Ceiling)
                smc->K2_SetRelativeLocation({ 0.0f, 0.0f, H }, false, nullptr, true);
                smc->K2_SetRelativeRotation({ 180.0f, 0.0f, 0.0f }, false, nullptr, true);
                smc->SetWorldScale3D({ D/planeUnit, (W*2)/planeUnit, 1.0f });
                break;

            case 1: // BOTTOM (Floor)
                smc->K2_SetRelativeLocation({ 0.0f, 0.0f, -H }, false, nullptr, true);
                smc->K2_SetRelativeRotation({ 0.0f, 0.0f, 0.0f }, false, nullptr, true);
                smc->SetWorldScale3D({ D/planeUnit, (W*2)/planeUnit, 1.0f });
                break;

			case 2: // BACK (The wall behind the screen)
				// Move it back by the full depth D
				smc->K2_SetRelativeLocation({ -D * 0.5f, 0.0f, 0.0f }, false, nullptr, true);
				smc->K2_SetRelativeRotation({ 90.0f, 0.0f, 0.0f }, false, nullptr, true);
				smc->SetWorldScale3D({ (H * 2) / planeUnit, (W * 2) / planeUnit, 1.0f });
				break;

			case 3: // LEFT (Side wall)
				smc->K2_SetRelativeLocation({ 0.0f, -W, 0.0f }, false, nullptr, true);
				smc->K2_SetRelativeRotation({ 90.0f, 0.0f, 90.0f }, false, nullptr, true);
				smc->SetWorldScale3D({ (H * 2) / planeUnit, D / planeUnit, 1.0f });
				break;

			case 4: // RIGHT (Side wall)
				smc->K2_SetRelativeLocation({ 0.0f, W, 0.0f }, false, nullptr, true);
				smc->K2_SetRelativeRotation({ 90.0f, 0.0f, 90.0f }, false, nullptr, true);
				smc->SetWorldScale3D({ (H * 2) / planeUnit, D / planeUnit, 1.0f });
				break;
			//case 5: // TOP (Ceiling)
			//	smc->K2_SetRelativeLocation({ 0.0f, 0.0f, H }, false, nullptr, true);
			//	smc->K2_SetRelativeRotation({ 0.0f, 0.0f, 0.0f }, false, nullptr, true);
			//	smc->SetWorldScale3D({ D / planeUnit, (W * 2) / planeUnit, 1.0f });
			//	break;

			//case 6: // BOTTOM (Floor)
			//	smc->K2_SetRelativeLocation({ 0.0f, 0.0f, -H }, false, nullptr, true);
			//	smc->K2_SetRelativeRotation({ 0.0f, 0.0f, 0.0f }, false, nullptr, true);
			//	smc->SetWorldScale3D({ D / planeUnit, (W * 2) / planeUnit, 1.0f });
			//	break;

			//case 7: // BACK (The wall behind the screen)
			//	// Move it back by the full depth D
			//	smc->K2_SetRelativeLocation({ -D * 0.5f, 0.0f, 0.0f }, false, nullptr, true);
			//	smc->K2_SetRelativeRotation({ 90.0f, 0.0f, 0.0f }, false, nullptr, true);
			//	smc->SetWorldScale3D({ (H * 2) / planeUnit, (W * 2) / planeUnit, 1.0f });
			//	break;

			//case 8: // LEFT (Side wall)
			//	smc->K2_SetRelativeLocation({ 0.0f, -W, 0.0f }, false, nullptr, true);
			//	smc->K2_SetRelativeRotation({ 90.0f, 0.0f, 90.0f }, false, nullptr, true);
			//	smc->SetWorldScale3D({ (H * 2) / planeUnit, D / planeUnit, 1.0f });
			//	break;

			//case 9: // RIGHT (Side wall)
			//	smc->K2_SetRelativeLocation({ 0.0f, W, 0.0f }, false, nullptr, true);
			//	smc->K2_SetRelativeRotation({ 90.0f, 0.0f, 90.0f }, false, nullptr, true);
			//	smc->SetWorldScale3D({ (H * 2) / planeUnit, D / planeUnit, 1.0f });
			//	break;
		}
	}
}

//void CameraController::ProcessCustom2DScreenPositionAndRotation()
//{
//	//grab uevr position and rotation from callback and apply to the custom 2D screen
//	// need to delete the logic that attach the planes to the camera manager.
//}

void CameraController::DestroyCustom2DScreen()
{
	if (Custom2DScreen)
	{
		Custom2DScreen->K2_DestroyActor();
		Custom2DScreen = nullptr;
	}
}

