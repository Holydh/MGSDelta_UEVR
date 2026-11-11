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
#include "ue/Reflection.hpp"

class HookManager {
private:
    WeaponManager* const weaponManager;
    CameraController* const cameraController;
    PlayerManager* const playerManager;
    static WeaponManager* s_weaponManager;
    static CameraController* s_cameraController;
    static PlayerManager* s_playerManager;


    static void* OnFireModded(uevr::API::UObject* equipcontroller, void* FireRequest);
    static void* OnFire(uevr::API::UObject* equipcontroller, void* FireRequest) {
        return OnFireModded(equipcontroller, FireRequest);
    };
    bool m_fire_hooked{ false };
    int32_t m_fire_hook_id{};
    using FireFn = void* (*)(uevr::API::UObject*, void*);
    inline static FireFn m_fire_hook_fn{};

    // Reads the state names from an OnEnteredState call and forwards them to the PlayerManager
    static void ForwardEnteredState(uevr::API::UFunction* fn, void* locals);

    // Blueprint hooks are kept so they can be applied again: a Blueprint class may not be loaded
    // yet when the plugin starts, or get unloaded and reloaded with the levels.
    struct BlueprintHook {
        std::wstring class_name;
        std::wstring fn_name;
        UEVR_UFunction_NativePreFn pre;
        UEVR_UFunction_NativePostFn post;
        bool use_native;
        ue::WeakObject hooked_function; // nullptr until hooked, or once that function is unloaded
    };
    std::vector<BlueprintHook> m_bp_hooks;
    static bool apply_bp_hook(BlueprintHook& hook);

public:
    HookManager(WeaponManager* wm, CameraController* cc, PlayerManager* pm) : weaponManager(wm), cameraController(cc), playerManager(pm) {};
    void Initialize();
    void HookFireFunction();

    // Standard hook :
    bool hook_bp_fn(std::wstring_view class_name, std::wstring_view fn_name, UEVR_UFunction_NativePreFn pre, UEVR_UFunction_NativePostFn post, bool use_native);
    // Applies the hooks whose class wasn't found yet, or whose function was unloaded since
    void RetryFailedHooks();
    static bool mod_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);
    static bool mod_PlayerFalling_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);
    static bool mod_PlayerKnife_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);
    static bool mod_PlayerCqcAction_OnEnteredState_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);
    
    static bool mod_GsrPlayer_OnChangeCamouflageApply_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);
    
    static bool mod_CobraLevelSequence_OnCameraCut_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);

    static bool mod_IcsActorActionModule_OnOverrideRootMotionVelocity_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);
};