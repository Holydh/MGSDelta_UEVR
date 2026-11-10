#pragma once
// Engine and game enums used by the plugin.
// Values copied from a Dumper-7 dump of the game; engine enums are fixed by the Unreal
// Engine version, game (Gsr/MGS3) enums could in theory change with a game update.
#include <cstdint>

namespace ue::enums_impl {

// from MGS3_structs.hpp
enum class EAimingStatus : uint8_t
{
    Aiming_None = 0,
    Aiming_Tps  = 1,
    Aiming_Fps  = 2,
    Aiming_Auto = 3,
    Aiming_MAX  = 4,
};
// from Engine_structs.hpp
enum class EAttachLocation : uint8_t
{
    KeepRelativeOffset         = 0,
    KeepWorldPosition          = 1,
    SnapToTarget               = 2,
    SnapToTargetIncludingScale = 3,
    EAttachLocation_MAX        = 4,
};
// from Engine_structs.hpp
enum class EAttachmentRule : uint8_t
{
    KeepRelative        = 0,
    KeepWorld           = 1,
    SnapToTarget        = 2,
    EAttachmentRule_MAX = 3,
};
// from Engine_structs.hpp
enum class EBlendMode : uint8_t
{
    BLEND_Opaque                          = 0,
    BLEND_Masked                          = 1,
    BLEND_Translucent                     = 2,
    BLEND_Additive                        = 3,
    BLEND_Modulate                        = 4,
    BLEND_AlphaComposite                  = 5,
    BLEND_AlphaHoldout                    = 6,
    BLEND_TranslucentColoredTransmittance = 7,
    BLEND_MAX                             = 8,
    BLEND_TranslucentGreyTransmittance    = 2,
    BLEND_ColoredTransmittanceOnly        = 4,
};
// from Engine_structs.hpp
enum class ECollisionEnabled : uint8_t
{
    NoCollision           = 0,
    QueryOnly             = 1,
    PhysicsOnly           = 2,
    QueryAndPhysics       = 3,
    ProbeOnly             = 4,
    QueryAndProbe         = 5,
    ECollisionEnabled_MAX = 6,
};
// from Engine_structs.hpp
enum class EComponentMobility : uint8_t
{
    Static                 = 0,
    Stationary             = 1,
    Movable                = 2,
    EComponentMobility_MAX = 3,
};
// from Gsr_structs.hpp
enum class EGsrGameStatus : uint8_t
{
    SenarioDemo                  = 0,
    InGame                       = 1,
    BomIsDisable                 = 2,
    GameOver                     = 3,
    FakeGameOver                 = 4,
    Cutscene                     = 5,
    Movie                        = 6,
    DisablePlayerControl         = 7,
    BossCutscene                 = 8,
    MiniDemo                     = 9,
    StageChanging                = 10,
    PhotoMode                    = 11,
    DemoTheater                  = 12,
    DisablePhotoMode             = 13,
    DisableQuickChange           = 14,
    DisableRadioMenu             = 15,
    Title                        = 16,
    DisableAutoSave              = 17,
    EvaForceLod0                 = 18,
    SwampStage                   = 19,
    MorterStage                  = 20,
    WaterStage                   = 21,
    BridgeStage                  = 22,
    PlaceC3EventStage            = 23,
    PendingInviteForChunkInstall = 24,
    Count                        = 25,
    EGsrGameStatus_MAX           = 26,
};
// from Gsr_structs.hpp
enum class EGsrItemId : uint8_t
{
    IT_None            = 0,
    IT_MedicalKit      = 1,
    IT_Pentazemin      = 2,
    IT_Asphyxia        = 3,
    IT_Analeptic       = 4,
    IT_Cigar           = 5,
    IT_Scope           = 6,
    IT_Thermal         = 7,
    IT_NightVision     = 8,
    IT_Camera          = 9,
    IT_MotionDetect    = 10,
    IT_ActiveSonar     = 11,
    IT_MineDetect      = 12,
    IT_VibrateDetect   = 13,
    IT_CBoxA           = 14,
    IT_CBoxB           = 15,
    IT_CBoxC           = 16,
    IT_CBoxD           = 17,
    IT_AlligatorCap    = 18,
    IT_KeyA            = 19,
    IT_KeyB            = 20,
    IT_KeyC            = 21,
    IT_Bandana         = 22,
    IT_Stealth         = 23,
    IT_Mothproof       = 24,
    IT_MonkeyMask      = 25,
    IT_ATCamo          = 26,
    IT_Compass         = 27,
    IT_AlligatorSuit   = 28,
    IT_GarkoMask       = 29,
    IT_KerotanMask     = 30,
    IT_AdditionalItem1 = 31,
    IT_AdditionalItem2 = 32,
    IT_AdditionalItem3 = 33,
    IT_AdditionalItem4 = 34,
    IT_AdditionalItem5 = 35,
    IT_MAX_MENU_ITEMS  = 36,
    IT_MAX_MENU        = 36,
    IT_MAX             = 37,
};
// from Gsr_structs.hpp
enum class EGsrPlayerCameraControllerCameraMode : uint8_t
{
    Around                                   = 0,
    Stage                                    = 1,
    Tps                                      = 2,
    Subjective                               = 3,
    CutIn                                    = 4,
    EGsrPlayerCameraControllerCameraMode_MAX = 5,
};
// from Gsr_structs.hpp
enum class EGsrPlayerStance : uint8_t
{
    Stand                = 0,
    Squat                = 1,
    Crawl                = 2,
    Count                = 3,
    Invalid              = 3,
    EGsrPlayerStance_MAX = 4,
};
// from Gsr_structs.hpp
enum class EGsrUIMenuType : uint8_t
{
    None               = 0,
    Pause              = 1,
    Radio              = 2,
    SV                 = 3,
    WeaponLMenu        = 4,
    ItemLMEnu          = 5,
    RadioShortcut      = 6,
    CamoPreset         = 7,
    Count              = 8,
    EGsrUIMenuType_MAX = 9,
};
// from Engine_structs.hpp
enum class EMaterialDomain : uint8_t
{
    MD_Surface               = 0,
    MD_DeferredDecal         = 1,
    MD_LightFunction         = 2,
    MD_Volume                = 3,
    MD_PostProcess           = 4,
    MD_UI                    = 5,
    MD_RuntimeVirtualTexture = 6,
    MD_MAX                   = 7,
};
// from Engine_structs.hpp
enum class EMaterialShadingModel : uint8_t
{
    MSM_Unlit                      = 0,
    MSM_DefaultLit                 = 1,
    MSM_Subsurface                 = 2,
    MSM_PreintegratedSkin          = 3,
    MSM_ClearCoat                  = 4,
    MSM_SubsurfaceProfile          = 5,
    MSM_TwoSidedFoliage            = 6,
    MSM_Hair                       = 7,
    MSM_Cloth                      = 8,
    MSM_Eye                        = 9,
    MSM_SingleLayerWater           = 10,
    MSM_ThinTranslucent            = 11,
    MSM_Strata                     = 12,
    MSM_HairIgnoreLumenScreenTrace = 13,
    MSM_NUM                        = 14,
    MSM_FromMaterialExpression     = 15,
    MSM_MAX                        = 16,
};
// from Engine_structs.hpp
enum class ERelativeTransformSpace : uint8_t
{
    RTS_World           = 0,
    RTS_Actor           = 1,
    RTS_Component       = 2,
    RTS_ParentBoneSpace = 3,
    RTS_MAX             = 4,
};
// from Engine_structs.hpp
enum class ESceneCapturePrimitiveRenderMode : uint8_t
{
    PRM_LegacySceneCapture    = 0,
    PRM_RenderScenePrimitives = 1,
    PRM_UseShowOnlyList       = 2,
    PRM_MAX                   = 3,
};
// from Engine_structs.hpp
enum class ESceneCaptureSource : uint8_t
{
    SCS_SceneColorHDR        = 0,
    SCS_SceneColorHDRNoAlpha = 1,
    SCS_FinalColorLDR        = 2,
    SCS_SceneColorSceneDepth = 3,
    SCS_SceneDepth           = 4,
    SCS_DeviceDepth          = 5,
    SCS_Normal               = 6,
    SCS_BaseColor            = 7,
    SCS_FinalColorHDR        = 8,
    SCS_FinalToneCurveHDR    = 9,
    SCS_MAX                  = 10,
};
// from UMG_structs.hpp
enum class ESlateVisibility : uint8_t
{
    Visible              = 0,
    Collapsed            = 1,
    Hidden               = 2,
    HitTestInvisible     = 3,
    SelfHitTestInvisible = 4,
    ESlateVisibility_MAX = 5,
};
// from Engine_structs.hpp
enum class ESpawnActorCollisionHandlingMethod : uint8_t
{
    Undefined                               = 0,
    AlwaysSpawn                             = 1,
    AdjustIfPossibleButAlwaysSpawn          = 2,
    AdjustIfPossibleButDontSpawnIfColliding = 3,
    DontSpawnIfColliding                    = 4,
    ESpawnActorCollisionHandlingMethod_MAX  = 5,
};
// from Engine_structs.hpp
enum class ESpawnActorScaleMethod : uint8_t
{
    OverrideRootScale          = 0,
    MultiplyWithRoot           = 1,
    SelectDefaultAtRuntime     = 2,
    ESpawnActorScaleMethod_MAX = 3,
};
// from Engine_structs.hpp
enum class ETextureRenderTargetFormat : uint8_t
{
    RTF_R8         = 0,
    RTF_RG8        = 1,
    RTF_RGBA8      = 2,
    RTF_RGBA8_SRGB = 3,
    RTF_R16f       = 4,
    RTF_RG16f      = 5,
    RTF_RGBA16f    = 6,
    RTF_R32f       = 7,
    RTF_RG32f      = 8,
    RTF_RGBA32f    = 9,
    RTF_RGB10A2    = 10,
    RTF_MAX        = 11,
};
// from UMG_structs.hpp
enum class EWidgetBlendMode : uint8_t
{
    Opaque               = 0,
    Masked               = 1,
    Transparent          = 2,
    EWidgetBlendMode_MAX = 3,
};

} // namespace ue::enums_impl

