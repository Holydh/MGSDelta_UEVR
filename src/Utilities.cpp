#include "Utilities.hpp"

uevr::API::UObject* Utilities::KismetMathLibrary = nullptr;
uevr::API::UObject* Utilities::KismetStringLibrary = nullptr;
uevr::API::UObject* Utilities::GameplayStatics = nullptr;
uevr::API::UClass* Utilities::ActorClass = nullptr;
uevr::API::UClass* Utilities::SceneComponentClass = nullptr;

uevr::API::UObject* Utilities::DebugMesh0 = nullptr;
uevr::API::UObject* Utilities::DebugMesh1 = nullptr;

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
	KismetStringLibrary = kismetStringLibrary_c->get_class_default_object();

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

	//uevr::API::get()->log_info("KismetMathLibrary: %p", KismetMathLibrary);
	//Logger::DebugPrint("%s", "KismethMathLibraryFound");
}

uevr::API::UObject* Utilities::SpawnActor(UEVR_Vector3d Position, uevr::API::UObject* Owner)
{
	auto WorldContextObject = ue::get_world();
	if (!WorldContextObject || !ActorClass || !SceneComponentClass)
	{
		Logger::DebugPrint("SpawnActor: no world, or helper classes not found");
		return nullptr;
	}
	Logger::DebugPrint(WorldContextObject->get_full_name());
	const ue::FTransform SpawnTransform{ ue::FQuat::Identity(), { Position.x, Position.y, Position.z }, { 1, 1, 1 } };

	auto deferredActor = ue::call_static<ue::UObject*>(ue::lib::GameplayStatics, L"BeginDeferredActorSpawnFromClass",
		WorldContextObject,
		ActorClass,
		SpawnTransform,
		ue::ESpawnActorCollisionHandlingMethod::AlwaysSpawn,
		Owner,
		ue::ESpawnActorScaleMethod::OverrideRootScale);
	if (!deferredActor)
	{
		Logger::DebugPrint("BeginDeferredActorSpawnFromClass failed");
		return nullptr;
	}

	auto actor = ue::call_static<ue::UObject*>(ue::lib::GameplayStatics, L"FinishSpawningActor",
		deferredActor,
		SpawnTransform,
		ue::ESpawnActorScaleMethod::OverrideRootScale);
	if (!actor)
	{
		Logger::DebugPrint("FinishSpawningActor failed");
		return nullptr;
	}
	Logger::DebugPrint("Actor Spawned");
	Logger::DebugPrint(actor->get_full_name());
	uevr::API::get()->sdk()->functions->add_component_by_class(actor->to_handle(), SceneComponentClass->to_handle(), false);
	return actor;
}

