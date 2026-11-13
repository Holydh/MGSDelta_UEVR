#include "WeaponManager.hpp"

namespace {
constinit ue::ClassRef SkeletalMeshComponent_c{ L"Class /Script/Engine.SkeletalMeshComponent" };
constinit ue::ClassRef StaticMeshComponent_c{ L"Class /Script/Engine.StaticMeshComponent" };
constinit ue::ClassRef SceneCaptureComponent2D_c{ L"Class /Script/Engine.SceneCaptureComponent2D" };
constinit ue::ClassRef WidgetComponent_c{ L"Class /Script/UMG.WidgetComponent" };
constinit ue::ClassRef Image_c{ L"Class /Script/UMG.Image" };
constinit ue::ClassRef GsrGun_c{ L"Class /Script/Gsr.GsrGun" };
constinit ue::ClassRef PlayerCameraManagerBP_c{ L"BlueprintGeneratedClass /Game/Blueprints/Camera/BP_PlayerCameraManager.BP_PlayerCameraManager_C" };
constinit ue::SingletonRef GsrPlayerCameraController_c{ L"Class /Script/Gsr.GsrPlayerCameraController" };
constinit ue::ClassRef CSightDragnovWidget_c{ L"Class /Script/CobraUI.CSightDragnovWidget" };
constinit ue::ClassRef CSightMosinWidget_c{ L"Class /Script/CobraUI.CSightMosinWidget" };
constinit ue::ClassRef CSightRpg7Widget_c{ L"Class /Script/CobraUI.CSightRpg7Widget" };

ue::UObject* firstObjectOf(ue::ClassRef& cls)
{
	return cls.get() ? cls.get()->get_first_object_matching(false) : nullptr;
}

void hideWidget(ue::UObject* widget)
{
	ue::call(widget, L"SetVisibility", ue::ESlateVisibility::Hidden);
}

// Image->Brush.TintColor.SpecifiedColor = tint, then Image->SetBrush(Brush)
void setImageTint(ue::UObject* image, const ue::FLinearColor& tint)
{
	auto brush = ue::get_struct(image, L"Brush");
	if (!brush)
		return;
	brush.sub(L"TintColor").set(L"SpecifiedColor", tint);
	ue::call(image, L"SetBrush", brush);
}
}

void WeaponManager::InitWeaponManager()
{
	tntSocketFname = ue::FName(L"TntTriggerSocket_LeftHand");
	cqcKnifeSocketFname = ue::FName(L"BaseBody_LeftHand_MTPA");
	DummyCqcKnifeMeshActor = nullptr;
}

void WeaponManager::ApplyHandMaterialToVRHands()
{
	auto pawn = uevr::API::get()->get_local_pawn(0);
	auto pawnHandMesh = ue::get<ue::UObject*>(pawn, L"SkeletalMesh_Hand");
	if (!pawnHandMesh || !playerManager->LeftHandMesh || !playerManager->RightHandMesh)
		return;
	auto handMaterial = ue::call<ue::TArray<ue::UObject*>>(pawnHandMesh, L"GetMaterials");
	auto previousMaterialCount = ue::call<int32_t>(playerManager->LeftHandMesh, L"GetNumMaterials");
	// Cleanup additional override material slots (left over from a hand material with more slots)
	for (int i = handMaterial.count; i < previousMaterialCount; i++)
	{
		ue::call(playerManager->LeftHandMesh, L"SetMaterial", i, nullptr);
		ue::call(playerManager->RightHandMesh, L"SetMaterial", i, nullptr);
	}

	for (int i = 0; i < handMaterial.count; i++)
	{
		ue::call(playerManager->LeftHandMesh, L"SetMaterial", i, handMaterial.data[i]);
		ue::call(playerManager->RightHandMesh, L"SetMaterial", i, handMaterial.data[i]);
	}
	auto handOverlayMaterial = ue::call<ue::UObject*>(pawnHandMesh, L"GetOverlayMaterial");
	ue::call(playerManager->RightHandMesh, L"SetOverlayMaterial", handOverlayMaterial);
	ue::call(playerManager->LeftHandMesh, L"SetOverlayMaterial", handOverlayMaterial);
}

