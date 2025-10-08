#pragma once
#include "uevr/API.hpp"
#include "glm/glm.hpp"
#include "Logger.hpp"

#include "SDK/CoreUObject_structs.hpp"
#include "SDK/Engine_parameters.hpp"

class Utilities {
private:

public:
	static uevr::API::UObject* KismetMathLibrary;
	static uevr::API::UObject* KismetStringLibrary;
	static uevr::API::UObject* GameplayStatics;
	static uevr::API::UClass* ActorClass;
	static uevr::API::UClass* SceneComponentClass;

	static void InitHelperClasses();
	static uevr::API::UObject* SpawnActor(uevr::API::UObject* WorldContextObject, UEVR_Vector3d Position, uint8_t CollisionMethod, uevr::API::UObject* Owner);

	//static glm::fvec3 OffsetLocalPositionFromWorld(glm::fvec3 worldPosition, glm::fvec3 forwardVector, glm::fvec3 upVector, glm::fvec3 rightVector, glm::fvec3 offsets);
};