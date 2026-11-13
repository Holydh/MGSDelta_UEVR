#include "WeaponManager.hpp"

using ue::EGsrEquipId;
using enum ue::EGsrEquipId;

namespace {
constinit ue::ClassRef SkeletalMeshComponent_c{ L"Class /Script/Engine.SkeletalMeshComponent" };
constinit ue::ClassRef GsrGun_c{ L"Class /Script/Gsr.GsrGun" };
constinit ue::SingletonRef GsrPlayerCameraController_c{ L"Class /Script/Gsr.GsrPlayerCameraController" };

}

void WeaponManager::InitWeaponManager()
{
	tntSocketFname = ue::FName(L"TntTriggerSocket_LeftHand");
	cqcKnifeSocketFname = ue::FName(L"BaseBody_LeftHand_MTPA");
	DummyCqcKnifeMeshActor = nullptr;
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

void WeaponManager::FetchWeaponOffsets(ue::EGsrEquipId currentWeapon, glm::fvec3& outPos, glm::fquat& outRot, ue::FRotator& outRightHandRotationOffset, ue::FVector& outLeftHandLocationOffset, ue::FRotator& outLeftHandRotationOffset)
{
	// Weapons without a config get the default offsets, not the previous weapon's ones
	const WeaponConfig defaults{};
	outPos = defaults.location_offset;
	outRot = defaults.rotation_offset;
	outRightHandRotationOffset = defaults.right_hand_rotation_offset;
	outLeftHandLocationOffset = defaults.left_hand_location_offset;
	outLeftHandRotationOffset = defaults.left_hand_rotation_offset;

	// Weapon config files are named after the weapon ID
	static_assert(magic_enum::enum_name(WP_MAX_PLAYER_WEAPONS) == "WP_MAX_PLAYER_WEAPONS");
	const std::string key{ magic_enum::enum_name(currentWeapon) };
    auto it = settingsManager->g_weaponConfigs.find(key);
	Logger::DebugPrint("Fetching weapon config for key: %s", key.c_str());
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
	auto pawn = gameState->Pawn;
	EquipController = ue::call<ue::UObject*>(pawn, L"GetEquipControllerBP");
	if (EquipController == NULL)
	{
		Logger::DebugPrint("No equipController found");
	}
}

void WeaponManager::SpawnDummyCqcKnifeMesh()
{
	if (!LeftHandWeaponRoot || DummyCqcKnifeMeshActor)
		return;

	if (DummyCqcKnifeMeshActor == nullptr && hands->LeftHandMesh)
	{
		constexpr ue::FVector cqcKnifeRelativeLocation = { 1.248f, -2.072f, -4.754f };
		constexpr ue::FRotator cqcKnifeRelativeRotation = { 7.228f, -169.534f, 58.205f};

		uevr::API::UObject* meshComponent = nullptr;
		const auto leftWeaponRootTransform = ue::call<ue::FTransform>(LeftHandWeaponRoot, L"GetRelativeTransform");
		DummyCqcKnifeMeshActor = Utilities::SpawnMesh(leftWeaponRootTransform, L"SkeletalMesh /Game/Gsr/Weapon/cqck/Mesh/SM_Cqck.SM_Cqck", meshComponent); //L"SkeletalMesh /Game/Gsr/Weapon/cqck/Mesh/SM_Cqck.SM_Cqck"
		if (!DummyCqcKnifeMeshActor)
			return;
		ue::call(DummyCqcKnifeMeshActor, L"K2_AttachRootComponentTo", hands->LeftHandMesh, ue::FName(L"None"), ue::EAttachLocation::SnapToTarget, false);
		ue::call(DummyCqcKnifeMeshActor, L"K2_SetActorRelativeLocation", cqcKnifeRelativeLocation, false, nullptr, true);
		ue::call(DummyCqcKnifeMeshActor, L"K2_SetActorRelativeRotation", cqcKnifeRelativeRotation, false, nullptr, true);
		DummyCqcKnifeMesh = meshComponent;
		ue::call(DummyCqcKnifeMesh, L"SetOnlyOwnerSee", true);
	}
}

void WeaponManager::UpdateDummyCqcKnife()
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

	if (!gameState->IsPlayerControlled)
	{
		destroyDummyKnife();
		return;
	}

	// While the left hand holds the weapon, the dummy knife (attached to the VR left hand)
	// replaces the game's left hand CQC knife.
	const bool showDummyKnife = currentWeaponTraits().holdCQCKnifeInLeftHand && hands->SecondHandAttachedToWeapon() && hands->LeftHandMesh;

	if (showDummyKnife && LeftHandWeaponRoot && !ue::is_valid(DummyCqcKnifeMesh))
	{
		destroyDummyKnife();
		SpawnDummyCqcKnifeMesh();
	}

	// Enforced every frame, not only on changes: the game recreates its left hand knife (e.g. when
	// leaving first person view), and the new one used to stay visible next to the dummy (two knives).
	if (ue::is_valid(DummyCqcKnifeMesh))
		setHidden(DummyCqcKnifeMesh, !showDummyKnife);
	if (currentWeaponTraits().holdCQCKnifeInLeftHand && LeftHandWeaponRoot)
		setHidden(LeftHandWeaponRoot, showDummyKnife);
}

