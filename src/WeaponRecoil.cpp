#include "WeaponRecoil.hpp"

using enum ue::EGsrEquipId;

void WeaponRecoil::Reset()
{
	CurrentRelativeWeaponPosition = DefaultRelativeWeaponPosition;
	CurrentRelativeWeaponRotation = DefaultRelativeWeaponRotation;
	CurrentRelativeWeaponRotationEuler = glm::eulerAngles(CurrentRelativeWeaponRotation);
}

void WeaponRecoil::Apply(ue::EGsrEquipId weaponId, ue::UObject* receiverMesh, bool secondHandAttached, float delta)
{
	if (settingsManager->Settings.disableAllRecoil)
		return;

	glm::fvec3 positionRecoilForce = { 0.0f, 0.0f, 0.0f };
	glm::fvec3 rotationRecoilForceEuler = { 0.0f, 0.0f, 0.0f };
	float recoilPositionRecoverySpeed = 10.0f;
	float recoilRotationRecoverySpeed = 8.0f;

	bool twoHandsRecoil = secondHandAttached || !settingsManager->Settings.enableTwoHandsRealisticRecoil;

	switch (weaponId)
	{
	case WP_Mk22:
		positionRecoilForce = { 0.0f, -0.75f, 0.005f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -1.0f };
		break;

	case WP_Gove:
		positionRecoilForce = { randomRange(-0.25f, 0.25f), -2.5f, 0.005f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -2.0f };
		recoilPositionRecoverySpeed = 14.0f;
		recoilRotationRecoverySpeed = 12.0f;
		break;

	case WP_EasyGun:
		positionRecoilForce = { 0.0f, -0.75f, 0.005f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -1.0f };
		break;

	case WP_SAArmy: //This one x is the z there for some reasons
		positionRecoilForce = { 0.0f, -5.0f, 0.005f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -10.0f };
		break;

	case WP_PatriotPostol:
		positionRecoilForce = { randomRange(-0.1f, 0.1f), -0.75f, 0.18f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -0.1f };
		break;

	case WP_Scorpion:
		positionRecoilForce = { randomRange(-0.2f, 0.2f), -1.0f, 0.25f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -0.2f };
		break;

	case WP_M16a1: // already corrected
		positionRecoilForce = { randomRange(-0.2f, 0.2f), -1.0f, 0.25f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -1.0f };
		break;

	case WP_Akm:
		positionRecoilForce = { randomRange(-0.2f, 0.2f), -1.0f, 0.25f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -1.0f };
		break;

	case WP_M63:
		positionRecoilForce = { randomRange(-0.2f, 0.2f), -2.0f, 0.25f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -1.0f };
		recoilPositionRecoverySpeed = 14.0f;
		recoilRotationRecoverySpeed = 12.0f;
		break;

	case WP_Ithaca:
		positionRecoilForce = { 1.0f, twoHandsRecoil ? -30.0f : -20.0f, 8.0f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, twoHandsRecoil ? -6.0f : -20.0f };
		recoilPositionRecoverySpeed = 5.0f;
		recoilRotationRecoverySpeed = 4.0f;
		break;

	case WP_Dragnov:
		positionRecoilForce = { 0.0f, -4.0f, 0.005f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -0.4f };
		break;

	case WP_MosinNagant:
		positionRecoilForce = { randomRange(-0.2f, 0.2f), -2.0f, 0.005f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -0.4f };
		break;

	case WP_Rpg:
		positionRecoilForce = { 0.0f, -15.0f, 1.0f };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -6.0f };
		recoilPositionRecoverySpeed = 3.0f;
		recoilRotationRecoverySpeed = 5.0f;
		break;

	default:
		return;
	}

	ue::FHitResult outHit;
	if (IsShooting)
	{
		if (!secondHandAttached && settingsManager->Settings.enableTwoHandsRealisticRecoil)
		{
			positionRecoilForce *= 2.0f;
			rotationRecoilForceEuler *= 2.0f;
		}

		CurrentRelativeWeaponPosition += positionRecoilForce;
		CurrentRelativeWeaponRotationEuler += rotationRecoilForceEuler;
		CurrentRelativeWeaponRotation = glm::fquat(glm::radians(CurrentRelativeWeaponRotationEuler));
		ue::call(receiverMesh, L"K2_SetRelativeLocation", ue::FVector{ CurrentRelativeWeaponPosition.x, CurrentRelativeWeaponPosition.y, CurrentRelativeWeaponPosition.z }, false, &outHit, false);
		ue::call(receiverMesh, L"K2_SetRelativeRotation", ue::FRotator{ CurrentRelativeWeaponRotationEuler.x, CurrentRelativeWeaponRotationEuler.y, CurrentRelativeWeaponRotationEuler.z }, false, &outHit, false);
		IsShooting = false;
	}
	else
	{
		if (!secondHandAttached && settingsManager->Settings.enableTwoHandsRealisticRecoil)
		{
			recoilPositionRecoverySpeed *= 0.75f;
			recoilRotationRecoverySpeed *= 0.75f;
		}
		// Smoothly return position to base
		CurrentRelativeWeaponPosition = glm::mix(CurrentRelativeWeaponPosition, DefaultRelativeWeaponPosition, delta * recoilPositionRecoverySpeed);
		CurrentRelativeWeaponRotation = glm::slerp(CurrentRelativeWeaponRotation, DefaultRelativeWeaponRotation, delta * recoilRotationRecoverySpeed);
		CurrentRelativeWeaponRotationEuler = glm::degrees(glm::eulerAngles(CurrentRelativeWeaponRotation));
		ue::call(receiverMesh, L"K2_SetRelativeLocation", ue::FVector{ CurrentRelativeWeaponPosition.x, CurrentRelativeWeaponPosition.y, CurrentRelativeWeaponPosition.z }, false, &outHit, false);
		ue::call(receiverMesh, L"K2_SetRelativeRotation", ue::FRotator{ CurrentRelativeWeaponRotationEuler.x, CurrentRelativeWeaponRotationEuler.y, CurrentRelativeWeaponRotationEuler.z }, false, &outHit, false);
	}
}

float WeaponRecoil::randomRange(float min, float max) {
	return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
}
