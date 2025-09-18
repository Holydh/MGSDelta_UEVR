#pragma once
#include <fstream>
#include <utility/Scan.hpp>
//#include <utility/Module.hpp>
//#include <utility/String.hpp>
//#include <utility/PointerHook.hpp>

#include "uevr/API.hpp"
#include "Logger.hpp"
#include "Utilities.hpp"
#include "WeaponController.hpp"
#include "CameraController.hpp"

class HookManager {
private:
	WeaponController* const weaponController;
	CameraController* const cameraController;
	static WeaponController* s_weaponController;
	static CameraController* s_cameraController;
    static void* OnFireModded(uevr::API::UObject* equipcontroller, Utilities::FGsrFireRequest* FireRequest);
    static void* OnFire(uevr::API::UObject* equipcontroller, Utilities::FGsrFireRequest* FireRequest){
        return OnFireModded(equipcontroller, FireRequest);
    };

    bool m_fire_hooked{false};
    int32_t m_fire_hook_id{};
    using FireFn = void*(*)(uevr::API::UObject*, Utilities::FGsrFireRequest*);
    inline static FireFn m_fire_hook_fn{};

    bool m_update_camera_recoil_hooked{false};
    int32_t m_update_camera_recoil_hook_id{};
    using UpdateCameraRecoilFn = void*(*)(float*);
    inline static UpdateCameraRecoilFn m_update_camera_recoil_hook_fn{};

    //inline hook:
    
    static void* on_ISetSmoothedAimPosition_internal(uevr::API::UObject* self, UEVR_Vector3d* position);
    static void* on_ISetSmoothedAimPosition(uevr::API::UObject* self, UEVR_Vector3d* position){
        return on_ISetSmoothedAimPosition_internal(self, position);
    };
    bool m_ISetSmoothedAimPosition_hooked{false};
    int32_t m_ISetSmoothedAimPosition_hook_id{};
    using ISetSmoothedAimPositionFn = void*(*)(uevr::API::UObject*, UEVR_Vector3d*);
    inline static ISetSmoothedAimPositionFn m_ISetSmoothedAimPosition_hook_fn{};



public:
    HookManager(WeaponController* wp, CameraController* cc) : weaponController(wp), cameraController(cc) {};
    void Initialize();
    //void SetGunAdjustValuesFromConfig(const std::filesystem::path& ConfigFile);
    void HookFireFunction();
    void hook_ISetSmoothedAimPosition_fn();

        // Standard hook :
    //bool hook_bp_fn(std::wstring_view class_name, std::wstring_view fn_name, UEVR_UFunction_NativePreFn pre, UEVR_UFunction_NativePostFn post, bool use_native);
    //static bool mod_ISetSmoothedAimPosition_pre(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);
};