void WeaponManager::ProcessTwoHandsGrabWeapon()
{
	if (!currentWeaponTraits().canBeTwoHanded)
		return;

	if (Inputs::SecondHandGripTriggeredBeforeDelay && !secondHandShouldAttachToWeapon)
	{
		secondHandShouldAttachToWeapon = true;
		AttachSecondHandNextFrame = true;
			
	}
	else if (Inputs::SecondHandGripTriggeredBeforeDelay && secondHandShouldAttachToWeapon)
	{
		secondHandShouldAttachToWeapon = false;
		hands->DetachSecondHand();
	}
}

void WeaponManager::ProcessTwoHandsGrabWeaponLocationAndRotation()
{
	// Pivot disabled: the weapon stays held by the right hand only (UEVR attachment)
	if (currentWeaponTraits().twoHandedPivot && settingsManager->Settings.enableTwoHandsPivot && WeaponRoot) {
		auto rightCtrlPos = ue::call<ue::FVector>(hands->RightController, L"K2_GetComponentLocation");
		auto leftCtrlPos = ue::call<ue::FVector>(hands->LeftController, L"K2_GetComponentLocation");

		auto weaponUpVector = ue::call<ue::FVector>(CurrentWeapon, L"GetActorUpVector");

		ue::FVector adjustedLeftPos = leftCtrlPos + (weaponUpVector * currentWeaponTraits().leftHandWeaponGrabOffset);

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
	if (items->IsScopeEquipped())
	{
		uevr::API::UObjectHook::remove_motion_controller_state(LeftHandWeaponRoot);
		uevr::API::UObjectHook::remove_motion_controller_state(WeaponRoot);
	}

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

	//Get weapon ID
	if (EquipController == NULL)
	{
		CurrentWeaponID = WP_None;
		Logger::DebugPrint("No equipController found");
		return;
	}

	auto weaponId = ue::call<EGsrEquipId>(EquipController, L"GetCurrentWeaponId");

	// Food and other items share one ID
	if (weaponId >= EGsrEquipId::WP_MAX_PLAYER_WEAPONS)
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
		if (GetWeaponTraits(CurrentWeaponID.Previous()).hasScope)
			ue::call(ReceiverMesh, L"SetVisibility", false, false);

		ReceiverMesh = nullptr;
		WeaponRoot = nullptr;
		LeftHandWeaponRoot = nullptr;
		
		if (hands->MainHandAttachedToWeapon())
			hands->DetachMainHand();
		if (hands->SecondHandAttachedToWeapon())
		{
			hands->DetachSecondHand();
		}


		SendHandStateToLua();
	

		Scope.SwitchScopeState(CurrentWeaponID, false);

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

		Recoil.Reset();

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
			Scope.SwitchScopeState(CurrentWeaponID, false);

			if (currentWeaponTraits().hasScope)
			{
				if (!ReceiverMesh)
				{
					Logger::DebugPrint("no Receiver mesh found");
					return;
				}
				Scope.SpawnScope(CurrentWeaponID, WeaponRoot, ReceiverMesh);
			}
		}

		if (CurrentWeaponID == EGsrEquipId::WP_CigarPistol)
		{
			ue::call(WeaponRoot, L"SetRelativeScale3D", ue::FVector{ 1.5f, 1.5f, 1.5f });
			return;
		}

		if (currentWeaponTraits().mainHandShouldAttachToWeapon)
			AttachHandNextFrame = true;
	}

	// The weapon is detached from the hand during melee attacks, while carrying, with the scope item, and
	// while the player has no control (menus, radio, cutscenes): put it back once none of these applies.
	if (WeaponRoot && !playerManager->MeleeAttackRunning && !playerManager->IsCarrying && !items->IsScopeEquipped() &&
		uevr::API::UObjectHook::get_motion_controller_state(WeaponRoot) == nullptr)
	{
		AttachWeaponRootToHand();
	}

	if (CurrentWeaponID == EGsrEquipId::WP_MAX_PLAYER_WEAPONS)
	{
		Food.Update(WeaponRoot, hands->LeftHandMesh, cameraController->YawProcessed, delta);
		return;
	}
		

	// The game recreates its left hand object (e.g. when leaving first person view): drop the destroyed one
	if (LeftHandWeaponRoot && !ue::is_valid(LeftHandWeaponRoot))
		LeftHandWeaponRoot = nullptr;

	if (!LeftHandWeaponRoot && currentWeaponTraits().hasObjectInLeftHand)
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

	//uevr seems to force uobject attachment each frame when a uobject file is present in uobjecthook folder
	//we need to reset the position to 0 each frame because of uevr reattach tentative
	if (playerManager->MeleeAttackRunning || playerManager->IsCarrying)
	{
		RemoveUobjectHookFromHand(Left);
		RemoveUobjectHookFromHand(Right);
	}

	// detach hands when melee attack
	if (playerManager->MeleeAttackRunning.Rose())
	{
		if (hands->SecondHandAttachedToWeapon())
		{
			hands->DetachSecondHand();
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
		if (CurrentWeapon != NULL && !hands->SecondHandAttachedToWeapon())
			ApplyRecoil(delta);

		if (!settingsManager->Settings.enableWeaponsScopes)
			return;

		if (currentWeaponTraits().hasScope)
		{
			if (!ue::call<bool>(ReceiverMesh, L"IsVisible"))
				ue::call(ReceiverMesh, L"SetVisibility", true, true);

			Scope.Update(CurrentWeaponID, ZoomInput);
		}
	}

	//left hand object gets deleted when in fps view, need to trigger the refetch every frames
	if (gameState->IsFirstPersonView)
		LeftHandWeaponRoot = nullptr;
}

