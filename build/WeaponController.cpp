#include "WeaponController.hpp"

void WeaponController::Initialize()
{
	static auto pawn = uevr::API::get()->get_local_pawn(0);
	struct ParameterEquipController {
		uevr::API::UObject* ReturnValue;
	}params;
	pawn->call_function(L"GetEquipControllerBP", &params);
	EquipController = params.ReturnValue;
	if (EquipController == NULL)
	{
		Logger::DebugPrint("No equipController found");
	}
	else
	{
		Logger::DebugPrint("EquipController found");
	}

	defaultWeaponPosition = glm::dvec3(-3.4766197204589844, 1.9591338634490967, 2.8622045516967773);
	defaultWeaponRotationEuler = glm::eulerAngles(glm::fquat(-0.5478599071502686, -0.03995904326438904, 0.8305291533470154, -0.09205403923988342));
	currentWeaponPosition = defaultWeaponPosition;
	currentWeaponRotationEuler = defaultWeaponRotationEuler;
}

void WeaponController::ProcessWeapon(float delta)
{
	//Get weapon ID
	//Logger::DebugPrint(wobblingController->get_full_name());
	struct ParameterWeaponID {
		EGsrEquipId ReturnValue;
	}params;
	if (EquipController == NULL)
	{
		CurrentWeaponEquippedID = WP_None;
		Logger::DebugPrint("No equipController found");
		return;
	}

	EquipController->call_function(L"GetCurrentWeaponId", &params);
	if (CurrentWeaponEquippedID != params.ReturnValue)
	{
		CurrentWeaponEquippedID = params.ReturnValue;
		Logger::DebugPrint("Weapon changed. New Weapon ID : %d", CurrentWeaponEquippedID);
		struct ParameterCurrentWeapon {
			uevr::API::UObject* ReturnValue;
		}params1;
		EquipController->call_function(L"GetCurrentWeapon", &params1);
		CurrentWeapon = params1.ReturnValue;

		if (CurrentWeapon == NULL) return;
		Logger::DebugPrint(CurrentWeapon->get_full_name());

		WeaponHook = CurrentWeapon->get_property<uevr::API::UObject*>(L"RootSceneComponent");
		if (WeaponHook == NULL) return;
		Logger::DebugPrint(WeaponHook->get_full_name());

		auto motionState = uevr::API::UObjectHook::get_or_add_motion_controller_state(WeaponHook);
		glm::dquat rotQuat = glm::dquat(defaultWeaponRotationEuler);
		UEVR_Vector3f pos = { defaultWeaponPosition.x, defaultWeaponPosition.y, defaultWeaponPosition.z };
		UEVR_Quaternionf rot = { rotQuat.w, rotQuat.x, rotQuat.y, rotQuat.z };
		motionState->set_location_offset(&pos);
		motionState->set_rotation_offset(&rot);
		motionState->set_permanent(true);
	}

	//Apply recoil
	ApplyRecoil(delta);
}

