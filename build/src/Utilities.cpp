#include "Utilities.hpp"

uevr::API::UObject* Utilities::KismetMathLibrary = nullptr;
uevr::API::UObject* Utilities::KismetStringLibrary = nullptr;
uevr::API::UObject* Utilities::GameplayStatics = nullptr;
uevr::API::UClass* Utilities::ActorClass = nullptr;
uevr::API::UClass* Utilities::SceneComponentClass = nullptr;
uevr::API::UClass* Utilities::ABP_SnakeBasicAction_C = nullptr;
uevr::API::UClass* Utilities::ABP_PlayerEludeAction_C = nullptr;
uevr::API::UClass* Utilities::ABP_PlayerKnifeReadyAction_C = nullptr;
uevr::API::UClass* Utilities::ABP_PlayerRollingAction_C = nullptr;
uevr::API::UClass* Utilities::ABP_PlayerBehindAction_C = nullptr;
uevr::API::UClass* Utilities::ABP_PlayerMeleeAction_C = nullptr;
uevr::API::UClass* Utilities::ABP_PlayerCarryAction_C = nullptr;
uevr::API::UClass* Utilities::ABP_EnemyCqc_C = nullptr;

SDK::AActor* Utilities::DebugMesh0 = nullptr;
SDK::AActor* Utilities::DebugMesh1 = nullptr;

