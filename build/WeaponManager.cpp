#include "WeaponManager.hpp"

void WeaponManager::InitWeaponManager()
{
	tntSocketFname = SDK::UKismetStringLibrary::Conv_StringToName(L"TntTriggerSocket_LeftHand");
	cqcKnifeSocketFname = SDK::UKismetStringLibrary::Conv_StringToName(L"BaseBody_LeftHand_MTPA");
}

void WeaponManager::ProcessItem()
{
	struct ParameterItemID {
		SDK::EGsrItemId ReturnValue;
	}params;
	if (ItemController == NULL)
	{
		CurrentItemID = SDK::EGsrItemId::IT_None;
		Logger::DebugPrint("No equipController found");
		return;
	}

	auto newItemID = ItemController->GetItemId();
	if (CurrentItemID != newItemID)
	{
		CurrentItemID = newItemID;
		Logger::DebugPrint("Item changed. New Item ID : %d", CurrentItemID);

		CurrentItem = ItemController->m_currentItem;
		Logger::DebugPrint(CurrentItem->GetFullName().c_str());

		if (CurrentItemID == SDK::EGsrItemId::IT_NightVision)
		{
			auto item = (SDK::ABP_IT_NightVision_C*)CurrentItem;
			item->SKM_nvgo_dmo_hd->SetOnlyOwnerSee(true);

		}
		if (CurrentItemID == SDK::EGsrItemId::IT_Thermal)
		{
			auto item = (SDK::ABP_IT_Thermal_C*)CurrentItem;
			item->SKM_thgo_dmo_hd->SetOnlyOwnerSee(true);
			auto static cameraManager_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"BlueprintGeneratedClass /Game/Blueprints/Camera/BP_PlayerCameraManager.BP_PlayerCameraManager_C");
			playerCameraManager = (SDK::ABP_PlayerCameraManager_C*)cameraManager_c->get_first_object_matching(false);
			if (playerCameraManager == nullptr)
			{
				Logger::DebugPrint("No player camera manager found");
				return;
			}
			ToggleThermalVisionScanlineNextFrame = true;
		}

		if (CurrentItemID == SDK::EGsrItemId::IT_Stealth && PreviousItemID != SDK::EGsrItemId::IT_Stealth)
		{
			auto pawn = (SDK::ABP_Player_C*)uevr::API::get()->get_local_pawn(0);
			auto stealthMaterial = pawn->SkeletalMesh_Hand->OverlayMaterial;
			playerManager->RightHandMesh->SetMaterial(0, stealthMaterial);
			playerManager->RightHandMesh->SetMaterial(1, stealthMaterial);
			playerManager->LeftHandMesh->SetMaterial(0, stealthMaterial);
			playerManager->LeftHandMesh->SetMaterial(1, stealthMaterial);
		}
		if (CurrentItemID != SDK::EGsrItemId::IT_Stealth && PreviousItemID == SDK::EGsrItemId::IT_Stealth)
		{
			if (playerManager->MainHandAttachedToWeapon)
			{
				DetachHandFromWeapon();
				if (playerManager->SecondHandAttachedToWeapon)
				{
					DetachSecondHandFromWeapon();
				}
			}
			uevr::API::get()->dispatch_lua_event("HandsReset", "0");
		}

		if (CurrentItemID == SDK::EGsrItemId::IT_CBoxA || CurrentItemID == SDK::EGsrItemId::IT_CBoxB || CurrentItemID == SDK::EGsrItemId::IT_CBoxC || CurrentItemID == SDK::EGsrItemId::IT_CBoxD)
		{
			cameraController->CardBoardEquipped = true;
			uevr::API::VR::set_mod_value("VR_RoomscaleMovement", false);
		}
		else
		{
			cameraController->CardBoardEquipped = false;
			uevr::API::VR::set_mod_value("VR_RoomscaleMovement", true);
		}
	}
}

void WeaponManager::SendHandStateToLua()
{
	//If food and others, use the default position
	if (CurrentWeaponID == EGsrEquipId::WP_MAX_PLAYER_WEAPONS)
	{
		uevr::API::get()->dispatch_lua_event("CurrentWeapon", "wp_none");
	}
	else
	{
		//Sending actual weapon to lua hands to set the pose for this weapon. Lua scripts takes a lowercase only value so we need to set it up :
		auto weaponNameView = magic_enum::enum_name(CurrentWeaponID);
		std::string weaponName(weaponNameView); // make a real string copy
		std::transform(weaponName.begin(), weaponName.end(), weaponName.begin(), ::tolower);
		uevr::API::get()->dispatch_lua_event("CurrentWeapon", weaponName);
	}
}

inline const char* WeaponManager::WeaponIdToKey(EGsrEquipId id)
{
	switch (id)
	{
	case WP_Knife:				LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = false;	canBeTwoHanded = false;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return "WP_Knife";
	case WP_Fork:				LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = false;	canBeTwoHanded = false;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return "WP_Fork";
	case WP_CigarPistol:		LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = true;	canBeTwoHanded = false;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return "WP_CigarPistol";
	case WP_Handkerchief:		LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = true;	canBeTwoHanded = false;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return "WP_Handkerchief";
	case WP_Mk22:				LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = true;			mainHandShouldAttachToWeapon = true;	canBeTwoHanded = true;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = true;		return "WP_Mk22";
	case WP_Gove:				LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = true;			mainHandShouldAttachToWeapon = true;	canBeTwoHanded = true;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = true;		return "WP_Gove";
	case WP_EasyGun:			LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = true;			mainHandShouldAttachToWeapon = true;	canBeTwoHanded = true;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = true;		return "WP_EasyGun";
	case WP_SAArmy:				LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = true;	canBeTwoHanded = true;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return "WP_SAArmy";
	case WP_PatriotPostol:		LeftHandWeaponGrabOffset = -4.68f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = true;	canBeTwoHanded = true;		twoHandedPivot = true;		holdCQCKnifeInLeftHand = false;		return "WP_PatriotPostol";
	case WP_Scorpion:			LeftHandWeaponGrabOffset = -1.75f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = true;	canBeTwoHanded = true;		twoHandedPivot = true;		holdCQCKnifeInLeftHand = false;		return "WP_Scorpion";
	case WP_M16a1:				LeftHandWeaponGrabOffset = -4.90f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = true;	canBeTwoHanded = true;		twoHandedPivot = true;		holdCQCKnifeInLeftHand = false;		return "WP_M16a1";
	case WP_Akm:				LeftHandWeaponGrabOffset = -3.94f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = true;	canBeTwoHanded = true;		twoHandedPivot = true;		holdCQCKnifeInLeftHand = false;		return "WP_Akm";
	case WP_M63:				LeftHandWeaponGrabOffset = -3.59f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = true;	canBeTwoHanded = true;		twoHandedPivot = true;		holdCQCKnifeInLeftHand = false;		return "WP_M63";
	case WP_Ithaca:				LeftHandWeaponGrabOffset = 1.91f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = true;	canBeTwoHanded = true;		twoHandedPivot = true;		holdCQCKnifeInLeftHand = false;		return "WP_Ithaca";
	case WP_Dragnov:			LeftHandWeaponGrabOffset = -3.22f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = true;	canBeTwoHanded = true;		twoHandedPivot = true;		holdCQCKnifeInLeftHand = false;		return "WP_Dragnov";
	case WP_MosinNagant:		LeftHandWeaponGrabOffset = -3.32f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = true;	canBeTwoHanded = true;		twoHandedPivot = true;		holdCQCKnifeInLeftHand = false;		return "WP_MosinNagant";
	case WP_Rpg:				LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = true;	canBeTwoHanded = true;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return "WP_Rpg";
	case WP_Torch:				LeftHandWeaponGrabOffset = 0.0f; 		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = true;	canBeTwoHanded = false;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return "WP_Torch";
	case WP_Grenade:			LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = true;	canBeTwoHanded = false;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return "WP_Grenade";
	case WP_FireGrenade:		LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = true;	canBeTwoHanded = false;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return "WP_FireGrenade";
	case WP_StunGrenade:		LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = true;	canBeTwoHanded = false;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return "WP_StunGrenade";
	case WP_ChaffGrenade:		LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = true;	canBeTwoHanded = false;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return "WP_ChaffGrenade";
	case WP_SmokeGrenade:		LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = true;	canBeTwoHanded = false;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return "WP_SmokeGrenade";
	case WP_Magazine:			LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = false;	canBeTwoHanded = false;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return "WP_Magazine";
	case WP_Tnt:				LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = true;			mainHandShouldAttachToWeapon = false;	canBeTwoHanded = false;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return "WP_Tnt";
	case WP_C3:					LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = false;	canBeTwoHanded = false;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return "WP_C3";
	case WP_Claymore:			LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = false;	canBeTwoHanded = false;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return "WP_Claymore";
	case WP_Book:				LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = false;	canBeTwoHanded = false;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return "WP_Book";
	case WP_RatTrap:			LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = false;	canBeTwoHanded = false;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return "WP_RatTrap";
	case WP_Microphone:			LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = false;	canBeTwoHanded = false;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return "WP_Microphone";
	case WP_MAX_PLAYER_WEAPONS:	LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = false;	canBeTwoHanded = false;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return "WP_MAX_PLAYER_WEAPONS";
	default:					LeftHandWeaponGrabOffset = 0.0f;		hasObjectInLeftHand = false;		mainHandShouldAttachToWeapon = false;	canBeTwoHanded = false;		twoHandedPivot = false;		holdCQCKnifeInLeftHand = false;		return nullptr;
	}
}

void WeaponManager::FetchWeaponOffsets(EGsrEquipId currentWeapon, glm::fvec3& outPos, glm::fquat& outRot, SDK::FVector& outHandLocationOffset, SDK::FRotator& outHandRotationOffset)
{
	const char* key = WeaponIdToKey(currentWeapon);
	if (!key)
	{
		Logger::DebugPrint("No valid weapon key found for ID: %d", currentWeapon);
		return;
	}

    auto it = settingsManager->g_weaponConfigs.find(key);
	Logger::DebugPrint("Fetching weapon config for key: %s", key);
    if (it == settingsManager->g_weaponConfigs.end()) return;
	//Logger::DebugPrint("Weapon config found for key: %s", it->first);
	//Logger::DebugPrint("Weapon Location Offset: x=%f, y=%f, z=%f", it->second.location_offset.x, it->second.location_offset.y, it->second.location_offset.z);
	//Logger::DebugPrint("Hand Location Offset: x=%f, y=%f, z=%f", it->second.hand_location_offset.X, it->second.hand_location_offset.Y, it->second.hand_location_offset.Z);
	//Logger::DebugPrint("Hand Rotation Offset: Pitch=%f, Yaw=%f, Roll=%f", it->second.hand_rotation_offset.Pitch, it->second.hand_rotation_offset.Yaw, it->second.hand_rotation_offset.Roll);

	const WeaponConfig& cfg = it->second;
    outPos = cfg.location_offset;
    outRot = cfg.rotation_offset;
	outHandLocationOffset = cfg.hand_location_offset;
	outHandRotationOffset = cfg.hand_rotation_offset;
}

