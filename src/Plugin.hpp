#pragma once

#include <windows.h>

#include <memory>
#include "uevr/API.hpp"
#include "uevr/Plugin.hpp"
#include <utility/PointerHook.hpp>

#define _CRT_SECURE_NO_WARNINGS 1

#define PLUGIN_LOG_ONCE(...) { \
    static bool _logged_ = false; \
    if (!_logged_) { \
        _logged_ = true; \
        API::get()->log_info(__VA_ARGS__); \
    } }

#define PLUGIN_LOG_ONCE_ERROR(...) { \
    static bool _logged_ = false; \
    if (!_logged_) { \
        _logged_ = true; \
        API::get()->log_error(__VA_ARGS__); \
    } }

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
} params;

struct FTransform
{
	UEVR_Quaternionf    Rotation;
	UEVR_Vector3d       Translation;
	uint8_t             Pad_38[0x8];
	UEVR_Vector3d       Scale3D;
	uint8_t             Pad_58[0x8];
} transform;

struct RotateAngleAxis
{
    UEVR_Vector3d       InVect;
    float               AngleDeg;
    UEVR_Vector3d       Axis;
    UEVR_Vector3d       Result;
} rotateaxis;

// Global accessor for our plugin.
class MGS3Plugin;
extern std::unique_ptr<MGS3Plugin> g_plugin;

class MGS3Plugin : public uevr::Plugin {
public:
    MGS3Plugin() = default;
    virtual ~MGS3Plugin();

    void on_initialize() override;

private:
    void hook_fire_fn();
    
    void SetGunAdjustValuesFromConfig(const std::filesystem::path& ConfigFile);
    static void* on_get_fire_internal(uevr::API::UObject* equipcontroller, FGsrFireRequest* FireRequest);
    static void* on_get_fire(uevr::API::UObject* equipcontroller, FGsrFireRequest* FireRequest) {
        return on_get_fire_internal(equipcontroller, FireRequest);
    }

    bool m_hooked{false};
    int32_t m_fire_hook_id{};
    using FireFn = void*(*)(uevr::API::UObject*, FGsrFireRequest*);
    inline static FireFn m_fire_hook_fn{};
};