void Utilities::InitHelperClasses()
{
	static auto kismetMathLibrary_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Engine.KismetMathLibrary");
	if (!kismetMathLibrary_c)
	{
		uevr::API::get()->log_error("kismetMathLibrary_c not found");
		return;
	}
	KismetMathLibrary = kismetMathLibrary_c->get_class_default_object();

	static auto kismetStringLibrary_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Engine.KismetStringLibrary");
	if (!kismetStringLibrary_c)
	{
		uevr::API::get()->log_error("kismetStringLibrary_c not found");
		return;
	}
	KismetStringLibrary = kismetMathLibrary_c->get_class_default_object();

	static auto gameplayStatics_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Engine.GameplayStatics");
	if (!gameplayStatics_c)
	{
		uevr::API::get()->log_error("gameplayStatics_c not found");
		return;
	}
	GameplayStatics = gameplayStatics_c->get_class_default_object();
	//Logger::DebugPrint(GameplayStatics->get_full_name());

	static auto actor_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Engine.Actor");
	if (!actor_c)
	{
		uevr::API::get()->log_error("actor_c not found");
		return;
	}
	ActorClass = actor_c;

	static auto sceneComponent_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Engine.SceneComponent");
	if (!sceneComponent_c)
	{
		uevr::API::get()->log_error("sceneComponent_c not found");
		return;
	}
	SceneComponentClass = sceneComponent_c;

	static auto abp_SnakeBasicAction_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_SnakeBasicAction.ABP_SnakeBasicAction_C");
	if (!abp_SnakeBasicAction_c)
	{
		uevr::API::get()->log_error("abp_SnakeBasicAction_c not found");
		return;
	}
	ABP_SnakeBasicAction_C = abp_SnakeBasicAction_c;

	static auto abp_PlayerEludeAction_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerEludeAction.ABP_PlayerEludeAction_C");
	if (!abp_PlayerEludeAction_c)
	{
		uevr::API::get()->log_error("abp_PlayerEludeAction_c not found");
		return;
	}
	//Logger::DebugPrint(abp_PlayerEludeAction_c->get_full_name());
	ABP_PlayerEludeAction_C = abp_PlayerEludeAction_c;

	static auto abp_PlayerKnifeReadyAction_C = uevr::API::get()->find_uobject<uevr::API::UClass>(L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerKnifeReadyAction.ABP_PlayerKnifeReadyAction_C");
	if (!abp_PlayerKnifeReadyAction_C)
	{
		uevr::API::get()->log_error("abp_PlayerKnifeReadyAction_C not found");
		return;
	}
	ABP_PlayerKnifeReadyAction_C = abp_PlayerKnifeReadyAction_C;

	static auto abp_PlayerMeleeAction_C = uevr::API::get()->find_uobject<uevr::API::UClass>(L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerMeleeAction.ABP_PlayerMeleeAction_C");
	if (!abp_PlayerMeleeAction_C)
	{
		uevr::API::get()->log_error("abp_PlayerKnifeReadyAction_C not found");
		return;
	}
	ABP_PlayerMeleeAction_C = abp_PlayerMeleeAction_C;

	static auto abp_PlayerCarryAction_C = uevr::API::get()->find_uobject<uevr::API::UClass>(L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerCarryAction.ABP_PlayerCarryAction_C");
	if (!abp_PlayerCarryAction_C)
	{
		uevr::API::get()->log_error("abp_PlayerCarryAction_C not found");
		return;
	}
	ABP_PlayerCarryAction_C = abp_PlayerCarryAction_C;

	static auto abp_EnemyCqc_C = uevr::API::get()->find_uobject<uevr::API::UClass>(L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Enemy/Animation/Cqc/ABP_EnemyCqc.ABP_EnemyCqc_C");
	if (!abp_EnemyCqc_C)
	{
		uevr::API::get()->log_error("abp_EnemyCqc_C not found");
		return;
	}
	ABP_EnemyCqc_C = abp_EnemyCqc_C;

	static auto abp_PlayerRollingAction_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerRollingAction.ABP_PlayerRollingAction_C");
	if (!abp_PlayerRollingAction_c)
	{
		uevr::API::get()->log_error("abp_PlayerRollingAction_c not found");
		return;
	}
	ABP_PlayerRollingAction_C = abp_PlayerRollingAction_c;

	static auto abp_PlayerBehindAction_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerBehindAction.ABP_PlayerBehindAction_C");
	if (!abp_PlayerBehindAction_c)
	{
		uevr::API::get()->log_error("abp_PlayerBehindAction_c not found");
		return;
	}
	ABP_PlayerBehindAction_C = abp_PlayerBehindAction_c;
	//uevr::API::get()->log_info("KismetMathLibrary: %p", KismetMathLibrary);
	//Logger::DebugPrint("%s", "KismethMathLibraryFound");
}

uevr::API::UObject* Utilities::SpawnActor(UEVR_Vector3d Position, uevr::API::UObject* Owner)
{
	auto gameEngine = uevr::API::get()->get_engine();
	auto WorldContextObject = gameEngine->get_property<uevr::API::UObject*>(L"GameViewport")->get_property<uevr::API::UObject*>(L"World");
	Logger::DebugPrint(WorldContextObject->get_full_name());
	SDK::FTransform SpawnTransform;
	SpawnTransform.Translation = { Position.x, Position.y, Position.z };
	SpawnTransform.Rotation = { 0, 0, 0, 1 };
	SpawnTransform.Scale3D = { 1, 1, 1 };

	SDK::Params::GameplayStatics_BeginDeferredActorSpawnFromClass params;
	params.WorldContextObject = (SDK::UObject*)WorldContextObject;
	params.ActorClass = (SDK::UClass*)ActorClass;
	params.SpawnTransform = SpawnTransform;
	params.CollisionHandlingOverride = SDK::ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	params.Owner = (SDK::AActor*)Owner;
	params.TransformScaleMethod = SDK::ESpawnActorScaleMethod::OverrideRootScale;

	GameplayStatics->call_function(L"BeginDeferredActorSpawnFromClass", &params);
	if (!params.ReturnValue)
	{
		Logger::DebugPrint("BeginDeferredActorSpawnFromClass failed");
		return nullptr;
	}

	SDK::Params::GameplayStatics_FinishSpawningActor finishParams;
	finishParams.Actor = params.ReturnValue;
	finishParams.SpawnTransform = SpawnTransform;
	finishParams.TransformScaleMethod = SDK::ESpawnActorScaleMethod::OverrideRootScale;

	Utilities::GameplayStatics->call_function(L"FinishSpawningActor", &finishParams);
	Logger::DebugPrint("Actor Spawned");
	//if (finishParams.ReturnValue)
	//{
	//	Logger::DebugPrint(finishParams.ReturnValue->get_full_name());
	//}
	auto actor = (uevr::API::UObject*)finishParams.ReturnValue;
	Logger::DebugPrint(actor->get_full_name());
	uevr::API::get()->sdk()->functions->add_component_by_class(actor->to_handle(), SceneComponentClass->to_handle(), false);
	return actor;
}

SDK::AActor* Utilities::SpawnMesh(SDK::FTransform worldTransform, std::wstring_view meshFullName, SDK::UObject*& outMeshComponent)
{
	UEVR_Vector3d uevrVector = { worldTransform.Translation.X, worldTransform.Translation.Y, worldTransform.Translation.Z };
	auto uevrActor = SpawnActor(uevrVector, nullptr);
	auto actor = (SDK::AActor*)uevrActor;
	if (!actor)
	{
		return nullptr;
	}
	
    // 1. Find the mesh asset already loaded in memory
    static SDK::UStaticMesh* staticMesh = nullptr;
	static SDK::USkeletalMesh* skeletalMesh = nullptr;
	auto meshUEVR = uevr::API::get()->find_uobject<uevr::API::UObject>(meshFullName);
	auto sdkMesh = (SDK::UObject*)meshUEVR;
    if (sdkMesh->IsA(SDK::UStaticMesh::StaticClass()))
    {
		staticMesh = (SDK::UStaticMesh*)meshUEVR;

        if (!staticMesh)
        {
            Logger::DebugPrint("Failed to find static mesh");
            return nullptr;
		}
		Logger::DebugPrint("Found static mesh %s : ", staticMesh->GetFullName().c_str());
	}
	else if (sdkMesh->IsA(SDK::USkeletalMesh::StaticClass()))
	{
		skeletalMesh = (SDK::USkeletalMesh*)meshUEVR;
		if (!skeletalMesh)
		{
			Logger::DebugPrint("Failed to find skeletal mesh");
			return nullptr;
		}
		Logger::DebugPrint("Found skeletal mesh %s : ", skeletalMesh->GetFullName().c_str());
	}
	else
	{
		Logger::DebugPrint("Asset found is not a StaticMesh or SkeletalMesh");
		return nullptr;
	}

	if (staticMesh)
	{
		// 2. Create a new Static Mesh Component as a sub-object of the Actor
		// This makes the Actor the "owner" of the new component.
		auto* staticMeshComponent = static_cast<SDK::UStaticMeshComponent*>(actor->AddComponentByClass(SDK::UStaticMeshComponent::StaticClass(),
			false, // bManualAttachment
			SDK::FTransform{}, // RelativeTransform
			false // bDeferConstruction
		));

		if (!staticMeshComponent)
		{
			Logger::DebugPrint("Failed to create UStaticMeshComponent");
			return nullptr;
		}
		Logger::DebugPrint("Created StaticMeshComponent: %s", staticMeshComponent->GetFullName().c_str());
		// 3. Assign the mesh asset to our new component instance
		staticMeshComponent->SetStaticMesh(staticMesh);
		Logger::DebugPrint("Assigned mesh to StaticMeshComponent");

		// 4. Attach the new component to the Actor's root component
		staticMeshComponent->K2_AttachToComponent(
			actor->K2_GetRootComponent(),                      // Parent
			SDK::FName(),                                      // SocketName (none)
			SDK::EAttachmentRule::KeepRelative,             // Use the relative location we just set
			SDK::EAttachmentRule::KeepRelative,             // Use the relative rotation we just set
			SDK::EAttachmentRule::KeepRelative,             // Use the relative scale we just set
			true                                            // bWeldSimulatedBodies
		);
		Logger::DebugPrint("Attached StaticMeshComponent to Actor's root component");

		// 6. Register the component to make it "live" and renderable in the world
		staticMeshComponent->Activate(true);
		Logger::DebugPrint("Activated StaticMeshComponent");

		// Optional: Disable collision if it's just for visuals
		staticMeshComponent->SetCollisionEnabled(SDK::ECollisionEnabled::NoCollision);

		staticMeshComponent->Activate(true);
		staticMeshComponent->SetVisibility(true, false);
		staticMeshComponent->SetHiddenInGame(false, false);
		staticMeshComponent->SetRenderCustomDepth(true);
		staticMeshComponent->SetWorldScale3D({ 1, 1, 1 });
		outMeshComponent = (SDK::UObject*)staticMeshComponent;
	}

	if (skeletalMesh)
	{
		// 2. Create a new Static Mesh Component as a sub-object of the Actor
		// This makes the Actor the "owner" of the new component.
		auto* skeletalMeshComponent = static_cast<SDK::USkeletalMeshComponent*>(actor->AddComponentByClass(SDK::USkeletalMeshComponent::StaticClass(),
			false, // bManualAttachment
			SDK::FTransform{}, // RelativeTransform
			false // bDeferConstruction
		));

		if (!skeletalMeshComponent)
		{
			Logger::DebugPrint("Failed to create USkeletalMeshComponent");
			return nullptr;
		}
		Logger::DebugPrint("Created SkeletalMeshComponent: %s", skeletalMeshComponent->GetFullName().c_str());
		// 3. Assign the mesh asset to our new component instance
		skeletalMeshComponent->SetSkeletalMeshAsset(skeletalMesh);
		Logger::DebugPrint("Assigned mesh to SkeletalMeshComponent");

		// 4. Attach the new component to the Actor's root component
		skeletalMeshComponent->K2_AttachToComponent(
			actor->K2_GetRootComponent(),                      // Parent
			SDK::FName(),                                      // SocketName (none)
			SDK::EAttachmentRule::KeepRelative,             // Use the relative location we just set
			SDK::EAttachmentRule::KeepRelative,             // Use the relative rotation we just set
			SDK::EAttachmentRule::KeepRelative,             // Use the relative scale we just set
			true                                            // bWeldSimulatedBodies
		);
		Logger::DebugPrint("Attached SkeletalMeshComponent to Actor's root component");

		// 6. Register the component to make it "live" and renderable in the world
		skeletalMeshComponent->Activate(true);
		Logger::DebugPrint("Activated SkeletalMeshComponent");

		// Optional: Disable collision if it's just for visuals
		skeletalMeshComponent->SetCollisionEnabled(SDK::ECollisionEnabled::NoCollision);

		skeletalMeshComponent->Activate(true);
		skeletalMeshComponent->SetVisibility(true, false);
		skeletalMeshComponent->SetHiddenInGame(false, false);
		skeletalMeshComponent->SetRenderCustomDepth(true);
		skeletalMeshComponent->SetWorldScale3D({ 1, 1, 1 });
		outMeshComponent = (SDK::UObject*)skeletalMeshComponent;
	}
    
	SDK::FHitResult outHit;
	actor->K2_SetActorTransform(worldTransform, false, &outHit, true);

	/*auto defaultMat = uevr::API::get()->find_uobject<uevr::API::UObject*>(L"Material /Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");

	if (defaultMat)
    ArrowComponent->SetMaterial(0, (SDK::UMaterialInterface*)defaultMat);
else
    Logger::DebugPrint("Could not find BasicShapeMaterial");
	ArrowComponent->bForceDisableNanite = true;*/

	Logger::DebugPrint("Disabled collision on StaticMeshComponent");
	// Store the actor reference for later use (e.g., to destroy it)
    Logger::DebugPrint("Successfully spawned debug mesh component!\n");
    return actor;
}

void Utilities::DestroyDebugMeshes()
{
	if (DebugMesh0)
	{
		DebugMesh0->K2_DestroyActor();
		DebugMesh0 = nullptr;
	}
	if (DebugMesh1)
	{
		DebugMesh1->K2_DestroyActor();
		DebugMesh1 = nullptr;
	}
}

float Utilities::Remap(float value, float inMin, float inMax, float outMin, float outMax)
{
    float t = glm::clamp((value - inMin) / (inMax - inMin), 0.0f, 1.0f);
    return glm::mix(outMin, outMax, t);
}

glm::quat Utilities::FRotatorToQuat(SDK::FRotator rot)
{
    float pitch = glm::radians(rot.Pitch);
    float yaw   = glm::radians(rot.Yaw);
    float roll  = glm::radians(rot.Roll);

    // Unreal order: Pitch(Y) -> Yaw(Z) -> Roll(X)
    glm::quat qPitch = glm::angleAxis(pitch, glm::vec3(1, 0, 0));   // Actually X, but matches UE's coordinate handedness when modding
    glm::quat qYaw   = glm::angleAxis(yaw,   glm::vec3(0, 0, 1));
    glm::quat qRoll  = glm::angleAxis(roll,  glm::vec3(0, 1, 0));

    // UE applies: Rot = QYaw * QPitch * QRoll
    return qYaw * qPitch * qRoll;
}

SDK::FRotator Utilities::QuatToFRotator(glm::quat quaternion)
{
	glm::vec3 e = glm::eulerAngles(quaternion);  // returns radians
    SDK::FRotator out;

    // Map back to UE Rotator fields
    out.Pitch = glm::degrees(e.x);
    out.Yaw   = glm::degrees(e.y);
    out.Roll  = glm::degrees(e.z);

    return out;
}


//glm::fvec3 Utilities::OffsetLocalPositionFromWorld(glm::fvec3 worldPosition, glm::fvec3 forwardVector, glm::fvec3 upVector, glm::fvec3 rightVector, glm::fvec3 offsets)
//{
//	return worldPosition + (forwardVector * offsets.x) + (rightVector * offsets.y) + (upVector * offsets.z);
//}