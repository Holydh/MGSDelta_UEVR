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
	void SetCameraRotation(double yaw);
};