void WeaponManager::FetchEquipController()
{
	auto pawn = uevr::API::get()->get_local_pawn(0);
	struct ParameterEquipController {
		uevr::API::UObject* ReturnValue;
	}params;
	pawn->call_function(L"GetEquipControllerBP", &params);
	EquipController = params.ReturnValue;
	//Logger::DebugPrint("Fetch : ");
	//Logger::DebugPrint(EquipController->get_full_name());
	if (EquipController == NULL)
	{
		Logger::DebugPrint("No equipController found");
	}

	struct ParameterItemController {
		uevr::API::UObject* ReturnValue;
	}params1;
	pawn->call_function(L"GetItemControllerBP", &params1);
	ItemController = (SDK::UGsrItemController*)params1.ReturnValue;
	//Logger::DebugPrint("Fetch : ");
	//Logger::DebugPrint(EquipController->get_full_name());
	if (ItemController == NULL)
	{
		Logger::DebugPrint("No itemController found");
	}
}

void WeaponManager::ToggleCQCKnifeMeshOnSecondHandGrab(bool enable)
{
	if (!LeftHandWeaponRoot)
		return;
	SDK::USceneComponent* sdkLeftWeaponRoot = (SDK::USceneComponent*)LeftHandWeaponRoot;
	SDK::USkeletalMeshComponent* sdkLeftWeaponMesh = (SDK::USkeletalMeshComponent*)LeftHandWeaponRoot;
	if (enable)
	{
		if (CqcKnifeMeshActor == nullptr)
		{
			constexpr SDK::FVector cqcKnifeRelativeLocation = {1.248f, -2.072f, -4.754f};
			constexpr SDK::FRotator cqcKnifeRelativeRotation = {7.228f, 58.205f, -169.534f};
			
			SDK::FTransform leftWeaponTsf = sdkLeftWeaponRoot->K2_GetComponentToWorld();

			CqcKnifeMeshActor = Utilities::SpawnMesh(leftWeaponTsf, L"SkeletalMesh /Game/Gsr/Weapon/cqck/Mesh/SM_Cqck.SM_Cqck"); //L"SkeletalMesh /Game/Gsr/Weapon/cqck/Mesh/SM_Cqck.SM_Cqck"
			CqcKnifeMeshActor->K2_AttachRootComponentTo(playerManager->LeftHandMesh, SDK::UKismetStringLibrary::Conv_StringToName(L"None"), SDK::EAttachLocation::KeepWorldPosition, false);
			SDK::USkeletalMeshComponent* cqcKnifeMesh = (SDK::USkeletalMeshComponent*)CqcKnifeMeshActor;
			cqcKnifeMesh->K2_SetRelativeLocationAndRotation(cqcKnifeRelativeLocation, cqcKnifeRelativeRotation, false, nullptr, true);
		}
		CqcKnifeMeshActor->SetActorHiddenInGame(false);
		sdkLeftWeaponMesh->SetOnlyOwnerSee(true);
	}
	else
	{
		CqcKnifeMeshActor->SetActorHiddenInGame(true);
		sdkLeftWeaponMesh->SetOnlyOwnerSee(false);
	}
}

void WeaponManager::ProcessTwoHandsGrabWeapon()
{
	if (!canBeTwoHanded)
		return;

	if (Inputs::SecondHandGripTriggeredBeforeDelay && !secondHandShouldAttachToWeapon)
	{
		//Logger::DebugPrint("Attaching second hand to weapon");
		secondHandShouldAttachToWeapon = true;
		AttachSecondHandNextFrame = true;
		if (holdCQCKnifeInLeftHand)
			ToggleCQCKnifeMeshOnSecondHandGrab(true);
			
		//Utilities::DebugCigar0 = Utilities::SpawnDebugCigars(playerManager->RightController->K2_GetComponentLocation());
		//Utilities::DebugCigar1 = Utilities::SpawnDebugCigars(playerManager->LeftController->K2_GetComponentLocation());
	}
	else if (Inputs::SecondHandGripTriggeredBeforeDelay && secondHandShouldAttachToWeapon)
	{
		//Logger::DebugPrint("Dettaching second hand to weapon");
		secondHandShouldAttachToWeapon = false;
		DetachSecondHandFromWeapon();
		if (holdCQCKnifeInLeftHand)
			ToggleCQCKnifeMeshOnSecondHandGrab(false);
		//Utilities::DestroyDebugCigars();
	}


	//Logger::DebugPrint("Right Hand Pos : x=%f, y=%f, z=%f", rightHandPos.X, rightHandPos.Y, rightHandPos.Z);
	if (!*settingsManager->EnableTwoHandsPivotPtr)
		return;

	if (playerManager->SecondHandAttachedToWeapon && twoHandedPivot && WeaponRoot) {
		SDK::USceneComponent* sdkWeaponRoot = (SDK::USceneComponent*)WeaponRoot;

		auto rightCtrlPos = playerManager->RightController->K2_GetComponentLocation();
		auto leftCtrlPos = playerManager->LeftController->K2_GetComponentLocation();

		auto sdkCurrentWeapon = (SDK::AActor*)CurrentWeapon;
		auto weaponUpVector = sdkCurrentWeapon->GetActorUpVector();
		
		//Debug:
		//if (ZoomInput > 0.8f)
		//{
		//	LeftHandWeaponGrabOffset += 0.01f; // Example: Add an extra 5 cm when the A button is pressed
		//}
		//if (ZoomInput < -0.8f)
		//{
		//	LeftHandWeaponGrabOffset -= 0.01f; // Example: Subtract 5 cm when the B button is pressed
		//}
		SDK::FVector adjustedLeftPos = leftCtrlPos + (weaponUpVector * LeftHandWeaponGrabOffset);

		SDK::FVector directionVec = adjustedLeftPos - rightCtrlPos;
		directionVec.Normalize();

		SDK::FRotator targetRot = SDK::UKismetMathLibrary::MakeRotFromXZ(directionVec, weaponUpVector);

		SDK::FRotator fixUp(0.f, -90.f, 0.f);
		targetRot = SDK::UKismetMathLibrary::ComposeRotators(fixUp, targetRot);

		SDK::FVector localOffset(DefaultUevrHookWeaponPosition.z, DefaultUevrHookWeaponPosition.x, DefaultUevrHookWeaponPosition.y);
		SDK::FVector rotatedOffset = SDK::UKismetMathLibrary::GreaterGreater_VectorRotator(localOffset, targetRot);

		SDK::FVector finalRootPos = rightCtrlPos - rotatedOffset;

		SDK::FHitResult outHit;
		sdkWeaponRoot->K2_SetWorldLocationAndRotation(finalRootPos, targetRot, false, &outHit, false);

		// Debug:
		//Utilities::DebugCigar0->K2_SetActorLocationAndRotation(rightCtrlPos, playerManager->RightController->K2_GetComponentRotation(), false, &outHit, true);
		//Utilities::DebugCigar1->K2_SetActorLocationAndRotation(leftCtrlPos, playerManager->LeftController->K2_GetComponentRotation(), false, &outHit, true);
	}
}


