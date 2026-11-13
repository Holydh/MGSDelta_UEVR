#pragma once
#include "uevr/API.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "Logger.hpp"
#include "ue/Reflection.hpp"

class Utilities {
private:

public:
	static uevr::API::UClass* ActorClass;
	static uevr::API::UClass* SceneComponentClass;

	//Animation states classes (Blueprint classes: resolved when first used, they may not be loaded at startup)
	static inline constinit ue::ClassRef ABP_SnakeBasicAction_C{ L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_SnakeBasicAction.ABP_SnakeBasicAction_C" };
	static inline constinit ue::ClassRef ABP_PlayerEludeAction_C{ L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerEludeAction.ABP_PlayerEludeAction_C" };
	static inline constinit ue::ClassRef ABP_PlayerRollingAction_C{ L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerRollingAction.ABP_PlayerRollingAction_C" };
	static inline constinit ue::ClassRef ABP_PlayerBehindAction_C{ L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerBehindAction.ABP_PlayerBehindAction_C" };
	static inline constinit ue::ClassRef ABP_PlayerKnifeReadyAction_C{ L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerKnifeReadyAction.ABP_PlayerKnifeReadyAction_C" };
	static inline constinit ue::ClassRef ABP_PlayerMeleeAction_C{ L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerMeleeAction.ABP_PlayerMeleeAction_C" };
	static inline constinit ue::ClassRef ABP_PlayerCarryAction_C{ L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerCarryAction.ABP_PlayerCarryAction_C" };
	static inline constinit ue::ClassRef ABP_EnemyCqc_C{ L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Enemy/Animation/Cqc/ABP_EnemyCqc.ABP_EnemyCqc_C" };

	static inline constinit ue::ClassRef GsrPlayerBasicAction_c{ L"Class /Script/Gsr.GsrPlayerBasicAction" };

	static void InitHelperClasses();
	static uevr::API::UObject* SpawnActor(UEVR_Vector3d Position, uevr::API::UObject* Owner);
	// A visible mesh component without collision, attached to the actor's root. Null if it couldn't be created.
	static ue::UObject* AddMeshComponent(ue::UObject* actor, ue::UObject* mesh, bool isStaticMesh);
	static uevr::API::UObject* SpawnMesh(const ue::FTransform& worldTransform, std::wstring_view meshFullName, uevr::API::UObject*& outMeshComponent);

	static ue::UObject* FirstObjectOf(ue::ClassRef& cls);

	static float Remap(float value, float inMin, float inMax, float outMin, float outMax);
};
