#include "CameraController.hpp"

uevr::API::UObject* CameraController::CameraAttachementActor = nullptr;

void CameraController::ToggleCameraWobblingController(bool enabled) {
	//static auto pawn = uevr::API::get()->get_local_pawn(0);
	//Logger::DebugPrint("Camera Recoil : " + enabled);
	auto static wobblingController_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Gsr.GsrWobblingController");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
	auto wobblingController = wobblingController_c->get_first_object_matching(false);
	//Logger::DebugPrint(wobblingController->get_full_name());
	bool setEnabled = enabled ? true : false;
	wobblingController->call_function(L"SetEnabled", &setEnabled);
}

bool CameraController::CutsceneStarts()
{
	return IsCutscenePlaying && !WasCutscenePlaying;
}

bool CameraController::CutsceneStops()
{
	return !IsCutscenePlaying && WasCutscenePlaying;
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

void CameraController::ProcessCamera(float deltaTime)
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

	glm::fvec3 tempPosition;
	glm::fvec3 newPosition;
	//Logger::DebugPrint("Current Player Stance : %i", playerManager.CurrentStance);
	//Logger::DebugPrint("Player Is Peeping: %i", playerManager.IsPeeping);

	// Cap deltaTime to prevent camera from yeeting into orbit when the game freezes or low fps.
	deltaTime = glm::min(deltaTime, 0.05f);

	// Reset camera position and rotation when player respawn or teleport
	if (InstantCameraPositionNeeded)
	{
		LastCameraPosition = { pawnPosition.x, pawnPosition.y, (float)headColliderPosition.z };
		//CameraHeightOffset = pawnPosition.z - headColliderPosition.z;
		OriginalCameraPosition = { pawnPosition.x, pawnPosition.y, (float)headColliderPosition.z };

		auto pawn = uevr::API::get()->get_local_pawn(0);
		UEVR_Rotatord pawnRotation;
		pawn->call_function(L"K2_GetActorRotation", &pawnRotation);
		ActualYaw = pawnRotation.yaw;
		LastCameraRotation.yaw = ActualYaw;
		CameraRotation.yaw = ActualYaw;
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
			CameraHeightOffset = 83.0f ;
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

	float remappedLerpXYMultiplier = Utilities::Remap(*settingsManager->CameraPositionXYLerpPtr, 0.0f, 10.0f, 4.0f, 0.1f);
	float remappedLerpZMultiplier = Utilities::Remap(*settingsManager->CameraPositionZLerpPtr, 0.0f, 10.0f, 4.0f, 0.1f);

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
			glm::mix(LastCameraPosition.z, tempPosition.z, alphaZ)):
			tempPosition;
	}

	//Logger::DebugPrint("%f", pawnPosition.z + CameraHeightOffset);
	//Logger::DebugPrint("%f, %f", remappedLerpZMultiplier, 2.25f * remappedLerpZMultiplier * DeltaTime);
	//Logger::DebugPrint("%f", DeltaTime);

	//Logger::DebugPrint("Camera Height Offset : %f", cameraController.CameraHeightOffset);
	LastCameraPosition = { newPosition.x, newPosition.y, newPosition.z };
	OriginalCameraPosition.x = newPosition.x;
	OriginalCameraPosition.y = newPosition.y;
	OriginalCameraPosition.z = newPosition.z;

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
	CameraRotation.yaw = *settingsManager->SmoothCameraRotationPtr ? 
		glm::mix(LastCameraRotation.yaw, ActualYaw, alpha) :
		ActualYaw;
	
	LastCameraRotation = CameraRotation;
	previousPawnPositionZ = pawnPosition.z;
	/*SetCameraRotation();*/
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

				if (slotName == "CanvasPanelSlot_10_10") {
					slot->Content->SetVisibility(SDK::ESlateVisibility::Hidden);
					Logger::DebugPrint("Found target slot at index: %i", i);
				}
				if (slotName == "CanvasPanelSlot_1_1")
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