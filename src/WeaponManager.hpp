#pragma once
#include "Utilities.hpp"
#include "uevr/API.hpp"
#include "SettingsManager.hpp"
#include "Logger.hpp"
#include "glm/glm.hpp"
#include <glm/gtc/type_ptr.hpp>
#include "magic_enum/magic_enum.hpp"

#include "ue/Reflection.hpp"

#include "GameState.hpp"
#include "CameraController.hpp"
#include "PlayerManager.hpp"
#include "Inputs.hpp"
#include "Tracked.hpp"
#include "WeaponTraits.hpp"
#include "ScopeSystem.hpp"
#include "WeaponRecoil.hpp"
#include "FoodHandling.hpp"
#include "VRHands.hpp"

class WeaponManager {
private:
	const GameState* const gameState;
	SettingsManager* const settingsManager;
	PlayerManager* const playerManager;
	VRHands* const hands;
	CameraController* const cameraController;

	ue::WeakObject playerCameraManager; // BP_PlayerCameraManager_C


	ue::FName tntSocketFname;
	ue::FName cqcKnifeSocketFname;

	const WeaponTraits& currentWeaponTraits() const { return GetWeaponTraits(CurrentWeaponID); }

	

public:
	WeaponManager(const GameState* gs, SettingsManager* sm, PlayerManager* pm, VRHands* vh, CameraController* cc) : gameState(gs), settingsManager(sm), playerManager(pm), hands(vh), cameraController(cc), Scope(gs, sm), Recoil(sm) {};

	void InitWeaponManager();

	ue::WeakObject EquipController;
	ue::WeakObject CurrentWeapon;
	ue::WeakObject WeaponRoot;
	ue::WeakObject LeftHandWeaponRoot;
	ue::WeakObject ReceiverMesh; // USkeletalMeshComponent

	ue::WeakObject ItemController; // UGsrItemController
	ue::WeakObject CurrentItem; // UObject
	Tracked<ue::EGsrItemId> CurrentItemID = ue::EGsrItemId::IT_None;

	float ZoomInput = 0.0f;
	ScopeSystem Scope;
	WeaponRecoil Recoil;


	glm::fquat DefaultUevrHookWeaponRotation{};
	glm::fvec3 DefaultUevrHookWeaponPosition{};
	ue::FRotator RightHandRotationOffset{};
	ue::FVector LeftHandLocationOffset{};
	ue::FRotator LeftHandRotationOffset{};

	glm::dvec3 FirePosition{};
	glm::dvec3 AimPosition{};

	ue::WeakObject DummyCqcKnifeMeshActor; // AActor
	ue::WeakObject DummyCqcKnifeMesh; // USkeletalMeshComponent

	bool secondHandShouldAttachToWeapon = false;

	FoodHandling Food;

	Tracked<ue::EGsrEquipId> CurrentWeaponID = ue::EGsrEquipId::WP_None;
	
	enum Hand
	{
		Left = 0,
		Right = 1
	};

	void FetchWeaponOffsets(ue::EGsrEquipId currentWeapon,
		glm::fvec3& outPos,
		glm::fquat& outRot,
		ue::FRotator& outRightHandRotationOffset,
		ue::FVector& outLeftHandLocationOffset,
		ue::FRotator& outLeftHandRotationOffset);

	void FetchEquipController();
	void SpawnDummyCqcKnifeMesh();
	void ToggleDummyCQCKnifeMeshVisibility();
	void AttachWeaponRootToHand();
	void RemoveUobjectHookFromHand(Hand handSide);
	void ProcessWeapon(float delta);
	void FetchWeaponMeshAimData();

	void SetAimDirection();

	// Sets up the scope of the current weapon, if it has one
	void SpawnScope();

	void Cleanup();
	// Commits this frame's tracked values: call last in the frame
	void EndFrame();

	void ToggleThermalVisionScanline();
	bool ToggleThermalVisionScanlineNextFrame = false;

	void ProcessItem();
	void SendHandStateToLua();
	void AttachMainHandToWeapon();
	void AttachSecondHandToWeapon();
	bool AttachHandNextFrame = false;
	bool NextFrame = false;
	bool AttachSecondHandNextFrame = false;

	void ProcessTwoHandsGrabWeapon();
	void ProcessTwoHandsGrabWeaponLocationAndRotation();
	void ApplyRecoil(float delta);
};