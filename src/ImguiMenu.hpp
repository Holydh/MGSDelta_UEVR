#pragma once
#include "Logger.hpp"
#include "uevr/API.hpp"
#include "Inputs.hpp"
#include "SettingsManager.hpp"
#include "WeaponManager.hpp"
#include "CameraController.hpp"

// only really necessary if you want to render to the screen
#include "renderlib/uevr_imgui/imgui_impl_dx11.h"
#include "renderlib/uevr_imgui/imgui_impl_dx12.h"
#include "renderlib/uevr_imgui/imgui_impl_win32.h"

#include "renderlib/rendering/d3d11.hpp"
#include "renderlib/rendering/d3d12.hpp"
#include "renderlib/imgui/imgui.h"

#include "glm/glm.hpp"
#include <glm/gtc/type_ptr.hpp>

class ImGuiMenu {
private:
    SettingsManager* const settingsManager;
    WeaponManager* const weaponManager;
	CameraController* const cameraController;
	PlayerManager* const playerManager;

	HWND m_wnd{};
    bool m_initialized = false;
    bool m_was_rendering_desktop = false;

    
    std::string_view Framework_UI_Distance = "UI_Framework_Distance";
    std::string_view Framework_UI_Size = "UI_Framework_Size";
    
    std::recursive_mutex m_imgui_mutex;

    bool initializeImGui();
    void onDeviceReset();
    void internalFrame();
    void handleVRcursor();
    void checkMouseFocus(float delta);
    float mouseUseTime = 1.0f;
    float mouseUseTimer = 1.0f;

    float imguiSize = 1.2f;
	bool imguiSettingsInitialized = false;

public:
    ImGuiMenu(SettingsManager* sm, WeaponManager* wm, CameraController* cm, PlayerManager* pm) : settingsManager(sm), weaponManager(wm), cameraController(cm), playerManager(pm) {};

    void InitializeImGuiSettings();

    void RenderImGuiFrame();
    void RenderImGuiVRFrameDX11(ID3D11DeviceContext* context, ID3D11Texture2D* texture, ID3D11RenderTargetView* rtv);
    void RenderImGuiVRFrameDX12(ID3D12GraphicsCommandList* command_list, ID3D12Resource* rt, D3D12_CPU_DESCRIPTOR_HANDLE* rtv);

    void ProcessImGui(float deltaTime);

	bool ImguiEnabled = true;
    bool MouseMovedThisFrame = false;
    bool MouseHasFocus = false;
    bool ControllerInput = true;
    ImVec2 LastMousePosition = {-1.0f, -1.0f};
};