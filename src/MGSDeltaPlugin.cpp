#define UEVR_PLUGIN_ENTRY
#include "MGSDeltaPlugin.hpp"

using namespace uevr;

std::unique_ptr<MGSDeltaPlugin> g_plugin = std::make_unique<MGSDeltaPlugin>();

MGSDeltaPlugin::MGSDeltaPlugin() : imGuiMenu(&settingsManager, &weaponManager, &cameraController, &playerManager, &performanceManager, &performanceMonitor),
cameraController(&playerManager, &settingsManager),
weaponManager(&settingsManager, &playerManager, &cameraController),
hookManager(&weaponManager, &cameraController, &playerManager), 
playerManager() {}; //Constructor
MGSDeltaPlugin::~MGSDeltaPlugin() {}//Destructor

void MGSDeltaPlugin::on_detach(HMODULE hmod)
{
	// Runs from DllMain (process exit, loader lock held): the engine and UEVR may already be torn
	// down, so no game or UEVR calls here. Only restore the patched game code.
	memoryManager.ToggleFrameOffsetCutsceneFixMemoryInstructions(true);
}

void MGSDeltaPlugin::on_initialize() {
	// Logs to the appdata UnrealVRMod log.txt file
	API::get()->log_info("%s", "on_initialize");
	std::filesystem::path debug_file = API::get()->get_persistent_dir() / "debug";
	// Weapon offsets (weapon and hands) read by the plugin. Kept out of the uobjecthook folder: UEVR loads every
	// file there as its own state files.
	std::filesystem::path weaponConfigsDir = API::get()->get_persistent_dir() / "weapons";
	// Check if the file exists
	g_DebugEnabled = std::filesystem::exists(debug_file);
	if (g_DebugEnabled) {
		AllocConsole();
		freopen("CONOUT$", "w", stdout);
	}
	Logger::DebugPrint("\nMGSDelta 6DOF plugin - This is a Work In Progress beta build !\n");
	Logger::DebugPrint("Game hasn't been fully tested yet. Expect bugs and issues.");
	Logger::DebugPrint("You can report issues and ask for help in the F2VR discord server in the UEVR MGS Delta channel.");
	Logger::DebugPrint("Don't reupload this plugin elsewhere.\n");

	Utilities::InitHelperClasses();
	weaponManager.InitWeaponManager();
	settingsManager.Load();
	settingsManager.LoadWeaponConfigs(weaponConfigsDir);
	performanceManager.Init();
	performanceMonitor.Start();
	playerManager.ConvertAllRequiredStringToFnames();
	hookManager.Initialize();
	hookManager.hook_bp_fn(L"Class /Script/Ics.IcsAnimInstance", L"OnEnteredState", NULL, (UEVR_UFunction_NativePostFn)HookManager::mod_OnEnteredState_post, true);
	hookManager.hook_bp_fn(L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerEludeAction.ABP_PlayerEludeAction_C", L"OnEnteredState", NULL, (UEVR_UFunction_NativePostFn)HookManager::mod_PlayerFalling_OnEnteredState_post, true);
	hookManager.hook_bp_fn(L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerKnifeReadyAction.ABP_PlayerKnifeReadyAction_C", L"OnEnteredState", NULL, (UEVR_UFunction_NativePostFn)HookManager::mod_PlayerKnife_OnEnteredState_post, true);
	hookManager.hook_bp_fn(L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerCqcAction.ABP_PlayerCqcAction_C", L"OnEnteredState", NULL, (UEVR_UFunction_NativePostFn)HookManager::mod_PlayerCqcAction_OnEnteredState_post, true);
	hookManager.hook_bp_fn(L"BlueprintGeneratedClass /Game/Gsr/Blueprints/Player/BP_Player.BP_Player_C", L"OnChangeCamouflageApply", NULL, (UEVR_UFunction_NativePostFn)HookManager::mod_GsrPlayer_OnChangeCamouflageApply_post, true);
	hookManager.hook_bp_fn(L"BlueprintGeneratedClass /Game/Cinematics/Blueprints/BP_CobraLevelSequence.BP_CobraLevelSequence_C", L"OnCameraCut", NULL, (UEVR_UFunction_NativePreFn)HookManager::mod_CobraLevelSequence_OnCameraCut_post, true);
	hookManager.hook_bp_fn(L"Class /Script/Ics.IcsActorActionModule", L"OnOverrideRootMotionVelocityBP", NULL, (UEVR_UFunction_NativePostFn)HookManager::mod_IcsActorActionModule_OnOverrideRootMotionVelocity_post, true);

	memoryManager.InitMemoryManager();
	memoryManager.ToggleFrameOffsetCutsceneFixMemoryInstructions(false);
	hookManager.HookFireFunction();
	hookManager.HookSetEnableWallCollision();
}

void MGSDeltaPlugin::on_present() {
	if (!imGuiMenu.ImguiEnabled)
		return;
	
	imGuiMenu.RenderImGuiFrame();
}

// Called before the swapchain is resized: release our backbuffer references or ResizeBuffers fails
void MGSDeltaPlugin::on_device_reset() {
	imGuiMenu.onDeviceReset();
}

void MGSDeltaPlugin::on_post_render_vr_framework_dx11(ID3D11DeviceContext* context, ID3D11Texture2D* texture, ID3D11RenderTargetView* rtv) {
	PLUGIN_LOG_ONCE("Post Render VR Framework DX11");
	imGuiMenu.RenderImGuiVRFrameDX11(context, texture, rtv);
}

void MGSDeltaPlugin::on_post_render_vr_framework_dx12(ID3D12GraphicsCommandList* command_list, ID3D12Resource* rt, D3D12_CPU_DESCRIPTOR_HANDLE* rtv) {
	PLUGIN_LOG_ONCE("Post Render VR Framework DX12");
	imGuiMenu.RenderImGuiVRFrameDX12(command_list, rt, rtv);
}

bool MGSDeltaPlugin::on_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (imGuiMenu.ImguiEnabled && API::get()->param()->functions->is_drawing_ui())
	{
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
		{
				return false;
		}
	}
	return true;
}

void MGSDeltaPlugin::on_custom_event(const char* event_name, const char* event_data) {
	if (strcmp(event_name, "HandsCreated") == 0)
	{
		// "0": the Lua script destroyed the VR hands
		if (strcmp(event_data, "0") == 0)
			playerManager.Cleanup();
		if (strcmp(event_data, "1") == 0)
		{
			playerManager.FetchMotionControllers();
			weaponManager.SendHandStateToLua();
			weaponManager.AttachHandNextFrame = true;
			weaponManager.ApplyHandMaterialToVRHands();
		}
	}
	// strtof: never throws on unexpected data (std::stof does, and an exception here crashes the game)
	if (strcmp(event_name, "MouseX") == 0)
	{
		imGuiMenu.LastMousePosition.x = std::strtof(event_data, nullptr);
	}
	if (strcmp(event_name, "MouseY") == 0)
	{
		imGuiMenu.LastMousePosition.y = std::strtof(event_data, nullptr);
	}
}

void MGSDeltaPlugin::dispatch_lua_event(const char* event_name, const char* event_data) {
	API::get()->dispatch_lua_event(event_name, event_data);
}

void MGSDeltaPlugin::Toggle6DOF(bool enable)
{
	if (enable && !vr_enabled)
	{
		appliedRoomscaleMovement.reset(); // written again by UpdateRoomscaleMovement
		cameraController.ToggleCameraWobblingController(false);
		playerManager.ToggleMeshesVisibility(false);
		playerManager.ToggleReticleVisibility(false);
		if (!cameraController.SpawnCameraAttachementActor())
			return;
		cameraController.AttachCameraToAttachementActor();
		API::VR::set_mod_value("VR_2DScreenMode", false);
		API::VR::set_mod_value("VR_WorldScale", 1.0f);
		API::get()->dispatch_lua_event("PlayerTookControl", "1");
		vr_enabled = enable;
	}

	if(!enable && vr_enabled)
	{
		cameraController.ToggleCameraWobblingController(true);
		playerManager.ToggleMeshesVisibility(true);
		playerManager.ToggleReticleVisibility(true);
		cameraController.DetachCameraFromAttachementActor();
		cameraController.DestroyCameraAttachementActor();
		API::get()->dispatch_lua_event("PlayerTookControl", "0");
		cameraController.DestroyCustom2DScreen();

		vr_enabled = enable;
	}
}

// The only writer of VR_RoomscaleMovement: off while the game moves the view around Snake (peeking,
// CQC grab, cardboard box). Written only when it changes, so the UEVR menu value isn't fought every frame.
void MGSDeltaPlugin::UpdateRoomscaleMovement()
{
	const bool roomscale = !(playerManager.IsPeepingInternal || playerManager.IsCqcGrab || cameraController.CardBoardEquipped);
	if (appliedRoomscaleMovement == roomscale)
		return;
	API::VR::set_mod_value("VR_RoomscaleMovement", roomscale);
	appliedRoomscaleMovement = roomscale;
}

void MGSDeltaPlugin::CheckLevelChange()
{
	auto world = ue::get_world();
	// A new world can be allocated at the address (and object slot) of the previous one, e.g. when
	// the same map is reloaded: its clock restarting from zero catches that case.
	const double worldTime = world ? ue::call_static<double>(ue::lib::GameplayStatics, L"GetRealTimeSeconds", world) : 0.0;
	if (currentWorld != world || worldTime < lastWorldTime)
	{
		currentWorld = world;
		levelChanged = true;
	}
	lastWorldTime = worldTime;
	if (world == nullptr)
		return;
	auto level = ue::get<ue::UObject*>(world, L"PersistentLevel");
	if (currentLevel != level)
	{
		currentLevel = level;
		levelChanged = true;
		Logger::DebugPrint("Level changed : Fetching new data");
	}
}

void MGSDeltaPlugin::on_pre_engine_tick(API::UGameEngine* engine, float delta) {
	PLUGIN_LOG_ONCE("Pre Engine Tick: %f", delta);
	performanceMonitor.OnPreEngineTickStart();
	deltaTime = delta;
	performanceManager.Update(delta);
	CheckLevelChange();

	if (levelChanged)
	{
		levelChanged = false;
		// Everything from the previous world is already destroyed by now: forget it (weak references
		// make any leftover call a no-op) and start again from a clean state.
		Toggle6DOF(false);
		cameraController.DestroyCustom2DScreen();
		weaponManager.Cleanup();
		playerManager.Cleanup();
		// Turns 6DOF back on below if the player still has control in the new level
		playerManager.IsPlayerControlled.SetPrevious(false);
		// Blueprint classes that weren't loaded at startup (or were reloaded) can be hooked now
		hookManager.RetryFailedHooks();
	}

	playerManager.FetchGameState();

	if (!playerManager.IngameUI())
	{
		playerManager.IsPlayerControlled = false;
	}

	if (playerManager.PlayerGotControl())
	{
		Toggle6DOF(true);
		Logger::DebugPrint("Player took control");
	}

	if (playerManager.PlayerLostControl())
	{
		Toggle6DOF(false);
		Logger::DebugPrint("Player lost control");
	}

	if (playerManager.IsPlayerControlled)
	{
		Inputs::CheckSecondHandGrip(delta);
		playerManager.UpdateForkAndKnifeMeleeState(delta);
		playerManager.GetCurrentAimStateAndCameraMode();
		weaponManager.FetchEquipController();
		weaponManager.ProcessItem();
		weaponManager.ProcessWeapon(delta);
		weaponManager.FetchWeaponMeshAimData();
		weaponManager.SetAimDirection();

		playerManager.UpdateLinearLocomotion(settingsManager.Settings.linearLocomotion, settingsManager.Settings.gameTakesMovementControl,
			cameraController.ViewRotation.Yaw, delta);
		playerManager.UpdateCompactCollision(settingsManager.Settings.linearLocomotion, delta);

		static Tracked<bool> collisionLogKeyPressed = false;
		collisionLogKeyPressed = g_DebugEnabled && Inputs::IsKeyPressed(VK_F9);
		if (collisionLogKeyPressed.Rose())
			playerManager.LogCollisionAndRotation(cameraController.ViewRotation.Yaw);
		collisionLogKeyPressed.Commit();

		playerManager.GetCurrentPlayerStance(delta);
		playerManager.UpdatePeepingAndCqcGrabState(delta);
		playerManager.ProcessVRHandsVisibility();
		cameraController.ProcessRoomscaleStance();

		weaponManager.ProcessTwoHandsGrabWeapon();

		UpdateRoomscaleMovement();
	}
	else
	{
		uevr::API::UObjectHook::remove_all_motion_controller_states();
		playerManager.LinearLocomotionActive = false;
		// Cutscenes and menus get the game's own collision back
		playerManager.UpdateCompactCollision(false, delta);

		cameraController.CheckCutscenePlaying();
		if (playerManager.IsPlayerControlled.Previous())
		{
			weaponManager.DetachHandFromWeapon();
			weaponManager.DetachSecondHandFromWeapon();
		}

		cameraController.ApplyCutsceneSettings();

		if (playerManager.CurrentMenuType.Entered(ue::EGsrUIMenuType::Radio))
		{
			cameraController.FixRadioMenuRendering();
			API::VR::set_mod_value("VR_WorldScale", settingsManager.Settings.enable3DRadio ? 1.0f : 0.0f);
		}
		if (playerManager.CurrentMenuType.Entered(ue::EGsrUIMenuType::SV))
		{
			cameraController.FixSurvivalViewerMenuRendering();
			API::VR::set_mod_value("VR_WorldScale", settingsManager.Settings.enable3DSurvivalView ? 1.0f : 0.0f);
		}
	}

	weaponManager.ToggleDummyCQCKnifeMeshVisibility();

	if (playerManager.IsPlayerControlled || cameraController.ActualCutsceneState == CameraController::CutsceneState::Playing3D)
		cameraController.ProcessCamera(delta);
	cameraController.ProcessCameraResetTimer(delta);

	imGuiMenu.ProcessImGui();

	// Last: the tracked values become last frame's values
	playerManager.EndFrame();
	weaponManager.EndFrame();
	cameraController.EndFrame();
	Inputs::EndFrame();
	ranOnce = false;
	performanceMonitor.OnPreEngineTickEnd();
}

void MGSDeltaPlugin::on_post_engine_tick(API::UGameEngine* engine, float delta) {
	PLUGIN_LOG_ONCE("Post Engine Tick: %f", delta);
	performanceMonitor.OnPostEngineTick();
}

void MGSDeltaPlugin::on_pre_slate_draw_window(UEVR_FSlateRHIRendererHandle renderer, UEVR_FViewportInfoHandle viewport_info) {
	PLUGIN_LOG_ONCE("Pre Slate Draw Window");
}

void MGSDeltaPlugin::on_post_slate_draw_window(UEVR_FSlateRHIRendererHandle renderer, UEVR_FViewportInfoHandle viewport_info) {
	PLUGIN_LOG_ONCE("Post Slate Draw Window");
}

void MGSDeltaPlugin::on_early_calculate_stereo_view_offset(UEVR_StereoRenderingDeviceHandle, int view_index, float world_to_meters, UEVR_Vector3f* position, UEVR_Rotatorf* rotation, bool is_double)
{
	PLUGIN_LOG_ONCE("Early Calculate Stereo View Offset");
	static ue::SingletonRef cobraCutsceneCameraCtrlMngr_c{ L"Class /Script/CobraCutScene.CobraCutsceneCameraControlManagerComponent" };
	auto cobraCutsceneCameraCtrlMngr = cobraCutsceneCameraCtrlMngr_c.get(); // may be null: ue::get then returns false
	if (playerManager.IsPlayerControlled ||
		(cameraController.ActualCutsceneState == CameraController::CutsceneState::Playing3D && (settingsManager.Settings.enableImmersiveMode || ue::get<bool>(cobraCutsceneCameraCtrlMngr, L"bIsFpv"))))
	{
		UEVR_Vector3d* pos = (UEVR_Vector3d*)position;
		UEVR_Rotatord* rot = (UEVR_Rotatord*)rotation;
		pos->x = cameraController.NewCameraPosition.x;
		pos->y = cameraController.NewCameraPosition.y;
		pos->z = cameraController.NewCameraPosition.z;
		rot->pitch = cameraController.NewCameraRotation.pitch;
		rot->yaw = cameraController.NewCameraRotation.yaw;
		rot->roll = cameraController.NewCameraRotation.roll;
	}

	//sv_main
	if (playerManager.CurrentMenuType == ue::EGsrUIMenuType::SV)
	{
		UEVR_Vector3d* pos = (UEVR_Vector3d*)position;
		UEVR_Rotatord* rot = (UEVR_Rotatord*)rotation;
		pos->y *= 0.4f;
		rot->pitch = 0.0f;
		rot->yaw = -90.0f;
		rot->roll = 0.0f;
	}
}

void MGSDeltaPlugin::on_pre_calculate_stereo_view_offset(UEVR_StereoRenderingDeviceHandle, int view_index, float world_to_meters,
	UEVR_Vector3f* position, UEVR_Rotatorf* rotation, bool is_double)
{
	PLUGIN_LOG_ONCE("Pre Calculate Stereo View Offset");
}

void MGSDeltaPlugin::on_post_calculate_stereo_view_offset(UEVR_StereoRenderingDeviceHandle, int view_index, float world_to_meters,
	UEVR_Vector3f* position, UEVR_Rotatorf* rotation, bool is_double)
{
	PLUGIN_LOG_ONCE("Post Calculate Stereo View Offset");
	// Final view rotation: base camera yaw (right stick) + headset, real or simulated
	if (is_double)
	{
		const auto* rot = (UEVR_Rotatord*)rotation;
		cameraController.ViewRotation = { rot->pitch, rot->yaw, rot->roll };
	}
	else
		cameraController.ViewRotation = { rotation->pitch, rotation->yaw, rotation->roll };

	//we need to override uevr attachement controls so we're running this later
	if (playerManager.IsPlayerControlled && playerManager.SecondHandAttachedToWeapon && ranOnce)
	{
		weaponManager.ApplyRecoil(deltaTime);
		weaponManager.ProcessTwoHandsGrabWeaponLocationAndRotation();
	}

	ranOnce = true;
}

void MGSDeltaPlugin::on_pre_viewport_client_draw(UEVR_UGameViewportClientHandle viewport_client, UEVR_FViewportHandle viewport, UEVR_FCanvasHandle canvas) {
	PLUGIN_LOG_ONCE("Pre Viewport Client Draw");
}

void MGSDeltaPlugin::on_post_viewport_client_draw(UEVR_UGameViewportClientHandle viewport_client, UEVR_FViewportHandle viewport, UEVR_FCanvasHandle canvas) {
}

void MGSDeltaPlugin::on_xinput_get_state(uint32_t* retval, uint32_t user_index, XINPUT_STATE* state)
{
	PLUGIN_LOG_ONCE("XInput Get State");

	// Must run first: may replace the state with the one UEVR built in a nested inner call
	// (GameInput XInput forwarder), so the code below and the game see the correct inputs.
	Inputs::FixNestedXInputCall(retval, user_index, state);

	float rawRightXInput = (float)state->Gamepad.sThumbRX / 32768.0f * 2.0f; // Normalize to -1.0 to 1.0 and scale
	cameraController.YawInput = rawRightXInput > settingsManager.Settings.cameraRotationDeadZone || rawRightXInput < -settingsManager.Settings.cameraRotationDeadZone ? rawRightXInput : 0.0f;

	float rawRightYInput = (float)state->Gamepad.sThumbRY / 32768.0f * 2.0f; // Normalize to -1.0 to 1.0 and scale
	weaponManager.ZoomInput = rawRightYInput > 0.5f || rawRightYInput < -0.5f ? rawRightYInput : 0.0f;

	Inputs::LeftGripInputPressed = (state->Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;

	playerManager.MoveStickX = (float)state->Gamepad.sThumbLX / 32768.0f;
	playerManager.MoveStickY = (float)state->Gamepad.sThumbLY / 32768.0f;
}

void MGSDeltaPlugin::on_xinput_set_state(uint32_t* retval, uint32_t user_index, XINPUT_VIBRATION* vibration)
{
	PLUGIN_LOG_ONCE("XInput Set State");
}