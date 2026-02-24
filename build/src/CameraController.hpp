#pragma once
#include "uevr/API.hpp"
#include "Utilities.hpp"
#include "Logger.hpp"

#include "SDK/Ics_parameters.hpp"
#include "SDK/Engine_structs.hpp"
#include "SDK/Engine_parameters.hpp"
#include "SDK/Engine_classes.hpp"
#include "SDK/sv_main_classes.hpp"

#include "PlayerManager.hpp"
#include "SettingsManager.hpp"

class CameraController {
private:
	PlayerManager* const playerManager;
	SettingsManager* const settingsManager;

	UEVR_Vector3d headColliderPosition{};
	float crawlOffset = 55.0f;
	float previousPawnPositionZ = 0.0f;

public:
	CameraController(PlayerManager* pm, SettingsManager* sm) : playerManager(pm), settingsManager(sm){};
	//virtual ~CameraController() = default;

	void ToggleCameraWobblingController(bool enabled);

	bool CutsceneStarts();
	bool CutsceneStops();

	double ActualYaw = 0.0f;
	
	float YawInput = 0.0f;
	float YawProcessed = 0.0f;
	//UEVR_Vector3d playerPosition = { 0.0, 0.0, 0.0 };
	/*void SetCameraRotation();*/
	//void SetCameraPosition();
	float CameraHeightOffset = 0.0f;
	UEVR_Vector3d OriginalCameraPosition = { 0.0f, 0.0f, 0.0f };
	glm::fvec3 LastCameraPosition = { 0.0f, 0.0f, 0.0f };
	UEVR_Rotatord CameraRotation = { 0.0f, 0.0f, 0.0f };
	UEVR_Rotatord LastCameraRotation = { 0.0f, 0.0f, 0.0f };
	SDK::FVector CameraForwardVector = { 1.0f, 0.0f, 0.0f };

	static uevr::API::UObject* CameraAttachementActor;
	bool SpawnCameraAttachementActor();
	void DestroyCameraAttachementActor();
	void AttachCameraToAttachementActor();
	void DetachCameraFromAttachementActor();
	bool IsCameraAttachedToAttachementActor = false;
	bool CameraAttachementActorAvailable = false;
	//bool CameraAttachementActorPositionSafeguard();

	void ProcessCamera(float deltaTime);
	float InstantCameraPositionTimer = 0.25f;
	bool InstantCameraPositionNeeded = false;
	void ProcessCameraResetTimer(float delta);
	//void DisableGrassFading();

	void ProcessRoomscaleStance();

	void FixUIMenuRendering();

	//void SetNearClipPlane(float nearClip);

	bool SmallUIOpen = false;
	//Radio, inventory, etc.
	bool IsMenuOpen = false;
	bool WasMenuOpen = false;

	bool IsCutscenePlaying = false;
	bool WasCutscenePlaying = false;

	bool CardBoardEquipped = false;

	SDK::UPanelSlot* UIMenuBackgroundPanelSlot = nullptr;
};