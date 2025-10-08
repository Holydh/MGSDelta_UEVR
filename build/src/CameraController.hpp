#pragma once
#include "uevr/API.hpp"
#include "Utilities.hpp"
#include "Logger.hpp"

#include "SDK/Ics_parameters.hpp"
#include "SDK/Engine_structs.hpp"
#include "SDK/Engine_parameters.hpp"
#include "SDK/Engine_classes.hpp"

#include "PlayerManager.hpp"

class CameraController {
private:
	PlayerManager* const playerManager;

	bool cameraInitialized = false;

public:
	CameraController(PlayerManager* pm) : playerManager(pm){};
	//virtual ~CameraController() = default;

	void ToggleGameCameraController(bool enabled);

	void ToggleCameraWobblingController(bool enabled);

	double actualYaw = 0.0f;
	float YawInput = 0.0f;
	float YawSpeed = 1.0f;
	//UEVR_Vector3d playerPosition = { 0.0, 0.0, 0.0 };
	void SetCameraRotation();
	//void SetCameraPosition();
	float CameraHeightOffset = 0.0f;
	glm::fvec3 CameraPosition = { 0.0f, 0.0f, 0.0f };

	uevr::API::UObject* CameraAttachementActor = nullptr;
	void SpawnCameraAttachementActor();
	void DestroyCameraAttachementActor();
	void AttachCameraToAttachementActor();
	void DetachCameraFromAttachementActor();
	bool CameraAttachementActorAvailable = false;

	void ProcessCamera(UEVR_Vector3d& cameraPosition, UEVR_Rotatord& cameraRotation);
};