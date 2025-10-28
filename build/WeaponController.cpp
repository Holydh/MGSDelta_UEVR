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

void WeaponController::FetchWeaponOffsets(EGsrEquipId currentWeapon, glm::fvec3& outPos, glm::fquat& outRot)
{
	const char* key = WeaponIdToKey(currentWeapon);
	if (!key)
	{
		Logger::DebugPrint("No valid weapon key found for ID: %d", currentWeapon);
		return;
	}

    auto it = settingsManager->g_weaponConfigs.find(key);
	Logger::DebugPrint("Fetching weapon config for key: %s", key);
    if (it == settingsManager->g_weaponConfigs.end()) return;
	Logger::DebugPrint("Weapon config found for key: %s", it->first);
	Logger::DebugPrint("Location Offset: x=%f, y=%f, z=%f", it->second.location_offset.x, it->second.location_offset.y, it->second.location_offset.z);
    const WeaponConfig& cfg = it->second;
    outPos = cfg.location_offset;
    outRot = cfg.rotation_offset;
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
		WeaponRoot = CurrentWeapon->get_property<uevr::API::UObject*>(L"RootSceneComponent");
		if (WeaponRoot == NULL) return;
		Logger::DebugPrint(WeaponRoot->get_full_name());

		FetchWeaponOffsets(CurrentWeaponEquippedID, DefaultWeaponPosition, DefaultWeaponRotation);
		Logger::DebugPrint("Applied weapon offsets from config");
		Logger::DebugPrint("Position Offset : x=%f, y=%f, z=%f", DefaultWeaponPosition.x, DefaultWeaponPosition.y, DefaultWeaponPosition.z);
		Logger::DebugPrint("Rotation Offset : w=%f, x=%f, y=%f, z=%f", DefaultWeaponRotation.w, DefaultWeaponRotation.x, DefaultWeaponRotation.y, DefaultWeaponRotation.z);
		CurrentWeaponPosition = DefaultWeaponPosition;
		CurrentWeaponRotation = DefaultWeaponRotation;
		CurrentWeaponRotationEuler = glm::eulerAngles(CurrentWeaponRotation);

		UEVR_Vector3f defaultPos = { DefaultWeaponPosition.x, DefaultWeaponPosition.y, DefaultWeaponPosition.z };
		UEVR_Quaternionf defaultRot = { DefaultWeaponRotation.x, DefaultWeaponRotation.y, DefaultWeaponRotation.z, DefaultWeaponRotation.w };

		auto motionState = uevr::API::UObjectHook::get_or_add_motion_controller_state(WeaponRoot);
		motionState->set_location_offset(&defaultPos);
		motionState->set_rotation_offset(&defaultRot);
		motionState->set_hand(1);
		motionState->set_permanent(true);

		if (CurrentWeaponEquippedID == EGsrEquipId::WP_Dragnov || CurrentWeaponEquippedID == EGsrEquipId::WP_MosinNagant)
		{
			SDK::USceneComponent* sdkCurrentWeapon = (SDK::USceneComponent*)WeaponRoot;
			auto childrenArray = sdkCurrentWeapon->AttachChildren;
			for (int i = 0; i < childrenArray.Num(); i++)
			{
				SDK::USceneComponent* child = childrenArray[i];
				if (!child) continue;
				if (child->IsA(SDK::USkeletalMeshComponent::StaticClass())) {
					ReceiverMesh = (SDK::USkeletalMeshComponent*)child;
					Logger::DebugPrint(child->GetFullName().c_str());
				}
				//Destroy any previous cylinder mesh if still attached
				if (child->IsA(SDK::UStaticMeshComponent::StaticClass())) {
					child->K2_DestroyComponent(child);
					ScopeCylinderComponent = nullptr;
					ScopeWidgetStaticMeshComponent = nullptr;
					Logger::DebugPrint("Old scope and red dot component destroyed");
				}
			}
			spawnScope();
			attachScopeToWeapon();
			SwitchScopeState(true);
		}
		else
		{
			SwitchScopeState(false);
		}
			
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
		SDK::FVector forward_params;
		CurrentWeapon->call_function(L"GetActorForwardVector", &forward_params);
		//Logger::DebugPrint("ForwardVector.x : %f, ForwardVector.y : %f, ForwardVector.z : %f", forward_params.x, forward_params.y, forward_params.z);
		
		SDK::Params::GsrGun_GetMuzzleTransform muzzle_transform;
		muzzle_transform.TransformSpace = SDK::ERelativeTransformSpace::RTS_World;
		CurrentWeapon->call_function(L"GetMuzzleTransform", &muzzle_transform);
		//Logger::DebugPrint("FirePosition.x : %f, FirePosition.y : %f, FirePosition.z : %f", muzzle_transform.ReturnValue.Translation.X, muzzle_transform.ReturnValue.Translation.Y, muzzle_transform.ReturnValue.Translation.Z);
		auto actor = (SDK::AActor*)CurrentWeapon;
		auto rotation = actor->K2_GetActorRotation();
		//Logger::DebugPrint("rotation.Pitch : %f, rotation.Roll : %f, rotation.Yaw : %f", rotation.Pitch, rotation.Roll, rotation.Yaw);

		FirePosition = { muzzle_transform.ReturnValue.Translation.X, muzzle_transform.ReturnValue.Translation.Y, muzzle_transform.ReturnValue.Translation.Z };
		float scaler = 8192.0;
		SDK::Params::KismetMathLibrary_RotateAngleAxis rotate_params;
		rotate_params.InVect = forward_params;
		rotate_params.AngleDeg = 90.0f;
		CurrentWeapon->call_function(L"GetActorUpVector", &rotate_params.Axis);
		Utilities::KismetMathLibrary->call_function(L"RotateAngleAxis", &rotate_params);
		forward_params = { rotate_params.ReturnValue.X, rotate_params.ReturnValue.Y, rotate_params.ReturnValue.Z };

		// Now, convert the final forward vector into a rotator.
        SDK::FRotator world_rotator;
        
        // This is a struct to pass the vector as a parameter to the function.
        SDK::Params::KismetMathLibrary_MakeRotFromX makeRotFromX_params;
        makeRotFromX_params.X = forward_params;

        // Call the Kismet Math Library function to perform the conversion.
        Utilities::KismetMathLibrary->call_function(L"MakeRotFromX", &makeRotFromX_params);
		
		CurrentWeaponWorldRotation = makeRotFromX_params.ReturnValue;
		//Logger::DebugPrint("CurrentWeaponWorldRotation.x : %f, CurrentWeaponWorldRotation.y : %f, CurrentWeaponWorldRotation.z : %f", 
		//	CurrentWeaponWorldRotation.Pitch, CurrentWeaponWorldRotation.Yaw, CurrentWeaponWorldRotation.Roll);

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
			FirePosition.x + forward_params.X * scaler, 
			FirePosition.y + forward_params.Y * scaler - aimOffsetValueHorizontal,
			FirePosition.z + forward_params.Z * scaler - aimOffsetValueVertical
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

	//Logger::DebugPrint(root_component->get_full_name());
	if (cameraController == nullptr || !CameraController::CameraAttachementActor)
		return;
	
	auto sdkActor = (SDK::AActor*)CameraController::CameraAttachementActor;
	SDK::FRotator params;
	params = sdkActor->K2_GetActorRotation();
	//Logger::DebugPrint("pitch : %f, yaw : %f, roll : %f", params.Pitch, params.Yaw, params.Roll);
	cameraController->call_function(L"SetCameraRotation", &params);
	

	//auto root_component = CurrentWeapon->get_property<uevr::API::UObject*>(L"m_cameraActor");
	////Logger::DebugPrint(root_component->get_full_name());
	//if (root_component != nullptr)
	//{
	//	auto cameraActor = (SDK::AActor*)root_component;
	//	SDK::FHitResult hitResult;
	//	cameraActor->K2_SetActorRelativeRotation(CurrentWeaponWorldRotation, false, &hitResult, true);
	//}
}