void WeaponManager::ProcessItem()
{
	if (ItemController == NULL)
	{
		CurrentItemID = ue::EGsrItemId::IT_None;
		return;
	}

	auto newItemID = ue::call<ue::EGsrItemId>(ItemController, L"GetItemId");
	if (CurrentItemID != newItemID)
	{
		CurrentItemID = newItemID;
		cameraController->CurrentItemID = CurrentItemID;
		Logger::DebugPrint("Item changed. New Item ID : %d", (int)CurrentItemID.Get());

		CurrentItem = ue::get<ue::UObject*>(ItemController, L"m_currentItem");
		if (CurrentItem)
			Logger::DebugPrint(CurrentItem->get_full_name());

		if (CurrentItemID == ue::EGsrItemId::IT_NightVision)
		{
			ue::call(ue::get<ue::UObject*>(CurrentItem, L"SKM_nvgo_dmo_hd"), L"SetOnlyOwnerSee", true);

		}
		if (CurrentItemID == ue::EGsrItemId::IT_Thermal)
		{
			ue::call(ue::get<ue::UObject*>(CurrentItem, L"SKM_thgo_dmo_hd"), L"SetOnlyOwnerSee", true);
			playerCameraManager = firstObjectOf(PlayerCameraManagerBP_c);
			if (playerCameraManager == nullptr)
			{
				Logger::DebugPrint("No player camera manager found");
				return;
			}
			ToggleThermalVisionScanlineNextFrame = true;
		}

		if (CurrentItemID.Entered(ue::EGsrItemId::IT_Stealth))
		{
			ApplyHandMaterialToVRHands();
		}
		if (CurrentItemID.Left(ue::EGsrItemId::IT_Stealth))
		{
			ApplyHandMaterialToVRHands();
		}

		if (CurrentItemID.Entered(ue::EGsrItemId::IT_Scope))
		{
			uevr::API::VR::set_decoupled_pitch_enabled(false);
		}
		if (CurrentItemID.Left(ue::EGsrItemId::IT_Scope))
		{
			uevr::API::get()->dispatch_lua_event("HideHands", "0");
			uevr::API::VR::set_decoupled_pitch_enabled(true);
		}

		cameraController->CardBoardEquipped = CurrentItemID == ue::EGsrItemId::IT_CBoxA || CurrentItemID == ue::EGsrItemId::IT_CBoxB ||
			CurrentItemID == ue::EGsrItemId::IT_CBoxC || CurrentItemID == ue::EGsrItemId::IT_CBoxD;
	}
	if (CurrentItemID == ue::EGsrItemId::IT_Scope)
	{
		uevr::API::get()->dispatch_lua_event("HideHands", "1");
		uevr::API::UObjectHook::remove_motion_controller_state(LeftHandWeaponRoot);
		uevr::API::UObjectHook::remove_motion_controller_state(WeaponRoot);
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
		auto weaponNameView = magic_enum::enum_name(CurrentWeaponID.Get());
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

void WeaponManager::FetchWeaponOffsets(EGsrEquipId currentWeapon, glm::fvec3& outPos, glm::fquat& outRot, ue::FRotator& outRightHandRotationOffset, ue::FVector& outLeftHandLocationOffset, ue::FRotator& outLeftHandRotationOffset)
{
	// Weapons without a config get the default offsets, not the previous weapon's ones
	const WeaponConfig defaults{};
	outPos = defaults.location_offset;
	outRot = defaults.rotation_offset;
	outRightHandRotationOffset = defaults.right_hand_rotation_offset;
	outLeftHandLocationOffset = defaults.left_hand_location_offset;
	outLeftHandRotationOffset = defaults.left_hand_rotation_offset;

	const char* key = WeaponIdToKey(currentWeapon);
	if (!key)
	{
		Logger::DebugPrint("No valid weapon key found for ID: %d", currentWeapon);
		return;
	}

    auto it = settingsManager->g_weaponConfigs.find(key);
	Logger::DebugPrint("Fetching weapon config for key: %s", key);
    if (it == settingsManager->g_weaponConfigs.end()) return;

	const WeaponConfig& cfg = it->second;
    outPos = cfg.location_offset;
    outRot = cfg.rotation_offset;
	outRightHandRotationOffset = cfg.right_hand_rotation_offset;
	outLeftHandLocationOffset = cfg.left_hand_location_offset;
	outLeftHandRotationOffset = cfg.left_hand_rotation_offset;
}

void WeaponManager::FetchEquipController()
{
	auto pawn = uevr::API::get()->get_local_pawn(0);
	EquipController = ue::call<ue::UObject*>(pawn, L"GetEquipControllerBP");
	if (EquipController == NULL)
	{
		Logger::DebugPrint("No equipController found");
	}

	ItemController = ue::call<ue::UObject*>(pawn, L"GetItemControllerBP");
	if (ItemController == NULL)
	{
		Logger::DebugPrint("No itemController found");
	}
}

void WeaponManager::SpawnDummyCqcKnifeMesh()
{
	if (!LeftHandWeaponRoot || DummyCqcKnifeMeshActor)
		return;

	if (DummyCqcKnifeMeshActor == nullptr && playerManager->LeftHandMesh)
	{
		constexpr ue::FVector cqcKnifeRelativeLocation = { 1.248f, -2.072f, -4.754f };
		constexpr ue::FRotator cqcKnifeRelativeRotation = { 7.228f, -169.534f, 58.205f};

		uevr::API::UObject* meshComponent = nullptr;
		const auto leftWeaponRootTransform = ue::call<ue::FTransform>(LeftHandWeaponRoot, L"GetRelativeTransform");
		DummyCqcKnifeMeshActor = Utilities::SpawnMesh(leftWeaponRootTransform, L"SkeletalMesh /Game/Gsr/Weapon/cqck/Mesh/SM_Cqck.SM_Cqck", meshComponent); //L"SkeletalMesh /Game/Gsr/Weapon/cqck/Mesh/SM_Cqck.SM_Cqck"
		if (!DummyCqcKnifeMeshActor)
			return;
		ue::call(DummyCqcKnifeMeshActor, L"K2_AttachRootComponentTo", playerManager->LeftHandMesh, ue::FName(L"None"), ue::EAttachLocation::SnapToTarget, false);
		ue::call(DummyCqcKnifeMeshActor, L"K2_SetActorRelativeLocation", cqcKnifeRelativeLocation, false, nullptr, true);
		ue::call(DummyCqcKnifeMeshActor, L"K2_SetActorRelativeRotation", cqcKnifeRelativeRotation, false, nullptr, true);
		DummyCqcKnifeMesh = meshComponent;
		ue::call(DummyCqcKnifeMesh, L"SetOnlyOwnerSee", true);
	}
}

void WeaponManager::ToggleDummyCQCKnifeMeshVisibility()
{
	// SetOnlyOwnerSee(true) hides a mesh from the VR view. Only call the engine when it differs.
	auto setHidden = [](ue::UObject* mesh, bool hidden)
	{
		if (mesh && ue::get<bool>(mesh, L"bOnlyOwnerSee") != hidden)
			ue::call(mesh, L"SetOnlyOwnerSee", hidden);
	};
	auto destroyDummyKnife = [this]()
	{
		DummyCqcKnifeMesh = nullptr;
		if (ue::is_valid(DummyCqcKnifeMeshActor))
			ue::call(DummyCqcKnifeMeshActor, L"K2_DestroyActor");
		DummyCqcKnifeMeshActor = nullptr;
	};

	if (!playerManager->IsPlayerControlled)
	{
		destroyDummyKnife();
		return;
	}

	// While the left hand holds the weapon, the dummy knife (attached to the VR left hand)
	// replaces the game's left hand CQC knife.
	const bool showDummyKnife = holdCQCKnifeInLeftHand && playerManager->SecondHandAttachedToWeapon && playerManager->LeftHandMesh;

	if (showDummyKnife && LeftHandWeaponRoot && !ue::is_valid(DummyCqcKnifeMesh))
	{
		destroyDummyKnife();
		SpawnDummyCqcKnifeMesh();
	}

	// Enforced every frame, not only on changes: the game recreates its left hand knife (e.g. when
	// leaving first person view), and the new one used to stay visible next to the dummy (two knives).
	if (ue::is_valid(DummyCqcKnifeMesh))
		setHidden(DummyCqcKnifeMesh, !showDummyKnife);
	if (holdCQCKnifeInLeftHand && LeftHandWeaponRoot)
		setHidden(LeftHandWeaponRoot, showDummyKnife);
}

void WeaponManager::ProcessTwoHandsGrabWeapon()
{
	if (!canBeTwoHanded)
		return;

	if (Inputs::SecondHandGripTriggeredBeforeDelay && !secondHandShouldAttachToWeapon)
	{
		secondHandShouldAttachToWeapon = true;
		AttachSecondHandNextFrame = true;
			
	}
	else if (Inputs::SecondHandGripTriggeredBeforeDelay && secondHandShouldAttachToWeapon)
	{
		secondHandShouldAttachToWeapon = false;
		DetachSecondHandFromWeapon();
	}
}

void WeaponManager::ProcessTwoHandsGrabWeaponLocationAndRotation()
{
	// Pivot disabled: the weapon stays held by the right hand only (UEVR attachment)
	if (twoHandedPivot && settingsManager->Settings.enableTwoHandsPivot && WeaponRoot) {
		auto rightCtrlPos = ue::call<ue::FVector>(playerManager->RightController, L"K2_GetComponentLocation");
		auto leftCtrlPos = ue::call<ue::FVector>(playerManager->LeftController, L"K2_GetComponentLocation");

		auto weaponUpVector = ue::call<ue::FVector>(CurrentWeapon, L"GetActorUpVector");

		ue::FVector adjustedLeftPos = leftCtrlPos + (weaponUpVector * LeftHandWeaponGrabOffset);

		ue::FVector directionVec = adjustedLeftPos - rightCtrlPos;
		directionVec.Normalize();

		ue::FRotator targetRot = ue::call_static<ue::FRotator>(ue::lib::KismetMath, L"MakeRotFromXZ", directionVec, weaponUpVector);

		const ue::FRotator fixUp(0.f, -90.f, 0.f);
		targetRot = ue::call_static<ue::FRotator>(ue::lib::KismetMath, L"ComposeRotators", fixUp, targetRot);

		const ue::FVector localOffset(DefaultUevrHookWeaponPosition.z, DefaultUevrHookWeaponPosition.x, DefaultUevrHookWeaponPosition.y);
		ue::FVector rotatedOffset = ue::call_static<ue::FVector>(ue::lib::KismetMath, L"GreaterGreater_VectorRotator", localOffset, targetRot);

		ue::FVector finalRootPos = rightCtrlPos - rotatedOffset;

		ue::FHitResult outHit;
		ue::call(WeaponRoot, L"K2_SetWorldLocationAndRotation", finalRootPos, targetRot, false, &outHit, false);

	}
}


// Makes UEVR hold the weapon in the right hand, with the offsets of the weapon's config file
void WeaponManager::AttachWeaponRootToHand()
{
	if (!WeaponRoot)
		return;
	UEVR_Vector3f defaultPos = { DefaultUevrHookWeaponPosition.x, DefaultUevrHookWeaponPosition.y, DefaultUevrHookWeaponPosition.z };
	UEVR_Quaternionf defaultRot = {DefaultUevrHookWeaponRotation.w, DefaultUevrHookWeaponRotation.x, DefaultUevrHookWeaponRotation.y, DefaultUevrHookWeaponRotation.z};

	auto motionState = uevr::API::UObjectHook::get_or_add_motion_controller_state(WeaponRoot);
	if (!motionState)
		return;
	motionState->set_location_offset(&defaultPos);
	motionState->set_rotation_offset(&defaultRot);
	motionState->set_hand(1);
	motionState->set_permanent(true);
}

void WeaponManager::RemoveUobjectHookFromHand(Hand handSide)
{
	ue::FHitResult outHit;
	if (handSide == Left)
	{
		if (LeftHandWeaponRoot)
		{
			uevr::API::UObjectHook::remove_motion_controller_state(LeftHandWeaponRoot);
			ue::call(LeftHandWeaponRoot, L"K2_SetRelativeLocationAndRotation", ue::FVector{ 0.0f, 0.0f, 0.0f }, ue::FRotator{ 0.0f, 0.0f, -90.0f }, false, &outHit, true);
		}
	}
	else
	{
		if (WeaponRoot)
		{
			ue::call(WeaponRoot, L"K2_SetRelativeLocationAndRotation", ue::FVector{ 0.0f, 0.0f, 0.0f }, ue::FRotator{ 0.0f, 0.0f, 0.0f }, false, &outHit, true);
			uevr::API::UObjectHook::remove_motion_controller_state(WeaponRoot);
		}
	}
}

void WeaponManager::ProcessWeapon(float delta)
{
	if (AttachSecondHandNextFrame && !playerManager->MeleeAttackRunning)
	{
		AttachSecondHandToWeapon();
	}

	if (AttachHandNextFrame && NextFrame && !playerManager->MeleeAttackRunning)
	{
		Logger::DebugPrint("Check main hand need attach");
		AttachMainHandToWeapon();
		AttachSecondHandNextFrame = true;
		SendHandStateToLua();
		NextFrame = false;
	}

	if (ToggleThermalVisionScanlineNextFrame)
	{
		ToggleThermalVisionScanline();
	}

	//Get weapon ID
	if (EquipController == NULL)
	{
		CurrentWeaponID = WP_None;
		Logger::DebugPrint("No equipController found");
		return;
	}

	auto weaponId = ue::call<EGsrEquipId>(EquipController, L"GetCurrentWeaponId");

	if (weaponId >= 31)
	{
		weaponId = EGsrEquipId::WP_MAX_PLAYER_WEAPONS;
	}

	if (CurrentWeaponID != weaponId)
	{
		CurrentWeaponID = weaponId;
		Logger::DebugPrint("Weapon changed. New Weapon ID : %d", (int)CurrentWeaponID.Get());
	}
	


	auto currentWeapon = ue::call<ue::UObject*>(EquipController, L"GetCurrentWeapon");

	// Also set up again if the weapon's root was freed: a respawned weapon can get the address of the previous one
	if (CurrentWeapon != currentWeapon || WeaponRoot.expired())
	{
		const auto previousWeaponID = CurrentWeaponID.Previous();
		if (previousWeaponID == EGsrEquipId::WP_Dragnov || previousWeaponID == EGsrEquipId::WP_MosinNagant || previousWeaponID == EGsrEquipId::WP_Rpg)
			ue::call(ReceiverMesh, L"SetVisibility", false, false);

		ReceiverMesh = nullptr;
		WeaponRoot = nullptr;
		LeftHandWeaponRoot = nullptr;
		
		if (playerManager->MainHandAttachedToWeapon)
			DetachHandFromWeapon();
		if (playerManager->SecondHandAttachedToWeapon)
		{
			DetachSecondHandFromWeapon();
		}


		SendHandStateToLua();
	

		SwitchScopeState(false);

		CurrentWeapon = currentWeapon;
		Logger::DebugPrint("Updating Current weapon ptr");
		if (CurrentWeapon)
			Logger::DebugPrint(CurrentWeapon->get_full_name());

		WeaponRoot = ue::get<ue::UObject*>(CurrentWeapon, L"RootComponent");
		if (WeaponRoot == nullptr)
		{
			Logger::DebugPrint("No weapon root found. Return");
			return;
		}
		Logger::DebugPrint(WeaponRoot->get_full_name());

		FetchWeaponOffsets(CurrentWeaponID, DefaultUevrHookWeaponPosition, DefaultUevrHookWeaponRotation, RightHandRotationOffset, LeftHandLocationOffset, LeftHandRotationOffset);



		Logger::DebugPrint("Applied weapon offsets from config");
		Logger::DebugPrint("Position Offset : x=%f, y=%f, z=%f", DefaultUevrHookWeaponPosition.x, DefaultUevrHookWeaponPosition.y, DefaultUevrHookWeaponPosition.z);

		CurrentRelativeWeaponPosition = DefaultRelativeWeaponPosition;
		CurrentRelativeWeaponRotation = DefaultRelativeWeaponRotation;
		CurrentRelativeWeaponRotationEuler = glm::eulerAngles(CurrentRelativeWeaponRotation);

		AttachWeaponRootToHand();



		auto childrenArray = ue::array_snapshot(WeaponRoot, L"AttachChildren");
		for (auto child : childrenArray)
		{
			if (!child) continue;
			if (ue::is_a(child, SkeletalMeshComponent_c)) {
				ReceiverMesh = child;
			}
		}

		if (settingsManager->Settings.enableWeaponsScopes)
		{
			SwitchScopeState(false);

			if (CurrentWeaponID == EGsrEquipId::WP_Dragnov || CurrentWeaponID == EGsrEquipId::WP_MosinNagant || CurrentWeaponID == EGsrEquipId::WP_Rpg)
			{
				if (!ReceiverMesh)
				{
					Logger::DebugPrint("no Receiver mesh found");
					return;
				}
				for (auto child : childrenArray)
				{
					if (!child) continue;
					//Destroy any previous cylinder mesh if still attached. Only ours (owned by the scope actor):
					//the weapon's own meshes belong to the game, which may still use them.
					if (ScopeActor && ue::is_a(child, StaticMeshComponent_c) && ue::call<ue::UObject*>(child, L"GetOwner") == ScopeActor) {
						ue::call(child, L"K2_DestroyComponent", child);
						ScopeStaticMeshComponent = nullptr;
					}
				}

				spawnScope();
				attachScopeToWeapon();
			}
		}

		if (CurrentWeaponID == EGsrEquipId::WP_CigarPistol)
		{
			ue::call(WeaponRoot, L"SetRelativeScale3D", ue::FVector{ 1.5f, 1.5f, 1.5f });
			return;
		}

		if (mainHandShouldAttachToWeapon)
			AttachHandNextFrame = true;
	}

	// The weapon is detached from the hand during melee attacks, while carrying, with the scope item, and
	// while the player has no control (menus, radio, cutscenes): put it back once none of these applies.
	if (WeaponRoot && !playerManager->MeleeAttackRunning && !playerManager->IsCarrying && CurrentItemID != ue::EGsrItemId::IT_Scope &&
		uevr::API::UObjectHook::get_motion_controller_state(WeaponRoot) == nullptr)
	{
		AttachWeaponRootToHand();
	}

	if (CurrentWeaponID == EGsrEquipId::WP_MAX_PLAYER_WEAPONS)
	{
		//Food floating animation
		ue::UObject* foodMesh = nullptr;
		for (auto child : ue::array_snapshot(WeaponRoot, L"AttachChildren"))
		{
			if (!child) continue;
			if (ue::is_a(child, StaticMeshComponent_c)) {
				const std::wstring meshName = child->get_fname()->to_string();
				if (meshName.find(L"ThrowableStaticMesh") != std::wstring::npos)
				{
					foodMesh = child;
					break;
				}
			}
		}

		if (!foodMesh)
		{
			Logger::DebugPrint("No food mesh found");
			return;
		}

		ue::FHitResult hitResult;

		if (Inputs::LeftGripInputPressed.Fell())
		{
			// Re-sync to current world rotation
			auto loc = ue::call<ue::FVector>(foodMesh, L"K2_GetComponentLocation");
			FoodIdleStart = glm::vec3(loc.X, loc.Y, loc.Z);
			FoodIdlePosition = FoodIdleStart;

			auto rotQuat = ue::call<ue::FTransform>(foodMesh, L"K2_GetComponentToWorld").Rotation;
			auto rot = ue::call<ue::FRotator>(foodMesh, L"K2_GetComponentRotation");
			FoodCurrentLocalRotation = {(float)rotQuat.W, (float)rotQuat.X, (float)rotQuat.Y, (float)rotQuat.Z};
			ActualFoodRotation = rot.Yaw;
		}

		if (Inputs::LeftGripInputPressed.Rose())
		{
			auto foodT = ue::call<ue::FTransform>(foodMesh, L"K2_GetComponentToWorld");
			auto handT = ue::call<ue::FTransform>(playerManager->LeftHandMesh, L"K2_GetComponentToWorld");

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
			auto weaponRootPos = ue::call<ue::FVector>(WeaponRoot, L"K2_GetComponentLocation");
			glm::vec3 targetPos(
				weaponRootPos.X,
				weaponRootPos.Y,
				weaponRootPos.Z + positionOffset
			);


			auto test = ue::call<ue::FVector>(foodMesh, L"K2_GetComponentLocation");
			FoodIdlePosition = glm::mix({test.X, test.Y, test.Z}, targetPos, delta * 8.0f);

			// apply
			ue::FTransform finalTsf = {};
			finalTsf.Rotation = {FoodCurrentLocalRotation.x, FoodCurrentLocalRotation.y, FoodCurrentLocalRotation.z, FoodCurrentLocalRotation.w};
			finalTsf.Translation = {FoodIdlePosition.x, FoodIdlePosition.y, FoodIdlePosition.z};
			finalTsf.Scale3D = ue::call<ue::FVector>(foodMesh, L"K2_GetComponentScale");

			

			ue::call(foodMesh, L"K2_SetWorldTransform", finalTsf, false, &hitResult, false);
		}
		else
		{
			// Follow left hand position 
			auto handT = ue::call<ue::FTransform>(playerManager->LeftHandMesh, L"K2_GetComponentToWorld");

			glm::quat handRot(handT.Rotation.W, handT.Rotation.X, handT.Rotation.Y, handT.Rotation.Z);
			glm::vec3 handPos(handT.Translation.X, handT.Translation.Y, handT.Translation.Z);

			// world transform reconstruction with the offset
			glm::quat newFoodRot = handRot * FoodGrabRelativeRotation;
			glm::vec3 newFoodPos = handPos + (handRot * FoodGrabRelativePosition);

			ue::FTransform out;
			out.Rotation = { newFoodRot.x, newFoodRot.y, newFoodRot.z, newFoodRot.w };
			out.Translation = { newFoodPos.x, newFoodPos.y, newFoodPos.z };
			out.Scale3D = ue::call<ue::FVector>(foodMesh, L"K2_GetComponentScale");

			ue::call(foodMesh, L"K2_SetWorldTransform", out, false, &hitResult, false);
		}

		return;
	}
		

	// The game recreates its left hand object (e.g. when leaving first person view): drop the destroyed one
	if (LeftHandWeaponRoot && !ue::is_valid(LeftHandWeaponRoot))
		LeftHandWeaponRoot = nullptr;

	if (!LeftHandWeaponRoot && hasObjectInLeftHand)
	{
		auto leftHandSocketFname = CurrentWeaponID == EGsrEquipId::WP_Tnt ? tntSocketFname : cqcKnifeSocketFname;

		auto parent = ue::call<ue::UObject*>(WeaponRoot, L"GetAttachParent");

		for (auto child : ue::array_snapshot(parent, L"AttachChildren"))
		{
			if (!child) continue;
			if (ue::is_a(child, SkeletalMeshComponent_c)) {
				if (ue::call<ue::FName>(child, L"GetAttachSocketName") == leftHandSocketFname)
				{
					LeftHandWeaponRoot = child;
				}
			}
		}

	}

	//uevr seems to force uobject attachement each frame when a uobject file is present in uobjecthook folder
	//we need to reset the position to 0 each frame because of uevr reattach tentative
	if (playerManager->MeleeAttackRunning || playerManager->IsCarrying)
	{
		RemoveUobjectHookFromHand(Left);
		RemoveUobjectHookFromHand(Right);
	}

	// detach hands when melee attack
	if (playerManager->MeleeAttackRunning.Rose())
	{
		if (playerManager->SecondHandAttachedToWeapon)
		{
			DetachSecondHandFromWeapon();
		}
	}
	if (playerManager->MeleeAttackRunning.Fell())
	{
		AttachSecondHandToWeapon();
	}

	//Toggle weapon visibility in FPS view;
	if (ReceiverMesh)
	{
		bool isReceiverMeshVisible = ue::call<bool>(ReceiverMesh, L"IsVisible");
		if (ue::is_a(CurrentWeapon, GsrGun_c) && !isReceiverMeshVisible && !playerManager->IsCarrying)
		{
			ue::call(ReceiverMesh, L"SetVisibility", true, false);
			bool isAttachedSuppressor = ue::call<bool>(CurrentWeapon, L"IsAttachedSuppressor");
			auto suppressorMesh = ue::get<ue::UObject*>(ue::get<ue::UObject*>(CurrentWeapon, L"GunPartsController"), L"m_suppressorMesh");
			if (suppressorMesh)
			{
				ue::call(suppressorMesh, L"SetVisibility", isAttachedSuppressor, true);
			}

			// Set cqc knife visibility. DOESN'T WORK. Something is overwritting it each frame.
		}

		//Apply recoil if one handed, two handed is processed later in frame
		if (CurrentWeapon != NULL && !playerManager->SecondHandAttachedToWeapon)
			ApplyRecoil(delta);

		if (!settingsManager->Settings.enableWeaponsScopes)
			return;

		if (CurrentWeaponID == EGsrEquipId::WP_Dragnov || CurrentWeaponID == EGsrEquipId::WP_MosinNagant || CurrentWeaponID == EGsrEquipId::WP_Rpg)
		{
			if (!ue::call<bool>(ReceiverMesh, L"IsVisible"))
				ue::call(ReceiverMesh, L"SetVisibility", true, true);

			SwitchScopeState(playerManager->IsAiming);


			auto pawn = uevr::API::get()->get_local_pawn(0);
			auto scopeMagnification = ue::call<float>(pawn, L"GetScopeMagnification");
			float scopeFov = scopeMagnification == 3.0f ? 10.0f : 3.0f;
			scopeFov = CurrentWeaponID == EGsrEquipId::WP_Rpg ? 16.0f : scopeFov;
			ue::set(SceneCaptureComponent, L"FOVAngle", scopeFov);
			//Toggle zoom :
			if (ZoomInput > 0.0f && scopeMagnification == 3.0f)
			{
				if (CurrentWeaponID == EGsrEquipId::WP_Dragnov)
				{
					ue::call(SourceDragnovScopeWidget, L"SightZoomIn");
					ue::call(SourceDragnovScopeWidget, L"SetCamZoomToggle");
				}
				if (CurrentWeaponID == EGsrEquipId::WP_MosinNagant)
				{
					ue::call(SourceMosinScopeWidget, L"SightZoomIn");
					ue::call(SourceMosinScopeWidget, L"SetCamZoomToggle");
				}
			}
			if (ZoomInput < 0.0f && scopeMagnification == 10.0f)
			{
				if (CurrentWeaponID == EGsrEquipId::WP_Dragnov)
				{
					ue::call(SourceDragnovScopeWidget, L"SightZoomOut");
					ue::call(SourceDragnovScopeWidget, L"SetCamZoomToggle");
				}
				if (CurrentWeaponID == EGsrEquipId::WP_MosinNagant)
				{
					ue::call(SourceMosinScopeWidget, L"SightZoomOut");
					ue::call(SourceMosinScopeWidget, L"SetCamZoomToggle");
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
	if (!mainHandShouldAttachToWeapon)
	{
		AttachHandNextFrame = false;
		return;
	}
	if (!playerManager->RightHandMesh)
	{
		playerManager->FetchMotionControllers();
		return;
	}

	if (!playerManager->MainHandAttachedToWeapon && mainHandShouldAttachToWeapon)
	{
		Logger::DebugPrint("attach Main Hand to weapon");
		ue::call(playerManager->RightHandMesh, L"DetachFromParent", true, false);
		ue::call(playerManager->RightHandMesh, L"K2_AttachToComponent",
			ReceiverMesh == nullptr ? WeaponRoot : ReceiverMesh,
			ue::FName(L"None"),
			ue::EAttachmentRule::KeepRelative,
			ue::EAttachmentRule::KeepRelative,
			ue::EAttachmentRule::KeepRelative,
			false
		);

		ue::FVector rightHandLocation = {-DefaultUevrHookWeaponPosition.z, DefaultUevrHookWeaponPosition.x, DefaultUevrHookWeaponPosition.y };
		ue::FHitResult outHit;
		ue::call(playerManager->RightHandMesh, L"K2_SetRelativeLocationAndRotation", rightHandLocation, RightHandRotationOffset, false, &outHit, true);
		playerManager->MainHandAttachedToWeapon = true;

		ue::call(playerManager->RightHandMesh, L"SetVisibility", true, true);
		ue::call(playerManager->LeftHandMesh, L"SetVisibility", true, true);
		AttachHandNextFrame = false;
	}
}

bool WeaponManager::DetachHandFromWeapon()
{
	if (playerManager->RightHandMesh && playerManager->MainHandAttachedToWeapon)
	{
		Logger::DebugPrint("Detach Main Hand from weapon");
		ue::call(playerManager->RightHandMesh, L"DetachFromParent", true, false);
		ue::call(playerManager->RightHandMesh, L"K2_AttachToComponent",
			playerManager->RightController,
			ue::FName(L"None"),
			ue::EAttachmentRule::KeepWorld,
			ue::EAttachmentRule::KeepWorld,
			ue::EAttachmentRule::KeepRelative,
			true
		);
		ue::FHitResult outHit;
		ue::call(playerManager->RightHandMesh, L"K2_SetRelativeLocation", ue::FVector{ 0.0f,0.0f,0.0f }, false, &outHit, false);
		ue::call(playerManager->RightHandMesh, L"K2_SetRelativeRotation", ue::FRotator{ 0.0f,-90.0f,0.0f }, false, &outHit, false);
		playerManager->MainHandAttachedToWeapon = false;
		ue::call(playerManager->RightHandMesh, L"SetVisibility", true, true);
		ue::call(playerManager->LeftHandMesh, L"SetVisibility", true, true);
		return true;
	}
	return false;
}

void WeaponManager::AttachSecondHandToWeapon()
{
	if (!playerManager->LeftHandMesh)
	{
		playerManager->FetchMotionControllers();
		return;
	}

	if (secondHandShouldAttachToWeapon && !canBeTwoHanded)
	{
		secondHandShouldAttachToWeapon = false;
		return;
	}

	if (!playerManager->SecondHandAttachedToWeapon && secondHandShouldAttachToWeapon)
	{
		Logger::DebugPrint("attach Second Hand to weapon");
		ue::call(playerManager->LeftHandMesh, L"DetachFromParent", true, false);
		ue::call(playerManager->LeftHandMesh, L"K2_AttachToComponent",
			ReceiverMesh,
			ue::FName(L"None"),
			ue::EAttachmentRule::KeepRelative,
			ue::EAttachmentRule::KeepRelative,
			ue::EAttachmentRule::KeepRelative,
			false
		);
		ue::FHitResult outHit;
		ue::call(playerManager->LeftHandMesh, L"K2_SetRelativeLocation", LeftHandLocationOffset, false, &outHit, false);
		ue::call(playerManager->LeftHandMesh, L"K2_SetRelativeRotation", LeftHandRotationOffset, false, &outHit, false);
		ue::call(playerManager->RightHandMesh, L"SetVisibility", true, true);
		ue::call(playerManager->LeftHandMesh, L"SetVisibility", true, true);
		uevr::API::get()->dispatch_lua_event("TwoHandsHandling", "1");
		playerManager->SecondHandAttachedToWeapon = true;
		AttachSecondHandNextFrame = false;
	}
}

bool WeaponManager::DetachSecondHandFromWeapon()
{
	if (playerManager->LeftHandMesh && playerManager->SecondHandAttachedToWeapon)
	{
		Logger::DebugPrint("Detach Second Hand from weapon");
		ue::call(playerManager->LeftHandMesh, L"DetachFromParent", true, false);
		ue::call(playerManager->LeftHandMesh, L"K2_AttachToComponent",
			playerManager->LeftController,
			ue::FName(L"None"),
			ue::EAttachmentRule::KeepWorld,
			ue::EAttachmentRule::KeepWorld,
			ue::EAttachmentRule::KeepRelative,
			true
		);
		ue::FHitResult outHit;
		ue::call(playerManager->LeftHandMesh, L"K2_SetRelativeLocation", ue::FVector{ 0.0f,0.0f,0.0f }, false, &outHit, false);
		ue::call(playerManager->LeftHandMesh, L"K2_SetRelativeRotation", ue::FRotator{ 0.0f,-90.0f,0.0f }, false, &outHit, false);
		playerManager->SecondHandAttachedToWeapon = false;
		ue::call(playerManager->RightHandMesh, L"SetVisibility", true, true);
		ue::call(playerManager->LeftHandMesh, L"SetVisibility", true, true);
		uevr::API::get()->dispatch_lua_event("TwoHandsHandling", "0");
		return true;
	}
	return false;
}



void WeaponManager::FetchWeaponMeshAimData()
{
	if (CurrentWeapon == nullptr)
		return;
	auto root_component = ue::get<ue::UObject*>(CurrentWeapon, L"RootComponent");
	if (root_component != nullptr)
	{
		ue::FVector forward_params = ue::call<ue::FVector>(CurrentWeapon, L"GetActorForwardVector");
		
		// Only guns have a muzzle: for other weapons (knife, food...) this returns a zero transform
		const auto muzzleTransform = ue::try_call<ue::FTransform>(CurrentWeapon, L"GetMuzzleTransform", ue::ERelativeTransformSpace::RTS_World);

		FirePosition = { muzzleTransform.Translation.X, muzzleTransform.Translation.Y, muzzleTransform.Translation.Z };
		float scaler = 8192.0;
		const auto upVector = ue::call<ue::FVector>(CurrentWeapon, L"GetActorUpVector");
		forward_params = ue::call_static<ue::FVector>(ue::lib::KismetMath, L"RotateAngleAxis", forward_params, 90.0f, upVector);

		float aimOffsetValueHorizontal = 0.0f;
		float aimOffsetValueVertical = 0.0f;
		switch (CurrentWeaponID)
		{
		case WP_Ithaca:
			aimOffsetValueHorizontal = -150.0f;
			aimOffsetValueVertical = 0.0f;
			break;

		default:
			aimOffsetValueHorizontal = 0.0f;
			aimOffsetValueVertical = 0.0f;
			break;
		}

		// Offsets relative to the weapon, so they follow its orientation. Right = Up x Forward (Unreal: X forward, Y right, Z up)
		const ue::FVector rightVector{
			upVector.Y * forward_params.Z - upVector.Z * forward_params.Y,
			upVector.Z * forward_params.X - upVector.X * forward_params.Z,
			upVector.X * forward_params.Y - upVector.Y * forward_params.X };
		const ue::FVector aimOffset = forward_params * scaler - rightVector * aimOffsetValueHorizontal - upVector * aimOffsetValueVertical;
		AimPosition = { FirePosition.x + aimOffset.X, FirePosition.y + aimOffset.Y, FirePosition.z + aimOffset.Z };
	}
}

void WeaponManager::SetAimDirection()
{
	if (CurrentItemID == ue::EGsrItemId::IT_Scope)
		return;
	//Will be used to control the grenades throw direction
	//This controls only the aim when the camera controller is disabled. When cc enabled, it controls the camera rotation but the inputs are conflicting.
	auto gsrCameraController = GsrPlayerCameraController_c.get();

	if (gsrCameraController == nullptr)
	{
		Logger::DebugPrint("SetAimDirection : No CameraController found");
		return;
	}

	// SetCameraRotation(EndRotation, InterpTime). The SDK-era call only passed the rotation,
	// so InterpTime was read from undefined memory. 0 = apply the rotation immediately.
	ue::call(gsrCameraController, L"SetCameraRotation", cameraController->ViewRotation, 0.0f);
	

}

void WeaponManager::SwitchScopeState(bool enable)
{
	// Hiding the capture isn't guaranteed to stop it from rendering the scene every frame
	if (SceneCaptureComponent) ue::set(SceneCaptureComponent, L"bCaptureEveryFrame", enable);
	if (SceneCaptureComponent) ue::call(SceneCaptureComponent, L"SetVisibility", enable, false);
	if (SceneCaptureUStaticMeshComponent) ue::call(SceneCaptureUStaticMeshComponent, L"SetVisibility", enable, false);
	if (ScopeStaticMeshComponent) ue::call(ScopeStaticMeshComponent, L"SetVisibility", enable, false);

	const auto visibility = enable ? ue::ESlateVisibility::Visible : ue::ESlateVisibility::Hidden;
	switch (CurrentWeaponID)
	{
		case EGsrEquipId::WP_Dragnov:
			if (SourceDragnovScopeWidget) ue::call(SourceDragnovScopeWidget, L"SetVisibility", visibility);
			if (SourceMosinScopeWidget) hideWidget(SourceMosinScopeWidget);
			if (SourceRpgScopeWidget) hideWidget(SourceRpgScopeWidget);
		break;
		case EGsrEquipId::WP_MosinNagant:
			if (SourceMosinScopeWidget) ue::call(SourceMosinScopeWidget, L"SetVisibility", visibility);
			if (SourceDragnovScopeWidget) hideWidget(SourceDragnovScopeWidget);
			if (SourceRpgScopeWidget) hideWidget(SourceRpgScopeWidget);
			break;
		case EGsrEquipId::WP_Rpg :
			if (SourceRpgScopeWidget) ue::call(SourceRpgScopeWidget, L"SetVisibility", visibility);
			if (SourceDragnovScopeWidget) hideWidget(SourceDragnovScopeWidget);
			if (SourceMosinScopeWidget) hideWidget(SourceMosinScopeWidget);
			break;
	}
}

void WeaponManager::SpawnScope()
{
	if (!ReceiverMesh)
		return;
	if (CurrentWeaponID == EGsrEquipId::WP_Dragnov || CurrentWeaponID == EGsrEquipId::WP_MosinNagant || CurrentWeaponID == EGsrEquipId::WP_Rpg)
	{
		spawnScope();
		attachScopeToWeapon();
	}
}

void WeaponManager::DestroyScope()
{
	if (ScopeStaticMeshComponent) ue::call(ScopeStaticMeshComponent, L"K2_DestroyComponent", ScopeStaticMeshComponent);
	if (SceneCaptureComponent) ue::call(SceneCaptureComponent, L"K2_DestroyComponent", SceneCaptureComponent);
	if (SceneCaptureUStaticMeshComponent) ue::call(SceneCaptureUStaticMeshComponent, L"K2_DestroyComponent", SceneCaptureUStaticMeshComponent);
	if (DragnovScopeWidget) ue::call(DragnovScopeWidget, L"K2_DestroyComponent", DragnovScopeWidget);
	if (MosinScopeWidget) ue::call(MosinScopeWidget, L"K2_DestroyComponent", MosinScopeWidget);
	if (RpgScopeWidget) ue::call(RpgScopeWidget, L"K2_DestroyComponent", RpgScopeWidget);
	if (ScopeActor) ue::call(ScopeActor, L"K2_DestroyActor");
	ScopeStaticMeshComponent = nullptr;
	SceneCaptureComponent = nullptr;
	SceneCaptureUStaticMeshComponent = nullptr;
	ScopeActor = nullptr;
	ScopeRenderTarget = nullptr;
	SourceDragnovScopeWidget = nullptr;
	DragnovScopeWidget = nullptr;
	SourceMosinScopeWidget = nullptr;
	MosinScopeWidget = nullptr;
	SourceRpgScopeWidget = nullptr;
	RpgScopeWidget = nullptr;
}

void WeaponManager::EndFrame()
{
	CurrentItemID.Commit();
	CurrentWeaponID.Commit();
	if (AttachHandNextFrame)
		NextFrame = true;
}

void WeaponManager::Cleanup()
{
	DetachHandFromWeapon();
	DetachSecondHandFromWeapon();
	DestroyScope();
	if (DummyCqcKnifeMeshActor) ue::call(DummyCqcKnifeMeshActor, L"K2_DestroyActor");
	DummyCqcKnifeMeshActor = nullptr;
	DummyCqcKnifeMesh = nullptr;
	LeftHandWeaponRoot = nullptr;
	// Set up the weapon again next frame, even if the new one gets the address of the previous one
	CurrentWeapon = nullptr;
	WeaponRoot = nullptr;
	ReceiverMesh = nullptr;
	playerCameraManager = nullptr;
}

void WeaponManager::ToggleThermalVisionScanline()
{
	if (playerCameraManager == nullptr)
	{
		Logger::DebugPrint("No player camera manager found");
		return;
	}
	auto thermalPostProcessActor = ue::get<ue::UObject*>(playerCameraManager, L"ThermalPostProcessActor");
	if (thermalPostProcessActor == nullptr)
	{
		return;
	}
	Logger::DebugPrint("Toggle Thermal Vision Scanline");
	ue::call(ue::get<ue::UObject*>(thermalPostProcessActor, L"Scanline"), L"SetVisibility", !settingsManager->Settings.disableThermalVisionScanline, true);
	ToggleThermalVisionScanlineNextFrame = false;
}

void WeaponManager::ApplyRecoil(float delta)
{
	if (settingsManager->Settings.disableAllRecoil)
		return;

	glm::fvec3 positionRecoilForce = { 0.0f, 0.0f, 0.0f };
	glm::fvec3 rotationRecoilForceEuler = { 0.0f, 0.0f, 0.0f };
	float recoilPositionRecoverySpeed = 10.0f;
	float recoilRotationRecoverySpeed = 8.0f;

	bool twoHandsRecoil = playerManager->SecondHandAttachedToWeapon || !settingsManager->Settings.enableTwoHandsRealisticRecoil;

	switch (CurrentWeaponID)
	{
	case WP_Mk22:
		positionRecoilForce = { 0.0f, -0.75f, 0.005f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -1.0f };
		break;

	case WP_Gove:
		positionRecoilForce = { randomRange(-0.25f, 0.25f), -2.5f, 0.005f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -2.0f };
		recoilPositionRecoverySpeed = 14.0f;
		recoilRotationRecoverySpeed = 12.0f;
		break;

	case WP_EasyGun:
		positionRecoilForce = { 0.0f, -0.75f, 0.005f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -1.0f };
		break;

	case WP_SAArmy: //This one x is the z there for some reasons
		positionRecoilForce = { 0.0f, -5.0f, 0.005f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -10.0f };
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
		rotationRecoilForceEuler = { 0.0f, 0.0f, -1.0f };
		break;

	case WP_Akm:
		positionRecoilForce = { randomRange(-0.2f, 0.2f), -1.0f, 0.25f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -1.0f };
		break;

	case WP_M63:
		positionRecoilForce = { randomRange(-0.2f, 0.2f), -2.0f, 0.25f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -1.0f };
		recoilPositionRecoverySpeed = 14.0f;
		recoilRotationRecoverySpeed = 12.0f;
		break;

	case WP_Ithaca:
		positionRecoilForce = { 1.0f, twoHandsRecoil ? -30.0f : -20.0f, 8.0f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, twoHandsRecoil ? -6.0f : -20.0f };
		recoilPositionRecoverySpeed = 5.0f;
		recoilRotationRecoverySpeed = 4.0f;
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
		positionRecoilForce = { 0.0f, -15.0f, 1.0f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -6.0f };
		recoilPositionRecoverySpeed = 3.0f;
		recoilRotationRecoverySpeed = 5.0f;
		break;

	default:
		return;
	}

	auto motionState = uevr::API::UObjectHook::get_motion_controller_state(WeaponRoot);
	ue::FHitResult outHit;
	if (IsShooting)
	{
		if (!playerManager->SecondHandAttachedToWeapon && settingsManager->Settings.enableTwoHandsRealisticRecoil)
		{
			positionRecoilForce *= 2.0f;
			rotationRecoilForceEuler *= 2.0f;
		}

		CurrentRelativeWeaponPosition += positionRecoilForce;
		CurrentRelativeWeaponRotationEuler += rotationRecoilForceEuler;
		CurrentRelativeWeaponRotation = glm::fquat(glm::radians(CurrentRelativeWeaponRotationEuler));
		ue::call(ReceiverMesh, L"K2_SetRelativeLocation", ue::FVector{ CurrentRelativeWeaponPosition.x, CurrentRelativeWeaponPosition.y, CurrentRelativeWeaponPosition.z }, false, &outHit, false);
		ue::call(ReceiverMesh, L"K2_SetRelativeRotation", ue::FRotator{ CurrentRelativeWeaponRotationEuler.x, CurrentRelativeWeaponRotationEuler.y, CurrentRelativeWeaponRotationEuler.z }, false, &outHit, false);
		IsShooting = false;
	}
	else
	{
		if (!playerManager->SecondHandAttachedToWeapon && settingsManager->Settings.enableTwoHandsRealisticRecoil)
		{
			recoilPositionRecoverySpeed *= 0.75f;
			recoilRotationRecoverySpeed *= 0.75f;
		}
		// Smoothly return position to base
		CurrentRelativeWeaponPosition = glm::mix(CurrentRelativeWeaponPosition, DefaultRelativeWeaponPosition, delta * recoilPositionRecoverySpeed);
		CurrentRelativeWeaponRotation = glm::slerp(CurrentRelativeWeaponRotation, DefaultRelativeWeaponRotation, delta * recoilRotationRecoverySpeed);
		CurrentRelativeWeaponRotationEuler = glm::degrees(glm::eulerAngles(CurrentRelativeWeaponRotation));
		ue::call(ReceiverMesh, L"K2_SetRelativeLocation", ue::FVector{ CurrentRelativeWeaponPosition.x, CurrentRelativeWeaponPosition.y, CurrentRelativeWeaponPosition.z }, false, &outHit, false);
		ue::call(ReceiverMesh, L"K2_SetRelativeRotation", ue::FRotator{ CurrentRelativeWeaponRotationEuler.x, CurrentRelativeWeaponRotationEuler.y, CurrentRelativeWeaponRotationEuler.z }, false, &outHit, false);
	}
}

float WeaponManager::randomRange(float min, float max) {
	return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
}

void WeaponManager::spawnScope()
{
	auto world = ue::get_world();

	getRenderTarget(world);

	if (!ScopeRenderTarget)
	{
		Logger::DebugPrint("Failed to get render target, destroying Scope actor");
		if (ScopeActor)
			ue::call(ScopeActor, L"K2_DestroyActor");
		ScopeStaticMeshComponent = nullptr;
		SceneCaptureComponent = nullptr;
		SceneCaptureUStaticMeshComponent = nullptr;
		return;
	}

	if (!ScopeActor)
	{
		ScopeStaticMeshComponent = nullptr;
		SceneCaptureComponent = nullptr;
		SceneCaptureUStaticMeshComponent = nullptr;
		DragnovScopeWidget = nullptr;
		MosinScopeWidget = nullptr;
		RpgScopeWidget = nullptr;
		ScopeActor = Utilities::SpawnActor(UEVR_Vector3d{0.0f, 0.0f, 0.0f}, ue::call<ue::UObject*>(ReceiverMesh, L"GetOwner"));
	}
	if (!ScopeStaticMeshComponent)
		spawnScopeStaticMeshComponent();
	if (!SceneCaptureComponent)
		spawnSceneCaptureComponent();
	spawnScopeWidget();
}

void WeaponManager::getRenderTarget(uevr::API::UObject* world)
{
	ue::FLinearColor linearColor{};
	if (ScopeRenderTarget)
		Logger::DebugPrint(ScopeRenderTarget->get_full_name());
	if (!ScopeRenderTarget)
	{
		ScopeRenderTarget = ue::call_static<ue::UObject*>(ue::lib::KismetRendering, L"CreateRenderTarget2D",
			world, settingsManager->Settings.scopeResolution, settingsManager->Settings.scopeResolution,
			ue::ETextureRenderTargetFormat::RTF_RGBA16f, linearColor, false, false);
		if (ScopeRenderTarget)
			Logger::DebugPrint(L"Render target created : " + ScopeRenderTarget->get_full_name());
	}
}

void WeaponManager::spawnScopeStaticMeshComponent()
{
	auto scopeStaticMeshComponent = ue::call<ue::UObject*>(ScopeActor, L"AddComponentByClass", StaticMeshComponent_c.get(), false, ue::FTransform{}, false);
	if (!scopeStaticMeshComponent/* || !scopeWidgetStaticMeshComponent*/)
	{
		Logger::DebugPrint("Failed to spawn scope mesh");
		return;
	}

	auto uDiskMaterial = ue::find_object(L"Material /Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial");

	if (!uDiskMaterial)
	{
		Logger::DebugPrint("Failed to find scopeDiskMaterialUObject");
		return;
	}

	ue::set(uDiskMaterial, L"TwoSided", false);
	ue::set(uDiskMaterial, L"BlendMode", ue::EBlendMode::BLEND_Opaque);
	ue::set(uDiskMaterial, L"bDisableDepthTest", true);
	ue::set(uDiskMaterial, L"ShadingModel", ue::EMaterialShadingModel::MSM_DefaultLit);


	auto cylinderMesh = ue::find_object(L"StaticMesh /Game/Art/VFX/Game_HD_VFX/Fire/Fire/TheFury_Fire/Mesh/SM_VFX_Disk20edge.SM_VFX_Disk20edge");

	if (!cylinderMesh)
	{
		Logger::DebugPrint("Can't find cylinder mesh");
		return;
	}
	ue::call(scopeStaticMeshComponent, L"SetStaticMesh", cylinderMesh);
	ue::call(scopeStaticMeshComponent, L"SetVisibility", false, false);
	ue::call(scopeStaticMeshComponent, L"SetCollisionEnabled", ue::ECollisionEnabled::NoCollision);
	ue::call(scopeStaticMeshComponent, L"SetMobility", ue::EComponentMobility::Movable);

	auto scopeDynamicMaterial = ue::call<ue::UObject*>(scopeStaticMeshComponent, L"CreateAndSetMaterialInstanceDynamicFromMaterial", 0, uDiskMaterial);
	//Applying the render texture to the material :
	ue::call(scopeDynamicMaterial, L"SetTextureParameterValue", ue::FName(L"LinearColor"), ScopeRenderTarget);

	ScopeStaticMeshComponent = scopeStaticMeshComponent;
}

void WeaponManager::spawnSceneCaptureComponent()
{
	SceneCaptureUStaticMeshComponent = ue::call<ue::UObject*>(ScopeActor, L"AddComponentByClass", StaticMeshComponent_c.get(), false, ue::FTransform{}, false);
	SceneCaptureComponent = ue::call<ue::UObject*>(ScopeActor, L"AddComponentByClass", SceneCaptureComponent2D_c.get(), false, ue::FTransform{}, false);

	if (!SceneCaptureUStaticMeshComponent || !SceneCaptureComponent)
	{
		Logger::DebugPrint("failed to spawn scene capture mesh and component");
		return;
	}
	ue::set(SceneCaptureComponent, L"TextureTarget", ScopeRenderTarget);
	ue::set(SceneCaptureComponent, L"bCaptureOnMovement", false);
	ue::set(SceneCaptureComponent, L"bCaptureEveryFrame", false); // enabled while aiming, see SwitchScopeState
	ue::set(SceneCaptureComponent, L"bUseRayTracingIfEnabled", false);
	ue::set(SceneCaptureComponent, L"bAlwaysPersistRenderingState", true);
	const auto postProcess = ue::get_struct(SceneCaptureComponent, L"PostProcessSettings");
	postProcess.set(L"bOverride_MotionBlurAmount", true);
	postProcess.set(L"MotionBlurAmount", 0.0f); //Disable motion blur
	postProcess.set(L"bOverride_ScreenSpaceReflectionIntensity", true);
	postProcess.set(L"ScreenSpaceReflectionIntensity", 0.0f); //Disable screen space reflections
	postProcess.set(L"bOverride_AmbientOcclusionIntensity", true);
	postProcess.set(L"AmbientOcclusionIntensity", 0.0f); //Disable ambient occlusion
	postProcess.set(L"bOverride_BloomIntensity", true);
	postProcess.set(L"BloomIntensity", 0.0f);
	postProcess.set(L"bOverride_LensFlareIntensity", true);
	postProcess.set(L"LensFlareIntensity", 0.0f); //Disable lens flares
	postProcess.set(L"bOverride_VignetteIntensity", true);
	postProcess.set(L"VignetteIntensity", 0.0f);//Disable vignette
	// Lumen in the scope view is a second full Lumen update each frame
	postProcess.set(L"bOverride_DynamicGlobalIlluminationMethod", true);
	postProcess.set(L"DynamicGlobalIlluminationMethod", ue::EDynamicGlobalIlluminationMethod::None);
	postProcess.set(L"bOverride_ReflectionMethod", true);
	postProcess.set(L"ReflectionMethod", ue::EReflectionMethod::None);
	ue::call(SceneCaptureComponent, L"SetVisibility", false, false);
	ue::call(SceneCaptureComponent, L"SetMobility", ue::EComponentMobility::Movable);

	ue::call(SceneCaptureUStaticMeshComponent, L"SetVisibility", false, false);
	ue::call(SceneCaptureUStaticMeshComponent, L"SetMobility", ue::EComponentMobility::Movable);

}

void ProcessAllChildWidgets(ue::UObject* ParentWidget, const ue::FLinearColor& NewTint)
{
	if (!ParentWidget) return;

	// Process this widget if it's an image
	if (ParentWidget->get_full_name().find(L"Image") != std::wstring::npos && ue::is_a(ParentWidget, Image_c))
	{
		setImageTint(ParentWidget, NewTint);
	}

	// Recurse into children (only panels have some)
	int ChildCount = ue::try_call<int32_t>(ParentWidget, L"GetChildrenCount");

	for (int i = 0; i < ChildCount; i++)
	{
		auto Child = ue::call<ue::UObject*>(ParentWidget, L"GetChildAt", i);
		if (Child)
		{
			ProcessAllChildWidgets(Child, NewTint);
		}
	}
}

void SetAllImageTints(ue::UObject* rootWidget, const ue::FLinearColor& newTint)
{
	if (!rootWidget) return;
	ProcessAllChildWidgets(rootWidget, newTint);
}


void WeaponManager::spawnScopeWidget()
{
	//Fetch existing widget :
	ue::UObject* sourceWidget = nullptr;
	switch (CurrentWeaponID)
	{
	case EGsrEquipId::WP_Dragnov:
	{
		if (DragnovScopeWidget) return;
		const auto widgets = CSightDragnovWidget_c.get() ? CSightDragnovWidget_c.get()->get_objects_matching() : std::vector<ue::UObject*>{};
		sourceWidget = widgets.size() > 1 ? widgets[1] : nullptr; //check
		break;
	}
	case EGsrEquipId::WP_MosinNagant:
		if (MosinScopeWidget) return;
		sourceWidget = firstObjectOf(CSightMosinWidget_c); //check if false works here and below
		break;

	case EGsrEquipId::WP_Rpg:
		if (RpgScopeWidget) return;
		sourceWidget = firstObjectOf(CSightRpg7Widget_c);
		break;
	}



	const ue::FVector2D drawSize = {2100.0f, 2100.0f}; //The original size of the widget found ingame

	if (!sourceWidget)
	{
		Logger::DebugPrint("Failed to find the existing widget");
		return;
	}

	auto widgetComponent = ue::call<ue::UObject*>(ScopeActor, L"AddComponentByClass", WidgetComponent_c.get(), false, ue::FTransform{}, false);

	if (widgetComponent == nullptr)
	{
		Logger::DebugPrint("Failed to create actor for widget creation");
		return;
	}

	ue::call(sourceWidget, L"RemoveFromViewport");
	ue::call(widgetComponent, L"SetWidget", sourceWidget);
	ue::call(widgetComponent, L"SetTwoSided", false);
	ue::call(widgetComponent, L"SetDrawSize", drawSize);
	ue::set(widgetComponent, L"BlendMode", ue::EWidgetBlendMode::Transparent);

	//Set arrows to white for better visibility
	const ue::FLinearColor color = { 1.0f, 1.0f, 1.0f, 1.0f };
	ue::UObject* arrowGroup = ue::get<ue::UObject*>(sourceWidget, L"ArrowGroup");

	// Every sight widget has these three boxes
	hideWidget(ue::get<ue::UObject*>(sourceWidget, L"BoxBg"));
	hideWidget(ue::get<ue::UObject*>(sourceWidget, L"RightBox"));
	hideWidget(ue::get<ue::UObject*>(sourceWidget, L"LeftBox"));

	switch (CurrentWeaponID)
	{
	case EGsrEquipId::WP_Dragnov:
	{
		SourceDragnovScopeWidget = sourceWidget;
		DragnovScopeWidget = widgetComponent;
		break;
	}
	case EGsrEquipId::WP_MosinNagant:
	{
		const ue::FLinearColor mosinColor = { 1.0f, 1.0f, 1.0f, 0.0f };
		auto root = ue::get<ue::UObject*>(ue::get<ue::UObject*>(sourceWidget, L"WidgetTree"), L"RootWidget");
		auto ImageWidget = ue::call<ue::UObject*>(root, L"GetChildAt", 0);
		if (ImageWidget)
		{
			Logger::DebugPrint(ImageWidget->get_full_name());
			setImageTint(ImageWidget, mosinColor);
		}
		SourceMosinScopeWidget = sourceWidget;
		MosinScopeWidget = widgetComponent;
		break;
	}
	case EGsrEquipId::WP_Rpg:
	{
		SourceRpgScopeWidget = sourceWidget;
		RpgScopeWidget = widgetComponent;
		break;
	}
	}

	SetAllImageTints(arrowGroup, color);


}

void WeaponManager::attachScopeToWeapon()
{
	if (!ReceiverMesh){	Logger::DebugPrint("attachScopeToWeapon() : ReceiverMesh not found"); return; }
	if (!SceneCaptureComponent){	Logger::DebugPrint("attachScopeToWeapon() : SceneCaptureComponent not found"); return; }
	if (!SceneCaptureUStaticMeshComponent){	Logger::DebugPrint("attachScopeToWeapon() : SceneCaptureComponentMesh not found"); return; }
	if (!ScopeStaticMeshComponent){	Logger::DebugPrint("attachScopeToWeapon() : ScopeCylinderComponent not found"); return; }

	Logger::DebugPrint("Attaching scope to weapon");
	ue::call(SceneCaptureUStaticMeshComponent, L"K2_AttachToComponent",
		ReceiverMesh,
		ue::FName(L"CNP_MUZZLE_FLASH"),
		ue::EAttachmentRule::SnapToTarget,
		ue::EAttachmentRule::SnapToTarget,
		ue::EAttachmentRule::KeepRelative,
		true
	);

	ue::call(SceneCaptureComponent, L"K2_AttachToComponent",
		SceneCaptureUStaticMeshComponent,
		ue::FName(L"None"),
		ue::EAttachmentRule::SnapToTarget,
		ue::EAttachmentRule::SnapToTarget,
		ue::EAttachmentRule::KeepRelative,
		true
	);

	ue::call(ScopeStaticMeshComponent, L"K2_AttachToComponent",
		ReceiverMesh,
		ue::FName(L"CNP_REAR_SIGHT"),
		ue::EAttachmentRule::SnapToTarget,
		ue::EAttachmentRule::SnapToTarget,
		ue::EAttachmentRule::KeepRelative,
		true
	);

	ue::FHitResult hitResult;
	ue::call(SceneCaptureComponent, L"SetVisibility", true, true);
	ue::call(SceneCaptureComponent, L"SetMobility", ue::EComponentMobility::Movable);
	ue::call(SceneCaptureComponent, L"Activate", true);
	ue::call(SceneCaptureComponent, L"K2_SetRelativeRotation", ue::FRotator{0.0f, 90.0f, 0.0f}, false, &hitResult, true);

	// Scope widget component, its socket offsets and scales per weapon
	bool isScopedWeapon = true;
	ue::UObject* scopeWidget = nullptr;
	ue::FVector captureMeshLocation{};
	ue::FVector scopeMeshLocation{};
	double scopeMeshScale = 1.0;
	ue::FVector widgetLocation{};
	double widgetScale = 1.0;
	switch (CurrentWeaponID)
	{
	case EGsrEquipId::WP_Dragnov:
		scopeWidget = DragnovScopeWidget;
		captureMeshLocation = { 0.0f, 0.0f, 0.0f };
		scopeMeshLocation = { 0.0f, -30.9, 0.0f };
		scopeMeshScale = 0.15f;
		widgetLocation = { 0.0f, 0.0f, 0.05f };
		widgetScale = 0.025f;
		break;
	case EGsrEquipId::WP_MosinNagant:
		scopeWidget = MosinScopeWidget;
		captureMeshLocation = { 0.0f, 0.0f, 0.0f };
		scopeMeshLocation = { 0.0f, -18.5f, 0.0f };
		scopeMeshScale = 0.12f;
		widgetLocation = { 0.0f, 0.0f, 0.05f };
		widgetScale = 0.023f;
		break;
	case EGsrEquipId::WP_Rpg:
		scopeWidget = RpgScopeWidget;
		captureMeshLocation = { 0.0f, 26.0f, 0.0f };
		scopeMeshLocation = { 0.0f, 1.33f, 0.0f };
		scopeMeshScale = 0.185f;
		widgetLocation = { 0.0f, 0.0f, 0.1f };
		widgetScale = 0.012f;
		break;
	default:
		isScopedWeapon = false;
		break;
	}

	if (isScopedWeapon)
	{
		ue::call(scopeWidget, L"K2_AttachToComponent",
			ScopeStaticMeshComponent,
			ue::FName(L"None"),
			ue::EAttachmentRule::SnapToTarget,
			ue::EAttachmentRule::SnapToTarget,
			ue::EAttachmentRule::KeepRelative,
			true
		);
		ue::call(SceneCaptureUStaticMeshComponent, L"K2_SetRelativeLocation", captureMeshLocation, false, &hitResult, true);
		ue::call(ScopeStaticMeshComponent, L"K2_SetRelativeLocation", scopeMeshLocation, false, &hitResult, true);
		ue::call(ScopeStaticMeshComponent, L"K2_SetRelativeRotation", ue::FRotator{ 180.0f, 0.0f, -90.0f }, false, &hitResult, true);
		ue::call(ScopeStaticMeshComponent, L"SetRelativeScale3D", ue::FVector{ scopeMeshScale, scopeMeshScale, scopeMeshScale });
		ue::call(scopeWidget, L"K2_SetRelativeLocation", widgetLocation, false, &hitResult, true);
		ue::call(scopeWidget, L"K2_SetRelativeRotation", ue::FRotator{ 90.0f, 0.0f, -90.0f }, false, &hitResult, true);
		ue::call(scopeWidget, L"SetRelativeScale3D", ue::FVector{ widgetScale, widgetScale, widgetScale });
	}

	ue::call(ScopeStaticMeshComponent, L"SetVisibility", false, false);
}



