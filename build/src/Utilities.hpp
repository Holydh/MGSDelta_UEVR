#pragma once
#include "uevr/API.hpp"
#include "glm/glm.hpp"
#include "Logger.hpp"

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

enum ESlateVisibility : uint8_t
{
	Visible                                  = 0,
	Collapsed                                = 1,
	Hidden                                   = 2,
	HitTestInvisible                         = 3,
	SelfHitTestInvisible                     = 4,
	ESlateVisibility_MAX                     = 5,
};

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
	UEVR_Quaterniond    Rotation;
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

struct MakeRotFromX {
	UEVR_Vector3d InVec;
	UEVR_Rotatord Result;
};

struct DummyPostProcessSettings
{
    uint8_t pad[0x06E0];
};

struct IcsAnimInstance_OnEnteredState
{
public:
	uevr::API::FName                                   StateMachineName;                                  // 0x0000(0x0008)(ConstParm, Parm, OutParm, ZeroConstructor, ReferenceParm, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uevr::API::FName                                   PrevStateName;                                     // 0x0008(0x0008)(ConstParm, Parm, OutParm, ZeroConstructor, ReferenceParm, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uevr::API::FName                                   NextStateName;                                     // 0x0010(0x0008)(ConstParm, Parm, OutParm, ZeroConstructor, ReferenceParm, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
};

struct FString {
    wchar_t* Data;
    int32_t NumElements;
    int32_t MaxElements;

	const wchar_t* CStr() const { return Data ? Data : L""; }

    std::wstring ToWString() const {
        if (!Data) return L"";
        return std::wstring(Data, NumElements > 0 ? NumElements - 1 : 0);
    }
};

struct KismetStringLibrary_Conv_NameToString final
{
public:
	uevr::API::FName                                InName;                                            // 0x0000(0x0008)(Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	FString											ReturnValue;                                       // 0x0008(0x0010)(Parm, OutParm, ZeroConstructor, ReturnParm, HasGetValueTypeHash, NativeAccessSpecifierPublic)
};

struct KismetStringLibrary_Conv_StringToName final
{
public:
	class FString									InString;                                          // 0x0000(0x0010)(Parm, ZeroConstructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uevr::API::FName								ReturnValue;                                       // 0x0010(0x0008)(Parm, OutParm, ZeroConstructor, ReturnParm, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
};

#pragma pack(push, 1)
struct FMinimalViewInfo final
{
public:
	UEVR_Vector3d                                Location;                                          // 0x0000(0x0018)(Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	UEVR_Rotatord                               Rotation;                                          // 0x0018(0x0018)(Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, NativeAccessSpecifierPublic)
	float                                         FOV;                                               // 0x0030(0x0004)(Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	float                                         DesiredFOV;                                        // 0x0034(0x0004)(ZeroConstructor, Transient, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	float                                         OrthoWidth;                                        // 0x0038(0x0004)(Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	float                                         OrthoNearClipPlane;                                // 0x003C(0x0004)(Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, Interp, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	float                                         OrthoFarClipPlane;                                 // 0x0040(0x0004)(Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, Interp, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	float                                         PerspectiveNearClipPlane;                          // 0x0044(0x0004)(Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, Interp, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	float                                         AspectRatio;                                       // 0x0048(0x0004)(Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uint8_t                                         Pad_4C[0x8];                                       // 0x004C(0x0008)(Fixing Size After Last Property [ Dumper-7 ])
	uint8_t                                         bConstrainAspectRatio : 1;                         // 0x0054(0x0001)(BitIndex: 0x00, PropSize: 0x0001 (Edit, BlueprintVisible, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic))
	uint8_t                                         bUseFieldOfViewForLOD : 1;                         // 0x0054(0x0001)(BitIndex: 0x01, PropSize: 0x0001 (Edit, BlueprintVisible, NoDestructor, AdvancedDisplay, HasGetValueTypeHash, NativeAccessSpecifierPublic))
	uint8_t                                         Pad_55[0x3];                                       // 0x0055(0x0003)(Fixing Size After Last Property [ Dumper-7 ])
	uint8_t											ProjectionMode;                                    // 0x0058(0x0001)(Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uint8_t                                         Pad_59[0x3];                                       // 0x0059(0x0003)(Fixing Size After Last Property [ Dumper-7 ])
	float                                         PostProcessBlendWeight;                            // 0x005C(0x0004)(BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	DummyPostProcessSettings                   PostProcessSettings;                               // 0x0060(0x06E0)(BlueprintVisible, NativeAccessSpecifierPublic)
	UEVR_Vector2d                              OffCenterProjectionOffset;                         // 0x0740(0x0010)(Edit, BlueprintVisible, ZeroConstructor, DisableEditOnTemplate, Transient, EditConst, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	bool                                          bIsDisableMFSR;                                    // 0x0750(0x0001)(Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, AdvancedDisplay, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uint8_t                                         Pad_751[0x7F];                                     // 0x0751(0x007F)(Fixing Struct Size After Last Property [ Dumper-7 ])
};
#pragma pack(pop)

