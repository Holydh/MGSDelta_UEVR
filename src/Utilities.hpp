#pragma once
#include "uevr/API.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "Logger.hpp"
#include "ue/Reflection.hpp"

namespace Utilities {
	inline uevr::API::UClass* ActorClass = nullptr;
	inline uevr::API::UClass* SceneComponentClass = nullptr;

	//Animation states classes (Blueprint classes: resolved when first used, they may not be loaded at startup)
	inline constinit ue::ClassRef ABP_SnakeBasicAction_C{ L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_SnakeBasicAction.ABP_SnakeBasicAction_C" };
	inline constinit ue::ClassRef ABP_PlayerEludeAction_C{ L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerEludeAction.ABP_PlayerEludeAction_C" };
	inline constinit ue::ClassRef ABP_PlayerRollingAction_C{ L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerRollingAction.ABP_PlayerRollingAction_C" };
	inline constinit ue::ClassRef ABP_PlayerBehindAction_C{ L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerBehindAction.ABP_PlayerBehindAction_C" };
	inline constinit ue::ClassRef ABP_PlayerKnifeReadyAction_C{ L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerKnifeReadyAction.ABP_PlayerKnifeReadyAction_C" };
	inline constinit ue::ClassRef ABP_PlayerMeleeAction_C{ L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerMeleeAction.ABP_PlayerMeleeAction_C" };
	inline constinit ue::ClassRef ABP_PlayerCarryAction_C{ L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerCarryAction.ABP_PlayerCarryAction_C" };
	inline constinit ue::ClassRef ABP_EnemyCqc_C{ L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Enemy/Animation/Cqc/ABP_EnemyCqc.ABP_EnemyCqc_C" };

	inline constinit ue::ClassRef GsrPlayerBasicAction_c{ L"Class /Script/Gsr.GsrPlayerBasicAction" };

	void InitHelperClasses();
	uevr::API::UObject* SpawnActor(UEVR_Vector3d Position, uevr::API::UObject* Owner);
	// A visible mesh component without collision, attached to the actor's root. Null if it couldn't be created.
	ue::UObject* AddMeshComponent(ue::UObject* actor, ue::UObject* mesh, bool isStaticMesh);
	uevr::API::UObject* SpawnMesh(const ue::FTransform& worldTransform, std::wstring_view meshFullName, uevr::API::UObject*& outMeshComponent);

	ue::UObject* FirstObjectOf(ue::ClassRef& cls);

	float Remap(float value, float inMin, float inMax, float outMin, float outMax);
}
