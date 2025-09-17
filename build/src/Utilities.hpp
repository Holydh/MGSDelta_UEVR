#pragma once
#include "uevr/API.hpp"
#include "glm/glm.hpp"
#include "Logger.hpp"

class Utilities {
private:

public:
	static uevr::API::UObject* KismetMathLibrary;
	static void InitHelperClasses();
	//static glm::fvec3 OffsetLocalPositionFromWorld(glm::fvec3 worldPosition, glm::fvec3 forwardVector, glm::fvec3 upVector, glm::fvec3 rightVector, glm::fvec3 offsets);


struct FGsrFireRequest 
{
    UEVR_Vector3d       FirePosition;
    bool                IsFirePositionOnMuzzle;
    uint8_t             Pad_19[0x7];
    UEVR_Vector3d       AimPosition;
    UEVR_Vector3d       DeviationOffset;
    uint8_t             OwnerType;
    bool                IsSubjective;
    bool                HasOffenseTarget;
    uint8_t             Color;
    uint32_t            WoundType;
    uint32_t            Force;
    bool                IsGhostSpreadBullet;
    uint8_t             Pad_5D[0x3];
    uint32_t            AmmoCount;
    uint32_t            BulletType;
};

struct FTransform
{
	UEVR_Quaternionf    Rotation;
	UEVR_Vector3d       Translation;
	uint8_t             Pad_38[0x8];
	UEVR_Vector3d       Scale3D;
	uint8_t             Pad_58[0x8];
};

struct RotateAngleAxis
{
    UEVR_Vector3d       InVect;
    float               AngleDeg;
    UEVR_Vector3d       Axis;
    UEVR_Vector3d       Result;
};

#pragma pack(push, 1)
	struct FRotator {
		float pitch;
		float yaw;
		float roll;
	};
#pragma pack(pop)

	struct ParameterSingleBool
	{
		bool boolValue = false;
	};

	struct ParameterSingleVector3d
	{
		UEVR_Vector3d vec3Value{};
	};

	struct ParameterGetSocketLocation
	{
		uevr::API::FName inSocketName;
		glm::fvec3 outLocation;
	};

	struct ParameterDetachFromParent
	{
		bool maintainWorldPosition;
		bool callModify;
	};

	struct ParameterFindLookAtRotation
	{
		glm::fvec3 start;
		glm::fvec3 target;
		Utilities::FRotator outRotation;
	};

	//struct ParameterWobbledAim
	//{
	//	UEVR_Rotatord* wobblingDirection = new UEVR_Rotatord();
	//	UEVR_Vector3d* wobblingTartgetPosition = new UEVR_Vector3d();
	//	UEVR_Vector3d& TargetPosition;
	//	UEVR_Vector3d& emitPosition;
	//};

#pragma pack(push, 1)
struct PlayerCameraManager_BlueprintUpdateCamera final
{
public:
	class AActor*                                 CameraTarget;                                      // 0x0000(0x0008)(Parm, ZeroConstructor, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	UEVR_Vector3d                                NewCameraLocation;                                 // 0x0008(0x0018)(Parm, OutParm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	UEVR_Rotatord                               NewCameraRotation;                                 // 0x0020(0x0018)(Parm, OutParm, ZeroConstructor, IsPlainOldData, NoDestructor, NativeAccessSpecifierPublic)
	float                                         NewCameraFOV;                                      // 0x0038(0x0004)(Parm, OutParm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	bool                                          ReturnValue;                                       // 0x003C(0x0001)(Parm, OutParm, ZeroConstructor, ReturnParm, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uint8_t                                         Pad_3D[0x3];                                       // 0x003D(0x0003)(Fixing Struct Size After Last Property [ Dumper-7 ])
};
#pragma pack(pop)

//Parameters taken from Dumper7 dump of the game. Padding allows to align with the size Unreal Engine expects for each parameters.
#pragma pack(push, 1)
	struct Parameter_K2_SetWorldOrRelativeLocation final
	{
		glm::fvec3 newLocation;
		bool bSweep;
		uint8_t pad_D[3];
		uint8_t padding[0x8C];
		bool bTeleport;
		uint8_t pad_9D[3];
	};
#pragma pack(pop)

#pragma pack(push, 1)
	struct Parameter_K2_SetWorldOrRelativeRotation final
	{
		FRotator newRotation;
		bool bSweep;
		uint8_t pad_D[3];
		uint8_t padding[0x8C];
		bool bTeleport;
		uint8_t pad_9D[3];
	};
#pragma pack(pop)
};