void WeaponController::SwitchScopeState(bool enable)
{
	if (SceneCaptureComponent) SceneCaptureComponent->SetVisibility(enable, false);
	if (SceneCaptureUStaticMeshComponent) SceneCaptureUStaticMeshComponent->SetVisibility(enable, false);
	if (ScopeCylinderComponent) ScopeCylinderComponent->SetVisibility(enable, false);
	if (SourceScopeWidget) SourceScopeWidget->SetVisibility(enable ? SDK::ESlateVisibility::Visible : SDK::ESlateVisibility::Hidden);
	if (ScopeWidgetStaticMeshComponent) ScopeWidgetStaticMeshComponent->SetVisibility(enable, false);
	//if (RedDotComponent) RedDotComponent->SetVisibility(enable, false);
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

	auto motionState = uevr::API::UObjectHook::get_motion_controller_state(WeaponRoot);
	
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

void WeaponController::spawnScope()
{
	SDK::UWorld* world = SDK::UWorld::GetWorld();
	SDK::USceneComponent* sdkCurrentWeapon = (SDK::USceneComponent*)WeaponRoot;

	getRenderTarget(world);

	if (!SniperRenderTarget)
	{
		Logger::DebugPrint("Failed to get render target, destroying Scope actor");
		if (ScopeActor)
			ScopeActor->K2_DestroyActor();
		ScopeCylinderComponent = nullptr;
		ScopeWidgetStaticMeshComponent = nullptr;
		SceneCaptureComponent = nullptr;
		SceneCaptureUStaticMeshComponent = nullptr;
		return;
	}

	SDK::FVector weaponPosition = sdkCurrentWeapon->K2_GetComponentLocation();
	if (!ScopeActor)
	{
		ScopeCylinderComponent = nullptr;
		ScopeWidgetStaticMeshComponent = nullptr;
		SceneCaptureComponent = nullptr;
		SceneCaptureUStaticMeshComponent = nullptr;
		ScopeWidget = nullptr;
		ScopeActor = (SDK::AActor*)Utilities::SpawnActor(UEVR_Vector3d{0.0f, 0.0f, 0.0f}, (uevr::API::UObject*)ReceiverMesh->GetOwner());
		Logger::DebugPrint("Scope Actor : %s", ScopeActor->GetFullName().c_str());
	}
	Logger::DebugPrint("spawnScopeStaticMeshComponent()");
	if (!ScopeCylinderComponent)
		spawnScopeStaticMeshComponent();
	Logger::DebugPrint("spawnSceneCaptureComponent()");
	if (!SceneCaptureComponent)
		spawnSceneCaptureComponent();
	Logger::DebugPrint("spawnScopeWidget()");
	if (!ScopeWidget)
		spawnScopeWidget();
}

void WeaponController::getRenderTarget(SDK::UWorld* world)
{
	SDK::FLinearColor linearColor{};
	
	if (!SniperRenderTarget)
	{
		SniperRenderTarget = SDK::UKismetRenderingLibrary::CreateRenderTarget2D(world, cameraRenderTargetResolution, cameraRenderTargetResolution, SDK::ETextureRenderTargetFormat::RTF_RGBA16f, linearColor, false, false);
		Logger::DebugPrint("Render target created : %s", SniperRenderTarget->GetFullName().c_str());
	}
	else
		Logger::DebugPrint("Render target already exists");
}

void WeaponController::spawnScopeStaticMeshComponent()
{
	SDK::FTransform transform{};
	auto scopeStaticMeshComponent = (SDK::UStaticMeshComponent*)ScopeActor->AddComponentByClass(SDK::UStaticMeshComponent::StaticClass(), false, transform, false);
	auto scopeWidgetStaticMeshComponent = (SDK::UStaticMeshComponent*)ScopeActor->AddComponentByClass(SDK::UStaticMeshComponent::StaticClass(), false, transform, false);
	if (!scopeStaticMeshComponent || !scopeWidgetStaticMeshComponent)
	{
		Logger::DebugPrint("Failed to spawn scope or red dot mesh");
		return;
	}
	Logger::DebugPrint("scopeStaticMeshComponent created : %s", scopeStaticMeshComponent->GetFullName().c_str());
	Logger::DebugPrint("redDotStaticMeshComponent created : %s", scopeWidgetStaticMeshComponent->GetFullName().c_str());

	auto scopeDiskMaterialUObject = uevr::API::get()->find_uobject<uevr::API::UObject>(L"Material /Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial");
	auto scopeSightWidgetMaterialUObject = uevr::API::get()->find_uobject<uevr::API::UObject>(
		L"Material /Engine/EngineMaterials/Widget3DPassThrough.Widget3DPassThrough");

	if (!scopeDiskMaterialUObject)
	{
		Logger::DebugPrint("Failed to find scopeDiskMaterialUObject");
		return;
	}
	if (!scopeSightWidgetMaterialUObject)
	{
		Logger::DebugPrint("Failed to find scopeSightWidgetMaterialUObject");
		return;
	}

	Logger::DebugPrint(scopeDiskMaterialUObject->get_full_name().c_str());
	Logger::DebugPrint(scopeSightWidgetMaterialUObject->get_full_name().c_str());

	auto uDiskMaterial = (SDK::UMaterial*)scopeDiskMaterialUObject;
	uDiskMaterial->TwoSided = false;
	uDiskMaterial->BlendMode = SDK::EBlendMode::BLEND_Opaque;
	uDiskMaterial->bDisableDepthTest = true;
	//uMaterial->MaterialDomain = SDK::EMaterialDomain::MD_Surface;
	uDiskMaterial->ShadingModel = SDK::EMaterialShadingModel::MSM_DefaultLit;

	auto uWidgetSightMaterial = (SDK::UMaterial*)scopeDiskMaterialUObject;
	uWidgetSightMaterial->TwoSided = false;
	//uWidgetSightMaterial->BlendMode = SDK::EBlendMode::BLEND_Translucent;
	uWidgetSightMaterial->bDisableDepthTest = true;
	//uWidgetSightMaterial->MaterialDomain = SDK::EMaterialDomain::MD_Surface;
	uWidgetSightMaterial->ShadingModel = SDK::EMaterialShadingModel::MSM_DefaultLit;

	auto cylinderMesh = uevr::API::get()->find_uobject<uevr::API::UObject>(L"StaticMesh /Game/Art/VFX/Game_HD_VFX/Fire/Fire/TheFury_Fire/Mesh/SM_VFX_Disk20edge.SM_VFX_Disk20edge");

	//need to find a texture
	//auto redDotTexture = uevr::API::get()->find_uobject<uevr::API::UObject>(L"StaticMesh /Engine/BasicShapes/Cylinder.Cylinder");
	
	if (!cylinderMesh)
	{
		Logger::DebugPrint("Can't find cylinder mesh");
		return;
	}
	Logger::DebugPrint("cylinder mesh found");
	scopeStaticMeshComponent->SetStaticMesh((SDK::UStaticMesh*)cylinderMesh);
	scopeStaticMeshComponent->SetVisibility(false, false);
	scopeStaticMeshComponent->SetCollisionEnabled(SDK::ECollisionEnabled::NoCollision);
	scopeStaticMeshComponent->SetMobility(SDK::EComponentMobility::Movable);

	scopeWidgetStaticMeshComponent->SetStaticMesh((SDK::UStaticMesh*)cylinderMesh);
	scopeWidgetStaticMeshComponent->SetVisibility(false, true);
	scopeWidgetStaticMeshComponent->SetCollisionEnabled(SDK::ECollisionEnabled::NoCollision);
	scopeWidgetStaticMeshComponent->SetMobility(SDK::EComponentMobility::Movable);

	auto scopeDynamicMaterial = scopeStaticMeshComponent->CreateAndSetMaterialInstanceDynamicFromMaterial(0, uDiskMaterial);
	auto scopeWidgetDynamicMaterial = scopeWidgetStaticMeshComponent->CreateAndSetMaterialInstanceDynamicFromMaterial(0, uWidgetSightMaterial);
	Logger::DebugPrint("scopeDynamicMaterial created %s", scopeDynamicMaterial->GetFullName().c_str());
	//Applying the render texture to the material :
	scopeDynamicMaterial->SetTextureParameterValue(SDK::UKismetStringLibrary::Conv_StringToName(L"LinearColor"), SniperRenderTarget);
	//SDK::FLinearColor color = {};
	//color.R = color.G = color.B = color.A = 2.0f;
	//scopeDynamicMaterial->SetVectorParameterValue(SDK::UKismetStringLibrary::Conv_StringToName(L"Color"), color);
	Logger::DebugPrint("render target applied to scopeDynamicMaterial");



	ScopeWidgetDynamicMaterial = scopeWidgetDynamicMaterial;
	ScopeCylinderComponent = scopeStaticMeshComponent;
	ScopeWidgetStaticMeshComponent = scopeWidgetStaticMeshComponent;
	Logger::DebugPrint("Sniper Cylinder Mesh spawned");
}

void WeaponController::spawnSceneCaptureComponent()
{
	SDK::FTransform transform{};
	SceneCaptureUStaticMeshComponent = (SDK::UStaticMeshComponent*)ScopeActor->AddComponentByClass(SDK::UStaticMeshComponent::StaticClass(), false, transform, false);
	SceneCaptureComponent = (SDK::USceneCaptureComponent2D*)ScopeActor->AddComponentByClass(SDK::USceneCaptureComponent2D::StaticClass(), false, transform, false);

	if (!SceneCaptureUStaticMeshComponent || !SceneCaptureComponent)
	{
		Logger::DebugPrint("failed to spawn scene capture mesh and component");
		return;
	}
	SceneCaptureComponent->TextureTarget = SniperRenderTarget;
	SceneCaptureComponent->bCaptureOnMovement = false;
	SceneCaptureComponent->bCaptureEveryFrame = true;
	SceneCaptureComponent->bUseRayTracingIfEnabled = false;
	//SceneCaptureComponent->PrimitiveRenderMode = SDK::ESceneCapturePrimitiveRenderMode::PRM_LegacySceneCapture;
	//SceneCaptureComponent->CaptureSource = SDK::ESceneCaptureSource::SCS_SceneColorHDR;
	SceneCaptureComponent->bAlwaysPersistRenderingState = true;
	SceneCaptureComponent->bCaptureEveryFrame = 1;
	SceneCaptureComponent->PostProcessSettings.bOverride_MotionBlurAmount = true;
	SceneCaptureComponent->PostProcessSettings.MotionBlurAmount = 0.0; //Disable motion blur
	SceneCaptureComponent->PostProcessSettings.bOverride_ScreenSpaceReflectionIntensity = true;
	SceneCaptureComponent->PostProcessSettings.ScreenSpaceReflectionIntensity = 0.0; //Disable screen space reflections
	SceneCaptureComponent->PostProcessSettings.bOverride_AmbientOcclusionIntensity = true;
	SceneCaptureComponent->PostProcessSettings.AmbientOcclusionIntensity = 0.0; //Disable ambient occlusion
	SceneCaptureComponent->PostProcessSettings.bOverride_BloomIntensity = true;
	SceneCaptureComponent->PostProcessSettings.BloomIntensity = 0.0;
	SceneCaptureComponent->PostProcessSettings.bOverride_LensFlareIntensity = true;
	SceneCaptureComponent->PostProcessSettings.LensFlareIntensity = 0.0; //Disable lens flares
	SceneCaptureComponent->PostProcessSettings.bOverride_VignetteIntensity = true;
	SceneCaptureComponent->PostProcessSettings.VignetteIntensity = 0.0;//Disable vignette
	SceneCaptureComponent->SetVisibility(false, false);
	SceneCaptureComponent->SetMobility(SDK::EComponentMobility::Movable);

	SceneCaptureUStaticMeshComponent->SetVisibility(false, false);
	SceneCaptureUStaticMeshComponent->SetMobility(SDK::EComponentMobility::Movable);

	//auto cigarMesh = uevr::API::get()->find_uobject<uevr::API::UObject>(L"StaticMesh /Game/Art/Environment/HD_GameplayAssets/Mesh/ciga_cigar_dmo_hd.ciga_cigar_dmo_hd");
	//SceneCaptureUStaticMeshComponent->SetStaticMesh((SDK::UStaticMesh*)cigarMesh);
}

void WeaponController::spawnScopeWidget()
{
	//Fetch existing widget :
	auto CSightDragnovWidget = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/CobraUI.CSightDragnovWidget");
	//auto SightDragnovWidget_C = uevr::API::get()->find_uobject<uevr::API::UClass>(L"WidgetBlueprintGeneratedClass /CobraUI/Blueprint/Sight/SightDragnovWidget.SightDragnovWidget_C");
	auto widgets = CSightDragnovWidget->get_objects_matching();

	SDK::UUserWidget* sourceWidget = (SDK::UUserWidget*)widgets[1];
	SDK::FVector2D drawSize = {2100.0f, 2100.0f};
	//for (int i = 0; i < widgets.size(); i++)
	//{
	//	auto child = (SDK::UUserWidget*)widgets[i];
	//	if (!child) continue;
	//	Logger::DebugPrint(child->GetFullName().c_str());
	//	auto sizeTemp = child->GetDesiredSize();
	//	Logger::DebugPrint("Widget Desired Size x : %f, y : %f", sizeTemp.X, sizeTemp.Y);
	//	if (sizeTemp.X == 0.0f) continue;
	//	sourceWidget = child;
	//	drawSize = sizeTemp;
	//	Logger::DebugPrint(sourceWidget->GetFullName().c_str());
	//	Logger::DebugPrint("Widget Desired Size x : %f, y : %f", drawSize.X, drawSize.Y);
	//}

	if (!sourceWidget)
	{
		Logger::DebugPrint("Failed to find the existing widget");
		return;
	}
	Logger::DebugPrint(sourceWidget->GetFullName().c_str());
	Logger::DebugPrint("Widget Desired Size x : %f, y : %f", drawSize.X, drawSize.Y);
	
	SDK::FTransform transform{};
	auto widgetComponent = (SDK::UWidgetComponent*)ScopeActor->AddComponentByClass(SDK::UWidgetComponent::StaticClass(), false, transform, false);
	
	if (widgetComponent == nullptr)
	{
		Logger::DebugPrint("Failed to create actor for widget creation");
		return;
	}

	sourceWidget->RemoveFromViewport();
	widgetComponent->SetWidget(sourceWidget);
	widgetComponent->SetTwoSided(false);
	widgetComponent->SetDrawSize(drawSize);
	widgetComponent->BlendMode = SDK::EWidgetBlendMode::Transparent;
	//Logger::DebugPrint("Blend mode : %i", (int)widgetComponent->BlendMode);
	sourceWidget->RenderOpacity = 0.0f;
	sourceWidget->ColorAndOpacity.A = 0.0f;
	sourceWidget->ForegroundColor.SpecifiedColor.A = 0.0f;
	SourceScopeWidget = (SDK::UCSightDragnovWidget*)sourceWidget;
	ScopeWidget = widgetComponent;
	Logger::DebugPrint(ScopeWidget->GetFullName().c_str());
	//ScopeWidget = (SDK::UWidgetComponent*)uObject;
}

void WeaponController::attachScopeToWeapon()
{
	if (!ReceiverMesh){	Logger::DebugPrint("attachScopeToWeapon() : ReceiverMesh not found"); return; }
	if (!SceneCaptureComponent){	Logger::DebugPrint("attachScopeToWeapon() : SceneCaptureComponent not found"); return; }
	if (!SceneCaptureUStaticMeshComponent){	Logger::DebugPrint("attachScopeToWeapon() : SceneCaptureComponentMesh not found"); return; }
	if (!ScopeCylinderComponent){	Logger::DebugPrint("attachScopeToWeapon() : ScopeCylinderComponent not found"); return; }
	//if (!ReceiverMesh){	Logger::DebugPrint("attachScopeToWeapon() : ReceiverMesh not found"); return; }

	Logger::DebugPrint("Attaching scope to weapon");
	SceneCaptureUStaticMeshComponent->K2_AttachToComponent(
		ReceiverMesh,
		SDK::UKismetStringLibrary::Conv_StringToName(L"CNP_MUZZLE_FLASH"),
		SDK::EAttachmentRule::SnapToTarget,
		SDK::EAttachmentRule::SnapToTarget,
		SDK::EAttachmentRule::KeepRelative,
		true
	);

	SceneCaptureComponent->K2_AttachToComponent(
		SceneCaptureUStaticMeshComponent,
		SDK::UKismetStringLibrary::Conv_StringToName(L"None"),
		SDK::EAttachmentRule::SnapToTarget,
		SDK::EAttachmentRule::SnapToTarget,
		SDK::EAttachmentRule::KeepRelative,
		true
	);

	ScopeCylinderComponent->K2_AttachToComponent(		
		ReceiverMesh,
		SDK::UKismetStringLibrary::Conv_StringToName(L"CNP_REAR_SIGHT"),
		SDK::EAttachmentRule::SnapToTarget,
		SDK::EAttachmentRule::SnapToTarget,
		SDK::EAttachmentRule::KeepRelative,
		true
	);

	ScopeWidgetStaticMeshComponent->K2_AttachToComponent(
		ReceiverMesh,
		SDK::UKismetStringLibrary::Conv_StringToName(L"CNP_REAR_SIGHT"),
		SDK::EAttachmentRule::SnapToTarget,
		SDK::EAttachmentRule::SnapToTarget,
		SDK::EAttachmentRule::KeepRelative,
		true
	);

	//ScopeWidget->K2_AttachToComponent(
	//	ScopeCylinderComponent,
	//	SDK::UKismetStringLibrary::Conv_StringToName(L"None"),
	//	SDK::EAttachmentRule::SnapToTarget,
	//	SDK::EAttachmentRule::SnapToTarget,
	//	SDK::EAttachmentRule::KeepRelative,
	//	true
	//);

	SDK::FHitResult hitResult;
	SceneCaptureComponent->SetVisibility(true, true);
	SceneCaptureComponent->SetMobility(SDK::EComponentMobility::Movable);
	SceneCaptureComponent->Activate(true);
	SceneCaptureComponent->K2_SetRelativeRotation({0.0f, 90.0f, 0.0f}, false, &hitResult, true);

	ScopeCylinderComponent->K2_SetRelativeLocation({ 0.0f, -26.0f, 0.0f }, false, &hitResult, true);
	ScopeCylinderComponent->K2_SetRelativeRotation({ 180.0f, 0.0f, -90.0f }, false, &hitResult, true);
	//ScopeCylinderComponent->SetRelativeScale3D({0.15f, 0.15f, 0.15f});
	ScopeCylinderComponent->SetRelativeScale3D({2.0f, 2.0f, 2.0f});
	ScopeCylinderComponent->SetVisibility(false, false);

	ScopeWidgetStaticMeshComponent->K2_SetRelativeLocation({ 0.0f, -26.0f, 0.0f }, false, &hitResult, true);
	ScopeWidgetStaticMeshComponent->K2_SetRelativeRotation({ 180.0f, 0.0f, -90.0f }, false, &hitResult, true);
	//ScopeWidgetStaticMeshComponent->SetRelativeScale3D({0.15f, 0.15f, 0.15f});
	ScopeWidgetStaticMeshComponent->SetRelativeScale3D({2.0f, 2.0f, 2.0f});
	ScopeWidgetStaticMeshComponent->SetVisibility(false, false);

	ScopeWidget->K2_SetRelativeLocation({ 0.0f, 0.0f, 1.0f }, false, &hitResult, true);
	ScopeWidget->K2_SetRelativeRotation({ 90.0f, 0.0f, -90.0f }, false, &hitResult, true);
	//ScopeCylinderComponent->SetRelativeScale3D({0.15f, 0.15f, 0.15f});
	ScopeWidget->SetRelativeScale3D({0.02f, 0.02f, 0.02f});
}



