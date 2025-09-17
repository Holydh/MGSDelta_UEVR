#include "WeaponController.hpp"

void WeaponController::Initialize()
{
	FetchEquipController();
	DefaultWeaponPosition = glm::dvec3(-3.4766197204589844, 1.9591338634490967, 2.8622045516967773);
	DefaultWeaponRotationEuler = glm::eulerAngles(glm::fquat(-0.5478599071502686, -0.03995904326438904, 0.8305291533470154, -0.09205403923988342));
	CurrentWeaponPosition = DefaultWeaponPosition;
	CurrentWeaponRotationEuler = DefaultWeaponRotationEuler;
}

void WeaponController::FetchEquipController()
{
	auto pawn = uevr::API::get()->get_local_pawn(0);
	struct ParameterEquipController {
		uevr::API::UObject* ReturnValue;
	}params;
	pawn->call_function(L"GetEquipControllerBP", &params);
	EquipController = params.ReturnValue;
	//Logger::DebugPrint("Fetch : ");
	//Logger::DebugPrint(EquipController->get_full_name());
	if (EquipController == NULL)
	{
		Logger::DebugPrint("No equipController found");
	}
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
	}

	struct ParameterCurrentWeapon {
		uevr::API::UObject* ReturnValue;
	}params1;
	EquipController->call_function(L"GetCurrentWeapon", &params1);

	if (CurrentWeapon != params1.ReturnValue)
	{
		CurrentWeapon = params1.ReturnValue;
		Logger::DebugPrint("Updating Current weapon ptr");
		Logger::DebugPrint(CurrentWeapon->get_full_name());
		WeaponHook = CurrentWeapon->get_property<uevr::API::UObject*>(L"RootSceneComponent");
		if (WeaponHook == NULL) return;
		Logger::DebugPrint(WeaponHook->get_full_name());

		auto motionState = uevr::API::UObjectHook::get_or_add_motion_controller_state(WeaponHook);
		glm::dquat rotQuat = glm::dquat(DefaultWeaponRotationEuler);
		UEVR_Vector3f pos = { DefaultWeaponPosition.x, DefaultWeaponPosition.y, DefaultWeaponPosition.z };
		UEVR_Quaternionf rot = { rotQuat.w, rotQuat.x, rotQuat.y, rotQuat.z };
		motionState->set_location_offset(&pos);
		motionState->set_rotation_offset(&rot);
		motionState->set_permanent(true);
	}
	
	//Apply recoil
	if (CurrentWeapon != NULL)
		ApplyRecoil(delta);
}