void WeaponManager::RemoveUobjectHookFromHand(Hand handSide)
{
	SDK::FHitResult outHit;
	if (handSide == Left)
	{
		if (LeftHandWeaponRoot)
		{
			uevr::API::UObjectHook::remove_motion_controller_state(LeftHandWeaponRoot);
			SDK::USceneComponent* sdkLeftWeaponRoot = (SDK::USceneComponent*)LeftHandWeaponRoot;
			sdkLeftWeaponRoot->K2_SetRelativeLocationAndRotation({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -90.0f }, false, &outHit, true);
		}
	}
	else
	{
		if (WeaponRoot)
		{
			SDK::USceneComponent* sdkWeaponRoot = (SDK::USceneComponent*)WeaponRoot;
			sdkWeaponRoot->K2_SetRelativeLocationAndRotation({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, false, &outHit, true);
			uevr::API::UObjectHook::remove_motion_controller_state(WeaponRoot);
		}
	}
}

void WeaponManager::ProcessWeapon(float delta)
{
	if (AttachSecondHandNextFrame && !playerManager->MeleeAttackRunning)
	{
		AttachSecondHandToWeapon();
		AttachSecondHandNextFrame = false;
		if (holdCQCKnifeInLeftHand)
			ToggleCQCKnifeMeshOnSecondHandGrab(true);
	}

	if (AttachHandNextFrame && NextFrame && !playerManager->MeleeAttackRunning)
	{
		Logger::DebugPrint("Attaching main hand rn");
		AttachMainHandToWeapon();
		AttachSecondHandNextFrame = true;
		SendHandStateToLua();
		AttachHandNextFrame = false;
		NextFrame = false;
	}

	if (ToggleThermalVisionScanlineNextFrame)
	{
		ToggleThermalVisionScanline();
	}

	//Get weapon ID
	//Logger::DebugPrint(wobblingController->get_full_name());
	struct ParameterWeaponID {
		EGsrEquipId ReturnValue;
	}params;
	if (EquipController == NULL)
	{
		CurrentWeaponID = WP_None;
		Logger::DebugPrint("No equipController found");
		return;
	}

	EquipController->call_function(L"GetCurrentWeaponId", &params);

	if (params.ReturnValue >= 31)
	{
		params.ReturnValue = EGsrEquipId::WP_MAX_PLAYER_WEAPONS;
	}

	if (CurrentWeaponID != params.ReturnValue)
	{
		CurrentWeaponID = params.ReturnValue;
		Logger::DebugPrint("Weapon changed. New Weapon ID : %d", CurrentWeaponID);
	}
	


	struct ParameterCurrentWeapon {
		uevr::API::UObject* ReturnValue;
	}params1;
	EquipController->call_function(L"GetCurrentWeapon", &params1);

	if (CurrentWeapon != params1.ReturnValue)
	{
		if (PreviousWeaponID == EGsrEquipId::WP_Dragnov || PreviousWeaponID == EGsrEquipId::WP_MosinNagant || PreviousWeaponID == EGsrEquipId::WP_Rpg)
			ReceiverMesh->SetVisibility(false, false);
		ToggleCQCKnifeMeshOnSecondHandGrab(false);
		ReceiverMesh = nullptr;
		WeaponRoot = nullptr;
		LeftHandWeaponRoot = nullptr;
		if (playerManager->MainHandAttachedToWeapon)
			DetachHandFromWeapon();
		if (playerManager->SecondHandAttachedToWeapon)
		{
			DetachSecondHandFromWeapon();
			if (CurrentWeaponID == WP_None)
				secondHandShouldAttachToWeapon = false;
		}


		SendHandStateToLua();
	

		SwitchScopeState(false);

		CurrentWeapon = params1.ReturnValue;
		Logger::DebugPrint("Updating Current weapon ptr");
		Logger::DebugPrint(CurrentWeapon->get_full_name());	

		WeaponRoot = CurrentWeapon->get_property<uevr::API::UObject*>(L"RootComponent");
		if (WeaponRoot == nullptr)
		{
			Logger::DebugPrint("No weapon root found. Return");
			return;
		}
		Logger::DebugPrint(WeaponRoot->get_full_name());

		FetchWeaponOffsets(CurrentWeaponID, DefaultUevrHookWeaponPosition, DefaultUevrHookWeaponRotation, HandLocationOffset, HandRotationOffset);

		//if (!weaponShouldAttach)
		//{
		//	auto sdkSceneComponent = (SDK::USceneComponent*)WeaponRoot;
		//	AttachParent = sdkSceneComponent->GetAttachParent();
		//}


		Logger::DebugPrint("Applied weapon offsets from config");
		Logger::DebugPrint("Position Offset : x=%f, y=%f, z=%f", DefaultUevrHookWeaponPosition.x, DefaultUevrHookWeaponPosition.y, DefaultUevrHookWeaponPosition.z);
		//Logger::DebugPrint("Rotation Offset : w=%f, x=%f, y=%f, z=%f", DefaultUevrHookWeaponRotation.w, DefaultUevrHookWeaponRotation.x, DefaultUevrHookWeaponRotation.y, DefaultUevrHookWeaponRotation.z);
		//Logger::DebugPrint("Hand Position Offset : x=%f, y=%f, z=%f", HandLocationOffset.X, HandLocationOffset.Y, HandLocationOffset.Z);
		//Logger::DebugPrint("Hand Rotation Offset : Pitch=%f, Yaw=%f, Roll=%f", HandRotationOffset.Pitch, HandRotationOffset.Yaw, HandRotationOffset.Roll);

		CurrentRelativeWeaponPosition = DefaultRelativeWeaponPosition;
		CurrentRelativeWeaponRotation = DefaultRelativeWeaponRotation;
		CurrentRelativeWeaponRotationEuler = glm::eulerAngles(CurrentRelativeWeaponRotation);

		UEVR_Vector3f defaultPos = { DefaultUevrHookWeaponPosition.x, DefaultUevrHookWeaponPosition.y, DefaultUevrHookWeaponPosition.z };
		UEVR_Quaternionf defaultRot = {DefaultUevrHookWeaponRotation.w, DefaultUevrHookWeaponRotation.x, DefaultUevrHookWeaponRotation.y, DefaultUevrHookWeaponRotation.z};

		auto motionState = uevr::API::UObjectHook::get_or_add_motion_controller_state(WeaponRoot);
		motionState->set_location_offset(&defaultPos);
		motionState->set_rotation_offset(&defaultRot);
		motionState->set_hand(1);
		motionState->set_permanent(true);



		SDK::USceneComponent* sdkWeaponRoot = (SDK::USceneComponent*)WeaponRoot;
		auto childrenArray = sdkWeaponRoot->AttachChildren;
		for (int i = 0; i < childrenArray.Num(); i++)
		{
			SDK::USceneComponent* child = childrenArray[i];
			if (!child) continue;
			if (child->IsA(SDK::USkeletalMeshComponent::StaticClass())) {
				ReceiverMesh = (SDK::USkeletalMeshComponent*)child;
				//Logger::DebugPrint(child->GetFullName().c_str());
			}
		}

		if (*settingsManager->EnableWeaponsScopesPtr)
		{
			SwitchScopeState(false);

			if (CurrentWeaponID == EGsrEquipId::WP_Dragnov || CurrentWeaponID == EGsrEquipId::WP_MosinNagant || CurrentWeaponID == EGsrEquipId::WP_Rpg)
			{
				if (!ReceiverMesh)
				{
					Logger::DebugPrint("no Receiver mesh found");
					return;
				}
				for (int i = 0; i < childrenArray.Num(); i++)
				{
					SDK::USceneComponent* child = childrenArray[i];
					if (!child) continue;
					//Destroy any previous cylinder mesh if still attached
					if (child->IsA(SDK::UStaticMeshComponent::StaticClass())) {
						child->K2_DestroyComponent(child);
						ScopeStaticMeshComponent = nullptr;
					}
				}

				spawnScope();
				attachScopeToWeapon();
			}
		}

		if (CurrentWeaponID == EGsrEquipId::WP_CigarPistol)
		{
			auto staticMesh = (SDK::UStaticMeshComponent*)WeaponRoot;
			staticMesh->SetRelativeScale3D({ 1.5f, 1.5f, 1.5f });
			return;
		}

		
		AttachHandNextFrame = true;	
	}

	if (CurrentWeaponID == EGsrEquipId::WP_MAX_PLAYER_WEAPONS)
	{
		//Food floating animation
		auto sdkWeaponRoot = (SDK::USceneComponent*)WeaponRoot;

		auto children = sdkWeaponRoot->AttachChildren;
		SDK::UStaticMeshComponent* foodMesh = nullptr;
		for (int i = 0; i < children.Num(); i++)
		{
			SDK::USceneComponent* child = children[i];
			if (!child) continue;
			if (child->IsA(SDK::UStaticMeshComponent::StaticClass())) {
				SDK::UStaticMeshComponent* staticMesh = (SDK::UStaticMeshComponent*)child;
				std::string meshName = staticMesh->GetName();
				if (meshName.find("ThrowableStaticMesh") != std::string::npos)
				{
					foodMesh = staticMesh;
					break;
				}
			}
		}

		if (!foodMesh)
		{
			Logger::DebugPrint("No food mesh found");
			return;
		}

		SDK::FHitResult hitResult;

		if (!Inputs::LeftGripInputPressed && Inputs::WasLeftGripInputPressed)
		{
			// Re-sync to current world rotation
			auto loc = foodMesh->K2_GetComponentLocation();
			FoodIdleStart = glm::vec3(loc.X, loc.Y, loc.Z);
			FoodIdlePosition = FoodIdleStart;

			auto rotQuat = foodMesh->K2_GetComponentToWorld().Rotation;
			auto rot = foodMesh->K2_GetComponentRotation();
			FoodCurrentLocalRotation = {(float)rotQuat.W, (float)rotQuat.X, (float)rotQuat.Y, (float)rotQuat.Z};
			ActualFoodRotation = rot.Yaw;
		}

		if (Inputs::LeftGripInputPressed && !Inputs::WasLeftGripInputPressed)
		{
			auto foodT = foodMesh->K2_GetComponentToWorld();
			auto handT = playerManager->LeftHandMesh->K2_GetComponentToWorld();

			glm::quat foodRot(foodT.Rotation.W, foodT.Rotation.X, foodT.Rotation.Y, foodT.Rotation.Z);
			glm::quat handRot(handT.Rotation.W, handT.Rotation.X, handT.Rotation.Y, handT.Rotation.Z);

			glm::vec3 foodPos(foodT.Translation.X, foodT.Translation.Y, foodT.Translation.Z);
			glm::vec3 handPos(handT.Translation.X, handT.Translation.Y, handT.Translation.Z);

			glm::quat handRotInv = glm::inverse(handRot);

			FoodGrabRelativeRotation = handRotInv * foodRot;
			FoodGrabRelativePosition = handRotInv * (foodPos - handPos);
		}

		if (!Inputs::LeftGripInputPressed)
		{

			ActualFoodRotation -= delta * 20.0f;
			if (ActualFoodRotation <= -360.0f)
				ActualFoodRotation += 360.0f;

			ActualFoodRotation += cameraController->YawProcessed;

			float radians = glm::radians(ActualFoodRotation);

			// Apply the float
			float positionOffset = sinf(radians * 2.0f) * 0.01f; // frequency * amplitude
			auto rotationOffset = sinf(radians * 4.0f) * 2.0f;

			// Build target rotation properly
			glm::vec3 targetEulerDeg(-rotationOffset, -rotationOffset, ActualFoodRotation);
			glm::vec3 targetRad = glm::radians(targetEulerDeg);

			glm::quat qTarget = glm::quat(glm::mat3(
				glm::rotate(glm::mat4(1.0f), targetRad.y, glm::vec3(0, 1, 0)) * // Yaw
				glm::rotate(glm::mat4(1.0f), targetRad.x, glm::vec3(1, 0, 0)) * // Pitch
				glm::rotate(glm::mat4(1.0f), targetRad.z, glm::vec3(0, 0, 1))   // Roll
			));


			// slerp from our *previous* smoothed value
			FoodCurrentLocalRotation = glm::slerp(FoodCurrentLocalRotation, qTarget, delta * 2.0f);

			// Target idle pos
			auto weaponRootPos = sdkWeaponRoot->K2_GetComponentLocation();
			glm::vec3 targetPos(
				weaponRootPos.X,
				weaponRootPos.Y,
				weaponRootPos.Z + positionOffset
			);


			auto test = foodMesh->K2_GetComponentLocation();
			FoodIdlePosition = glm::mix({test.X, test.Y, test.Z}, targetPos, delta * 8.0f);

			//Logger::DebugPrint("FoodCurrentWorldPosition : x=%f, y=%f, z=%f", FoodCurrentWorldPosition.X, FoodCurrentWorldPosition.Y, FoodCurrentWorldPosition.Z);
			//Logger::DebugPrint("Weapon root pos : x=%f, y=%f, z=%f", weaponRootPos.X, weaponRootPos.Y, weaponRootPos.Z);
			// apply
			SDK::FTransform finalTsf = {};
			finalTsf.Rotation = {FoodCurrentLocalRotation.x, FoodCurrentLocalRotation.y, FoodCurrentLocalRotation.z, FoodCurrentLocalRotation.w};
			finalTsf.Translation = {FoodIdlePosition.x, FoodIdlePosition.y, FoodIdlePosition.z};
			finalTsf.Scale3D = foodMesh->K2_GetComponentScale();

			

			foodMesh->K2_SetWorldTransform(finalTsf, false, &hitResult, false);
		}
		else
		{
			// Follow left hand position 
			auto handT = playerManager->LeftHandMesh->K2_GetComponentToWorld();

			glm::quat handRot(handT.Rotation.W, handT.Rotation.X, handT.Rotation.Y, handT.Rotation.Z);
			glm::vec3 handPos(handT.Translation.X, handT.Translation.Y, handT.Translation.Z);

			// reconstruction du world transform du food en utilisant l'offset
			glm::quat newFoodRot = handRot * FoodGrabRelativeRotation;
			glm::vec3 newFoodPos = handPos + (handRot * FoodGrabRelativePosition);

			SDK::FTransform out;
			out.Rotation = { newFoodRot.x, newFoodRot.y, newFoodRot.z, newFoodRot.w };
			out.Translation = { newFoodPos.x, newFoodPos.y, newFoodPos.z };
			out.Scale3D = foodMesh->K2_GetComponentScale();

			foodMesh->K2_SetWorldTransform(out, false, &hitResult, false);
		}

		return;
	}
		

	if (!LeftHandWeaponRoot && hasObjectInLeftHand)
	{
		auto leftHandSocketFname = CurrentWeaponID == EGsrEquipId::WP_Tnt ? tntSocketFname : cqcKnifeSocketFname;

		SDK::USceneComponent* sdkWeaponRoot = (SDK::USceneComponent*)WeaponRoot;
		auto parent = sdkWeaponRoot->GetAttachParent();
		auto childrenArray = parent->AttachChildren;

		for (int i = 0; i < childrenArray.Num(); i++)
		{
			SDK::USceneComponent* child = childrenArray[i];
			if (!child) continue;
			if (child->IsA(SDK::USkeletalMeshComponent::StaticClass())) {
				if (child->GetAttachSocketName() == leftHandSocketFname)
				{
					LeftHandWeaponRoot = (uevr::API::UObject*)child;
					//Logger::DebugPrint(leftHandSocketFname.ToString().c_str());
				}
			}
		}

		//Logger::DebugPrint(LeftHandWeaponRoot->get_full_name());

		//SocketName = "BaseBody_LeftHand_MTPA"
		//	Get all children and check their socket name. If match, then it's the knife'
	}

	//uevr seems to force uobject attachement each frame when a uobject file is present in uobjecthook folder
	//we need to reset the position to 0 each frame because of uevr reattach tentative
	if (playerManager->MeleeAttackRunning)
	{
		RemoveUobjectHookFromHand(Left);
		RemoveUobjectHookFromHand(Right);
	}

	// detach hands when melee attack
	if (!playerManager->WasMeleeAttackRunning && playerManager->MeleeAttackRunning)
	{
		if (playerManager->SecondHandAttachedToWeapon)
		{
			DetachSecondHandFromWeapon();
		}
	}
	if (playerManager->WasMeleeAttackRunning && !playerManager->MeleeAttackRunning)
	{
		AttachSecondHandToWeapon();
	}

	if (holdCQCKnifeInLeftHand)
	{
		if (!playerManager->WasMeleeAttackRunning && playerManager->MeleeAttackRunning)
			ToggleCQCKnifeMeshOnSecondHandGrab(false);
		if (playerManager->WasMeleeAttackRunning && !playerManager->MeleeAttackRunning)
			ToggleCQCKnifeMeshOnSecondHandGrab(true);
	}

	//Toggle weapon visibility in FPS view;
	if (ReceiverMesh)
	{
		bool isReceiverMeshVisible = ReceiverMesh->IsVisible();
		static auto const gsrGunClass = (uevr::API::UClass*)SDK::AGsrGun::StaticClass();
		if (CurrentWeapon->is_a(gsrGunClass) && !isReceiverMeshVisible)
		{
			ReceiverMesh->SetVisibility(true, false);
			auto sdkGun = (SDK::AGsrGun*)CurrentWeapon;
			//Logger::DebugPrint("IsAttachedSuppressor : %i", sdkGun->IsAttachedSuppressor());
			bool isAttachedSuppressor = sdkGun->IsAttachedSuppressor();
			if (sdkGun->GunPartsController->m_suppressorMesh)
			{
				sdkGun->GunPartsController->m_suppressorMesh->SetVisibility(isAttachedSuppressor, true);
			}

			// Set cqc knife visibility. DOESN'T WORK. Something is overwritting it each frame.
			//Logger::DebugPrint("CanCqc : %i", sdkGun->CanCqc());
			//if (sdkGun->CanCqc())
			//{
			//	auto sdkEquipController = (SDK::UGsrEquipController*)EquipController;
			//	Logger::DebugPrint("Knife Is Visible : %i", sdkEquipController->m_knifeActor->SkeletalMeshComponent->IsVisible());
			//	sdkEquipController->m_knifeActor->SkeletalMeshComponent->SetVisibility(true, true);
			//}
		}

		//Apply recoil
		if (CurrentWeapon != NULL)
			ApplyRecoil(delta);

		if (!*settingsManager->EnableWeaponsScopesPtr)
			return;

		if (CurrentWeaponID == EGsrEquipId::WP_Dragnov || CurrentWeaponID == EGsrEquipId::WP_MosinNagant || CurrentWeaponID == EGsrEquipId::WP_Rpg)
		{
			if (!ReceiverMesh->IsVisible())
				ReceiverMesh->SetVisibility(true, true);

			SwitchScopeState(playerManager->IsAiming);


			auto pawn = (SDK::ABP_Player_C*)uevr::API::get()->get_local_pawn(0);
			auto scopeMagnification = pawn->GetScopeMagnification();
			//Logger::DebugPrint("scope magnification : %f", scopeMagnification);
			float scopeFov = scopeMagnification == 3.0f ? 10.0f : 3.0f;
			scopeFov = CurrentWeaponID == EGsrEquipId::WP_Rpg ? 16.0f : scopeFov;
			SceneCaptureComponent->FOVAngle = scopeFov;
			//Toggle zoom :
			if (ZoomInput > 0.0f && scopeMagnification == 3.0f)
			{
				if (CurrentWeaponID == EGsrEquipId::WP_Dragnov)
				{
					auto widget = (SDK::USightDragnovWidget_C*)SourceDragnovScopeWidget;
					widget->SightZoomIn();
					widget->SetCamZoomToggle();
				}
				if (CurrentWeaponID == EGsrEquipId::WP_MosinNagant)
				{
					auto widget = (SDK::USightMosinWidget_C*)SourceMosinScopeWidget;
					widget->SightZoomIn();
					widget->SetCamZoomToggle();
				}
			}
			if (ZoomInput < 0.0f && scopeMagnification == 10.0f)
			{
				if (CurrentWeaponID == EGsrEquipId::WP_Dragnov)
				{
					auto widget = (SDK::USightDragnovWidget_C*)SourceDragnovScopeWidget;
					widget->SightZoomOut();
					widget->SetCamZoomToggle();
				}
				if (CurrentWeaponID == EGsrEquipId::WP_MosinNagant)
				{
					auto widget = (SDK::USightMosinWidget_C*)SourceMosinScopeWidget;
					widget->SightZoomOut();
					widget->SetCamZoomToggle();
				}
			}
		}
	}

	//left hand object gets deleted when in fps view, need to trigger the refetch every frames
	if (playerManager->IsFirstPersonView)
		LeftHandWeaponRoot = nullptr;
}

void WeaponManager::AttachMainHandToWeapon()
{
	//return;
	if (!playerManager->RightHandMesh) return;

	if (!playerManager->MainHandAttachedToWeapon && mainHandShouldAttachToWeapon)
	{

		Logger::DebugPrint("attach Main Hand to weapon");
		playerManager->RightHandMesh->DetachFromParent(true, false);
		playerManager->RightHandMesh->K2_AttachToComponent(
			(SDK::USceneComponent*)ReceiverMesh == nullptr ? (SDK::USceneComponent*)WeaponRoot : (SDK::USceneComponent*)ReceiverMesh,
			SDK::UKismetStringLibrary::Conv_StringToName(L"None"),
			SDK::EAttachmentRule::KeepWorld,
			SDK::EAttachmentRule::KeepWorld,
			SDK::EAttachmentRule::KeepRelative,
			false
		);
		playerManager->MainHandAttachedToWeapon = true;
		//SDK::FHitResult outHit;
		//playerManager->RightHandMesh->K2_AddRelativeLocation(HandLocationOffest, false, &outHit, false);
		//playerManager->RightHandMesh->K2_AddRelativeRotation(HandRotationOffset, false, &outHit, false);
		playerManager->RightHandMesh->SetVisibility(true, true);
		playerManager->LeftHandMesh->SetVisibility(true, true);
	}
}

bool WeaponManager::DetachHandFromWeapon()
{
	Logger::DebugPrint("Detach Main Hand from weapon");
	if (playerManager->RightHandMesh && playerManager->MainHandAttachedToWeapon)
	{
		playerManager->RightHandMesh->DetachFromParent(true, false);
		playerManager->RightHandMesh->K2_AttachToComponent(
			playerManager->RightController,
			SDK::UKismetStringLibrary::Conv_StringToName(L"None"),
			SDK::EAttachmentRule::KeepWorld,
			SDK::EAttachmentRule::KeepWorld,
			SDK::EAttachmentRule::KeepRelative,
			true
		);
		SDK::FHitResult outHit;
		playerManager->RightHandMesh->K2_SetRelativeLocation({ 0.0f,0.0f,0.0f }, false, &outHit, false);
		playerManager->RightHandMesh->K2_SetRelativeRotation({ 0.0f,-90.0f,0.0f }, false, &outHit, false);
		playerManager->MainHandAttachedToWeapon = false;
		playerManager->RightHandMesh->SetVisibility(true, true);
		playerManager->LeftHandMesh->SetVisibility(true, true);
		return true;
	}
	return false;
}

void WeaponManager::AttachSecondHandToWeapon()
{
	if (!playerManager->LeftHandMesh) return;

	if (!playerManager->SecondHandAttachedToWeapon && secondHandShouldAttachToWeapon)
	{
		Logger::DebugPrint("attach Second Hand to weapon");
		playerManager->LeftHandMesh->DetachFromParent(true, false);
		playerManager->LeftHandMesh->K2_AttachToComponent(
			(SDK::USceneComponent*)ReceiverMesh,
			SDK::UKismetStringLibrary::Conv_StringToName(L"None"),
			SDK::EAttachmentRule::KeepRelative,
			SDK::EAttachmentRule::KeepRelative,
			SDK::EAttachmentRule::KeepRelative,
			false
		);
		SDK::FHitResult outHit;
		playerManager->LeftHandMesh->K2_SetRelativeLocation(HandLocationOffset, false, &outHit, false);
		playerManager->LeftHandMesh->K2_SetRelativeRotation(HandRotationOffset, false, &outHit, false);
		playerManager->RightHandMesh->SetVisibility(true, true);
		playerManager->LeftHandMesh->SetVisibility(true, true);
		uevr::API::get()->dispatch_lua_event("TwoHandsHandling", "1");
		playerManager->SecondHandAttachedToWeapon = true;
	}
}

bool WeaponManager::DetachSecondHandFromWeapon()
{
	Logger::DebugPrint("Detach Second Hand from weapon");
	if (playerManager->LeftHandMesh && playerManager->SecondHandAttachedToWeapon)
	{
		playerManager->LeftHandMesh->DetachFromParent(true, false);
		playerManager->LeftHandMesh->K2_AttachToComponent(
			playerManager->LeftController,
			SDK::UKismetStringLibrary::Conv_StringToName(L"None"),
			SDK::EAttachmentRule::KeepWorld,
			SDK::EAttachmentRule::KeepWorld,
			SDK::EAttachmentRule::KeepRelative,
			true
		);
		SDK::FHitResult outHit;
		playerManager->LeftHandMesh->K2_SetRelativeLocation({ 0.0f,0.0f,0.0f }, false, &outHit, false);
		playerManager->LeftHandMesh->K2_SetRelativeRotation({ 0.0f,-90.0f,0.0f }, false, &outHit, false);
		playerManager->SecondHandAttachedToWeapon = false;
		playerManager->RightHandMesh->SetVisibility(true, true);
		playerManager->LeftHandMesh->SetVisibility(true, true);
		uevr::API::get()->dispatch_lua_event("TwoHandsHandling", "0");
		return true;
	}
	return false;
}



void WeaponManager::FetchWeaponMeshAimData()
{
	//Logger::DebugPrint(CurrentWeapon->get_full_name());
	if (CurrentWeapon == nullptr)
	{
		Logger::DebugPrint("No CurrentWeapon found");
		return;
	}
	auto root_component = CurrentWeapon->get_property<uevr::API::UObject*>(L"RootComponent");
	//Logger::DebugPrint(root_component->get_full_name());
	if (root_component != nullptr)
	{
		SDK::FVector forward_params;
		CurrentWeapon->call_function(L"GetActorForwardVector", &forward_params);
		//Logger::DebugPrint("ForwardVector.x : %f, ForwardVector.y : %f, ForwardVector.z : %f", forward_params.x, forward_params.y, forward_params.z);
		
		SDK::Params::GsrGun_GetMuzzleTransform muzzle_transform;
		muzzle_transform.TransformSpace = SDK::ERelativeTransformSpace::RTS_World;
		CurrentWeapon->call_function(L"GetMuzzleTransform", &muzzle_transform);
		//Logger::DebugPrint("FirePosition.x : %f, FirePosition.y : %f, FirePosition.z : %f", muzzle_transform.ReturnValue.Translation.X, muzzle_transform.ReturnValue.Translation.Y, muzzle_transform.ReturnValue.Translation.Z);
		auto actor = (SDK::AActor*)CurrentWeapon;
		auto rotation = actor->K2_GetActorRotation();
		//Logger::DebugPrint("rotation.Pitch : %f, rotation.Roll : %f, rotation.Yaw : %f", rotation.Pitch, rotation.Roll, rotation.Yaw);

		FirePosition = { muzzle_transform.ReturnValue.Translation.X, muzzle_transform.ReturnValue.Translation.Y, muzzle_transform.ReturnValue.Translation.Z };
		float scaler = 8192.0;
		SDK::Params::KismetMathLibrary_RotateAngleAxis rotate_params;
		rotate_params.InVect = forward_params;
		rotate_params.AngleDeg = 90.0f;
		CurrentWeapon->call_function(L"GetActorUpVector", &rotate_params.Axis);
		Utilities::KismetMathLibrary->call_function(L"RotateAngleAxis", &rotate_params);
		forward_params = { rotate_params.ReturnValue.X, rotate_params.ReturnValue.Y, rotate_params.ReturnValue.Z };

		// Now, convert the final forward vector into a rotator.
        SDK::FRotator world_rotator;
        
        // This is a struct to pass the vector as a parameter to the function.
        SDK::Params::KismetMathLibrary_MakeRotFromX makeRotFromX_params;
        makeRotFromX_params.X = forward_params;

        // Call the Kismet Math Library function to perform the conversion.
        Utilities::KismetMathLibrary->call_function(L"MakeRotFromX", &makeRotFromX_params);
		
		CurrentWeaponWorldRotation = makeRotFromX_params.ReturnValue;
		//Logger::DebugPrint("CurrentWeaponWorldRotation.x : %f, CurrentWeaponWorldRotation.y : %f, CurrentWeaponWorldRotation.z : %f", 
		//	CurrentWeaponWorldRotation.Pitch, CurrentWeaponWorldRotation.Yaw, CurrentWeaponWorldRotation.Roll);

		float aimOffsetValueHorizontal = 0.0f;
		float aimOffsetValueVertical = 0.0f;
		switch (CurrentWeaponID)
		{
		//case WP_Mk22:
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		//case WP_Gove:
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		//case WP_EasyGun:
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		//case WP_SAArmy: //This one x is the z there for some reasons
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		//case WP_PatriotPostol:
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		//case WP_Scorpion:
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		//case WP_M16a1: // {forward,  up down, left right}
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		//case WP_Akm:
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		//case WP_M63:
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		case WP_Ithaca:
			aimOffsetValueHorizontal = -150.0f;
			aimOffsetValueVertical = 0.0f;
			break;
		//case WP_Dragnov:
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		//case WP_MosinNagant:
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		//case WP_Rpg:
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;

		default:
			aimOffsetValueHorizontal = 0.0f;
			aimOffsetValueVertical = 0.0f;
			break;
		}

		AimPosition = { 
			FirePosition.x + forward_params.X * scaler, 
			FirePosition.y + forward_params.Y * scaler - aimOffsetValueHorizontal,
			FirePosition.z + forward_params.Z * scaler - aimOffsetValueVertical
		};
		//Logger::DebugPrint("AimPosition.x : %f, AimPosition.y : %f, AimPosition.z : %f", AimPosition.x, AimPosition.y, AimPosition.z);
	}
}

void WeaponManager::ApplyAimDataToMemory()
{
	//Logger::DebugPrint("AimPosition.x : %f, AimPosition.y : %f, AimPosition.z : %f", AimPosition.x, AimPosition.y, AimPosition.z);
	*(reinterpret_cast<double*>(memoryManager->AimPositionAddresses.x)) = AimPosition.x;
	*(reinterpret_cast<double*>(memoryManager->AimPositionAddresses.y)) = AimPosition.y;
	*(reinterpret_cast<double*>(memoryManager->AimPositionAddresses.z)) = AimPosition.z;
}

void WeaponManager::SetAimDirection()
{
	//Will be used to control the grenades throw direction
	//This controls only the aim when the camera controller is disabled. When cc enabled, it controls the camera rotation but the inputs are conflicting.
	auto cameraController_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Gsr.GsrPlayerCameraController");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
	auto cameraController = cameraController_c->get_first_object_matching(false);

	//Logger::DebugPrint(root_component->get_full_name());
	if (cameraController == nullptr || !CameraController::CameraAttachementActor)
	{
		Logger::DebugPrint("SetAimDirection : No CameraController or no CameraAttachementActor found");
		return;
	}

	auto sdkActor = (SDK::AActor*)CameraController::CameraAttachementActor;
	SDK::FRotator params;
	params = sdkActor->K2_GetActorRotation();
	//Logger::DebugPrint("pitch : %f, yaw : %f, roll : %f", params.Pitch, params.Yaw, params.Roll);
	cameraController->call_function(L"SetCameraRotation", &params);
	

	//auto root_component = CurrentWeapon->get_property<uevr::API::UObject*>(L"m_cameraActor");
	////Logger::DebugPrint(root_component->get_full_name());
	//if (root_component != nullptr)
	//{
	//	auto cameraActor = (SDK::AActor*)root_component;
	//	SDK::FHitResult hitResult;
	//	cameraActor->K2_SetActorRelativeRotation(CurrentWeaponWorldRotation, false, &hitResult, true);
	//}
}

void WeaponManager::SwitchScopeState(bool enable)
{
	if (SceneCaptureComponent) SceneCaptureComponent->SetVisibility(enable, false);
	if (SceneCaptureUStaticMeshComponent) SceneCaptureUStaticMeshComponent->SetVisibility(enable, false);
	if (ScopeStaticMeshComponent) ScopeStaticMeshComponent->SetVisibility(enable, false);

	switch (CurrentWeaponID)
	{
		case EGsrEquipId::WP_Dragnov:
			if (SourceDragnovScopeWidget) SourceDragnovScopeWidget->SetVisibility(enable ? SDK::ESlateVisibility::Visible : SDK::ESlateVisibility::Hidden);
			if (SourceMosinScopeWidget) SourceMosinScopeWidget->SetVisibility(SDK::ESlateVisibility::Hidden);
			if (SourceRpgScopeWidget) SourceRpgScopeWidget->SetVisibility(SDK::ESlateVisibility::Hidden);
		break;
		case EGsrEquipId::WP_MosinNagant:
			if (SourceMosinScopeWidget) SourceMosinScopeWidget->SetVisibility(enable ? SDK::ESlateVisibility::Visible : SDK::ESlateVisibility::Hidden);
			if (SourceDragnovScopeWidget) SourceDragnovScopeWidget->SetVisibility(SDK::ESlateVisibility::Hidden);
			if (SourceRpgScopeWidget) SourceRpgScopeWidget->SetVisibility(SDK::ESlateVisibility::Hidden);
			break;
		case EGsrEquipId::WP_Rpg :
			if (SourceRpgScopeWidget) SourceRpgScopeWidget->SetVisibility(enable ? SDK::ESlateVisibility::Visible : SDK::ESlateVisibility::Hidden);
			if (SourceDragnovScopeWidget) SourceDragnovScopeWidget->SetVisibility(SDK::ESlateVisibility::Hidden);
			if (SourceMosinScopeWidget) SourceMosinScopeWidget->SetVisibility(SDK::ESlateVisibility::Hidden);
			break;
	}
}

void WeaponManager::SpawnScope()
{
	spawnScope();
	attachScopeToWeapon();
}

void WeaponManager::DestroyScope()
{
	if (ScopeStaticMeshComponent) ScopeStaticMeshComponent->K2_DestroyComponent(ScopeStaticMeshComponent);
	if (SceneCaptureComponent) SceneCaptureComponent->K2_DestroyComponent(SceneCaptureComponent);
	if (SceneCaptureUStaticMeshComponent) SceneCaptureUStaticMeshComponent->K2_DestroyComponent(SceneCaptureUStaticMeshComponent);
	if (DragnovScopeWidget) DragnovScopeWidget->K2_DestroyComponent(DragnovScopeWidget);
	if (MosinScopeWidget) MosinScopeWidget->K2_DestroyComponent(DragnovScopeWidget);
	if (ScopeActor) ScopeActor->K2_DestroyActor();
	ScopeStaticMeshComponent = nullptr;
	SceneCaptureComponent = nullptr;
	SceneCaptureUStaticMeshComponent = nullptr;
	ScopeActor = nullptr;
	ScopeRenderTarget = nullptr;
	SourceDragnovScopeWidget = nullptr;
	DragnovScopeWidget = nullptr;
	SourceMosinScopeWidget = nullptr;
	MosinScopeWidget = nullptr;

	if (CqcKnifeMeshActor) CqcKnifeMeshActor->K2_DestroyActor();
	CqcKnifeMeshActor = nullptr;
}

void WeaponManager::Cleanup()
{
	DestroyScope();
	if (CqcKnifeMeshActor) CqcKnifeMeshActor->K2_DestroyActor();
	CqcKnifeMeshActor = nullptr;
}

void WeaponManager::ToggleThermalVisionScanline()
{
	if (playerCameraManager == nullptr)
	{
		Logger::DebugPrint("No player camera manager found");
		return;
	}
	if (playerCameraManager->ThermalPostProcessActor == nullptr)
	{
		return;
	}
	Logger::DebugPrint("Toggle Thermal Vision Scanline");
	playerCameraManager->ThermalPostProcessActor->Scanline->SetVisibility(!*settingsManager->DisableThermalVisionScanlinePtr, true);
	ToggleThermalVisionScanlineNextFrame = false;
}

void WeaponManager::ApplyRecoil(float delta)
{
	glm::fvec3 positionRecoilForce = { 0.0f, 0.0f, 0.0f };
	glm::fvec3 rotationRecoilForceEuler = { 0.0f, 0.0f, 0.0f };
	float recoilPositionRecoverySpeed = 10.0f;
	float recoilRotationRecoverySpeed = 8.0f;

	switch (CurrentWeaponID)
	{
	case WP_Mk22:
		positionRecoilForce = { 0.0f, -0.75f, 0.005f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -2.0f };
		break;

	case WP_Gove:
		positionRecoilForce = { randomRange(-0.25f, 0.25f), -4.0f, 0.005f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -2.0f };
		recoilPositionRecoverySpeed = 14.0f;
		recoilRotationRecoverySpeed = 12.0f;
		break;

	case WP_EasyGun:
		positionRecoilForce = { 0.0f, -0.75f, 0.005f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -2.0f };
		break;

	case WP_SAArmy: //This one x is the z there for some reasons
		positionRecoilForce = { 0.0f, -5.0f, 0.005f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -15.0f };
		break;

	case WP_PatriotPostol:
		positionRecoilForce = { randomRange(-0.1f, 0.1f), -0.75f, 0.18f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -0.1f };
		break;

	case WP_Scorpion:
		positionRecoilForce = { randomRange(-0.2f, 0.2f), -1.0f, 0.25f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -0.2f };
		break;

	case WP_M16a1: // already corrected
		positionRecoilForce = { randomRange(-0.2f, 0.2f), -1.0f, 0.25f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -0.2f };
		break;

	case WP_Akm:
		positionRecoilForce = { randomRange(-0.2f, 0.2f), -1.0f, 0.25f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -0.2f };
		break;

	case WP_M63:
		positionRecoilForce = { randomRange(-0.2f, 0.2f), -2.0f, 0.25f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -0.2f };
		recoilPositionRecoverySpeed = 14.0f;
		recoilRotationRecoverySpeed = 12.0f;
		break;

	case WP_Ithaca:
		positionRecoilForce = { 1.0f, -30.0f, 10.0f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -6.0f };
		recoilPositionRecoverySpeed = 7.0f;
		recoilRotationRecoverySpeed = 5.0f;
		break;

	case WP_Dragnov:
		positionRecoilForce = { 0.0f, -4.0f, 0.005f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -0.4f };
		break;

	case WP_MosinNagant:
		positionRecoilForce = { randomRange(-0.2f, 0.2f), -2.0f, 0.005f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -0.4f };
		break;

	case WP_Rpg:
		positionRecoilForce = { 0.0f, -5.0f, 0.005f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -2.0f };
		break;

	default:
		return;
	}

	auto motionState = uevr::API::UObjectHook::get_motion_controller_state(WeaponRoot);
	SDK::FHitResult outHit;
	//Logger::DebugPrint("%d", IsShooting);
	if (IsShooting)
	{
		CurrentRelativeWeaponPosition += positionRecoilForce;
		CurrentRelativeWeaponRotationEuler += rotationRecoilForceEuler;
		CurrentRelativeWeaponRotation = glm::fquat(glm::radians(CurrentRelativeWeaponRotationEuler));
		ReceiverMesh->K2_SetRelativeLocation({ CurrentRelativeWeaponPosition.x, CurrentRelativeWeaponPosition.y, CurrentRelativeWeaponPosition.z }, false, &outHit, false);
		ReceiverMesh->K2_SetRelativeRotation({ CurrentRelativeWeaponRotationEuler.x, CurrentRelativeWeaponRotationEuler.y, CurrentRelativeWeaponRotationEuler.z }, false, &outHit, false);
		IsShooting = false;
	}
	else
	{
		// Smoothly return position to base
		CurrentRelativeWeaponPosition = glm::mix(CurrentRelativeWeaponPosition, DefaultRelativeWeaponPosition, delta * recoilPositionRecoverySpeed);
		CurrentRelativeWeaponRotation = glm::slerp(CurrentRelativeWeaponRotation, DefaultRelativeWeaponRotation, delta * recoilRotationRecoverySpeed);
		CurrentRelativeWeaponRotationEuler = glm::degrees(glm::eulerAngles(CurrentRelativeWeaponRotation));
		ReceiverMesh->K2_SetRelativeLocation({ CurrentRelativeWeaponPosition.x, CurrentRelativeWeaponPosition.y, CurrentRelativeWeaponPosition.z }, false, &outHit, false);
		ReceiverMesh->K2_SetRelativeRotation({ CurrentRelativeWeaponRotationEuler.x, CurrentRelativeWeaponRotationEuler.y, CurrentRelativeWeaponRotationEuler.z }, false, &outHit, false);
	}
}

float WeaponManager::randomRange(float min, float max) {
	return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
}

void WeaponManager::spawnScope()
{
	SDK::UWorld* world = SDK::UWorld::GetWorld();
	SDK::USceneComponent* sdkCurrentWeapon = (SDK::USceneComponent*)WeaponRoot;

	getRenderTarget(world);

	if (!ScopeRenderTarget)
	{
		Logger::DebugPrint("Failed to get render target, destroying Scope actor");
		if (ScopeActor)
			ScopeActor->K2_DestroyActor();
		ScopeStaticMeshComponent = nullptr;
		SceneCaptureComponent = nullptr;
		SceneCaptureUStaticMeshComponent = nullptr;
		return;
	}

	SDK::FVector weaponPosition = sdkCurrentWeapon->K2_GetComponentLocation();
	if (!ScopeActor)
	{
		ScopeStaticMeshComponent = nullptr;
		SceneCaptureComponent = nullptr;
		SceneCaptureUStaticMeshComponent = nullptr;
		DragnovScopeWidget = nullptr;
		MosinScopeWidget = nullptr;
		RpgScopeWidget = nullptr;
		ScopeActor = (SDK::AActor*)Utilities::SpawnActor(UEVR_Vector3d{0.0f, 0.0f, 0.0f}, (uevr::API::UObject*)ReceiverMesh->GetOwner());
		//Logger::DebugPrint("Scope Actor : %s", ScopeActor->GetFullName().c_str());
	}
	//Logger::DebugPrint("spawnScopeStaticMeshComponent()");
	if (!ScopeStaticMeshComponent)
		spawnScopeStaticMeshComponent();
	//Logger::DebugPrint("spawnSceneCaptureComponent()");
	if (!SceneCaptureComponent)
		spawnSceneCaptureComponent();
	//Logger::DebugPrint("spawnScopeWidget()");
	spawnScopeWidget();
}

void WeaponManager::getRenderTarget(SDK::UWorld* world)
{
	SDK::FLinearColor linearColor{};
	Logger::DebugPrint(ScopeRenderTarget->GetFullName().c_str());
	if (!ScopeRenderTarget)
	{
		ScopeRenderTarget = SDK::UKismetRenderingLibrary::CreateRenderTarget2D(world, *settingsManager->ScopeResolutionPtr, *settingsManager->ScopeResolutionPtr, SDK::ETextureRenderTargetFormat::RTF_RGBA16f, linearColor, false, false);
		Logger::DebugPrint("Render target created : %s", ScopeRenderTarget->GetFullName().c_str());
	}
	//else
	//	Logger::DebugPrint("Render target already exists");
}

void WeaponManager::spawnScopeStaticMeshComponent()
{
	SDK::FTransform transform{};
	auto scopeStaticMeshComponent = (SDK::UStaticMeshComponent*)ScopeActor->AddComponentByClass(SDK::UStaticMeshComponent::StaticClass(), false, transform, false);
	if (!scopeStaticMeshComponent/* || !scopeWidgetStaticMeshComponent*/)
	{
		Logger::DebugPrint("Failed to spawn scope mesh");
		return;
	}
	
	auto scopeDiskMaterialUObject = uevr::API::get()->find_uobject<uevr::API::UObject>(L"Material /Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial");

	if (!scopeDiskMaterialUObject)
	{
		Logger::DebugPrint("Failed to find scopeDiskMaterialUObject");
		return;
	}

	auto uDiskMaterial = (SDK::UMaterial*)scopeDiskMaterialUObject;
	uDiskMaterial->TwoSided = false;
	uDiskMaterial->BlendMode = SDK::EBlendMode::BLEND_Opaque;
	uDiskMaterial->bDisableDepthTest = true;
	//uMaterial->MaterialDomain = SDK::EMaterialDomain::MD_Surface;
	uDiskMaterial->ShadingModel = SDK::EMaterialShadingModel::MSM_DefaultLit;


	auto cylinderMesh = uevr::API::get()->find_uobject<uevr::API::UObject>(L"StaticMesh /Game/Art/VFX/Game_HD_VFX/Fire/Fire/TheFury_Fire/Mesh/SM_VFX_Disk20edge.SM_VFX_Disk20edge");
	
	if (!cylinderMesh)
	{
		Logger::DebugPrint("Can't find cylinder mesh");
		return;
	}
	//Logger::DebugPrint("cylinder mesh found");
	scopeStaticMeshComponent->SetStaticMesh((SDK::UStaticMesh*)cylinderMesh);
	scopeStaticMeshComponent->SetVisibility(false, false);
	scopeStaticMeshComponent->SetCollisionEnabled(SDK::ECollisionEnabled::NoCollision);
	scopeStaticMeshComponent->SetMobility(SDK::EComponentMobility::Movable);

	auto scopeDynamicMaterial = scopeStaticMeshComponent->CreateAndSetMaterialInstanceDynamicFromMaterial(0, uDiskMaterial);
	//Logger::DebugPrint("scopeDynamicMaterial created %s", scopeDynamicMaterial->GetFullName().c_str());
	//Applying the render texture to the material :
	scopeDynamicMaterial->SetTextureParameterValue(SDK::UKismetStringLibrary::Conv_StringToName(L"LinearColor"), ScopeRenderTarget);
	//SDK::FLinearColor color = {};
	//color.R = color.G = color.B = color.A = 2.0f;
	//scopeDynamicMaterial->SetVectorParameterValue(SDK::UKismetStringLibrary::Conv_StringToName(L"Color"), color);
	//Logger::DebugPrint("render target applied to scopeDynamicMaterial");

	ScopeStaticMeshComponent = scopeStaticMeshComponent;
	//Logger::DebugPrint("Sniper Cylinder Mesh spawned");
}

void WeaponManager::spawnSceneCaptureComponent()
{
	SDK::FTransform transform{};
	SceneCaptureUStaticMeshComponent = (SDK::UStaticMeshComponent*)ScopeActor->AddComponentByClass(SDK::UStaticMeshComponent::StaticClass(), false, transform, false);
	SceneCaptureComponent = (SDK::USceneCaptureComponent2D*)ScopeActor->AddComponentByClass(SDK::USceneCaptureComponent2D::StaticClass(), false, transform, false);

	if (!SceneCaptureUStaticMeshComponent || !SceneCaptureComponent)
	{
		Logger::DebugPrint("failed to spawn scene capture mesh and component");
		return;
	}
	SceneCaptureComponent->TextureTarget = ScopeRenderTarget;
	SceneCaptureComponent->bCaptureOnMovement = false;
	SceneCaptureComponent->bCaptureEveryFrame = true;
	SceneCaptureComponent->bUseRayTracingIfEnabled = false;
	//SceneCaptureComponent->PrimitiveRenderMode = SDK::ESceneCapturePrimitiveRenderMode::PRM_LegacySceneCapture;
	//SceneCaptureComponent->CaptureSource = SDK::ESceneCaptureSource::SCS_SceneColorHDR;
	SceneCaptureComponent->bAlwaysPersistRenderingState = true;
	SceneCaptureComponent->PostProcessSettings.bOverride_MotionBlurAmount = true;
	SceneCaptureComponent->PostProcessSettings.MotionBlurAmount = 0.0; //Disable motion blur
	SceneCaptureComponent->PostProcessSettings.bOverride_ScreenSpaceReflectionIntensity = true;
	SceneCaptureComponent->PostProcessSettings.ScreenSpaceReflectionIntensity = 0.0; //Disable screen space reflections
	SceneCaptureComponent->PostProcessSettings.bOverride_AmbientOcclusionIntensity = true;
	SceneCaptureComponent->PostProcessSettings.AmbientOcclusionIntensity = 0.0; //Disable ambient occlusion
	SceneCaptureComponent->PostProcessSettings.bOverride_BloomIntensity = true;
	SceneCaptureComponent->PostProcessSettings.BloomIntensity = 0.0;
	SceneCaptureComponent->PostProcessSettings.bOverride_LensFlareIntensity = true;
	SceneCaptureComponent->PostProcessSettings.LensFlareIntensity = 0.0; //Disable lens flares
	SceneCaptureComponent->PostProcessSettings.bOverride_VignetteIntensity = true;
	SceneCaptureComponent->PostProcessSettings.VignetteIntensity = 0.0;//Disable vignette
	SceneCaptureComponent->SetVisibility(false, false);
	SceneCaptureComponent->SetMobility(SDK::EComponentMobility::Movable);

	SceneCaptureUStaticMeshComponent->SetVisibility(false, false);
	SceneCaptureUStaticMeshComponent->SetMobility(SDK::EComponentMobility::Movable);

	//auto cigarMesh = uevr::API::get()->find_uobject<uevr::API::UObject>(L"StaticMesh /Game/Art/Environment/HD_GameplayAssets/Mesh/ciga_cigar_dmo_hd.ciga_cigar_dmo_hd");
	//SceneCaptureUStaticMeshComponent->SetStaticMesh((SDK::UStaticMesh*)cigarMesh);
}

void ProcessAllChildWidgets(SDK::UWidget* ParentWidget, SDK::FLinearColor NewTint)
{
	if (!ParentWidget) return;

	// Process this widget if it's an image
	if (strstr(ParentWidget->GetFullName().c_str(), "Image"))
	{
		SDK::UImage* ImageWidget = (SDK::UImage*)ParentWidget;
		//Logger::DebugPrint("Found image: %s", ParentWidget->GetFullName().c_str());
		//ImageWidget->SetColorAndOpacity(NewTint);
		//auto oriColor = ImageWidget->ColorAndOpacity;
		//ImageWidget->SetColorAndOpacity(NewTint);

		SDK::FSlateBrush Brush = ImageWidget->Brush;
		auto color = Brush.TintColor.SpecifiedColor;
		//Logger::DebugPrint("Color A : %f, R : %f, G : %f, B : %f ", color.A, color.R, color.G, color.B);
		Brush.TintColor.SpecifiedColor = NewTint;
		ImageWidget->SetBrush(Brush);
	}
	
	// Recurse into children
	SDK::UPanelWidget* Panel = (SDK::UPanelWidget*)ParentWidget;
	if (Panel)
	{
		int ChildCount = Panel->GetChildrenCount();
		//Logger::DebugPrint("Panel has %i children", ChildCount);
		
		for (int i = 0; i < ChildCount; i++)
		{
			SDK::UWidget* Child = Panel->GetChildAt(i);
			if (Child)
			{
				ProcessAllChildWidgets(Child, NewTint);
			}
		}
	}
}

void SetAllImageTints(SDK::UUserWidget* rootWidget, SDK::FLinearColor newTint)
{
	if (!rootWidget) return;
	
	SDK::UPanelWidget* rootPanelWidget = (SDK::UPanelWidget*)rootWidget;
	if (rootPanelWidget)
	{
		ProcessAllChildWidgets(rootPanelWidget, newTint);
	}
}


void WeaponManager::spawnScopeWidget()
{
	//Fetch existing widget :
	uevr::API::UClass* scopeWidgetClass = nullptr;
	SDK::UUserWidget* sourceWidget = nullptr;;
	//if (!DragnovScopeWidget || !MosinScopeWidget || !RpgScopeWidget)
	switch (CurrentWeaponID)
	{
	case EGsrEquipId::WP_Dragnov:
		if (DragnovScopeWidget) return;
		scopeWidgetClass = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/CobraUI.CSightDragnovWidget");
		sourceWidget = (SDK::UUserWidget*)scopeWidgetClass->get_objects_matching()[1]; //check
		break;

	case EGsrEquipId::WP_MosinNagant:
		if (MosinScopeWidget) return;
		scopeWidgetClass = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/CobraUI.CSightMosinWidget");
		sourceWidget = (SDK::UUserWidget*)scopeWidgetClass->get_first_object_matching(false); //check if false works here and below
		break;

	case EGsrEquipId::WP_Rpg:
		if (RpgScopeWidget) return;
		scopeWidgetClass = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/CobraUI.CSightRpg7Widget");
		sourceWidget = (SDK::UUserWidget*)scopeWidgetClass->get_first_object_matching(false);
		break;
	}


	
	SDK::FVector2D drawSize = {2100.0f, 2100.0f}; //The original size of the widget found ingame

	if (!sourceWidget)
	{
		Logger::DebugPrint("Failed to find the existing widget");
		return;
	}
	//Logger::DebugPrint(sourceWidget->GetFullName().c_str());
	//Logger::DebugPrint("Widget Desired Size x : %f, y : %f", drawSize.X, drawSize.Y);
	
	SDK::FTransform transform{};
	auto widgetComponent = (SDK::UWidgetComponent*)ScopeActor->AddComponentByClass(SDK::UWidgetComponent::StaticClass(), false, transform, false);
	
	if (widgetComponent == nullptr)
	{
		Logger::DebugPrint("Failed to create actor for widget creation");
		return;
	}

	//Logger::DebugPrint("Remove From Viewport");
	sourceWidget->RemoveFromViewport();
	widgetComponent->SetWidget(sourceWidget);
	widgetComponent->SetTwoSided(false);
	widgetComponent->SetDrawSize(drawSize);
	widgetComponent->BlendMode = SDK::EWidgetBlendMode::Transparent;
	//Logger::DebugPrint("Blend mode : %i", (int)widgetComponent->BlendMode);
	//sourceWidget->RenderOpacity = 0.0f;
	//sourceWidget->ColorAndOpacity.A = 0.0f;
	//sourceWidget->ForegroundColor.SpecifiedColor.A = 0.0f;

	//SDK::FLinearColor color = { 0.4f, 0.6f, 0.6f, 0.6f };
	//widgetComponent->MaterialInstance->SetVectorParameterValue(SDK::UKismetStringLibrary::Conv_StringToName(L"TintColorAndOpacity"), color);
	//widgetComponent->MaterialInstance->SetScalarParameterValue(SDK::UKismetStringLibrary::Conv_StringToName(L"OpacityFromTexture"), 0.75f);
	//widgetComponent->MaterialInstance->BasePropertyOverrides.bOverride_BlendMode = true;
	//widgetComponent->MaterialInstance->BasePropertyOverrides.BlendMode = SDK::EBlendMode::BLEND_Translucent;
	//widgetComponent->MaterialInstance->SetVectorParameterValue(SDK::UKismetStringLibrary::Conv_StringToName(L"BackColor"), color);

	
	//Set arrows to white for better visibility
	SDK::FLinearColor color = { 1.0f, 1.0f, 1.0f, 1.0f }; 
	SDK::UCanvasPanel* arrowGroup = nullptr;

	switch (CurrentWeaponID)
	{
	case EGsrEquipId::WP_Dragnov:
	{
		auto uSightDragnovWidget_C = (SDK::USightDragnovWidget_C*)sourceWidget;
		arrowGroup = uSightDragnovWidget_C->ArrowGroup;
		uSightDragnovWidget_C->BoxBg->SetVisibility(SDK::ESlateVisibility::Hidden);
		uSightDragnovWidget_C->RightBox->SetVisibility(SDK::ESlateVisibility::Hidden);
		uSightDragnovWidget_C->LeftBox->SetVisibility(SDK::ESlateVisibility::Hidden);

		SourceDragnovScopeWidget = sourceWidget;
		DragnovScopeWidget = widgetComponent;
		//Logger::DebugPrint(DragnovScopeWidget->GetFullName().c_str());
		break;
	}
	case EGsrEquipId::WP_MosinNagant:
	{
		auto uSightMosinWidget_C = (SDK::USightMosinWidget_C*)sourceWidget;
		arrowGroup = (SDK::UCanvasPanel*)uSightMosinWidget_C->ArrowGroup;

		uSightMosinWidget_C->BoxBg->SetVisibility(SDK::ESlateVisibility::Hidden);
		uSightMosinWidget_C->RightBox->SetVisibility(SDK::ESlateVisibility::Hidden);
		uSightMosinWidget_C->LeftBox->SetVisibility(SDK::ESlateVisibility::Hidden);

		SDK::FLinearColor mosinColor = { 1.0f, 1.0f, 1.0f, 0.0f };
		auto root = (SDK::UCanvasPanel*)uSightMosinWidget_C->WidgetTree->RootWidget;
		auto ImageWidget = (SDK::UImage*)root->GetChildAt(0);
		Logger::DebugPrint(ImageWidget->GetFullName().c_str());
		SDK::FSlateBrush Brush = ImageWidget->Brush;
		auto color = Brush.TintColor.SpecifiedColor;
		Brush.TintColor.SpecifiedColor = mosinColor;
		ImageWidget->SetBrush(Brush);
		SourceMosinScopeWidget = sourceWidget;
		MosinScopeWidget = widgetComponent;
		//Logger::DebugPrint(MosinScopeWidget->GetFullName().c_str());
		break;
	}
	case EGsrEquipId::WP_Rpg:
	{
		auto uSightRpg7Widget_C = (SDK::USightRPG7Widget_C*)sourceWidget;
		arrowGroup = uSightRpg7Widget_C->ArrowGroup;
		uSightRpg7Widget_C->BoxBg->SetVisibility(SDK::ESlateVisibility::Hidden);
		uSightRpg7Widget_C->RightBox->SetVisibility(SDK::ESlateVisibility::Hidden);
		uSightRpg7Widget_C->LeftBox->SetVisibility(SDK::ESlateVisibility::Hidden);

		SourceRpgScopeWidget = sourceWidget;
		RpgScopeWidget = widgetComponent;
		//Logger::DebugPrint(DragnovScopeWidget->GetFullName().c_str());
		break;
	}
	}

	SetAllImageTints((SDK::UUserWidget*)arrowGroup, color);


	//ScopeWidget = (SDK::UWidgetComponent*)uObject;
}

void WeaponManager::attachScopeToWeapon()
{
	if (!ReceiverMesh){	Logger::DebugPrint("attachScopeToWeapon() : ReceiverMesh not found"); return; }
	if (!SceneCaptureComponent){	Logger::DebugPrint("attachScopeToWeapon() : SceneCaptureComponent not found"); return; }
	if (!SceneCaptureUStaticMeshComponent){	Logger::DebugPrint("attachScopeToWeapon() : SceneCaptureComponentMesh not found"); return; }
	if (!ScopeStaticMeshComponent){	Logger::DebugPrint("attachScopeToWeapon() : ScopeCylinderComponent not found"); return; }
	//if (!ReceiverMesh){	Logger::DebugPrint("attachScopeToWeapon() : ReceiverMesh not found"); return; }

	Logger::DebugPrint("Attaching scope to weapon");
	SceneCaptureUStaticMeshComponent->K2_AttachToComponent(
		ReceiverMesh,
		SDK::UKismetStringLibrary::Conv_StringToName(L"CNP_MUZZLE_FLASH"),
		SDK::EAttachmentRule::SnapToTarget,
		SDK::EAttachmentRule::SnapToTarget,
		SDK::EAttachmentRule::KeepRelative,
		true
	);

	SceneCaptureComponent->K2_AttachToComponent(
		SceneCaptureUStaticMeshComponent,
		SDK::UKismetStringLibrary::Conv_StringToName(L"None"),
		SDK::EAttachmentRule::SnapToTarget,
		SDK::EAttachmentRule::SnapToTarget,
		SDK::EAttachmentRule::KeepRelative,
		true
	);

	ScopeStaticMeshComponent->K2_AttachToComponent(		
		ReceiverMesh,
		SDK::UKismetStringLibrary::Conv_StringToName(L"CNP_REAR_SIGHT"),
		SDK::EAttachmentRule::SnapToTarget,
		SDK::EAttachmentRule::SnapToTarget,
		SDK::EAttachmentRule::KeepRelative,
		true
	);

	//ScopeWidgetStaticMeshComponent->K2_AttachToComponent(
	//	ReceiverMesh,
	//	SDK::UKismetStringLibrary::Conv_StringToName(L"CNP_REAR_SIGHT"),
	//	SDK::EAttachmentRule::SnapToTarget,
	//	SDK::EAttachmentRule::SnapToTarget,
	//	SDK::EAttachmentRule::KeepRelative,
	//	true
	//);

	SDK::FHitResult hitResult;
	SceneCaptureComponent->SetVisibility(true, true);
	SceneCaptureComponent->SetMobility(SDK::EComponentMobility::Movable);
	SceneCaptureComponent->Activate(true);
	SceneCaptureComponent->K2_SetRelativeRotation({0.0f, 90.0f, 0.0f}, false, &hitResult, true);

	switch (CurrentWeaponID)
	{
	case EGsrEquipId::WP_Dragnov:
		DragnovScopeWidget->K2_AttachToComponent(
			ScopeStaticMeshComponent,
			SDK::UKismetStringLibrary::Conv_StringToName(L"None"),
			SDK::EAttachmentRule::SnapToTarget,
			SDK::EAttachmentRule::SnapToTarget,
			SDK::EAttachmentRule::KeepRelative,
			true
		);
		SceneCaptureUStaticMeshComponent->K2_SetRelativeLocation({ 0.0f, 0.0f, 0.0f }, false, &hitResult, true);
		ScopeStaticMeshComponent->K2_SetRelativeLocation({ 0.0f, -30.9, 0.0f }, false, &hitResult, true);
		ScopeStaticMeshComponent->K2_SetRelativeRotation({ 180.0f, 0.0f, -90.0f }, false, &hitResult, true);
		ScopeStaticMeshComponent->SetRelativeScale3D({ 0.15f, 0.15f, 0.15f });
		DragnovScopeWidget->K2_SetRelativeLocation({ 0.0f, 0.0f, 0.05f }, false, &hitResult, true);
		DragnovScopeWidget->K2_SetRelativeRotation({ 90.0f, 0.0f, -90.0f }, false, &hitResult, true);
		DragnovScopeWidget->SetRelativeScale3D({ 0.025f, 0.025f, 0.025f });
		break;
	case EGsrEquipId::WP_MosinNagant:
		MosinScopeWidget->K2_AttachToComponent(
			ScopeStaticMeshComponent,
			SDK::UKismetStringLibrary::Conv_StringToName(L"None"),
			SDK::EAttachmentRule::SnapToTarget,
			SDK::EAttachmentRule::SnapToTarget,
			SDK::EAttachmentRule::KeepRelative,
			true
		);
		SceneCaptureUStaticMeshComponent->K2_SetRelativeLocation({ 0.0f, 0.0f, 0.0f }, false, &hitResult, true);
		ScopeStaticMeshComponent->K2_SetRelativeLocation({ 0.0f, -18.5f, 0.0f }, false, &hitResult, true);
		ScopeStaticMeshComponent->K2_SetRelativeRotation({ 180.0f, 0.0f, -90.0f }, false, &hitResult, true);
		ScopeStaticMeshComponent->SetRelativeScale3D({ 0.12f, 0.12f, 0.12f });
		MosinScopeWidget->K2_SetRelativeLocation({ 0.0f, 0.0f, 0.05f }, false, &hitResult, true);
		MosinScopeWidget->K2_SetRelativeRotation({ 90.0f, 0.0f, -90.0f }, false, &hitResult, true);
		MosinScopeWidget->SetRelativeScale3D({ 0.023f, 0.023f, 0.023f });
		break;
	case EGsrEquipId::WP_Rpg:
		RpgScopeWidget->K2_AttachToComponent(
			ScopeStaticMeshComponent,
			SDK::UKismetStringLibrary::Conv_StringToName(L"None"),
			SDK::EAttachmentRule::SnapToTarget,
			SDK::EAttachmentRule::SnapToTarget,
			SDK::EAttachmentRule::KeepRelative,
			true
		);
		SceneCaptureUStaticMeshComponent->K2_SetRelativeLocation({ 0.0f, 26.0f, 0.0f }, false, &hitResult, true);
		ScopeStaticMeshComponent->K2_SetRelativeLocation({ 0.0f, 1.33f, 0.0f }, false, &hitResult, true);
		ScopeStaticMeshComponent->K2_SetRelativeRotation({ 180.0f, 0.0f, -90.0f }, false, &hitResult, true);
		ScopeStaticMeshComponent->SetRelativeScale3D({ 0.185f, 0.185f, 0.185f });
		RpgScopeWidget->K2_SetRelativeLocation({ 0.0f, 0.0f, 0.1f }, false, &hitResult, true);
		RpgScopeWidget->K2_SetRelativeRotation({ 90.0f, 0.0f, -90.0f }, false, &hitResult, true);
		RpgScopeWidget->SetRelativeScale3D({ 0.012f, 0.012f, 0.012f });
		break;
	}

	ScopeStaticMeshComponent->SetVisibility(false, false);

	//ScopeWidgetStaticMeshComponent->K2_SetRelativeLocation({ 0.0f, -26.0f, 0.0f }, false, &hitResult, true);
	//ScopeWidgetStaticMeshComponent->K2_SetRelativeRotation({ 180.0f, 0.0f, -90.0f }, false, &hitResult, true);
	////ScopeWidgetStaticMeshComponent->SetRelativeScale3D({0.15f, 0.15f, 0.15f});
	//ScopeWidgetStaticMeshComponent->SetRelativeScale3D({2.0f, 2.0f, 2.0f});
	//ScopeWidgetStaticMeshComponent->SetVisibility(false, false);


	//ScopeWidget->SetRelativeScale3D({0.02f, 0.02f, 0.02f});
}



