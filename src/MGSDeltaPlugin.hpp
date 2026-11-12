#pragma once
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

//#include <memory>
#include "uevr/API.hpp"
#include "uevr/Plugin.hpp"

//#include <codecvt>

#include "ImguiMenu.hpp"
#include "CameraController.hpp"
#include "WeaponManager.hpp"
#include "MemoryManager.hpp"
#include "HookManager.hpp"
#include "PlayerManager.hpp"
#include "HudManager.hpp"
#include "Logger.hpp"
#include "ue/Reflection.hpp"

#define _CRT_SECURE_NO_WARNINGS 1

// Global accessor for our plugin.
class MGSDeltaPlugin;
extern std::unique_ptr<MGSDeltaPlugin> g_plugin;

class MGSDeltaPlugin : public uevr::Plugin {
private:
	void Toggle6DOF(bool enable);
    void CheckLevelChange();
	ue::WeakObject currentWorld;
	ue::WeakObject currentLevel;
	double lastWorldTime = 0.0;
    bool levelChanged = false;
    bool vr_enabled = false;

    //on_post_calculate_stereo_view_offset run twice per frame, so we need to run this only once
    bool ranOnce = false;
	float deltaTime = 0.0f;
public:
    MGSDeltaPlugin();
    virtual ~MGSDeltaPlugin();

    ImGuiMenu imGuiMenu;
    Inputs inputs;
    CameraController cameraController;
	WeaponManager weaponManager;
	MemoryManager memoryManager;
	HookManager hookManager;
	SettingsManager settingsManager;
	PlayerManager playerManager;
	PerformanceManager performanceManager;
	HudManager hudManager;
	PerformanceMonitor performanceMonitor;

    void on_initialize() override;
    //void on_dllmain(HMODULE hmod) override;
	void on_detach(HMODULE hmod) override;
    void on_present() override;
    void reset_height();
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
    void on_early_calculate_stereo_view_offset(UEVR_StereoRenderingDeviceHandle, int view_index, float world_to_meters,
        UEVR_Vector3f* position, UEVR_Rotatorf* rotation, bool is_double) override;
    void on_pre_calculate_stereo_view_offset(UEVR_StereoRenderingDeviceHandle, int view_index, float world_to_meters,
        UEVR_Vector3f* position, UEVR_Rotatorf* rotation, bool is_double) override;
    void on_post_calculate_stereo_view_offset(UEVR_StereoRenderingDeviceHandle, int view_index, float world_to_meters,
        UEVR_Vector3f* position, UEVR_Rotatorf* rotation, bool is_double);
    void on_pre_viewport_client_draw(UEVR_UGameViewportClientHandle viewport_client, UEVR_FViewportHandle viewport, UEVR_FCanvasHandle canvas);
    void on_post_viewport_client_draw(UEVR_UGameViewportClientHandle viewport_client, UEVR_FViewportHandle viewport, UEVR_FCanvasHandle canvas);

	void on_xinput_get_state(uint32_t* retval, uint32_t user_index, XINPUT_STATE* state) override;
	void on_xinput_set_state(uint32_t* retval, uint32_t user_index, XINPUT_VIBRATION* vibration) override;
};