void WeaponManager::AttachMainHandToWeapon()
{
	if (!currentWeaponTraits().mainHandShouldAttachToWeapon)
	{
		AttachHandNextFrame = false;
		return;
	}
	if (!hands->RightHandMesh)
	{
		hands->FetchMotionControllers();
		return;
	}

	if (!hands->MainHandAttachedToWeapon() && currentWeaponTraits().mainHandShouldAttachToWeapon)
	{
		const ue::FVector rightHandLocation = {-DefaultUevrHookWeaponPosition.z, DefaultUevrHookWeaponPosition.x, DefaultUevrHookWeaponPosition.y };
		hands->AttachMainHand(ReceiverMesh == nullptr ? WeaponRoot : ReceiverMesh, rightHandLocation, RightHandRotationOffset);
		AttachHandNextFrame = false;
	}
}

void WeaponManager::AttachSecondHandToWeapon()
{
	if (!hands->LeftHandMesh)
	{
		hands->FetchMotionControllers();
		return;
	}

	if (secondHandShouldAttachToWeapon && !currentWeaponTraits().canBeTwoHanded)
	{
		secondHandShouldAttachToWeapon = false;
		return;
	}

	if (!hands->SecondHandAttachedToWeapon() && secondHandShouldAttachToWeapon)
	{
		hands->AttachSecondHand(ReceiverMesh, LeftHandLocationOffset, LeftHandRotationOffset);
		AttachSecondHandNextFrame = false;
	}
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
	if (items->IsScopeEquipped())
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


void WeaponManager::SpawnScope()
{
	if (!ReceiverMesh)
		return;
	if (currentWeaponTraits().hasScope)
		Scope.SpawnScope(CurrentWeaponID, WeaponRoot, ReceiverMesh);
}


void WeaponManager::EndFrame()
{
	CurrentWeaponID.Commit();
	if (AttachHandNextFrame)
		NextFrame = true;
}

void WeaponManager::Cleanup()
{
	hands->DetachMainHand();
	hands->DetachSecondHand();
	Scope.DestroyScope();
	if (DummyCqcKnifeMeshActor) ue::call(DummyCqcKnifeMeshActor, L"K2_DestroyActor");
	DummyCqcKnifeMeshActor = nullptr;
	DummyCqcKnifeMesh = nullptr;
	LeftHandWeaponRoot = nullptr;
	// Set up the weapon again next frame, even if the new one gets the address of the previous one
	CurrentWeapon = nullptr;
	WeaponRoot = nullptr;
	ReceiverMesh = nullptr;
}

void WeaponManager::ApplyRecoil(float delta)
{
	Recoil.Apply(CurrentWeaponID, ReceiverMesh, hands->SecondHandAttachedToWeapon(), delta);
}