#if MGSDELTA_HAS_SDK
// TEMPORARY while the Dumper-7 SDK is still used: alias the SDK's own enum types so
// ported and not-yet-ported code exchange the same types. EnumsCheck.cpp verifies the
// copies above match the SDK values exactly.
#include "SDK/Engine_structs.hpp"
#include "SDK/Gsr_structs.hpp"
#include "SDK/MGS3_structs.hpp"
#include "SDK/UMG_structs.hpp"
namespace ue {
using ::SDK::EAimingStatus;
using ::SDK::EAttachLocation;
using ::SDK::EAttachmentRule;
using ::SDK::EBlendMode;
using ::SDK::ECollisionEnabled;
using ::SDK::EComponentMobility;
using ::SDK::EGsrGameStatus;
using ::SDK::EGsrItemId;
using ::SDK::EGsrPlayerCameraControllerCameraMode;
using ::SDK::EGsrPlayerStance;
using ::SDK::EGsrUIMenuType;
using ::SDK::EMaterialDomain;
using ::SDK::EMaterialShadingModel;
using ::SDK::ERelativeTransformSpace;
using ::SDK::ESceneCapturePrimitiveRenderMode;
using ::SDK::ESceneCaptureSource;
using ::SDK::ESlateVisibility;
using ::SDK::ESpawnActorCollisionHandlingMethod;
using ::SDK::ESpawnActorScaleMethod;
using ::SDK::ETextureRenderTargetFormat;
using ::SDK::EWidgetBlendMode;
}
#else
namespace ue {
using enums_impl::EAimingStatus;
using enums_impl::EAttachLocation;
using enums_impl::EAttachmentRule;
using enums_impl::EBlendMode;
using enums_impl::ECollisionEnabled;
using enums_impl::EComponentMobility;
using enums_impl::EGsrGameStatus;
using enums_impl::EGsrItemId;
using enums_impl::EGsrPlayerCameraControllerCameraMode;
using enums_impl::EGsrPlayerStance;
using enums_impl::EGsrUIMenuType;
using enums_impl::EMaterialDomain;
using enums_impl::EMaterialShadingModel;
using enums_impl::ERelativeTransformSpace;
using enums_impl::ESceneCapturePrimitiveRenderMode;
using enums_impl::ESceneCaptureSource;
using enums_impl::ESlateVisibility;
using enums_impl::ESpawnActorCollisionHandlingMethod;
using enums_impl::ESpawnActorScaleMethod;
using enums_impl::ETextureRenderTargetFormat;
using enums_impl::EWidgetBlendMode;
}
#endif
