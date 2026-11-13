#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ue/Reflection.hpp"

// Held food (and the other items sharing its ID) floats and turns in front of Snake; while the left grip is
// held, the left VR hand holds it instead
class FoodHandling {
private:
	float ActualFoodRotation = 0.0f;
	glm::vec3 FoodIdleStart = {0.0f, 0.0f, 0.0f};
	glm::vec3 FoodIdlePosition = {0.0f, 0.0f, 0.0f};
	glm::fquat FoodCurrentLocalRotation = {1.0, 0.0, 0.0, 0.0};
	glm::quat FoodGrabRelativeRotation = {};
	glm::vec3 FoodGrabRelativePosition = {};

public:
	// Every frame while food is held. weaponRoot: where the food floats. cameraYawDelta: yaw the camera turned
	// this frame, the food turns with the view
	void Update(ue::UObject* weaponRoot, ue::UObject* leftHandMesh, float cameraYawDelta, float delta);
};
