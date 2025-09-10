#include "WeaponController.hpp"

void WeaponController::ProcessWeapon()
{
	//Get weapon ID
	//Logger::DebugPrint(wobblingController->get_full_name());
	struct ParameterWeaponID {
		EGsrEquipId ReturnValue;
	}params;
	if (equipController == NULL)
	{
		currentWeaponEquippedID = WP_None;
		Logger::DebugPrint("No equipController found");
		return;
	}

	equipController->call_function(L"GetCurrentWeaponId", &params);
	if (currentWeaponEquippedID != params.ReturnValue)
	{
		currentWeaponEquippedID = params.ReturnValue;
		Logger::DebugPrint("Weapon changed. New Weapon ID : %d", currentWeaponEquippedID);
	}
}

void WeaponController::ApplyRecoil(uevr::API::UObject* weaponMesh, bool isShooting, float delta)
{
	glm::fvec3 positionRecoilForce = { 0.0f, 0.0f, 0.0f };
	glm::fvec3 rotationRecoilForceEuler = { 0.0f, 0.0f, 0.0f };

	switch (currentWeaponEquippedID)
	{
	case WP_Mk22:
		positionRecoilForce = { 0.0f, -0.005f, -0.5f };
		rotationRecoilForceEuler = { -0.08f, 0.0f, 0.0f };
		break;
		//case PistolSilenced:
		//	positionRecoilForce = { 0.0f, -0.005f, -0.5f };
		//	rotationRecoilForceEuler = { -0.05f, 0.0f, 0.0f };
		//	break;
		//case DesertEagle:
		//	positionRecoilForce = { 0.0f, -0.005f, -2.5f };
		//	rotationRecoilForceEuler = { -0.15f, 0.0f, 0.0f };
		//	break;
		//case Shotgun:
		//	positionRecoilForce = { 0.0f, -0.005f, -5.0f };
		//	rotationRecoilForceEuler = { -0.3f, 0.0f, 0.0f };
		//	break;
		//case Sawnoff:
		//	positionRecoilForce = { 0.0f, -0.005f, -5.0f };
		//	rotationRecoilForceEuler = { -0.3f, 0.0f, 0.0f };
		//	break;
		//case Spas12:
		//	positionRecoilForce = { 0.0f, -0.005f, -5.0f };
		//	rotationRecoilForceEuler = { -0.3f, 0.0f, 0.0f };
		//	break;
		//case MicroUzi:
		//	positionRecoilForce = { 0.0f, -0.005f, -1.0f };
		//	rotationRecoilForceEuler = { -0.01f, 0.0f, 0.0f };
		//	break;
		//case Mp5:
		//	positionRecoilForce = { 0.0f, -0.005f, -1.0f };
		//	rotationRecoilForceEuler = { -0.01f, 0.0f, 0.0f };
		//	break;
		//case Ak47:
		//	positionRecoilForce = { 0.0f, -0.005f, -1.0f };
		//	rotationRecoilForceEuler = { -0.01f, 0.0f, 0.0f };
		//	break;
		//case M4:
		//	positionRecoilForce = { 0.0f, -0.005f, -1.0f };
		//	rotationRecoilForceEuler = { -0.01f, 0.0f, 0.0f };
		//	break;
		//case Tec9:
		//	positionRecoilForce = { 0.0f, -0.005f, -1.0f };
		//	rotationRecoilForceEuler = { -0.01f, 0.0f, 0.0f };
		//	break;
		//case Rifle: // "cuntgun"
		//	positionRecoilForce = { 0.0f, -0.005f, -2.0f };
		//	rotationRecoilForceEuler = { -0.02f, 0.0f, 0.0f };
		//	shootDetectionFromMemory = true;

		//	break;
		//case Sniper:
		//	positionRecoilForce = { 0.0f, -0.005f, -2.0f };
		//	rotationRecoilForceEuler = { -0.02f, 0.0f, 0.0f };
		//	shootDetectionFromMemory = true;
		//	break;
		//case RocketLauncher:
		//	positionRecoilForce = { 0.0f, -0.005f, -3.0f };
		//	rotationRecoilForceEuler = { -0.02f, 0.0f, 0.0f };
		//	shootDetectionFromMemory = true;
		//	break;
		//case RocketLauncherHs : // RocketLauncherHeatSeek
		//	positionRecoilForce = { 0.0f, -0.005f, -3.0f };
		//	rotationRecoilForceEuler = { -0.02f, 0.0f, 0.0f };
		//	shootDetectionFromMemory = true;
		//	break;
		//case Flamethrower:
		//	positionRecoilForce = { -0.0f, 0.5f, -0.5f };
		//	rotationRecoilForceEuler = { -0.0f, 0.0f, 0.0f };
		//	break;
		//case Minigun:	
		//	//positionRecoilForce = { 0.0f, -0.005f, -5.0f };
		//	//rotationRecoilForceEuler = { -0.3f, 0.0f, 0.0f };
		//	positionRecoilForce = { 0.0f, -0.005f, -1.5f };
		//	rotationRecoilForceEuler = { -0.01f, 0.0f, 0.0f };
		//	break;
		//case Detonator:
		//	return;
		//case SprayCan:
		//	return;
		//case Extinguisher:
		//	return;
		//case Camera:
		//	HandleCameraWeaponAiming();
		//	return;

		//default:

		//	UnhookAndRepositionWeapon();
		//	return;
		//}
	}
}

