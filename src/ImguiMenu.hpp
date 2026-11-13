#pragma once
#include "Logger.hpp"
#include "uevr/API.hpp"
#include "Inputs.hpp"
#include "GameState.hpp"
#include "SettingsManager.hpp"
#include "WeaponManager.hpp"
#include "CameraController.hpp"
#include "CutsceneManager.hpp"
#include "PerformanceManager.hpp"
#include "PerformanceMonitor.hpp"

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
	const GameState* const gameState;
    SettingsManager* const settingsManager;
    WeaponManager* const weaponManager;
	ItemManager* const itemManager;
	CameraController* const cameraController;
	CutsceneManager* const cutsceneManager;
	PerformanceManager* const performanceManager;
	PerformanceMonitor* const performanceMonitor;

	HWND m_wnd{};
    bool m_initialized = false;
    bool m_was_rendering_desktop = false;

    std::recursive_mutex m_imgui_mutex;

    bool initializeImGui();
    void internalFrame();
    void handleVRcursor();

    float imguiSize = 1.2f;
	bool imguiSettingsInitialized = false;

public:
    ImGuiMenu(const GameState* gs, SettingsManager* sm, WeaponManager* wm, ItemManager* im, CameraController* cm, CutsceneManager* csm, PerformanceManager* perf, PerformanceMonitor* perfMonitor) : gameState(gs), settingsManager(sm), weaponManager(wm), itemManager(im), cameraController(cm), cutsceneManager(csm), performanceManager(perf), performanceMonitor(perfMonitor) {};

    void RenderImGuiFrame();
    void onDeviceReset();
    void RenderImGuiVRFrameDX11(ID3D11DeviceContext* context, ID3D11Texture2D* texture, ID3D11RenderTargetView* rtv);
    void RenderImGuiVRFrameDX12(ID3D12GraphicsCommandList* command_list, ID3D12Resource* rt, D3D12_CPU_DESCRIPTOR_HANDLE* rtv);

    void ProcessImGui();

	bool ImguiEnabled = true;
    bool ControllerInput = true;
    ImVec2 LastMousePosition = {-1.0f, -1.0f};
};