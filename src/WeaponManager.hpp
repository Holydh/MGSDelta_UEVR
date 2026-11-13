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

class WeaponManager {
private:
	const GameState* const gameState;
	SettingsManager* const settingsManager;
	PlayerManager* const playerManager;
	CameraController* const cameraController;

	ue::WeakObject playerCameraManager; // BP_PlayerCameraManager_C

	float randomRange(float min, float max);
	void spawnScope();
	void getRenderTarget(uevr::API::UObject* world);
	void spawnScopeStaticMeshComponent();
	void spawnSceneCaptureComponent();
	void spawnScopeWidget();
	void attachScopeToWeapon();

	ue::FName tntSocketFname;
	ue::FName cqcKnifeSocketFname;

	const WeaponTraits& currentWeaponTraits() const { return GetWeaponTraits(CurrentWeaponID); }

	

public:
	WeaponManager(const GameState* gs, SettingsManager* sm, PlayerManager* pm, CameraController* cc) : gameState(gs), settingsManager(sm), playerManager(pm), cameraController(cc) {};

	void InitWeaponManager();

	void ApplyHandMaterialToVRHands();

	bool IsShooting = false;
	ue::WeakObject EquipController;
	ue::WeakObject CurrentWeapon;
	ue::WeakObject WeaponRoot;
	ue::WeakObject LeftHandWeaponRoot;
	ue::WeakObject ReceiverMesh; // USkeletalMeshComponent

	ue::WeakObject ItemController; // UGsrItemController
	ue::WeakObject CurrentItem; // UObject
	Tracked<ue::EGsrItemId> CurrentItemID = ue::EGsrItemId::IT_None;

	float ZoomInput = 0.0f;
	ue::WeakObject ScopeActor; // AActor
	ue::WeakObject SourceDragnovScopeWidget; // UUserWidget
	ue::WeakObject DragnovScopeWidget; // UWidgetComponent
	ue::WeakObject SourceMosinScopeWidget; // UUserWidget
	ue::WeakObject MosinScopeWidget; // UWidgetComponent
	ue::WeakObject SourceRpgScopeWidget; // UUserWidget
	ue::WeakObject RpgScopeWidget; // UWidgetComponent

	ue::WeakObject ScopeStaticMeshComponent; // UStaticMeshComponent
	ue::WeakObject SceneCaptureComponent; // USceneCaptureComponent2D
	ue::WeakObject SceneCaptureUStaticMeshComponent; // UStaticMeshComponent

	ue::WeakObject ScopeRenderTarget; // UTextureRenderTarget2D


	glm::fquat DefaultUevrHookWeaponRotation{};
	glm::fvec3 DefaultUevrHookWeaponPosition{};
	glm::fquat CurrentRelativeWeaponRotation = {};
	glm::fvec3 CurrentRelativeWeaponRotationEuler {0.0f, 0.0f, 0.0f};
	glm::fvec3 CurrentRelativeWeaponPosition {0.0f, 0.0f, 0.0f};
	glm::fvec3 DefaultRelativeWeaponPosition {0.0f, 0.0f, 0.0f};
	glm::fquat DefaultRelativeWeaponRotation { 1.0f, 0.0f, 0.0f, 0.0f };
	ue::FRotator RightHandRotationOffset{};
	ue::FVector LeftHandLocationOffset{};
	ue::FRotator LeftHandRotationOffset{};

	glm::dvec3 FirePosition{};
	glm::dvec3 AimPosition{};

	ue::WeakObject DummyCqcKnifeMeshActor; // AActor
	ue::WeakObject DummyCqcKnifeMesh; // USkeletalMeshComponent

	bool secondHandShouldAttachToWeapon = false;

	float ActualFoodRotation = 0.0f;
	glm::vec3 FoodIdleStart = {0.0f, 0.0f, 0.0f};
	glm::vec3 FoodIdlePosition = {0.0f, 0.0f, 0.0f};
	glm::fquat FoodCurrentLocalRotation = {1.0, 0.0, 0.0, 0.0};
	glm::quat FoodGrabRelativeRotation = {};
	glm::vec3 FoodGrabRelativePosition = {};

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

	void SwitchScopeState(bool enable);
	void SpawnScope();
	void DestroyScope();

	void Cleanup();
	// Commits this frame's tracked values: call last in the frame
	void EndFrame();

	void ToggleThermalVisionScanline();
	bool ToggleThermalVisionScanlineNextFrame = false;

	void ProcessItem();
	void SendHandStateToLua();
	void AttachMainHandToWeapon();
	bool DetachHandFromWeapon();
	void AttachSecondHandToWeapon();
	bool DetachSecondHandFromWeapon();
	bool AttachHandNextFrame = false;
	bool NextFrame = false;
	bool AttachSecondHandNextFrame = false;

	void ProcessTwoHandsGrabWeapon();
	void ProcessTwoHandsGrabWeaponLocationAndRotation();
	void ApplyRecoil(float delta);
};