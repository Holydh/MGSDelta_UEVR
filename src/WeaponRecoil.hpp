#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "ue/Reflection.hpp"
#include "SettingsManager.hpp"

// Kicks the weapon mesh back when a shot is fired, then brings it back to its rest pose
class WeaponRecoil {
private:
	SettingsManager* const settingsManager;

	float randomRange(float min, float max);

	glm::fquat CurrentRelativeWeaponRotation = {};
	glm::fvec3 CurrentRelativeWeaponRotationEuler {0.0f, 0.0f, 0.0f};
	glm::fvec3 CurrentRelativeWeaponPosition {0.0f, 0.0f, 0.0f};
	glm::fvec3 DefaultRelativeWeaponPosition {0.0f, 0.0f, 0.0f};
	glm::fquat DefaultRelativeWeaponRotation { 1.0f, 0.0f, 0.0f, 0.0f };

public:
	WeaponRecoil(SettingsManager* sm) : settingsManager(sm) {};

	// Set by the fire hook, applied by the next Apply
	bool IsShooting = false;

	// New weapon: back to the rest pose
	void Reset();
	// receiverMesh: the weapon's skeletal mesh, moved relative to the weapon root
	void Apply(ue::EGsrEquipId weaponId, ue::UObject* receiverMesh, bool secondHandAttached, float delta);
};