void WeaponController::FetchWeaponMeshAimData()
{
	//Logger::DebugPrint(CurrentWeapon->get_full_name());
	if (CurrentWeapon == nullptr)
	{
		Logger::DebugPrint("No CurrentWeapon found");
		return;
	}
	auto root_component = CurrentWeapon->get_property<uevr::API::UObject*>(L"RootComponent");
	//Logger::DebugPrint(root_component->get_full_name());
	if (root_component != nullptr)
	{
		UEVR_Vector3d forward_params;
		root_component->call_function(L"GetForwardVector", &forward_params);
		//Logger::DebugPrint("ForwardVector.x : %f, ForwardVector.y : %f, ForwardVector.z : %f", forward_params.x, forward_params.y, forward_params.z);
		Utilities::FTransform muzzle_transform;
		root_component->call_function(L"GetMuzzleTransform", &muzzle_transform);
		//Logger::DebugPrint("FirePosition.x : %f, FirePosition.y : %f, FirePosition.z : %f", muzzle_transform.Translation.x, muzzle_transform.Translation.y, muzzle_transform.Translation.z);
		float scaler = 8192.0;
		Utilities::RotateAngleAxis rotate_params;
		rotate_params.InVect = forward_params;
		rotate_params.AngleDeg = 90.0f;
		root_component->call_function(L"GetUpVector", &rotate_params.Axis);
		Utilities::KismetMathLibrary->call_function(L"RotateAngleAxis", &rotate_params);
		forward_params = rotate_params.Result;

		float aimOffsetValueHorizontal = 0.0f;
		float aimOffsetValueVertical = 0.0f;
		switch (CurrentWeaponEquippedID)
		{
		case WP_Mk22:
			aimOffsetValueHorizontal = 150.0f;
			aimOffsetValueVertical = 0.0f;
			break;
		case WP_Gove:
			aimOffsetValueHorizontal = 150.0f;
			aimOffsetValueVertical = 0.0f;
			break;
		case WP_EasyGun:
			aimOffsetValueHorizontal = 0.0f;
			aimOffsetValueVertical = 0.0f;
			break;
		case WP_SAArmy: //This one x is the z there for some reasons
			aimOffsetValueHorizontal = 150.0f;
			aimOffsetValueVertical = 0.0f;
			break;
		case WP_PatriotPostol:
			aimOffsetValueHorizontal = 150.0f;
			aimOffsetValueVertical = 0.0f;
			break;
		case WP_Scorpion:
			aimOffsetValueHorizontal = 0.0f;
			aimOffsetValueVertical = 0.0f;
			break;
		case WP_M16a1: // {forward,  up down, left right}
			aimOffsetValueHorizontal = 150.0f;
			aimOffsetValueVertical = 0.0f;
			break;
		case WP_Akm:
			aimOffsetValueHorizontal = 150.0f;
			aimOffsetValueVertical = 0.0f;
			break;
		case WP_M63:
			aimOffsetValueHorizontal = 150.0f;
			aimOffsetValueVertical = 0.0f;
			break;
		case WP_Ithaca:
			aimOffsetValueHorizontal = -1000.0f;
			aimOffsetValueVertical = 0.0f;
			break;
		case WP_Dragnov:
			aimOffsetValueHorizontal = 150.0f;
			aimOffsetValueVertical = 0.0f;
			break;
		case WP_MosinNagant:
			aimOffsetValueHorizontal = 150.0f;
			aimOffsetValueVertical = 0.0f;
			break;
		case WP_Rpg:
			aimOffsetValueHorizontal = 150.0f;
			aimOffsetValueVertical = 0.0f;
			break;

		default:
			return;

		}

		AimPosition = { 
			FirePosition.x + forward_params.x * scaler, 
			FirePosition.y + forward_params.y * scaler - aimOffsetValueHorizontal,
			FirePosition.z + forward_params.z * scaler - aimOffsetValueVertical
		};
		//Logger::DebugPrint("AimPosition.x : %f, AimPosition.y : %f, AimPosition.z : %f", AimPosition.x, AimPosition.y, AimPosition.z);
	}
}

void WeaponController::ApplyAimDataToMemory()
{
	//Logger::DebugPrint("AimPosition.x : %f, AimPosition.y : %f, AimPosition.z : %f", AimPosition.x, AimPosition.y, AimPosition.z);
	*(reinterpret_cast<double*>(memoryManager->AimPositionAddresses.x)) = AimPosition.x;
	*(reinterpret_cast<double*>(memoryManager->AimPositionAddresses.y)) = AimPosition.y;
	*(reinterpret_cast<double*>(memoryManager->AimPositionAddresses.z)) = AimPosition.z;
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

	default:
		return;
		
	}

	auto motionState = uevr::API::UObjectHook::get_motion_controller_state(WeaponHook);
	
	//Logger::DebugPrint("%d", IsShooting);
	if (IsShooting)
	{
		CurrentWeaponPosition += positionRecoilForce;
		CurrentWeaponRotationEuler += rotationRecoilForceEuler;

		glm::dquat rotQuat = glm::dquat(CurrentWeaponRotationEuler);
		UEVR_Vector3f pos = { CurrentWeaponPosition.x, CurrentWeaponPosition.y, CurrentWeaponPosition.z };
		UEVR_Quaternionf rot = { rotQuat.w, rotQuat.x, rotQuat.y, rotQuat.z };

		motionState->set_location_offset(&pos);
		motionState->set_rotation_offset(&rot);
		motionState->set_permanent(true);
		IsShooting = false;
	}
	else
	{
		// Smoothly return position to base
		CurrentWeaponPosition = glm::mix(CurrentWeaponPosition, DefaultWeaponPosition, delta * recoilPositionRecoverySpeed);

		glm::fquat currentQuat = glm::fquat(CurrentWeaponRotationEuler);
		glm::fquat targetQuat = glm::fquat(DefaultWeaponRotationEuler);
		glm::fquat smoothedQuat = glm::slerp(currentQuat, targetQuat, delta * recoilRotationRecoverySpeed);
		CurrentWeaponRotationEuler = glm::eulerAngles(smoothedQuat);

		UEVR_Vector3f pos = { CurrentWeaponPosition.x, CurrentWeaponPosition.y, CurrentWeaponPosition.z };
		UEVR_Quaternionf rot = { smoothedQuat.w, smoothedQuat.x, smoothedQuat.y, smoothedQuat.z };

		motionState->set_location_offset(&pos);
		motionState->set_rotation_offset(&rot);
	}
}
	float WeaponController::RandomRange(float min, float max) {
    return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
	}

