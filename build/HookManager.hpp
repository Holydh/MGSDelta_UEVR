#pragma once
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <utility/Scan.hpp>
#include <utility/Module.hpp>
#include <utility/String.hpp>
#include <utility/PointerHook.hpp>

#include "uevr/API.hpp"
#include "Logger.hpp"
#include "WeaponController.hpp"
#include "CameraController.hpp"

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

class HookManager {
private:
	WeaponController* const weaponController;
	CameraController* const cameraController;
	static WeaponController* s_weaponController;
	static CameraController* s_cameraController;
    static void* on_get_fire_internal(uevr::API::UObject* equipcontroller, FGsrFireRequest* FireRequest);
    static void* on_get_fire(uevr::API::UObject* equipcontroller, FGsrFireRequest* FireRequest){
        return on_get_fire_internal(equipcontroller, FireRequest);
    };

    bool m_fire_hooked{false};
    int32_t m_fire_hook_id{};
    using FireFn = void*(*)(uevr::API::UObject*, FGsrFireRequest*);
    inline static FireFn m_fire_hook_fn{};
public:
    HookManager(WeaponController* wp, CameraController* cc) : weaponController(wp), cameraController(cc) {};
    void Init();
    void SetGunAdjustValuesFromConfig(const std::filesystem::path& ConfigFile);
    void hook_fire_fn();
};