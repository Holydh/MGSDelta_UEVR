#include "WeaponController.hpp"

inline const char* WeaponController::WeaponIdToKey(EGsrEquipId id)
{
	switch (id)
    {
    case WP_Knife:         return "WP_Knife";
    case WP_Fork:          return "WP_Fork";
    case WP_CigarPistol:   return "WP_CigarPistol";
    case WP_Handkerchief:  return "WP_Handkerchief";
    case WP_Mk22:          return "WP_Mk22";
    case WP_Gove:          return "WP_Gove";
    case WP_EasyGun:       return "WP_EasyGun";
    case WP_SAArmy:        return "WP_SAArmy";
    case WP_PatriotPostol: return "WP_PatriotPostol";
    case WP_Scorpion:      return "WP_Scorpion";
    case WP_M16a1:         return "WP_M16a1";
    case WP_Akm:           return "WP_Akm";
    case WP_M63:           return "WP_M63";
    case WP_Ithaca:        return "WP_Ithaca";
    case WP_Dragnov:       return "WP_Dragnov";
    case WP_MosinNagant:   return "WP_MosinNagant";
    case WP_Rpg:           return "WP_Rpg";
    case WP_Torch:         return "WP_Torch";
    case WP_Grenade:       return "WP_Grenade";
    case WP_FireGrenade:   return "WP_FireGrenade";
    case WP_StunGrenade:   return "WP_StunGrenade";
    case WP_ChaffGrenade:  return "WP_ChaffGrenade";
    case WP_SmokeGrenade:  return "WP_SmokeGrenade";
    case WP_Magazine:      return "WP_Magazine";
    case WP_Tnt:           return "WP_Tnt";
    case WP_C3:            return "WP_C3";
    case WP_Claymore:      return "WP_Claymore";
    case WP_Book:          return "WP_Book";
    case WP_RatTrap:       return "WP_RatTrap";
    case WP_Microphone:    return "WP_Microphone";
    default: return nullptr;
    }
}

void WeaponController::ApplyWeaponOffsets(EGsrEquipId currentWeapon, glm::fvec3& outPos, glm::fquat& outRot)
{
	const char* key = WeaponIdToKey(currentWeapon);
    if (!key) return;

    auto it = settingsManager->g_weaponConfigs.find(key);
    if (it == settingsManager->g_weaponConfigs.end()) return;

    const WeaponConfig& cfg = it->second;
    outPos = cfg.location_offset;
    outRot = cfg.rotation_offset;
}

//void WeaponController::Initialize()
//{
//	FetchEquipController();
//}

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

		ApplyWeaponOffsets(CurrentWeaponEquippedID, DefaultWeaponPosition, DefaultWeaponRotation);
		Logger::DebugPrint("Applied weapon offsets from config");
		Logger::DebugPrint("Position Offset : x=%f, y=%f, z=%f", DefaultWeaponPosition.x, DefaultWeaponPosition.y, DefaultWeaponPosition.z);
		Logger::DebugPrint("Rotation Offset : w=%f, x=%f, y=%f, z=%f", DefaultWeaponRotation.w, DefaultWeaponRotation.x, DefaultWeaponRotation.y, DefaultWeaponRotation.z);
		CurrentWeaponPosition = DefaultWeaponPosition;
		CurrentWeaponRotation = DefaultWeaponRotation;
		CurrentWeaponRotationEuler = glm::eulerAngles(CurrentWeaponRotation);

		UEVR_Vector3f defaultPos = { DefaultWeaponPosition.x, DefaultWeaponPosition.y, DefaultWeaponPosition.z };
		UEVR_Quaternionf defaultRot = { DefaultWeaponRotation.x, DefaultWeaponRotation.y, DefaultWeaponRotation.z, DefaultWeaponRotation.w };

		auto motionState = uevr::API::UObjectHook::get_or_add_motion_controller_state(WeaponHook);
		motionState->set_location_offset(&defaultPos);
		motionState->set_rotation_offset(&defaultRot);
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
		FirePosition = { muzzle_transform.Translation.x, muzzle_transform.Translation.y, muzzle_transform.Translation.z };
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
		//case WP_Mk22:
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		//case WP_Gove:
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		//case WP_EasyGun:
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		//case WP_SAArmy: //This one x is the z there for some reasons
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		//case WP_PatriotPostol:
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		//case WP_Scorpion:
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		//case WP_M16a1: // {forward,  up down, left right}
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		//case WP_Akm:
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		//case WP_M63:
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		case WP_Ithaca:
			aimOffsetValueHorizontal = -150.0f;
			aimOffsetValueVertical = 0.0f;
			break;
		//case WP_Dragnov:
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		//case WP_MosinNagant:
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;
		//case WP_Rpg:
		//	aimOffsetValueHorizontal = 0.0f;
		//	aimOffsetValueVertical = 0.0f;
		//	break;

		default:
			aimOffsetValueHorizontal = 0.0f;
			aimOffsetValueVertical = 0.0f;
			break;
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

void WeaponController::SetAimDirection()
{
	//Will be used to control the grenades throw direction
	//This controls only the aim when the camera controller is disabled. When cc enabled, it controls the camera rotation but the inputs are conflicting.
	auto cameraController_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Gsr.GsrPlayerCameraController");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
	auto cameraController = cameraController_c->get_first_object_matching(false);
	struct Rotator{
		UEVR_Rotatord rotation;
	}params;
	params.rotation.yaw = 0.0f;
	//cameraController.YawInput
	cameraController->call_function(L"SetCameraRotation", &params);
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
		CurrentWeaponRotation = glm::fquat(CurrentWeaponRotationEuler);
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
		glm::fquat smoothedQuat = glm::slerp(CurrentWeaponRotation, DefaultWeaponRotation, delta * recoilRotationRecoverySpeed);
		CurrentWeaponRotation = smoothedQuat;
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

