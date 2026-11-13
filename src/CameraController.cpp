#include "CameraController.hpp"

void CameraController::ToggleCameraWobblingController(bool enabled) {
	static ue::ClassRef wobblingController_c{ L"Class /Script/Gsr.GsrWobblingController" };
	auto wobblingController = wobblingController_c.get() ? wobblingController_c.get()->get_first_object_matching(false) : nullptr;
	if (!wobblingController)
		return;
	ue::call(wobblingController, L"SetEnabled", enabled);
}

bool CameraController::SpawnCameraAttachementActor()
{
	CameraAttachementActor = Utilities::SpawnActor(UEVR_Vector3d{ LastCameraPosition.x, LastCameraPosition.y, LastCameraPosition.z }, nullptr);
	return CameraAttachementActor != nullptr;
}

void CameraController::DestroyCameraAttachementActor()
{
	if (CameraAttachementActor != nullptr) {
		ue::call(CameraAttachementActor, L"K2_DestroyActor");
		Logger::DebugPrint("Camera Attachement Actor Destroyed");
	}
	// Also when the actor is already gone (destroyed with its level)
	CameraAttachementActor = nullptr;
}

void CameraController::AttachCameraToAttachementActor()
{
	if (!CameraAttachementActor) return;
	auto rootComponent = ue::get<ue::UObject*>(CameraAttachementActor, L"RootComponent");
	if (rootComponent) {
		auto motionState = uevr::API::UObjectHook::get_or_add_motion_controller_state(rootComponent);
		motionState->set_hand(2);
		motionState->set_permanent(true);
		Logger::DebugPrint("Actor attached to VR Camera");
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
    } else {
        Logger::DebugPrint("DetachCameraFromAttachementActor: RootComponent is null");
    }
}

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
	float scale = gameState->CurrentCutsceneState == GameState::Playing3D ? 60.0f : 120.0f;
	forwardOffset = (zoomFactor - 1.0f) * scale; // 100.0f = scale, tune this
}

