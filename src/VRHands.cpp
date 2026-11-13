#include "VRHands.hpp"
#include "Logger.hpp"

void VRHands::FetchMotionControllers()
{
	static ue::ClassRef motionControllerComponent_c{ L"Class /Script/HeadMountedDisplay.MotionControllerComponent" };
	static const ue::FName leftSource{ L"Left" };
	static const ue::FName rightSource{ L"Right" };
	if (!motionControllerComponent_c.get())
		return;

	LeftController = nullptr;
	LeftHandMesh = nullptr;
	RightController = nullptr;
	RightHandMesh = nullptr;
	for (auto motionControllerComponent : motionControllerComponent_c.get()->get_objects_matching(false))
	{
		// Skip the previous hands' controllers: destroyed by the Lua script, but not garbage collected yet
		if (!ue::is_valid(motionControllerComponent))
			continue;
		const auto motionSource = ue::get<ue::FName>(motionControllerComponent, L"MotionSource");
		if (motionSource != leftSource && motionSource != rightSource)
			continue;

		auto handMesh = ue::call<ue::UObject*>(motionControllerComponent, L"GetChildComponent", 0);
		if (motionSource == leftSource)
		{
			LeftController = motionControllerComponent;
			LeftHandMesh = handMesh;
			Logger::DebugPrint(L"Found Left Motion Controller: " + motionControllerComponent->get_full_name());
			if (handMesh)
				Logger::DebugPrint(L"Found Left Hand Mesh: " + handMesh->get_full_name());
		}
		else
		{
			RightController = motionControllerComponent;
			RightHandMesh = handMesh;
			Logger::DebugPrint(L"Found Right Motion Controller: " + motionControllerComponent->get_full_name());
			if (handMesh)
				Logger::DebugPrint(L"Found Right Hand Mesh: " + handMesh->get_full_name());
		}
	}
}

void VRHands::Cleanup()
{
	// Cleared so the hands get attached again once new ones are created
	LeftController = nullptr;
	LeftHandMesh = nullptr;
	RightController = nullptr;
	RightHandMesh = nullptr;
	mainHandAttachedToWeapon = false;
	setSecondHandAttachedToWeapon(false);
}

void VRHands::setSecondHandAttachedToWeapon(bool attached)
{
	if (secondHandAttachedToWeapon == attached)
		return;
	secondHandAttachedToWeapon = attached;
	uevr::API::get()->dispatch_lua_event("TwoHandsHandling", attached ? "1" : "0");
}

void VRHands::AttachMainHand(ue::UObject* parent, const ue::FVector& location, const ue::FRotator& rotation)
{
	Logger::DebugPrint("attach Main Hand to weapon");
	ue::call(RightHandMesh, L"DetachFromParent", true, false);
	ue::call(RightHandMesh, L"K2_AttachToComponent",
		parent,
		ue::FName(L"None"),
		ue::EAttachmentRule::KeepRelative,
		ue::EAttachmentRule::KeepRelative,
		ue::EAttachmentRule::KeepRelative,
		false
	);

	ue::FHitResult outHit;
	ue::call(RightHandMesh, L"K2_SetRelativeLocationAndRotation", location, rotation, false, &outHit, true);
	mainHandAttachedToWeapon = true;

	ue::call(RightHandMesh, L"SetVisibility", true, true);
	ue::call(LeftHandMesh, L"SetVisibility", true, true);
}

bool VRHands::DetachMainHand()
{
	if (RightHandMesh && mainHandAttachedToWeapon)
	{
		Logger::DebugPrint("Detach Main Hand from weapon");
		ue::call(RightHandMesh, L"DetachFromParent", true, false);
		ue::call(RightHandMesh, L"K2_AttachToComponent",
			RightController,
			ue::FName(L"None"),
			ue::EAttachmentRule::KeepWorld,
			ue::EAttachmentRule::KeepWorld,
			ue::EAttachmentRule::KeepRelative,
			true
		);
		ue::FHitResult outHit;
		ue::call(RightHandMesh, L"K2_SetRelativeLocation", ue::FVector{ 0.0f,0.0f,0.0f }, false, &outHit, false);
		ue::call(RightHandMesh, L"K2_SetRelativeRotation", ue::FRotator{ 0.0f,-90.0f,0.0f }, false, &outHit, false);
		mainHandAttachedToWeapon = false;
		ue::call(RightHandMesh, L"SetVisibility", true, true);
		ue::call(LeftHandMesh, L"SetVisibility", true, true);
		return true;
	}
	return false;
}

