#pragma once
#include "uevr/API.hpp"
#include "Utilities.hpp"
#include "Logger.hpp"

class CameraController {
private:

public:
	CameraController() {};
	//virtual ~CameraController() = default;

	void ToggleGameCameraController(bool enabled);

	void ToggleCameraWobblingController(bool enabled);

	double actualYaw = 0.0f;
	float YawInput = 0.0f;
	float YawSpeed = 1.0f;
	UEVR_Vector3d playerPosition = { 0.0, 0.0, 0.0 };
	void SetCameraRotation();
	//void SetCameraPosition();

	uevr::API::UObject* CameraAttachementActor = nullptr;
	void SpawnCameraAttachementActor();
	void DestroyCameraAttachementActor();
	void AttachCameraToAttachementActor();
	void DetachCameraFromAttachementActor();
	bool CameraAttachementActorAvailable = false;

	void FetchAnimationState();
	bool start = false;
	Utilities::IcsAnimInstance_OnEnteredState AnimationState;
};