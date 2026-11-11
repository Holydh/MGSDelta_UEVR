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
	static uevr::API::UObject* KismetMathLibrary;
	static uevr::API::UObject* KismetStringLibrary;
	static uevr::API::UObject* GameplayStatics;
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

	//Debug Arrow Actor
	static uevr::API::UObject* DebugMesh0;
	static uevr::API::UObject* DebugMesh1;

	static void InitHelperClasses();
	static uevr::API::UObject* SpawnActor(UEVR_Vector3d Position, uevr::API::UObject* Owner);
	static uevr::API::UObject* SpawnMesh(const ue::FTransform& worldTransform, std::wstring_view meshFullName, uevr::API::UObject*& outMeshComponent);
	static void DestroyDebugMeshes();

	static float Remap(float value, float inMin, float inMax, float outMin, float outMax);
	static glm::quat FRotatorToQuat(const ue::FRotator& rotator);
	static ue::FRotator QuatToFRotator(glm::quat quaternion);
	//static glm::fvec3 OffsetLocalPositionFromWorld(glm::fvec3 worldPosition, glm::fvec3 forwardVector, glm::fvec3 upVector, glm::fvec3 rightVector, glm::fvec3 offsets);
};
