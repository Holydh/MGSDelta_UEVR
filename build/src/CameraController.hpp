#pragma once
#include "uevr/API.hpp"
#include "Utilities.hpp"
#include "Logger.hpp"

#include "SDK/Ics_parameters.hpp"
#include "SDK/Engine_structs.hpp"
#include "SDK/Engine_parameters.hpp"
#include "SDK/Engine_classes.hpp"

#include "PlayerManager.hpp"
#include "SettingsManager.hpp"

class CameraController {
private:
	PlayerManager* const playerManager;
	SettingsManager* const settingsManager;

public:
	CameraController(PlayerManager* pm, SettingsManager* sm) : playerManager(pm), settingsManager(sm){};
	//virtual ~CameraController() = default;

	float DeltaTime = 0.0f;

	void ToggleCameraWobblingController(bool enabled);

	double ActualYaw = 0.0f;
	
	float YawInput = 0.0f;
	//UEVR_Vector3d playerPosition = { 0.0, 0.0, 0.0 };
	/*void SetCameraRotation();*/
	//void SetCameraPosition();
	float CameraHeightOffset = 0.0f;
	glm::fvec3 LastCameraPosition = { 0.0f, 0.0f, 0.0f };
	UEVR_Rotatord LastCameraRotation = { 0.0f, 0.0f, 0.0f };
	SDK::FVector CameraForwardVector = { 1.0f, 0.0f, 0.0f };

	static uevr::API::UObject* CameraAttachementActor;
	void SpawnCameraAttachementActor();
	void DestroyCameraAttachementActor();
	void AttachCameraToAttachementActor();
	void DetachCameraFromAttachementActor();
	bool CameraAttachementActorAvailable = false;

	void ProcessCamera(UEVR_Vector3d& cameraPosition, UEVR_Rotatord& cameraRotation);
	float InstantCameraPositionTimer = 1.0f;
	bool InstantCameraPositionNeeded = false;
	void ProcessCameraResetTimer(float delta);
};