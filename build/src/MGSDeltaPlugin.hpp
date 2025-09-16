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

#include <memory>
#include "uevr/API.hpp"
#include "uevr/Plugin.hpp"
#include <utility/PointerHook.hpp>

// only really necessary if you want to render to the screen
#include "renderlib/uevr_imgui/imgui_impl_dx11.h"
#include "renderlib/uevr_imgui/imgui_impl_dx12.h"
#include "renderlib/uevr_imgui/imgui_impl_win32.h"

#include "renderlib/rendering/d3d11.hpp"
#include "renderlib/rendering/d3d12.hpp"
#include "renderlib/imgui/imgui.h"

#include <codecvt>

#include "CameraController.hpp"
#include "WeaponController.hpp"
#include "Logger.hpp"

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
class MGSDeltaPlugin;
extern std::unique_ptr<MGSDeltaPlugin> g_plugin;

class MGSDeltaPlugin : public uevr::Plugin {
private:
    bool initialize_imgui();
    void internal_frame();

    HWND m_wnd;
    bool m_initialized;
    bool m_was_rendering_desktop;
    
    std::recursive_mutex m_imgui_mutex;

    void hook_fire_fn();
    
    void SetGunAdjustValuesFromConfig(const std::filesystem::path& ConfigFile);
    static void* on_get_fire_internal(uevr::API::UObject* equipcontroller, FGsrFireRequest* FireRequest);
    static void* on_get_fire(uevr::API::UObject* equipcontroller, FGsrFireRequest* FireRequest){
        return on_get_fire_internal(equipcontroller, FireRequest);
    };

    bool m_fire_hooked{false};
    int32_t m_fire_hook_id{};
    using FireFn = void*(*)(uevr::API::UObject*, FGsrFireRequest*);
    inline static FireFn m_fire_hook_fn{};


    //bool m_update_camera_recoil_hooked{false};
    //int32_t m_update_camera_recoil_hook_id{};
    //using UpdateCameraRecoilFn = void*(*)(float*);
    //inline static UpdateCameraRecoilFn m_update_camera_recoil_hook_fn{};

    //inline hook:
    //void hook_get_current_aim_pitch_fn();
    //static void on_get_current_aim_pitch_internal(uevr::API::UObject* self/*, Utilities::FRotator* rotation*/);
    //static void on_get_current_aim_pitch(uevr::API::UObject* self/*, Utilities::FRotator* rotation*/){
    //    return on_get_current_aim_pitch_internal(self/*, rotation*/);
    //};
    //bool m_GetCurrectAimPitch_hooked{false};
    //int32_t m_GetCurrectAimPitch_hook_id{};
    //using GetCurrentAimPitchFn = void(*)(uevr::API::UObject*, Utilities::FRotator*);
    //inline static GetCurrentAimPitchFn m_GetCurrentAimPitch_hook_fn{};

    // Standard hook :
    bool hook_bp_fn(std::wstring_view class_name, std::wstring_view fn_name, UEVR_UFunction_NativePreFn pre, UEVR_UFunction_NativePostFn post, bool use_native, bool findRealCall);
    static bool mod_GetCurrentAimPitch_post(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);

public:
    MGSDeltaPlugin() : cameraController(), weaponController() {};
    virtual ~MGSDeltaPlugin();

    CameraController cameraController;
	WeaponController weaponController;

    void on_initialize() override;
    //void on_dllmain(HMODULE hmod) override;
	void on_detach(HMODULE hmod) override;
    void on_present() override;
    void reset_height();
    void on_device_reset() override;
    void on_post_render_vr_framework_dx11(ID3D11DeviceContext* context, ID3D11Texture2D* texture, ID3D11RenderTargetView* rtv) override;
    void on_post_render_vr_framework_dx12(ID3D12GraphicsCommandList* command_list, ID3D12Resource* rt, D3D12_CPU_DESCRIPTOR_HANDLE* rtv) override;
    bool on_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override;
    void on_custom_event(const char* event_name, const char* event_data) override;
    void dispatch_lua_event(const char* event_name, const char* event_data) override;

    void print_all_objects();
    void test_console_manager();
    void test_engine(uevr::API::UGameEngine* engine);

    void on_pre_engine_tick(uevr::API::UGameEngine* engine, float delta) override;
    void on_post_engine_tick(uevr::API::UGameEngine* engine, float delta) override;
    void on_pre_slate_draw_window(UEVR_FSlateRHIRendererHandle renderer, UEVR_FViewportInfoHandle viewport_info) override;
    void on_post_slate_draw_window(UEVR_FSlateRHIRendererHandle renderer, UEVR_FViewportInfoHandle viewport_info) override;
    void on_pre_calculate_stereo_view_offset(UEVR_StereoRenderingDeviceHandle, int view_index, float world_to_meters,
        UEVR_Vector3f* position, UEVR_Rotatorf* rotation, bool is_double) override;
    void on_post_calculate_stereo_view_offset(UEVR_StereoRenderingDeviceHandle, int view_index, float world_to_meters,
        UEVR_Vector3f* position, UEVR_Rotatorf* rotation, bool is_double);
    void on_pre_viewport_client_draw(UEVR_UGameViewportClientHandle viewport_client, UEVR_FViewportHandle viewport, UEVR_FCanvasHandle canvas);
    void on_post_viewport_client_draw(UEVR_UGameViewportClientHandle viewport_client, UEVR_FViewportHandle viewport, UEVR_FCanvasHandle canvas);

	void on_xinput_get_state(uint32_t* retval, uint32_t user_index, XINPUT_STATE* state) override;
	void on_xinput_set_state(uint32_t* retval, uint32_t user_index, XINPUT_VIBRATION* vibration) override;
};