void VRHands::AttachSecondHand(ue::UObject* parent, const ue::FVector& location, const ue::FRotator& rotation)
{
	Logger::DebugPrint("attach Second Hand to weapon");
	ue::call(LeftHandMesh, L"DetachFromParent", true, false);
	ue::call(LeftHandMesh, L"K2_AttachToComponent",
		parent,
		ue::FName(L"None"),
		ue::EAttachmentRule::KeepRelative,
		ue::EAttachmentRule::KeepRelative,
		ue::EAttachmentRule::KeepRelative,
		false
	);
	ue::FHitResult outHit;
	ue::call(LeftHandMesh, L"K2_SetRelativeLocation", location, false, &outHit, false);
	ue::call(LeftHandMesh, L"K2_SetRelativeRotation", rotation, false, &outHit, false);
	ue::call(RightHandMesh, L"SetVisibility", true, true);
	ue::call(LeftHandMesh, L"SetVisibility", true, true);
	setSecondHandAttachedToWeapon(true);
}

bool VRHands::DetachSecondHand()
{
	if (LeftHandMesh && secondHandAttachedToWeapon)
	{
		Logger::DebugPrint("Detach Second Hand from weapon");
		ue::call(LeftHandMesh, L"DetachFromParent", true, false);
		ue::call(LeftHandMesh, L"K2_AttachToComponent",
			LeftController,
			ue::FName(L"None"),
			ue::EAttachmentRule::KeepWorld,
			ue::EAttachmentRule::KeepWorld,
			ue::EAttachmentRule::KeepRelative,
			true
		);
		ue::FHitResult outHit;
		ue::call(LeftHandMesh, L"K2_SetRelativeLocation", ue::FVector{ 0.0f,0.0f,0.0f }, false, &outHit, false);
		ue::call(LeftHandMesh, L"K2_SetRelativeRotation", ue::FRotator{ 0.0f,-90.0f,0.0f }, false, &outHit, false);
		ue::call(RightHandMesh, L"SetVisibility", true, true);
		ue::call(LeftHandMesh, L"SetVisibility", true, true);
		setSecondHandAttachedToWeapon(false);
		return true;
	}
	return false;
}

void VRHands::ApplyPawnHandMaterial()
{
	// Also called from the Lua HandsCreated event, outside the engine tick: the pawn is looked up here
	auto pawn = uevr::API::get()->get_local_pawn(0);
	auto pawnHandMesh = ue::get<ue::UObject*>(pawn, L"SkeletalMesh_Hand");
	if (!pawnHandMesh || !LeftHandMesh || !RightHandMesh)
		return;
	auto handMaterial = ue::call<ue::TArray<ue::UObject*>>(pawnHandMesh, L"GetMaterials");
	auto previousMaterialCount = ue::call<int32_t>(LeftHandMesh, L"GetNumMaterials");
	// Cleanup additional override material slots (left over from a hand material with more slots)
	for (int i = handMaterial.count; i < previousMaterialCount; i++)
	{
		ue::call(LeftHandMesh, L"SetMaterial", i, nullptr);
		ue::call(RightHandMesh, L"SetMaterial", i, nullptr);
	}

	for (int i = 0; i < handMaterial.count; i++)
	{
		ue::call(LeftHandMesh, L"SetMaterial", i, handMaterial.data[i]);
		ue::call(RightHandMesh, L"SetMaterial", i, handMaterial.data[i]);
	}
	auto handOverlayMaterial = ue::call<ue::UObject*>(pawnHandMesh, L"GetOverlayMaterial");
	ue::call(RightHandMesh, L"SetOverlayMaterial", handOverlayMaterial);
	ue::call(LeftHandMesh, L"SetOverlayMaterial", handOverlayMaterial);
}
