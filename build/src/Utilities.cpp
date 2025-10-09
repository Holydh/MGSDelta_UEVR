#include "Utilities.hpp"

uevr::API::UObject* Utilities::KismetMathLibrary = nullptr;
uevr::API::UObject* Utilities::KismetStringLibrary = nullptr;
uevr::API::UObject* Utilities::GameplayStatics = nullptr;
uevr::API::UClass* Utilities::ActorClass = nullptr;
uevr::API::UClass* Utilities::SceneComponentClass = nullptr;
uevr::API::UClass* Utilities::ABP_SnakeBasicAction_C = nullptr;
uevr::API::UClass* Utilities::ABP_PlayerEludeAction_C = nullptr;
uevr::API::UClass* Utilities::ABP_PlayerRollingAction_C = nullptr;
uevr::API::UClass* Utilities::ABP_PlayerBehindAction_C = nullptr;

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
	Logger::DebugPrint(abp_PlayerEludeAction_c->get_full_name());
	ABP_PlayerEludeAction_C = abp_PlayerEludeAction_c;

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

uevr::API::UObject* Utilities::SpawnActor(uevr::API::UObject* WorldContextObject, UEVR_Vector3d Position, uint8_t CollisionMethod, uevr::API::UObject* Owner)
{
	SDK::FTransform SpawnTransform;
	SpawnTransform.Translation = {Position.x, Position.y, Position.z};
	SpawnTransform.Rotation = { 0, 0, 0, 1 };
	SpawnTransform.Scale3D = { 1, 1, 1 };

	SDK::Params::GameplayStatics_BeginDeferredActorSpawnFromClass params;
	params.WorldContextObject = (SDK::UObject*)WorldContextObject;
	params.ActorClass = (SDK::UClass*)ActorClass;
	params.SpawnTransform = SpawnTransform;
	params.CollisionHandlingOverride = (SDK::ESpawnActorCollisionHandlingMethod)CollisionMethod; // 1 = AdjustIfPossibleButAlwaysSpawn
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
	uevr::API::get()->sdk()->functions->add_component_by_class(actor->to_handle(), SceneComponentClass->to_handle(), false);
	return actor;
}

//glm::fvec3 Utilities::OffsetLocalPositionFromWorld(glm::fvec3 worldPosition, glm::fvec3 forwardVector, glm::fvec3 upVector, glm::fvec3 rightVector, glm::fvec3 offsets)
//{
//	return worldPosition + (forwardVector * offsets.x) + (rightVector * offsets.y) + (upVector * offsets.z);
//}