uevr::API::UObject* Utilities::SpawnMesh(const ue::FTransform& worldTransform, std::wstring_view meshFullName, uevr::API::UObject*& outMeshComponent)
{
	static ue::ClassRef StaticMesh_c{ L"Class /Script/Engine.StaticMesh" };
	static ue::ClassRef SkeletalMesh_c{ L"Class /Script/Engine.SkeletalMesh" };
	static ue::ClassRef StaticMeshComponent_c{ L"Class /Script/Engine.StaticMeshComponent" };
	static ue::ClassRef SkeletalMeshComponent_c{ L"Class /Script/Engine.SkeletalMeshComponent" };

	// 1. Find the mesh asset already loaded in memory. Checked before spawning the actor: a failure
	// must not leave an actor behind (the caller retries every frame).
	auto mesh = ue::find_object(meshFullName);
	if (!mesh)
	{
		Logger::DebugPrint(L"Mesh asset not found: " + std::wstring{ meshFullName });
		return nullptr;
	}

	const bool isStaticMesh = ue::is_a(mesh, StaticMesh_c);
	const bool isSkeletalMesh = !isStaticMesh && ue::is_a(mesh, SkeletalMesh_c);
	if (isStaticMesh)
	{
		Logger::DebugPrint(L"Found static mesh : " + mesh->get_full_name());
	}
	else if (isSkeletalMesh)
	{
		Logger::DebugPrint(L"Found skeletal mesh : " + mesh->get_full_name());
	}
	else
	{
		Logger::DebugPrint("Asset found is not a StaticMesh or SkeletalMesh");
		return nullptr;
	}

	UEVR_Vector3d uevrVector = { worldTransform.Translation.X, worldTransform.Translation.Y, worldTransform.Translation.Z };
	auto actor = SpawnActor(uevrVector, nullptr);
	if (!actor)
	{
		return nullptr;
	}

	// 2. Create a new mesh component as a sub-object of the Actor
	// This makes the Actor the "owner" of the new component.
	auto meshComponent = ue::call<ue::UObject*>(actor, L"AddComponentByClass",
		isStaticMesh ? StaticMeshComponent_c.get() : SkeletalMeshComponent_c.get(),
		false,            // bManualAttachment
		ue::FTransform{}, // RelativeTransform
		false);           // bDeferredFinish

	if (!meshComponent)
	{
		Logger::DebugPrint("Failed to create mesh component");
		ue::call(actor, L"K2_DestroyActor");
		return nullptr;
	}
	Logger::DebugPrint(L"Created mesh component: " + meshComponent->get_full_name());

	// 3. Assign the mesh asset to our new component instance
	if (isStaticMesh)
		ue::call(meshComponent, L"SetStaticMesh", mesh);
	else
		ue::call(meshComponent, L"SetSkeletalMeshAsset", mesh);
	Logger::DebugPrint("Assigned mesh to component");

	// 4. Attach the new component to the Actor's root component
	ue::call(meshComponent, L"K2_AttachToComponent",
		ue::call<ue::UObject*>(actor, L"K2_GetRootComponent"), // Parent
		ue::FName{},                                          // SocketName (none)
		ue::EAttachmentRule::KeepRelative,                    // Use the relative location we just set
		ue::EAttachmentRule::KeepRelative,                    // Use the relative rotation we just set
		ue::EAttachmentRule::KeepRelative,                    // Use the relative scale we just set
		true);                                                // bWeldSimulatedBodies
	Logger::DebugPrint("Attached mesh component to Actor's root component");

	// 5. Register the component to make it "live" and renderable in the world
	ue::call(meshComponent, L"Activate", true);
	Logger::DebugPrint("Activated mesh component");

	// Optional: Disable collision if it's just for visuals
	ue::call(meshComponent, L"SetCollisionEnabled", ue::ECollisionEnabled::NoCollision);

	ue::call(meshComponent, L"Activate", true);
	ue::call(meshComponent, L"SetVisibility", true, false);
	ue::call(meshComponent, L"SetHiddenInGame", false, false);
	ue::call(meshComponent, L"SetRenderCustomDepth", true);
	ue::call(meshComponent, L"SetWorldScale3D", ue::FVector{ 1, 1, 1 });
	outMeshComponent = meshComponent;

	ue::FHitResult outHit;
	ue::call(actor, L"K2_SetActorTransform", worldTransform, false, &outHit, true);

	Logger::DebugPrint("Disabled collision on mesh component");
	// Store the actor reference for later use (e.g., to destroy it)
	Logger::DebugPrint("Successfully spawned debug mesh component!\n");
	return actor;
}

void Utilities::DestroyDebugMeshes()
{
	if (DebugMesh0)
	{
		ue::call(DebugMesh0, L"K2_DestroyActor");
		DebugMesh0 = nullptr;
	}
	if (DebugMesh1)
	{
		ue::call(DebugMesh1, L"K2_DestroyActor");
		DebugMesh1 = nullptr;
	}
}

float Utilities::Remap(float value, float inMin, float inMax, float outMin, float outMax)
{
    float t = glm::clamp((value - inMin) / (inMax - inMin), 0.0f, 1.0f);
    return glm::mix(outMin, outMax, t);
}

glm::quat Utilities::FRotatorToQuat(const ue::FRotator& rot)
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

ue::FRotator Utilities::QuatToFRotator(glm::quat quaternion)
{
	glm::vec3 e = glm::eulerAngles(quaternion);  // returns radians
    ue::FRotator out;

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