void WeaponController::ApplyRecoil(float delta)
{
	glm::fvec3 positionRecoilForce = { 0.0f, 0.0f, 0.0f };
	glm::fvec3 rotationRecoilForceEuler = { 0.0f, 0.0f, 0.0f };
	float recoilPositionRecoverySpeed = 10.0f;
	float recoilRotationRecoverySpeed = 8.0f;

	switch (CurrentWeaponEquippedID)
	{
	case WP_Mk22:
		positionRecoilForce = { 0.75f, -0.005f, 0.0f };
		rotationRecoilForceEuler = { -0.1f, 0.0f, 0.0f };
		break;
	case WP_Gove:
		positionRecoilForce = { 4.0f, -0.005f, RandomRange(-0.25, 0.25) };
		rotationRecoilForceEuler = { -0.1f, 0.0f, 0.0f };
		recoilPositionRecoverySpeed = 14.0f;
		recoilRotationRecoverySpeed = 12.0f;
		break;
	case WP_EasyGun:
		positionRecoilForce = { 0.75f, -0.005f, 0.0f };
		rotationRecoilForceEuler = { -0.1f, 0.0f, 0.0f };
		break;
	case WP_SAArmy: //This one x is the z there for some reasons
		positionRecoilForce = { 5.0f, -0.005f, 0.0f /*RandomRange(-0.15, 0.15)*/ };
		rotationRecoilForceEuler = { 0.0f, 0.0f, -0.75f };
		//recoilPositionRecoverySpeed = 7.0f;
		//recoilRotationRecoverySpeed = 5.0f;
		break;
	case WP_PatriotPostol:
		positionRecoilForce = { 0.75f, -0.18f, RandomRange(-0.1, 0.1)};
		rotationRecoilForceEuler = { -0.005f, 0.0f, 0.0f };
		break;
	case WP_Scorpion:
		positionRecoilForce = { 1.0f, -0.25f, RandomRange(-0.2, 0.2)};
		rotationRecoilForceEuler = { 0.0f, 0.0f, -0.01f };
		break;
	case WP_M16a1: // {forward,  up down, left right}
		positionRecoilForce = { 1.0f, -0.25f, RandomRange(-0.2, 0.2)};
		rotationRecoilForceEuler = { -0.01f, 0.0f, 0.0f };
		break;
	case WP_Akm:
		positionRecoilForce = { 1.0f, -0.25f, RandomRange(-0.2, 0.2)};
		rotationRecoilForceEuler = { -0.01f, 0.0f, 0.0f };
		break;
	case WP_M63:
		positionRecoilForce = { 2.0f, -0.25f, RandomRange(-0.2, 0.2)};
		rotationRecoilForceEuler = { -0.01f, 0.0f, 0.0f };
		recoilPositionRecoverySpeed = 14.0f;
		recoilRotationRecoverySpeed = 12.0f;
		break;
	case WP_Ithaca:
		positionRecoilForce = { 30.0f, -10.0f, 1.0 };
		rotationRecoilForceEuler = { -0.3f, 0.0f, 0.0f };
		recoilPositionRecoverySpeed = 7.0f;
		recoilRotationRecoverySpeed = 5.0f;
		break;
	case WP_Dragnov:
		positionRecoilForce = { 4.0f, -0.005f, 0.0f };
		rotationRecoilForceEuler = { -0.02f, 0.0f, 0.0f };
		break;
	case WP_MosinNagant:
		positionRecoilForce = { 2.0f, -0.005f, RandomRange(-0.2, 0.2)};
		rotationRecoilForceEuler = { -0.02f, 0.0f, 0.0f };
		break;
	case WP_Rpg:
		positionRecoilForce = { 5.0f, -0.005f, 0.0f };
		rotationRecoilForceEuler = { -0.1f, 0.0f, 0.0f };
		break;
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

	default:
		return;
		
	}

	auto motionState = uevr::API::UObjectHook::get_motion_controller_state(WeaponHook);
	
	//Logger::DebugPrint("%d", IsShooting);
	if (IsShooting)
	{
		currentWeaponPosition += positionRecoilForce;
		currentWeaponRotationEuler += rotationRecoilForceEuler;

		glm::dquat rotQuat = glm::dquat(currentWeaponRotationEuler);
		UEVR_Vector3f pos = { currentWeaponPosition.x, currentWeaponPosition.y, currentWeaponPosition.z };
		UEVR_Quaternionf rot = { rotQuat.w, rotQuat.x, rotQuat.y, rotQuat.z };

		motionState->set_location_offset(&pos);
		motionState->set_rotation_offset(&rot);
		motionState->set_permanent(true);
		IsShooting = false;
	}
	else
	{
		// Smoothly return position to base
		currentWeaponPosition = glm::mix(currentWeaponPosition, defaultWeaponPosition, delta * recoilPositionRecoverySpeed);

		glm::fquat currentQuat = glm::fquat(currentWeaponRotationEuler);
		glm::fquat targetQuat = glm::fquat(defaultWeaponRotationEuler);
		glm::fquat smoothedQuat = glm::slerp(currentQuat, targetQuat, delta * recoilRotationRecoverySpeed);
		currentWeaponRotationEuler = glm::eulerAngles(smoothedQuat);

		UEVR_Vector3f pos = { currentWeaponPosition.x, currentWeaponPosition.y, currentWeaponPosition.z };
		UEVR_Quaternionf rot = { smoothedQuat.w, smoothedQuat.x, smoothedQuat.y, smoothedQuat.z };

		motionState->set_location_offset(&pos);
		motionState->set_rotation_offset(&rot);
	}
}
	float WeaponController::RandomRange(float min, float max) {
    return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
	}