void CameraController::ProcessCamera(float deltaTime)
{
	// Cap deltaTime to prevent camera from yeeting into orbit when the game freezes or low fps.
	deltaTime = glm::min(deltaTime, 0.05f);
	glm::fvec3 tempPosition;
	glm::fvec3 newPosition;
	glm::fvec3 newRotation;
	float remappedLerpXYMultiplier = Utilities::Remap(settingsManager->Settings.cameraPositionXYLerp, 0.0f, 10.0f, 4.0f, 0.1f);
	float remappedLerpZMultiplier = Utilities::Remap(settingsManager->Settings.cameraPositionZLerp, 0.0f, 10.0f, 4.0f, 0.1f);

	static ue::SingletonRef cobraCutsceneCameraCtrlMngr_c{ L"Class /Script/CobraCutScene.CobraCutsceneCameraControlManagerComponent" };
	auto cobraCutsceneCameraCtrlMngr = cobraCutsceneCameraCtrlMngr_c.get(); // may be null: ue::get then returns false

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

	if (gameState->CurrentCutsceneState == GameState::Playing3D)
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

		if (settingsManager->Settings.enableImmersiveMode)
		{
			if (settingsManager->Settings.smoothCameraMovementsInImmersiveCutscenes)
			{
				// prevent camera yeeting into orbit on low fps
				float remappedCutscenePositionLerpMultiplier = Utilities::Remap(settingsManager->Settings.cutsceneCameraPositionLerp, 0.0f, 10.0f, 4.0f, 0.1f);
				float speedXY = 11.25f * remappedCutscenePositionLerpMultiplier;
				float alphaXY = 1.0f - expf(-speedXY * deltaTime);
				float speedZ = 2.25f * remappedCutscenePositionLerpMultiplier;
				float alphaZ = 1.0f - expf(-speedZ * deltaTime);
				newPosition = settingsManager->Settings.smoothCameraMovementsInImmersiveCutscenes ?
					glm::fvec3(
						glm::mix(LastCameraPosition.x, originalCamLocation.x, alphaXY),
						glm::mix(LastCameraPosition.y, originalCamLocation.y, alphaXY),
						glm::mix(LastCameraPosition.z, originalCamLocation.z, alphaZ)) :
					originalCamLocation;

				NewCameraPosition = { newPosition.x, newPosition.y, newPosition.z };
				LastCameraPosition = newPosition;

				float remappedRotationLerpMultiplier = Utilities::Remap(settingsManager->Settings.cutsceneCameraRotationLerp, 0.0f, 10.0f, 2.0f, 0.1f);
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
		auto pawn = gameState->Pawn;
		if (!pawn)
			return;
		const auto pawnLocation = ue::call<ue::FVector>(pawn, L"K2_GetActorLocation");
		UEVR_Vector3d pawnPosition{ pawnLocation.X, pawnLocation.Y, pawnLocation.Z };
		auto grassVolumeCollisionHead = ue::get<ue::UObject*>(pawn, L"GrassVolumeCollisionHead");

		const auto headLocation = ue::call<ue::FVector>(grassVolumeCollisionHead, L"K2_GetComponentLocation");
		headColliderPosition = { headLocation.X, headLocation.Y, headLocation.Z };

		// Reset camera position and rotation when player respawn or teleport
		if (InstantCameraPositionNeeded)
		{
			LastCameraPosition = { pawnPosition.x, pawnPosition.y, (float)headColliderPosition.z };
			NewCameraPosition = { pawnPosition.x, pawnPosition.y, (float)headColliderPosition.z };

			const auto pawnRotation = ue::call<ue::FRotator>(pawn, L"K2_GetActorRotation");
			ActualYaw = pawnRotation.Yaw;
			LastCameraRotation.yaw = ActualYaw;
			NewCameraRotation.yaw = ActualYaw;
			return;
		}

		if (!settingsManager->Settings.enableRoomscaleProneAndCrouch)
		{
			switch (gameState->CurrentStance)
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
			switch (gameState->CurrentStance)
			{
			case ue::EGsrPlayerStance::Stand:
				CameraHeightOffset = 83.0f; //-85.0f
				break;
			case ue::EGsrPlayerStance::Squat:
				CameraHeightOffset = 83.0f;
				break;
			case ue::EGsrPlayerStance::Crawl:
				// playerPosition.Z actualise one frame after change stance, we need to compensate for that or it results in a sudden camera jump
				CameraHeightOffset = gameState->CurrentStance.Previous() == ue::EGsrPlayerStance::Squat ? 83.0f : 83.0f + crawlOffset;
				break;
			default:
				break;
			}
		}





		if (playerManager->IsPeepingInternal) // initial peep movement
		{
			tempPosition = settingsManager->Settings.attachCameraToSnakeHead ?
				glm::fvec3(headColliderPosition.x, headColliderPosition.y, headColliderPosition.z) :
				glm::fvec3(headColliderPosition.x, headColliderPosition.y, pawnPosition.z + CameraHeightOffset);
			// prevent camera yeeting into orbit on low fps
			constexpr float speed = 3.0f;
			float alpha = 1.0f - expf(-speed * deltaTime);
			newPosition = settingsManager->Settings.smoothCameraPosition ?
				glm::mix(LastCameraPosition, tempPosition, alpha) :
				tempPosition;
		}
		else if (playerManager->IsPeeping) // stops peeping
		{
			tempPosition = settingsManager->Settings.attachCameraToSnakeHead ?
				glm::fvec3(headColliderPosition.x, headColliderPosition.y, headColliderPosition.z) :
				glm::fvec3(pawnPosition.x, pawnPosition.y, pawnPosition.z + CameraHeightOffset);
			// prevent camera yeeting into orbit on low fps
			float speedXY = 4.5f * remappedLerpXYMultiplier;
			float alphaXY = 1.0f - expf(-speedXY * deltaTime);
			float speedZ = 2.25f * remappedLerpZMultiplier;
			float alphaZ = 1.0f - expf(-speedZ * deltaTime);
			newPosition = settingsManager->Settings.smoothCameraPosition ?
				glm::fvec3(
					glm::mix(LastCameraPosition.x, tempPosition.x, alphaXY),
					glm::mix(LastCameraPosition.y, tempPosition.y, alphaXY),
					glm::mix(LastCameraPosition.z, tempPosition.z, alphaZ)) :
				tempPosition;
		}

		else
		{
			tempPosition = settingsManager->Settings.attachCameraToSnakeHead || CardBoardEquipped || (gameState->IsFirstPersonView && !settingsManager->Settings.enableRoomscaleProneAndCrouch) ?
				glm::fvec3(headColliderPosition.x, headColliderPosition.y, headColliderPosition.z) :
				glm::fvec3(pawnPosition.x, pawnPosition.y, pawnPosition.z + CameraHeightOffset);
			// prevent camera yeeting into orbit on low fps
			float speedXY = 11.25f * remappedLerpXYMultiplier;
			float alphaXY = 1.0f - expf(-speedXY * deltaTime);
			float speedZ = (CardBoardEquipped ? 5.0f : 2.25f) * remappedLerpZMultiplier;
			float alphaZ = 1.0f - expf(-speedZ * deltaTime);
			newPosition = settingsManager->Settings.smoothCameraPosition ?
				glm::fvec3(
					glm::mix(LastCameraPosition.x, tempPosition.x, alphaXY),
					glm::mix(LastCameraPosition.y, tempPosition.y, alphaXY),
					glm::mix(LastCameraPosition.z, tempPosition.z, alphaZ)) :
				tempPosition;
		}

		LastCameraPosition = newPosition;
		NewCameraPosition.x = newPosition.x;
		NewCameraPosition.y = newPosition.y;
		NewCameraPosition.z = newPosition.z;

		if (gameState->CurrentMenuType == ue::EGsrUIMenuType::None)
		{
			YawProcessed = YawInput * 3.0f * settingsManager->Settings.cameraRotationSpeed * deltaTime;
			if (YawInput != 0.0)
				ActualYaw += YawProcessed;
		}

		/*Logger::DebugPrint("rotation->yaw: %f", rotation->yaw);*/

		//The Imgui menu shows the smoothness "quantity", not the lerp factor. We need the inverse for the calculation. Makes more sense for users.
		float remappedRotationLerpMultiplier = Utilities::Remap(settingsManager->Settings.cameraYawLerp, 0.0f, 10.0f, 2.0f, 0.1f);
		// prevent camera yeeting into orbit on low fps
		float speed = 10.25f * remappedRotationLerpMultiplier;
		float alpha = 1.0f - expf(-speed * deltaTime);
		NewCameraRotation.yaw = settingsManager->Settings.smoothCameraRotation ?
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
	}
}

void CameraController::ProcessCameraResetTimer(float delta)
{
	if (gameState->PlayerGotControl())
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

void CameraController::ProcessRoomscaleStance()
{
	if (!settingsManager->Settings.enableRoomscaleProneAndCrouch)
		return;

	if (!CameraAttachementActor)
	{
		Logger::DebugPrint("ProcessRoomscaleStance: CameraAttachementActor is null");
		return;
	}
	UEVR_Matrix4x4f hmdTransform{};
	uevr::API::get()->param()->vr->get_transform(uevr::API::VR::get_hmd_index(), &hmdTransform);
	auto hmdHeight = hmdTransform.m[3][1];

	if (gameState->CurrentStance == ue::EGsrPlayerStance::Stand && hmdHeight <= settingsManager->Settings.squatUnderThisValue)
		playerManager->SetPlayerStance(ue::EGsrPlayerStance::Squat);

	if (gameState->CurrentStance == ue::EGsrPlayerStance::Squat && hmdHeight <= settingsManager->Settings.crawlUnderThisValue)
		playerManager->SetPlayerStance(ue::EGsrPlayerStance::Crawl);

	if (gameState->CurrentStance == ue::EGsrPlayerStance::Squat && hmdHeight >= settingsManager->Settings.squatUnderThisValue)
		playerManager->SetPlayerStance(ue::EGsrPlayerStance::Stand);

	if (gameState->CurrentStance == ue::EGsrPlayerStance::Crawl && hmdHeight >= settingsManager->Settings.crawlUnderThisValue)
		playerManager->SetPlayerStance(ue::EGsrPlayerStance::Squat);
}


