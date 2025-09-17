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
public:
    HookManager(WeaponController* wp, CameraController* cc) : weaponController(wp), cameraController(cc) {};
    void Init();
    void SetGunAdjustValuesFromConfig(const std::filesystem::path& ConfigFile);
    void HookFireFunction();
};