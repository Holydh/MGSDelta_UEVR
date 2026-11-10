#pragma once
#include "uevr/API.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "Logger.hpp"

#include "SDK/CoreUObject_structs.hpp"
#include "SDK/Engine_parameters.hpp"
#include "SDK/Engine_classes.hpp"

class Utilities {
private:

public:
	static uevr::API::UObject* KismetMathLibrary;
	static uevr::API::UObject* KismetStringLibrary;
	static uevr::API::UObject* GameplayStatics;
	static uevr::API::UClass* ActorClass;
	static uevr::API::UClass* SceneComponentClass;

	//Animation states classes
	static uevr::API::UClass* ABP_SnakeBasicAction_C;
	static uevr::API::UClass* ABP_PlayerEludeAction_C;
	static uevr::API::UClass* ABP_PlayerRollingAction_C;
	static uevr::API::UClass* ABP_PlayerBehindAction_C;
	static uevr::API::UClass* ABP_PlayerKnifeReadyAction_C;
	static uevr::API::UClass* ABP_PlayerMeleeAction_C;
	static uevr::API::UClass* ABP_PlayerCarryAction_C;
	static uevr::API::UClass* ABP_EnemyCqc_C;

	//Debug Arrow Actor
	static SDK::AActor* DebugMesh0;
	static SDK::AActor* DebugMesh1;

	static void InitHelperClasses();
	static uevr::API::UObject* SpawnActor(UEVR_Vector3d Position, uevr::API::UObject* Owner);
	static SDK::AActor* SpawnMesh(SDK::FTransform worldTransform, std::wstring_view meshFullName, SDK::UObject*& outMeshComponent);
	static void DestroyDebugMeshes();

	static float Remap(float value, float inMin, float inMax, float outMin, float outMax);
	static glm::quat FRotatorToQuat(SDK::FRotator rotator);
	static SDK::FRotator QuatToFRotator(glm::quat quaternion);
	//static glm::fvec3 OffsetLocalPositionFromWorld(glm::fvec3 worldPosition, glm::fvec3 forwardVector, glm::fvec3 upVector, glm::fvec3 rightVector, glm::fvec3 offsets);
};