#pragma once
#include <fstream>
#include <utility/Scan.hpp>
//#include <utility/Module.hpp>
//#include <utility/String.hpp>
//#include <utility/PointerHook.hpp>
#include "magic_enum/magic_enum.hpp"

#include "uevr/API.hpp"
#include "Logger.hpp"
#include "Utilities.hpp"
#include "WeaponManager.hpp"
#include "CameraController.hpp"
#include "PlayerManager.hpp"

#include "SDK/Gsr_structs.hpp"
#include "SDK/Ics_parameters.hpp"
#include "SDK/Gsr_parameters.hpp"
#include "SDK/CobraUI_parameters.hpp"

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

public:
    HookManager(WeaponManager* wm, CameraController* cc, PlayerManager* pm) : weaponManager(wm), cameraController(cc), playerManager(pm) {};
    void Initialize();
    void HookFireFunction();

    // Standard hook :
    bool hook_bp_fn(std::wstring_view class_name, std::wstring_view fn_name, UEVR_UFunction_NativePreFn pre, UEVR_UFunction_NativePostFn post, bool use_native);
    static bool mod_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);
    static bool mod_PlayerFalling_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);
    static bool mod_PlayerKnife_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);
    static bool mod_PlayerCqcAction_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);
    
    static bool mod_GsrPlayer_OnChangeCamouflageApply_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);
};