#include "Utilities.hpp"

uevr::API::UObject* Utilities::KismetMathLibrary = nullptr;
uevr::API::UObject* Utilities::KismetStringLibrary = nullptr;
uevr::API::UObject* Utilities::GameplayStatics = nullptr;
uevr::API::UClass* Utilities::ActorClass = nullptr;
uevr::API::UClass* Utilities::SceneComponentClass = nullptr;

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
	//uevr::API::get()->log_info("KismetMathLibrary: %p", KismetMathLibrary);
	//Logger::DebugPrint("%s", "KismethMathLibraryFound");
}

uevr::API::UObject* Utilities::SpawnActor(uevr::API::UObject* WorldContextObject, UEVR_Vector3d Position, uint8_t CollisionMethod, uevr::API::UObject* Owner)
{
	Utilities::FTransform SpawnTransform;
	SpawnTransform.Translation = Position;
	SpawnTransform.Rotation = { 0, 0, 0, 1 };
	SpawnTransform.Scale3D = { 1, 1, 1 };

	Utilities::GameplayStatics_BeginDeferredActorSpawnFromClass params;
	params.WorldContextObject = WorldContextObject;
	params.ActorClass = ActorClass;
	params.SpawnTransform = SpawnTransform;
	params.CollisionHandlingOverride = CollisionMethod; // 1 = AdjustIfPossibleButAlwaysSpawn
	params.Owner = Owner;
	params.TransformScaleMethod = 0; // 0 = None

	GameplayStatics->call_function(L"BeginDeferredActorSpawnFromClass", &params);
	if (!params.ReturnValue)
	{
		Logger::DebugPrint("BeginDeferredActorSpawnFromClass failed");
		return nullptr;
	}
	
	Utilities::GameplayStatics_FinishSpawningActor finishParams;
	finishParams.Actor = params.ReturnValue;
	finishParams.SpawnTransform = SpawnTransform;
	finishParams.TransformScaleMethod = 0; // 0 = None

	Utilities::GameplayStatics->call_function(L"FinishSpawningActor", &finishParams);
	Logger::DebugPrint("Actor Spawned");
	//if (finishParams.ReturnValue)
	//{
	//	Logger::DebugPrint(finishParams.ReturnValue->get_full_name());
	//}
	
	uevr::API::get()->sdk()->functions->add_component_by_class(finishParams.ReturnValue->to_handle(), SceneComponentClass->to_handle(), false);
	return finishParams.ReturnValue;
}

//glm::fvec3 Utilities::OffsetLocalPositionFromWorld(glm::fvec3 worldPosition, glm::fvec3 forwardVector, glm::fvec3 upVector, glm::fvec3 rightVector, glm::fvec3 offsets)
//{
//	return worldPosition + (forwardVector * offsets.x) + (rightVector * offsets.y) + (upVector * offsets.z);
//}