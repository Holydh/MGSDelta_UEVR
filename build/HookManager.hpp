#pragma once
#include <fstream>
#include <utility/Scan.hpp>
//#include <utility/Module.hpp>
//#include <utility/String.hpp>
//#include <utility/PointerHook.hpp>

#include "uevr/API.hpp"
#include "Logger.hpp"
#include "Utilities.hpp"
#include "WeaponManager.hpp"
#include "CameraController.hpp"
#include "PlayerManager.hpp"

#include "SDK/Gsr_structs.hpp"
#include "SDK/Ics_parameters.hpp"
#include "SDK/Gsr_parameters.hpp"

class HookManager {
private:
    WeaponManager* const weaponManager;
    CameraController* const cameraController;
    PlayerManager* const playerManager;
    static WeaponManager* s_weaponManager;
    static CameraController* s_cameraController;
	static PlayerManager* s_playerManager;


    static void* OnFireModded(uevr::API::UObject* equipcontroller, SDK::FGsrFireRequest* FireRequest);
    static void* OnFire(uevr::API::UObject* equipcontroller, SDK::FGsrFireRequest* FireRequest) {
        return OnFireModded(equipcontroller, FireRequest);
    };
    bool m_fire_hooked{ false };
    int32_t m_fire_hook_id{};
    using FireFn = void* (*)(uevr::API::UObject*, SDK::FGsrFireRequest*);
    inline static FireFn m_fire_hook_fn{};

    //static void* OnCameraModifyModded(uevr::API::UObject* cameraModifier, float* deltaTime, SDK::FVector* viewLocation, SDK::FRotator* viewRotation, float* FOV, SDK::FVector* newViewLocation, SDK::FRotator* newViewRotation, float* newFOV);
    //static void* OnCameraModify(uevr::API::UObject* cameraModifier, float* deltaTime, SDK::FVector* viewLocation, SDK::FRotator* viewRotation, float* FOV, SDK::FVector* newViewLocation, SDK::FRotator* newViewRotation, float* newFOV) {
    //    return OnCameraModifyModded(cameraModifier, deltaTime, viewLocation, viewRotation, FOV, newViewLocation, newViewRotation, newFOV);
    //};
    //bool m_camera_modify_hooked{ false };
    //int32_t m_camera_modify_hook_id{};
    //using CameraModifyFn = void* (*)(uevr::API::UObject*, float*, SDK::FVector*, SDK::FRotator*, float*, SDK::FVector*, SDK::FRotator*, float*);
    //inline static CameraModifyFn m_camera_modify_hook_fn{};

    //inline hook:
    //static void* on_ISetSmoothedAimPosition_internal(uevr::API::UObject* self, UEVR_Rotatord* rotation);
    //static void* on_ISetSmoothedAimPosition(uevr::API::UObject* self, UEVR_Rotatord* rotation){
    //    return on_ISetSmoothedAimPosition_internal(self, rotation);
    //};
    //bool m_ISetSmoothedAimPosition_hooked{false};
    //int32_t m_ISetSmoothedAimPosition_hook_id{};
    //using ISetSmoothedAimPositionFn = void*(*)(uevr::API::UObject*, UEVR_Rotatord*);
    //inline static ISetSmoothedAimPositionFn m_ISetSmoothedAimPosition_hook_fn{};



public:
    HookManager(WeaponManager* wm, CameraController* cc, PlayerManager* pm) : weaponManager(wm), cameraController(cc), playerManager(pm) {};
    void Initialize();
    //void SetGunAdjustValuesFromConfig(const std::filesystem::path& ConfigFile);
    void HookFireFunction();
    void HookCameraModifyFunction();
    /*void hook_ISetSmoothedAimPosition_fn();*/

    // Standard hook :
    bool hook_bp_fn(std::wstring_view class_name, std::wstring_view fn_name, UEVR_UFunction_NativePreFn pre, UEVR_UFunction_NativePostFn post, bool use_native);
    static bool mod_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);
    static bool mod_PlayerFalling_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);
    //static bool mod_CameraModifier_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);
    /*static bool mod_SetCameraRotation_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);*/
    /*static bool mod_ISetAimPosition_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);*/
};