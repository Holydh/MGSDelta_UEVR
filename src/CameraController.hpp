#pragma once
#include "uevr/API.hpp"
#include "Utilities.hpp"
#include "Logger.hpp"
#include "magic_enum/magic_enum.hpp"
#include "ue/Reflection.hpp"

#include "GameState.hpp"
#include "PlayerManager.hpp"
#include "ItemManager.hpp"
#include "SettingsManager.hpp"

class CameraController {
private:
	const GameState* const gameState;
	PlayerManager* const playerManager;
	const ItemManager* const items;
	SettingsManager* const settingsManager;

	UEVR_Vector3d headColliderPosition{};
	float crawlOffset = 55.0f;

public:
	CameraController(const GameState* gs, PlayerManager* pm, const ItemManager* im, SettingsManager* sm) : gameState(gs), playerManager(pm), items(im), settingsManager(sm){};

	void SetCameraWobblingControllerEnabled(bool enabled);

	double CurrentYaw = 0.0f;
	
	float YawInput = 0.0f;
	float YawProcessed = 0.0f;
	float CameraHeightOffset = 0.0f;
	UEVR_Vector3d NewCameraPosition = { 0.0f, 0.0f, 0.0f };
	glm::fvec3 LastCameraPosition = { 0.0f, 0.0f, 0.0f };
	UEVR_Rotatord NewCameraRotation = { 0.0f, 0.0f, 0.0f };
	UEVR_Rotatord LastCameraRotation = { 0.0f, 0.0f, 0.0f };
	ue::FRotator ViewRotation{}; // rotation of the VR view rendered last frame

	ue::WeakObject CameraAttachmentActor;
	bool SpawnCameraAttachmentActor();
	void DestroyCameraAttachmentActor();
	void AttachCameraToAttachmentActor();
	void DetachCameraFromAttachmentActor();

	void CalculateForwardOffset(const ue::FVector& location, const ue::FRotator& rotation, float fov, float& forwardOffset, glm::fvec3& forward);

	void ProcessCamera(float deltaTime);
	float InstantCameraPositionTimer = 0.25f;
	bool InstantCameraPositionNeeded = false;
	void ProcessCameraResetTimer(float delta);

	void ProcessRoomscaleStance();
};