#pragma pack(push, 1)
struct GameplayStatics_BeginDeferredActorSpawnFromClass final
{
public:
	uevr::API::UObject*                          WorldContextObject;                                // 0x0000(0x0008)(ConstParm, Parm, ZeroConstructor, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uevr::API::UClass*		                      ActorClass;                                        // 0x0008(0x0008)(Parm, ZeroConstructor, NoDestructor, UObjectWrapper, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	struct FTransform                             SpawnTransform;                                    // 0x0010(0x0060)(ConstParm, Parm, OutParm, ReferenceParm, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uint8_t							            CollisionHandlingOverride;  // set to 1                      // 0x0070(0x0001)(Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uint8_t                                         Pad_71[0x7];                                       // 0x0071(0x0007)(Fixing Size After Last Property [ Dumper-7 ])
	uevr::API::UObject*	                                Owner;                                             // 0x0078(0x0008)(Parm, ZeroConstructor, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uint8_t			                        TransformScaleMethod; //Set to 0                              // 0x0080(0x0001)(Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uint8_t                                         Pad_81[0x7];                                       // 0x0081(0x0007)(Fixing Size After Last Property [ Dumper-7 ])
	uevr::API::UObject*                                 ReturnValue;                                       // 0x0088(0x0008)(Parm, OutParm, ZeroConstructor, ReturnParm, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
};
#pragma pack(pop)

#pragma pack(push, 1)
struct GameplayStatics_FinishSpawningActor final
{
public:
	uevr::API::UObject*                                 Actor;                                             // 0x0000(0x0008)(Parm, ZeroConstructor, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uint8_t                                         Pad_8[0x8];                                        // 0x0008(0x0008)(Fixing Size After Last Property [ Dumper-7 ])
	struct FTransform                             SpawnTransform;                                    // 0x0010(0x0060)(ConstParm, Parm, OutParm, ReferenceParm, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uint8_t                        TransformScaleMethod;                              // 0x0070(0x0001)(Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uint8_t                                         Pad_71[0x7];                                       // 0x0071(0x0007)(Fixing Size After Last Property [ Dumper-7 ])
	uevr::API::UObject*                                 ReturnValue;                                       // 0x0078(0x0008)(Parm, OutParm, ZeroConstructor, ReturnParm, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
};
#pragma pack(pop)

#pragma pack(push, 1)
struct FCameraCacheEntry final
{
public:
	float                                         Timestamp;                                         // 0x0000(0x0004)(ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uint8_t                                         Pad_4[0xC];                                        // 0x0004(0x000C)(Fixing Size After Last Property [ Dumper-7 ])
	FMinimalViewInfo							POV;                                               // 0x0010(0x07D0)(NativeAccessSpecifierPublic)
};
#pragma pack(pop)

#pragma pack(push, 1)
	struct FRotator {
		float pitch;
		float yaw;
		float roll;
	};
#pragma pack(pop)


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
	uevr::API::UClass*                                 CameraTargetActor;                                      // 0x0000(0x0008)(Parm, ZeroConstructor, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
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
		UEVR_Vector3d newLocation;
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
		UEVR_Rotatord newRotation;
		bool bSweep;
		uint8_t pad_D[0x0007];
		uint8_t SweepHitResult[0x00E8];
		bool bTeleport;
		uint8_t pad_9D[0x0007];
	};
#pragma pack(pop)
};