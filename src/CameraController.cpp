#include "CameraController.hpp"

void CameraController::ToggleCameraWobblingController(bool enabled) {
	//static auto pawn = uevr::API::get()->get_local_pawn(0);
	//Logger::DebugPrint("Camera Recoil : " + enabled);
	static ue::ClassRef wobblingController_c{ L"Class /Script/Gsr.GsrWobblingController" };
	auto wobblingController = wobblingController_c.get() ? wobblingController_c.get()->get_first_object_matching(false) : nullptr;
	if (!wobblingController)
		return;
	//Logger::DebugPrint(wobblingController->get_full_name());
	ue::call(wobblingController, L"SetEnabled", enabled);
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
		ue::call(CameraAttachementActor, L"K2_DestroyActor");
		Logger::DebugPrint("Camera Attachement Actor Destroyed");
	}
	// Also when the actor is already gone (destroyed with its level)
	CameraAttachementActor = nullptr;
	IsCameraAttachedToAttachementActor = false;
	CameraAttachementActorAvailable = false;
}

void CameraController::AttachCameraToAttachementActor()
{
	if (!CameraAttachementActor) return;
	auto rootComponent = ue::get<ue::UObject*>(CameraAttachementActor, L"RootComponent");
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
	auto rootComponent = ue::get<ue::UObject*>(CameraAttachementActor, L"RootComponent");
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

void CameraController::CalculateForwardOffset(const ue::FVector& location, const ue::FRotator& rotation, float fov, float &forwardOffset, glm::fvec3 &forward)
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

	static ue::SingletonRef cobraCutsceneCameraCtrlMngr_c{ L"Class /Script/CobraCutScene.CobraCutsceneCameraControlManagerComponent" };
	auto cobraCutsceneCameraCtrlMngr = cobraCutsceneCameraCtrlMngr_c.get();
	if (!cobraCutsceneCameraCtrlMngr)
	{
		Logger::DebugPrint("CobraCutsceneCameraCtrlMngr not found");
		return;
	}

	static ue::SingletonRef playerCameraManager_c{ L"Class /Script/Cobra.CobraPlayerCameraManager" };
	auto playerCameraManager = playerCameraManager_c.get();
	if (!playerCameraManager)
	{
		Logger::DebugPrint("PlayerCameraManager not found");
		return;
	}
	// PlayerCameraManager->CameraCachePrivate.POV
	const auto pov = ue::get_struct(playerCameraManager, L"CameraCachePrivate").sub(L"POV");
	if (!pov)
		return;
	const auto ueLocation = pov.get<ue::FVector>(L"Location");
	const auto ueRotation = pov.get<ue::FRotator>(L"Rotation");
	glm::fvec3 originalCamLocation = { ueLocation.X, ueLocation.Y, ueLocation.Z };
	glm::fvec3 originalCamRotation = { ueRotation.Pitch, ueRotation.Yaw, ueRotation.Roll };

	float actualFOV = pov.get<float>(L"FOV");

	if (ActualCutsceneState == CutsceneState::Playing3D)
	{
		if (ue::get<bool>(cobraCutsceneCameraCtrlMngr, L"bIsFpv"))
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
		if (!pawn)
			return;
		//UEVR_Vector3d floorPosition;
		//pawn->call_function(L"GetFloorPosition", &floorPosition);
		//Logger::DebugPrint("GetFloorPosition : %f, %f, %f", floorPosition.x, floorPosition.y, floorPosition.z);
		const auto pawnLocation = ue::call<ue::FVector>(pawn, L"K2_GetActorLocation");
		UEVR_Vector3d pawnPosition{ pawnLocation.X, pawnLocation.Y, pawnLocation.Z };
		//Logger::DebugPrint("Pawn Position : %f, %f, %f", pawnPosition.x, pawnPosition.y, pawnPosition.z);
		auto grassVolumeCollisionHead = ue::get<ue::UObject*>(pawn, L"GrassVolumeCollisionHead");

		const auto headLocation = ue::call<ue::FVector>(grassVolumeCollisionHead, L"K2_GetComponentLocation");
		headColliderPosition = { headLocation.X, headLocation.Y, headLocation.Z };
		//Logger::DebugPrint("headColliderPosition : %f, %f, %f", headColliderPosition.x, headColliderPosition.y, headColliderPosition.z);

		// Reset camera position and rotation when player respawn or teleport
		if (InstantCameraPositionNeeded)
		{
			LastCameraPosition = { pawnPosition.x, pawnPosition.y, (float)headColliderPosition.z };
			//CameraHeightOffset = pawnPosition.z - headColliderPosition.z;
			NewCameraPosition = { pawnPosition.x, pawnPosition.y, (float)headColliderPosition.z };

			const auto pawnRotation = ue::call<ue::FRotator>(pawn, L"K2_GetActorRotation");
			ActualYaw = pawnRotation.Yaw;
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
			case ue::EGsrPlayerStance::Stand:
				CameraHeightOffset = 83.0f; //-85.0f
				break;
			case ue::EGsrPlayerStance::Squat:
				CameraHeightOffset = 6.7f;
				break;
			case ue::EGsrPlayerStance::Crawl:
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
			case ue::EGsrPlayerStance::Stand:
				CameraHeightOffset = 83.0f; //-85.0f
				break;
			case ue::EGsrPlayerStance::Squat:
				CameraHeightOffset = 83.0f;
				break;
			case ue::EGsrPlayerStance::Crawl:
				// playerPosition.Z actualise one frame after change stance, we need to compensate for that or it results in a sudden camera jump
				CameraHeightOffset = playerManager->PreviousStance == ue::EGsrPlayerStance::Squat ? 83.0f : 83.0f + crawlOffset;
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

		if (playerManager->CurrentMenuType == ue::EGsrUIMenuType::None)
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

		if (CurrentItemID == ue::EGsrItemId::IT_Scope)
		{
			// if fpv is enabled, we need to push the camera forward based on the FOV to prevent it from being too zoomed out
			float forwardOffset = 0.0f;
			glm::fvec3 forward{};
			const auto hmdRotation = ue::call<ue::FRotator>(CameraAttachementActor, L"K2_GetActorRotation");
			CalculateForwardOffset({newPosition.x, newPosition.y, newPosition.z}, hmdRotation, actualFOV, forwardOffset, forward);
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
//	static ue::ClassRef cameraFadingComponent_c{ L"Class /Script/Cobra.CameraFadingComponent" };
//	if (!cameraFadingComponent_c.get())
//		return;
//	for (auto component : cameraFadingComponent_c.get()->get_objects_matching())
//	{
//		Logger::DebugPrint(component->get_full_name());
//		ue::set(component, L"bAutoActivate", false);
//		ue::set(component, L"GrassFadingRadius", 0.0f);
//		ue::call(component, L"ClearCache");
//		ue::call(component, L"Deactivate");
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
	auto cameraAttachementPosition = ue::call<ue::FVector>(CameraAttachementActor, L"K2_GetActorLocation");
	//Logger::DebugPrint("cameraAttachementPosition : %f", cameraAttachementPosition.Z);
	//Logger::DebugPrint("headColliderPosition : %f", headColliderPosition.z);
	auto difference = cameraAttachementPosition.Z - headColliderPosition.z;
	//Logger::DebugPrint("Head Collider - cameraAttachementPosition Difference : %f", difference);

	UEVR_Matrix4x4f hmdTransform{};
	uevr::API::get()->param()->vr->get_transform(uevr::API::VR::get_hmd_index(), &hmdTransform);
	auto hmdHeight = hmdTransform.m[3][1];
	//Logger::DebugPrint("HMD Transform Z Position : %f", hmdTransform.m[3][1]);

	if (playerManager->CurrentStance == ue::EGsrPlayerStance::Stand && hmdHeight <= *settingsManager->SquatUnderThisValuePtr)
		playerManager->SetPlayerStance(ue::EGsrPlayerStance::Squat);

	if (playerManager->CurrentStance == ue::EGsrPlayerStance::Squat && hmdHeight <= *settingsManager->CrawlUnderThisValuePtr)
		playerManager->SetPlayerStance(ue::EGsrPlayerStance::Crawl);

	if (playerManager->CurrentStance == ue::EGsrPlayerStance::Squat && hmdHeight >= *settingsManager->SquatUnderThisValuePtr)
		playerManager->SetPlayerStance(ue::EGsrPlayerStance::Stand);

	if (playerManager->CurrentStance == ue::EGsrPlayerStance::Crawl && hmdHeight >= *settingsManager->CrawlUnderThisValuePtr)
		playerManager->SetPlayerStance(ue::EGsrPlayerStance::Squat);
}

//void CameraController::SetNearClipPlane(float nearClip)
//{
//	static ue::ClassRef playerCameraActor_c{ L"Class /Script/Gsr.GsrPlayerCameraActor" };
//	auto playerCamera = playerCameraActor_c.get() ? playerCameraActor_c.get()->get_first_object_matching(false) : nullptr;
//	auto cameraComponent = ue::get<ue::UObject*>(playerCamera, L"CameraComponent");
//	ue::call(cameraComponent, L"SetOrthoNearClipPlane", nearClip);
//	Logger::DebugPrint("%f", ue::get<float>(cameraComponent, L"OrthoNearClipPlane"));
//}

void CameraController::FixSurvivalViewerMenuRendering()
{
	static ue::ClassRef sv_main_c{ L"WidgetBlueprintGeneratedClass /CobraUI/Blueprint/sv_main.sv_main_C" };
	auto mainWidget = sv_main_c.get() ? sv_main_c.get()->get_first_object_matching(false) : nullptr;
	if (!mainWidget)
		return;

	ue::call(ue::get<ue::UObject*>(mainWidget, L"ModelCaptureCanvas"), L"SetVisibility", ue::ESlateVisibility::Hidden);
	//ue::call(ue::get<ue::UObject*>(mainWidget, L"menuBLIcon"), L"SetVisibility", ue::ESlateVisibility::Visible);
	//ue::call(ue::get<ue::UObject*>(mainWidget, L"Mask"), L"SetVisibility", ue::ESlateVisibility::Hidden);
	//
	// Get sub menu type : mainWidget->GetSvStep();

	auto rootWidgetCanvasPanel = ue::get<ue::UObject*>(ue::get<ue::UObject*>(mainWidget, L"WidgetTree"), L"RootWidget");
	auto rootSlots = ue::get_ptr<ue::TArray<ue::UObject*>>(rootWidgetCanvasPanel, L"Slots");
	if (!rootSlots)
		return;
	Logger::DebugPrint(rootWidgetCanvasPanel->get_full_name());

	// Loop through slots of the root panel
	Logger::DebugPrint("Root widget slots:");
	for (auto slot : *rootSlots)
	{
		if (!slot)
			continue;
		Logger::DebugPrint(slot->get_full_name());

		// Access the widget contained in this slot
		auto containedWidget = ue::get<ue::UObject*>(slot, L"Content");
		if (containedWidget) {
			Logger::DebugPrint(L"  Contains: " + containedWidget->get_full_name());
		}
	}

	// If you want to access a specific child panel's slots
	// Find a specific child widget by name or index
	if (!rootSlots->empty()) {
		static ue::ClassRef panelWidget_c{ L"Class /Script/UMG.PanelWidget" };

		// Get the first slot's content (which might be another panel)
		auto firstChildWidget = ue::get<ue::UObject*>(rootSlots->data[0], L"Content");

		// If it's a panel, you can access its slots
		if (ue::is_a(firstChildWidget, panelWidget_c)) {
			auto childSlots = ue::get_ptr<ue::TArray<ue::UObject*>>(firstChildWidget, L"Slots");
			for (int i = 0; childSlots && i < childSlots->count; i++) {
				auto slot = childSlots->data[i];
				if (!slot)
					continue;
				const std::wstring slotName = slot->get_fname()->to_string(); // Get just the name without path

				if (slotName == L"CanvasPanelSlot_10") {
					ue::call(ue::get<ue::UObject*>(slot, L"Content"), L"SetVisibility", ue::ESlateVisibility::Hidden);
					Logger::DebugPrint("Found target slot at index: %i", i);
				}
				if (slotName == L"CanvasPanelSlot_1")
				{
					ue::call(ue::get<ue::UObject*>(slot, L"Content"), L"SetVisibility", ue::ESlateVisibility::Hidden);
					Logger::DebugPrint("Found target slot at index: %i", i);
				}
			}
		}
	}
}

void CameraController::FixRadioMenuRendering()
{
	static ue::ClassRef Radio_c{ L"WidgetBlueprintGeneratedClass /CobraUI/Blueprint/Radio/Radio.Radio_C" };
	auto mainWidget = Radio_c.get() ? Radio_c.get()->get_first_object_matching(false) : nullptr;
	ue::call(ue::get<ue::UObject*>(mainWidget, L"ModelBg"), L"SetVisibility", ue::ESlateVisibility::Hidden);
}

void CameraController::CheckCutscenePlaying()
{
	static ue::SingletonRef gameStateSubsystem_c{ L"Class /Script/Gsr.GsrGameStateSubsystem" };
	auto gameStateSubsystem = gameStateSubsystem_c.get();
	bool cutscene = false;
	bool movie = false;

	if (gameStateSubsystem)
	{
		cutscene = ue::call<bool>(gameStateSubsystem, L"CheckGameStatus", ue::EGsrGameStatus::Cutscene);
		movie = ue::call<bool>(gameStateSubsystem, L"CheckGameStatus", ue::EGsrGameStatus::Movie);
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
		Logger::DebugPrint("Cutscene State changed to : %s", std::string(magic_enum::enum_name(ActualCutsceneState)).c_str());
		uevr::API::VR::set_mod_value("VR_2DScreenMode", ActualCutsceneState == CameraController::CutsceneState::Playing2D ?	true : false);
		uevr::API::VR::set_mod_value("VR_WorldScale", *settingsManager->Enable3DCutscenesPtr ? 1.0f : 0.0f);
		uevr::API::VR::set_mod_value("VR_DecoupledPitch", *settingsManager->EnableImmersiveModePtr);
	}
	if (CutsceneStops())
	{
		DestroyCustom2DScreen();
		Logger::DebugPrint("Cutscene State changed to : %s", std::string(magic_enum::enum_name(ActualCutsceneState)).c_str());
		uevr::API::VR::set_mod_value("VR_DecoupledPitch", true);
		uevr::API::VR::set_mod_value("VR_2DScreenMode", false);
		uevr::API::VR::set_mod_value("VR_WorldScale", 1.0f);
	}

	if (CutscenePlaying())
	{
		static ue::SingletonRef cobraCutsceneCameraCtrlMngr_c{ L"Class /Script/CobraCutScene.CobraCutsceneCameraControlManagerComponent" };
		auto cobraCutsceneCameraCtrlMngr = cobraCutsceneCameraCtrlMngr_c.get();
		if (!cobraCutsceneCameraCtrlMngr)
		{
			Logger::DebugPrint("CobraCutsceneCameraCtrlMngr not found");
			return;
		}

		if (!*settingsManager->EnableImmersiveModePtr)
		{
			if (ue::get<bool>(cobraCutsceneCameraCtrlMngr, L"bIsFpv"))
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

	static ue::ClassRef staticMesh_c{ L"Class /Script/Engine.StaticMesh" };
	static ue::ClassRef staticMeshComponent_c{ L"Class /Script/Engine.StaticMeshComponent" };

    // 1. Find the assets already loaded in memory. Checked before spawning the actor: this runs every
	// frame of a cutscene until it succeeds, so a failure must not leave an actor behind.
	auto staticMesh = ue::find_object(L"StaticMesh /Engine/BasicShapes/Plane.Plane");
    if (ue::is_a(staticMesh, staticMesh_c))
    {
		Logger::DebugPrint(L"Found static mesh : " + staticMesh->get_full_name());
	}
	else
	{
		Logger::DebugPrint("Asset found is not a StaticMesh or SkeletalMesh");
		return;
	}

	auto translucentMat = ue::find_object(L"Material /Engine/EngineDebugMaterials/DebugMeshMaterial.DebugMeshMaterial");
	if (!translucentMat)
	{
		Logger::DebugPrint("Failed to find material for 2D screen");
		return;
	}

	UEVR_Vector3d uevrVector = { 0.0f, 0.0f, 0.0f };
	auto actor = Utilities::SpawnActor(uevrVector, nullptr);
	if (!actor)
	{
		return;
	}

	ue::set(translucentMat, L"TwoSided", true);
	ue::set(translucentMat, L"bDisableDepthTest", true);
	ue::set(translucentMat, L"BlendMode", ue::EBlendMode::BLEND_Translucent);
	//ue::set(translucentMat, L"MaterialDomain", ue::EMaterialDomain::MD_Surface);
	ue::set(translucentMat, L"ShadingModel", ue::EMaterialShadingModel::MSM_Unlit);
	////Material /Engine/EngineMaterials/DefaultMaterial.DefaultMaterial

	//auto textureUEVR = uevr::API::get()->find_uobject<uevr::API::UObject>(L"Texture2D /CobraUI/Material/HUD/T_GoggleVignette.T_GoggleVignette");

	// create 5 planes for the box
	for (int i = 0; i < 5; i++)
	{
		auto staticMeshComponent = ue::call<ue::UObject*>(actor, L"AddComponentByClass",
			staticMeshComponent_c.get(),
			false,            // bManualAttachment
			ue::FTransform{}, // RelativeTransform
			false);           // bDeferredFinish

		if (!staticMeshComponent)
		{
			Logger::DebugPrint("Failed to create UStaticMeshComponent");
			ue::call(actor, L"K2_DestroyActor");
			return;
		}
		Logger::DebugPrint(L"Created StaticMeshComponent: " + staticMeshComponent->get_full_name());
		// 3. Assign the mesh asset to our new component instance
		ue::call(staticMeshComponent, L"SetStaticMesh", staticMesh);
		Logger::DebugPrint("Assigned mesh to StaticMeshComponent");

		// 4. Attach the new component to the Actor's root component
		ue::call(staticMeshComponent, L"K2_AttachToComponent",
			ue::call<ue::UObject*>(actor, L"K2_GetRootComponent"), // Parent
			ue::FName{},                                          // SocketName (none)
			ue::EAttachmentRule::KeepRelative,                    // Use the relative location we just set
			ue::EAttachmentRule::KeepRelative,                    // Use the relative rotation we just set
			ue::EAttachmentRule::KeepRelative,                    // Use the relative scale we just set
			false);                                               // bWeldSimulatedBodies
		Logger::DebugPrint("Attached StaticMeshComponent to Actor's root component");

		// 6. Register the component to make it "live" and renderable in the world
		ue::call(staticMeshComponent, L"Activate", true);
		Logger::DebugPrint("Activated StaticMeshComponent");

		// Optional: Disable collision if it's just for visuals
		ue::call(staticMeshComponent, L"SetCollisionEnabled", ue::ECollisionEnabled::NoCollision);
		ue::call(staticMeshComponent, L"Activate", true);
		ue::call(staticMeshComponent, L"SetVisibility", true, false);
		ue::call(staticMeshComponent, L"SetHiddenInGame", false, false);
		ue::call(staticMeshComponent, L"SetWorldScale3D", ue::FVector{ 1, 1, 1 });
		ue::call(staticMeshComponent, L"SetMobility", ue::EComponentMobility::Movable);

		ue::call(staticMeshComponent, L"SetRenderCustomDepth", true);

		ue::set<int32_t>(staticMeshComponent, L"TranslucencySortPriority", 1000);
		ue::call(staticMeshComponent, L"SetTranslucencySortDistanceOffset", 1000.0f);

		auto dynamicMaterial = ue::call<ue::UObject*>(staticMeshComponent, L"CreateAndSetMaterialInstanceDynamicFromMaterial", 0, translucentMat);
		ue::call(dynamicMaterial, L"SetVectorParameterValue",
			ue::FName(L"Color"),
			ue::FLinearColor{ 0.0f, 0.0f, 0.0f, 9999.0f }
		);

		//ue::call(dynamicMaterial, L"SetVectorParameterValue", ue::FName(L"TintColorAndOpacity"), ue::FLinearColor{ 0.0f, 0.0f, 0.0f, 99999.0f });
		//ue::call(dynamicMaterial, L"SetVectorParameterValue", ue::FName(L"BackColor"), ue::FLinearColor{ 0.0f, 0.0f, 0.0f, 99999.0f });
		//ue::call(dynamicMaterial, L"SetScalarParameterValue", ue::FName(L"OpacityFromTexture"), 9999.0f);
		//ue::call(dynamicMaterial, L"SetTextureParameterValue", ue::FName(L"SlateUI"), textureUEVR);
	}

	Custom2DScreen = actor;

	ResizeBlackBars();

	static ue::ClassRef cameraManager_c{ L"BlueprintGeneratedClass /Game/Blueprints/Camera/BP_PlayerCameraManager.BP_PlayerCameraManager_C" };
	auto cameraManagerActor = cameraManager_c.get() ? cameraManager_c.get()->get_first_object_matching(false) : nullptr;

	ue::call(Custom2DScreen, L"K2_AttachToActor", cameraManagerActor,
				ue::FName{},                                      // SocketName (none)
				ue::EAttachmentRule::KeepRelative,             // Use the relative location we just set
				ue::EAttachmentRule::KeepRelative,             // Use the relative rotation we just set
				ue::EAttachmentRule::KeepRelative,             // Use the relative scale we just set
				false                                            // bWeldSimulatedBodies
			);
	if (auto parentActor = ue::call<ue::UObject*>(actor, L"GetParentActor"))
		Logger::DebugPrint(L"custom box parent : " + parentActor->get_full_name());
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
	auto root = ue::call<ue::UObject*>(Custom2DScreen, L"K2_GetRootComponent");
	ue::call(root, L"K2_SetRelativeLocation", ue::FVector{*settingsManager->BlackBarsSizePtr * 100.0f -1000.0f , 0.0f, 0.0f }, false, nullptr, true);

    auto children = ue::get_ptr<ue::TArray<ue::UObject*>>(root, L"AttachChildren");
    for (int i = 0; children && i < children->count; i++)
    {
        auto smc = children->data[i];
        switch (i)
        {
            case 0: // TOP (Ceiling)
                ue::call(smc, L"K2_SetRelativeLocation", ue::FVector{ 0.0f, 0.0f, H }, false, nullptr, true);
                ue::call(smc, L"K2_SetRelativeRotation", ue::FRotator{ 180.0f, 0.0f, 0.0f }, false, nullptr, true);
                ue::call(smc, L"SetWorldScale3D", ue::FVector{ D/planeUnit, (W*2)/planeUnit, 1.0f });
                break;

            case 1: // BOTTOM (Floor)
                ue::call(smc, L"K2_SetRelativeLocation", ue::FVector{ 0.0f, 0.0f, -H }, false, nullptr, true);
                ue::call(smc, L"K2_SetRelativeRotation", ue::FRotator{ 0.0f, 0.0f, 0.0f }, false, nullptr, true);
                ue::call(smc, L"SetWorldScale3D", ue::FVector{ D/planeUnit, (W*2)/planeUnit, 1.0f });
                break;

			case 2: // BACK (The wall behind the screen)
				// Move it back by the full depth D
				ue::call(smc, L"K2_SetRelativeLocation", ue::FVector{ -D * 0.5f, 0.0f, 0.0f }, false, nullptr, true);
				ue::call(smc, L"K2_SetRelativeRotation", ue::FRotator{ 90.0f, 0.0f, 0.0f }, false, nullptr, true);
				ue::call(smc, L"SetWorldScale3D", ue::FVector{ (H * 2) / planeUnit, (W * 2) / planeUnit, 1.0f });
				break;

			case 3: // LEFT (Side wall)
				ue::call(smc, L"K2_SetRelativeLocation", ue::FVector{ 0.0f, -W, 0.0f }, false, nullptr, true);
				ue::call(smc, L"K2_SetRelativeRotation", ue::FRotator{ 90.0f, 0.0f, 90.0f }, false, nullptr, true);
				ue::call(smc, L"SetWorldScale3D", ue::FVector{ (H * 2) / planeUnit, D / planeUnit, 1.0f });
				break;

			case 4: // RIGHT (Side wall)
				ue::call(smc, L"K2_SetRelativeLocation", ue::FVector{ 0.0f, W, 0.0f }, false, nullptr, true);
				ue::call(smc, L"K2_SetRelativeRotation", ue::FRotator{ 90.0f, 0.0f, 90.0f }, false, nullptr, true);
				ue::call(smc, L"SetWorldScale3D", ue::FVector{ (H * 2) / planeUnit, D / planeUnit, 1.0f });
				break;
			//case 5: // TOP (Ceiling)
			//	ue::call(smc, L"K2_SetRelativeLocation", ue::FVector{ 0.0f, 0.0f, H }, false, nullptr, true);
			//	ue::call(smc, L"K2_SetRelativeRotation", ue::FRotator{ 0.0f, 0.0f, 0.0f }, false, nullptr, true);
			//	ue::call(smc, L"SetWorldScale3D", ue::FVector{ D / planeUnit, (W * 2) / planeUnit, 1.0f });
			//	break;

			//case 6: // BOTTOM (Floor)
			//	ue::call(smc, L"K2_SetRelativeLocation", ue::FVector{ 0.0f, 0.0f, -H }, false, nullptr, true);
			//	ue::call(smc, L"K2_SetRelativeRotation", ue::FRotator{ 0.0f, 0.0f, 0.0f }, false, nullptr, true);
			//	ue::call(smc, L"SetWorldScale3D", ue::FVector{ D / planeUnit, (W * 2) / planeUnit, 1.0f });
			//	break;

			//case 7: // BACK (The wall behind the screen)
			//	// Move it back by the full depth D
			//	ue::call(smc, L"K2_SetRelativeLocation", ue::FVector{ -D * 0.5f, 0.0f, 0.0f }, false, nullptr, true);
			//	ue::call(smc, L"K2_SetRelativeRotation", ue::FRotator{ 90.0f, 0.0f, 0.0f }, false, nullptr, true);
			//	ue::call(smc, L"SetWorldScale3D", ue::FVector{ (H * 2) / planeUnit, (W * 2) / planeUnit, 1.0f });
			//	break;

			//case 8: // LEFT (Side wall)
			//	ue::call(smc, L"K2_SetRelativeLocation", ue::FVector{ 0.0f, -W, 0.0f }, false, nullptr, true);
			//	ue::call(smc, L"K2_SetRelativeRotation", ue::FRotator{ 90.0f, 0.0f, 90.0f }, false, nullptr, true);
			//	ue::call(smc, L"SetWorldScale3D", ue::FVector{ (H * 2) / planeUnit, D / planeUnit, 1.0f });
			//	break;

			//case 9: // RIGHT (Side wall)
			//	ue::call(smc, L"K2_SetRelativeLocation", ue::FVector{ 0.0f, W, 0.0f }, false, nullptr, true);
			//	ue::call(smc, L"K2_SetRelativeRotation", ue::FRotator{ 90.0f, 0.0f, 90.0f }, false, nullptr, true);
			//	ue::call(smc, L"SetWorldScale3D", ue::FVector{ (H * 2) / planeUnit, D / planeUnit, 1.0f });
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
		ue::call(Custom2DScreen, L"K2_DestroyActor");
		Custom2DScreen = nullptr;
	}
}

