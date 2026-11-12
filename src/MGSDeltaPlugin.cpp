#define UEVR_PLUGIN_ENTRY
#include "MGSDeltaPlugin.hpp"

using namespace uevr;

#define MAX_DEBUG_MESSAGE_SIZE 2048

std::unique_ptr<MGSDeltaPlugin> g_plugin = std::make_unique<MGSDeltaPlugin>();

MGSDeltaPlugin::MGSDeltaPlugin() : imGuiMenu(&settingsManager, &weaponManager, &cameraController, &playerManager, &performanceManager, &performanceMonitor),
inputs(),
cameraController(&playerManager, &settingsManager), 
weaponManager(/*&memoryManager,*/ &settingsManager, &playerManager, &cameraController),
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
	std::filesystem::path config_file = API::get()->get_persistent_dir() / "plugins" / "aimfix.txt";
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

	//hookManager.SetGunAdjustValuesFromConfig(config_file);
	/*hookManager.hook_ISetSmoothedAimPosition_fn();*/
	Utilities::InitHelperClasses();
	weaponManager.InitWeaponManager();
	settingsManager.InitSettingsManager();
	settingsManager.GetOrCreateConfigFile();
	settingsManager.LoadAllConfigs(weaponConfigsDir);
	performanceManager.Init();
	performanceMonitor.Start();
	playerManager.ConvertAllRequiredStringToFnames();
	hookManager.Initialize();
	hookManager.hook_bp_fn(L"Class /Script/Ics.IcsAnimInstance", L"OnEnteredState", NULL, (UEVR_UFunction_NativePostFn)HookManager::mod_OnEnteredState_post, true);
	hookManager.hook_bp_fn(L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerEludeAction.ABP_PlayerEludeAction_C", L"OnEnteredState", NULL, (UEVR_UFunction_NativePostFn)HookManager::mod_PlayerFalling_OnEnteredState_post, true);
	hookManager.hook_bp_fn(L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerKnifeReadyAction.ABP_PlayerKnifeReadyAction_C", L"OnEnteredState", NULL, (UEVR_UFunction_NativePostFn)HookManager::mod_PlayerKnife_OnEnteredState_post, true);
	hookManager.hook_bp_fn(L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerCqcAction.ABP_PlayerCqcAction_C", L"OnEnteredState", NULL, (UEVR_UFunction_NativePostFn)HookManager::mod_PlayerCqcAction_OnEnteredState_post, true);
	hookManager.hook_bp_fn(L"BlueprintGeneratedClass /Game/Gsr/Blueprints/Player/BP_Player.BP_Player_C", L"OnChangeCamouflageApply", NULL, (UEVR_UFunction_NativePostFn)HookManager::mod_GsrPlayer_OnChangeCamouflageApply_post, true);
	//hookManager.hook_bp_fn(L"BlueprintGeneratedClass /Game/Gsr/Blueprints/Player/BP_Player.BP_Player_C", L"OnStartUIMenu", NULL, (UEVR_UFunction_NativePostFn)HookManager::mod_GsrPlayer_OnStartUIMenu_post, true);
	//hookManager.hook_bp_fn(L"BlueprintGeneratedClass /Game/Gsr/Blueprints/Player/BP_Player.BP_Player_C", L"OnEndUIMenu", NULL, (UEVR_UFunction_NativePostFn)HookManager::mod_GsrPlayer_OnEndUIMenu_post, true);
	//hookManager.hook_bp_fn(L"BlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Effect/BP_GsrPlayerDamageFilterController.BP_GsrPlayerDamageFilterController_C", L"OnStartUIMenu", NULL, (UEVR_UFunction_NativePostFn)HookManager::mod_BP_GsrPlayerDamageFilterController_OnStartUIMenu_post, true);
	//hookManager.hook_bp_fn(L"BlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Effect/BP_GsrPlayerDamageFilterController.BP_GsrPlayerDamageFilterController_C", L"OnEndUIMenu", NULL, (UEVR_UFunction_NativePostFn)HookManager::mod_BP_GsrPlayerDamageFilterController_OnEndUIMenu_post, true);
	//hookManager.hook_bp_fn(L"WidgetBlueprintGeneratedClass /CobraUI/Blueprint/sv_main.sv_main_C", L"OnSurvivalViewerChanged", (UEVR_UFunction_NativePreFn)HookManager::mod_sv_main_OnSurvivalViewerChanged_pre, NULL, false);

	hookManager.hook_bp_fn(L"BlueprintGeneratedClass /Game/Cinematics/Blueprints/BP_CobraLevelSequence.BP_CobraLevelSequence_C", L"OnCameraCut", NULL, (UEVR_UFunction_NativePreFn)HookManager::mod_CobraLevelSequence_OnCameraCut_post, true);

	//hookManager.hook_bp_fn(L"Class /Script/MGS3.CutsceneObject", L"OnSequencePlay", NULL, (UEVR_UFunction_NativePostFn)HookManager::mod_CutsceneObject_OnSequencePlay_post, true);
	//hookManager.hook_bp_fn(L"Class /Script/MGS3.CutsceneObject", L"OnSequencePause", NULL, (UEVR_UFunction_NativePostFn)HookManager::mod_CutsceneObject_OnSequencePause_post, true);
	//hookManager.hook_bp_fn(L"Class /Script/MGS3.CutsceneObject", L"PlaySequenceFinished", NULL, (UEVR_UFunction_NativePostFn)HookManager::mod_CutsceneObject_PlaySequenceFinished_post, true);
	//hookManager.hook_bp_fn(L"BlueprintGeneratedClass /Game/Blueprints/Cutscene/BP_CutsceneCameraControlManager.BP_CutsceneCameraControlManager_C", L"PlayMediaPlayer", NULL, (UEVR_UFunction_NativePreFn)HookManager::mod_CobraCutsceneCameraControlManager_PlayMediaPlayer_post, true);
	//hookManager.hook_bp_fn(L"BlueprintGeneratedClass /Game/Blueprints/Cutscene/BP_CutsceneCameraControlManager.BP_CutsceneCameraControlManager_C", L"PauseMediaPlayer", NULL, (UEVR_UFunction_NativePreFn)HookManager::mod_CobraCutsceneCameraControlManager_PauseMediaPlayer_post, true);
	
	//AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerKnifeReadyAction_C.ABP_PlayerKnifeReadyAction_C
	//hookManager.hook_bp_fn(L"Class /Script/Gsr.GsrPlayerCameraInfoInterface", L"ISetCameraRequestModelVisibility", (UEVR_UFunction_NativePostFn)HookManager::mod_ISetCameraRequestModelVisibility_post ,NULL, true);

	//weaponController.Initialize();
	memoryManager.InitMemoryManager();
	memoryManager.ToggleFrameOffsetCutsceneFixMemoryInstructions(false);
	hookManager.HookFireFunction();
	//hookManager.HookCameraModifyFunction();
}

void MGSDeltaPlugin::on_present() {
	if (!imGuiMenu.ImguiEnabled)
		return;
	
	imGuiMenu.RenderImGuiFrame();
}

void MGSDeltaPlugin::reset_height() {
	auto& api = API::get();
	auto vr = api->param()->vr;
	UEVR_Vector3f origin{};
	vr->get_standing_origin(&origin);

	UEVR_Vector3f hmd_pos{};
	UEVR_Quaternionf hmd_rot{};
	vr->get_pose(vr->get_hmd_index(), &hmd_pos, &hmd_rot);

	origin.y = hmd_pos.y;

	vr->set_standing_origin(&origin);
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
			imGuiMenu.MouseMovedThisFrame = true;
			return false;
		}
	}
	return true;
}

void MGSDeltaPlugin::on_custom_event(const char* event_name, const char* event_data) {
	/*Logger::DebugPrint("Custom Event: %s %s", event_name, event_data);*/
	if (strcmp(event_name, "HandsCreated") == 0)
	{
		// "0": the Lua script destroyed the VR hands
		if (strcmp(event_data, "0") == 0)
			playerManager.Cleanup();
		if (strcmp(event_data, "1") == 0)
		{
			//Toggle6DOF(false);
			//Toggle6DOF(true);
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
		//Logger::DebugPrint("MouseX: %f", io.MousePos.x);
	}
	if (strcmp(event_name, "MouseY") == 0)
	{
		imGuiMenu.LastMousePosition.y = std::strtof(event_data, nullptr);
		//Logger::DebugPrint("MouseY: %f", io.MousePos.y);
	}
}

void MGSDeltaPlugin::dispatch_lua_event(const char* event_name, const char* event_data) {
	API::get()->dispatch_lua_event(event_name, event_data);
}

void MGSDeltaPlugin::print_all_objects() {
	API::get()->log_info("Printing all objects...");

	API::get()->log_info("Chunked: %i", API::FUObjectArray::is_chunked());
	API::get()->log_info("Inlined: %i", API::FUObjectArray::is_inlined());
	API::get()->log_info("Objects offset: %i", API::FUObjectArray::get_objects_offset());
	API::get()->log_info("Item distance: %i", API::FUObjectArray::get_item_distance());
	API::get()->log_info("Object count: %i", API::FUObjectArray::get()->get_object_count());

	const auto objects = API::FUObjectArray::get();

	if (objects == nullptr) {
		API::get()->log_error("Failed to get FUObjectArray");
		return;
	}

	for (int32_t i = 0; i < objects->get_object_count(); ++i) {
		const auto object = objects->get_object(i);

		if (object == nullptr) {
			continue;
		}

		const auto name = object->get_full_name();

		if (name.empty()) {
			continue;
		}

		std::string name_narrow{ std::wstring_convert<std::codecvt_utf8<wchar_t>>{}.to_bytes(name) };

		API::get()->log_info(" [%d]: %s", i, name_narrow.c_str());
	}
}


void MGSDeltaPlugin::test_console_manager() {
	const auto console_manager = API::get()->get_console_manager();

	if (console_manager != nullptr) {
		API::get()->log_info("Console manager @ 0x%p", console_manager);
		const auto& objects = console_manager->get_console_objects();

		for (const auto& object : objects) {
			if (object.key != nullptr) {
				// convert from wide to narrow string (we do not have utility::narrow in this context).
				std::string key_narrow{ std::wstring_convert<std::codecvt_utf8<wchar_t>>{}.to_bytes(object.key) };
				if (object.value != nullptr) {
					const auto command = object.value->as_command();

					if (command != nullptr) {
						API::get()->log_info(" Console COMMAND: %s @ 0x%p", key_narrow.c_str(), object.value);
					}
					else {
						API::get()->log_info(" Console VARIABLE: %s @ 0x%p", key_narrow.c_str(), object.value);
					}
				}
			}
		}

		auto cvar = console_manager->find_variable(L"r.Color.Min");

		if (cvar != nullptr) {
			API::get()->log_info("Found r.Color.Min @ 0x%p (%f)", cvar, cvar->get_float());
		}
		else {
			API::get()->log_error("Failed to find r.Color.Min");
		}

		auto cvar2 = console_manager->find_variable(L"r.Upscale.Quality");

		if (cvar2 != nullptr) {
			API::get()->log_info("Found r.Upscale.Quality @ 0x%p (%d)", cvar2, cvar2->get_int());
			cvar2->set(cvar2->get_int() + 1);
		}
		else {
			API::get()->log_error("Failed to find r.Upscale.Quality");
		}
	}
	else {
		API::get()->log_error("Failed to find console manager");
	}
}

void MGSDeltaPlugin::test_engine(API::UGameEngine* engine) {
	// Log the UEngine name.
	const auto uengine_name = engine->get_full_name();

	// Convert from wide to narrow string (we do not have utility::narrow in this context).
	std::string uengine_name_narrow{ std::wstring_convert<std::codecvt_utf8<wchar_t>>{}.to_bytes(uengine_name) };

	API::get()->log_info("Engine name: %s", uengine_name_narrow.c_str());

	// Test if we can dcast to UObject.
	{
		const auto engine_as_object = engine->dcast<API::UObject>();

		if (engine != nullptr) {
			API::get()->log_info("Engine successfully dcast to UObject");
		}
		else {
			API::get()->log_error("Failed to dcast Engine to UObject");
		}
	}

	// Go through all of engine's fields and log their names.
	const auto engine_class_ours = (API::UStruct*)engine->get_class();
	for (auto super = engine_class_ours; super != nullptr; super = super->get_super()) {
		for (auto field = super->get_child_properties(); field != nullptr; field = field->get_next()) {
			const auto field_fname = field->get_fname();
			const auto field_name = field_fname->to_string();
			const auto field_class = field->get_class();

			std::wstring prepend{};

			if (field_class != nullptr) {
				const auto field_class_fname = field_class->get_fname();
				const auto field_class_name = field_class_fname->to_string();

				prepend = field_class_name + L" ";
			}

			// Convert from wide to narrow string (we do not have utility::narrow in this context).
			std::string field_name_narrow{ std::wstring_convert<std::codecvt_utf8<wchar_t>>{}.to_bytes(prepend + field_name) };
			API::get()->log_info(" Field name: %s", field_name_narrow.c_str());
		}
	}

	// Check if we can find the GameInstance and call is_a() on it.
	const auto game_instance = engine->get_property<API::UObject*>(L"GameInstance");

	if (game_instance != nullptr) {
		const auto game_instance_class = API::get()->find_uobject<API::UClass>(L"Class /Script/Engine.GameInstance");

		if (game_instance->is_a(game_instance_class)) {
			const auto& local_players = game_instance->get_property<API::TArray<API::UObject*>>(L"LocalPlayers");

			if (local_players.count > 0 && local_players.data != nullptr) {
				const auto local_player = local_players.data[0];


			}
			else {
				API::get()->log_error("Failed to find LocalPlayers");
			}

			API::get()->log_info("GameInstance is a UGameInstance");
		}
		else {
			API::get()->log_error("GameInstance is not a UGameInstance");
		}
	}
	else {
		API::get()->log_error("Failed to find GameInstance");
	}

	// Find the Engine object and compare it to the one we have.
	const auto engine_class = API::get()->find_uobject<API::UClass>(L"Class /Script/Engine.GameEngine");
	if (engine_class != nullptr) {
		// Round 1, check if we can find it via get_first_object_by_class.
		const auto engine_searched = engine_class->get_first_object_matching<API::UGameEngine>(false);

		if (engine_searched != nullptr) {
			if (engine_searched == engine) {
				API::get()->log_info("Found Engine object @ 0x%p", engine_searched);
			}
			else {
				API::get()->log_error("Found Engine object @ 0x%p, but it's not the same as the one we have", engine_searched);
			}
		}
		else {
			API::get()->log_error("Failed to find Engine object");
		}

		// Round 2, check if we can find it via get_objects_by_class.
		const auto objects = engine_class->get_objects_matching<API::UGameEngine>(false);

		if (!objects.empty()) {
			for (const auto& obj : objects) {
				if (obj == engine) {
					API::get()->log_info("Found Engine object @ 0x%p", obj);
				}
				else {
					API::get()->log_info("Found unrelated Engine object @ 0x%p", obj);
				}
			}
		}
		else {
			API::get()->log_error("Failed to find Engine objects");
		}
	}
	else {
		API::get()->log_error("Failed to find Engine class");
	}
}

void MGSDeltaPlugin::Toggle6DOF(bool enable)
{
	if (enable && !vr_enabled)
	{
		//cameraController.DisableGrassFading();
		cameraController.ToggleCameraWobblingController(false);
		//memoryManager.ToggleAimRelatedMemoryInstructions(false);
		playerManager.ToggleMeshesVisibility(bodyIK.Enabled, false);
		playerManager.ToggleReticleVisibility(false);
		if (!cameraController.SpawnCameraAttachementActor())
			return;
		cameraController.AttachCameraToAttachementActor();
		API::VR::set_mod_value("VR_2DScreenMode", false);
		API::VR::set_mod_value("VR_WorldScale", 1.0f);
		uevr::API::VR::set_mod_value("VR_RoomscaleMovement", true);
		//Utilities::DebugCigar0 = Utilities::SpawnDebugCigars(UEVR_Vector3d{114.0f, -3495.0f, 500.0f});
		//Utilities::DebugCigar1 = Utilities::SpawnDebugCigars(UEVR_Vector3d{114.0f, -3495.0f, 500.0f});
		API::get()->dispatch_lua_event("PlayerTookControl", "1");
		//cameraController.SetNearClipPlane(10000.0f);
		vr_enabled = enable;
	}

	if(!enable && vr_enabled)
	{
		cameraController.ToggleCameraWobblingController(true);
		//memoryManager.ToggleAimRelatedMemoryInstructions(true);
		playerManager.ToggleMeshesVisibility(true, true);
		playerManager.ToggleReticleVisibility(true);
		cameraController.DetachCameraFromAttachementActor();
		cameraController.DestroyCameraAttachementActor();
		API::get()->dispatch_lua_event("PlayerTookControl", "0");
		cameraController.DestroyCustom2DScreen();

		vr_enabled = enable;
	}
	//Logger::DebugPrint("%i", API::VR::get_mod_value<bool>("VR_2DScreenMode"));
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
	//auto start = std::chrono::high_resolution_clock::now();
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
		bodyIK.Cleanup();
		// Turns 6DOF back on below if the player still has control in the new level
		playerManager.WasPlayerControlled = false;
		// Blueprint classes that weren't loaded at startup (or were reloaded) can be hooked now
		hookManager.RetryFailedHooks();
	}

	playerManager.FetchGameState();

	if (!playerManager.IngameUI())
	{
		playerManager.IsPlayerControlled = false;
	}	

	//test
	//if (!cameraController.IsMenuOpen && cameraController.WasMenuOpen)
	//{
	//	playerManager.SetPlayerStance(ue::EGsrPlayerStance::Crawl);

	//}

	if (playerManager.PlayerGotControl())
	{
		Toggle6DOF(true);
		Logger::DebugPrint("Player took control");
		//playerManager.SetPlayerStance(ue::EGsrPlayerStance::Crawl);
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
		/*Logger::DebugPrint("Before ResolveAddresses");*/
		/*memoryManager.ResolveAddresses();*/
	/*	Logger::DebugPrint("Before FetchEquipController");*/
		weaponManager.FetchEquipController();
		weaponManager.ProcessItem();
		/*Logger::DebugPrint("Before ProcessWeapon");*/
		weaponManager.ProcessWeapon(delta);
		//Logger::DebugPrint("Before FetchWeaponMeshAimData");

		weaponManager.FetchWeaponMeshAimData();

		/*Logger::DebugPrint("Before ApplyAimDataToMemory");*/
		//weaponController.ApplyAimDataToMemory();
		//Logger::DebugPrint("Before SetAimDirection");
		
		weaponManager.SetAimDirection();
	
		/*	Logger::DebugPrint("Before GetCurrentPlayerStance");*/
		playerManager.GetCurrentPlayerStance(delta);
		//Logger::DebugPrint("Before UpdatePeepingState");
		playerManager.UpdatePeepingAndCqcGrabState(delta);
		//Logger::DebugPrint("Before UpdateHandVisibility");
		playerManager.ProcessVRHandsVisibility();
		
		//Logger::DebugPrint("Before ProcessCamera");


		cameraController.ProcessRoomscaleStance();

		weaponManager.ProcessTwoHandsGrabWeapon();
	}
	else
	{
		uevr::API::UObjectHook::remove_all_motion_controller_states();

		cameraController.CheckCutscenePlaying();
		if (playerManager.WasPlayerControlled)
		{
			weaponManager.DetachHandFromWeapon();
			weaponManager.DetachSecondHandFromWeapon();
		}

		cameraController.ApplyCutsceneSettings();

		if (playerManager.CurrentMenuType == ue::EGsrUIMenuType::Radio && playerManager.PreviousMenuType != ue::EGsrUIMenuType::Radio)
		{
			cameraController.FixRadioMenuRendering();
			API::VR::set_mod_value("VR_WorldScale", *settingsManager.Enable3DRadioPtr ? 1.0f : 0.0f);
		}
		if (playerManager.CurrentMenuType == ue::EGsrUIMenuType::SV && playerManager.PreviousMenuType != ue::EGsrUIMenuType::SV)
		{
			cameraController.FixSurvivalViewerMenuRendering();
			API::VR::set_mod_value("VR_WorldScale", *settingsManager.Enable3DSurvivalViewPtr ? 1.0f : 0.0f);
		}
	}

	weaponManager.ToggleDummyCQCKnifeMeshVisibility();

	if (playerManager.IsPlayerControlled || cameraController.ActualCutsceneState == CameraController::CutsceneState::Playing3D)
		cameraController.ProcessCamera(delta);
	cameraController.ProcessCameraResetTimer(delta);

	// Full body IK toggled in VR: show or hide the body (the head stays hidden)
	if (vr_enabled && bodyIK.Enabled != playerManager.BodyMeshesVisible)
		playerManager.ToggleMeshesVisibility(bodyIK.Enabled, false);
	bodyIK.Update(hookManager, playerManager, cameraController.CameraAttachementActor,
		cameraController.NewCameraPosition, cameraController.NewCameraRotation);


	imGuiMenu.ProcessImGui(delta);

	playerManager.WasSecondHandAttachedToWeapon = playerManager.SecondHandAttachedToWeapon;
	playerManager.WasPlayerControlled = playerManager.IsPlayerControlled;
	playerManager.WasPeepingInternal = playerManager.IsPeepingInternal;
	playerManager.PreviousStance = playerManager.CurrentStance;
	playerManager.WasFirstPersonView = playerManager.IsFirstPersonView;
	playerManager.WasAiming = playerManager.IsAiming;
	playerManager.PreviousCameraMode = playerManager.CurrentCameraMode;
	playerManager.WasMeleeAttackRunning = playerManager.MeleeAttackRunning;
	playerManager.WasPeeping = playerManager.IsPeeping;
	playerManager.WasCqcGrab = playerManager.IsCqcGrab;
	playerManager.WasCarrying = playerManager.IsCarrying;
	playerManager.PreviousMenuType = playerManager.CurrentMenuType;
	weaponManager.PreviousWeaponID = weaponManager.CurrentWeaponID;
	weaponManager.PreviousItemID = weaponManager.CurrentItemID;
	cameraController.PreviousCutsceneState = cameraController.ActualCutsceneState;
	Inputs::WasLeftGripInputPressed = Inputs::LeftGripInputPressed;
	imGuiMenu.MouseMovedThisFrame = false;
	//auto end = std::chrono::high_resolution_clock::now();
	//auto duration_ms = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	//Logger::DebugPrint("execution time : %lld micro seconds", duration_ms.count());
	if (weaponManager.AttachHandNextFrame)
		weaponManager.NextFrame = true;
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
	auto cobraCutsceneCameraCtrlMngr = cobraCutsceneCameraCtrlMngr_c.get();
	if (!cobraCutsceneCameraCtrlMngr)
	{
		Logger::DebugPrint("CobraCutsceneCameraCtrlMngr not found");
		return;
	}
	if (playerManager.IsPlayerControlled || 
		(cameraController.ActualCutsceneState == CameraController::CutsceneState::Playing3D && (*settingsManager.EnableImmersiveModePtr || ue::get<bool>(cobraCutsceneCameraCtrlMngr, L"bIsFpv"))))
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
		//	pos->x = 0.0f;
		//pos->y = 220.0f;
		//pos->z = 90.0f;
		//rot->pitch = 0.0f;
		//rot->yaw = -90.0f;
		//rot->roll = 0.0f;
	}

	//if (playerManager.CurrentMenuType == ue::EGsrUIMenuType::Radio)
	//{
	//	if (Inputs::IsKeyPressed(VK_UP))
	//		z += 0.05f;
	//	if (Inputs::IsKeyPressed(VK_DOWN))
	//		z -= 0.05f;
	//	if (Inputs::IsKeyPressed(VK_RIGHT))
	//		x += 0.05f;
	//	if (Inputs::IsKeyPressed(VK_LEFT))
	//		x -= 0.05f;
	//	if (Inputs::IsKeyPressed(VK_NUMPAD0))
	//		y += 0.05f;
	//	if (Inputs::IsKeyPressed(VK_NUMPAD1))
	//		y -= 0.05f;

	//	UEVR_Vector3d* pos = (UEVR_Vector3d*)position;
	//	UEVR_Rotatord* rot = (UEVR_Rotatord*)rotation;
	//	Logger::DebugPrint("camera multiplier: x : %f, y : %f, z : %f", x, y, z);
	//	//Logger::DebugPrint("camera rotation before: pitch : %f, yaw : %f, roll : %f", rot->pitch, rot->yaw, rot->roll);
	//	pos->x *= 1.85f;
	//	pos->y *= 2.3f;
	//	pos->z *= 0.65f;
	//	//rot->pitch = 0.0f;
	//	//rot->yaw = -90.0f;
	//	//rot->roll = 0.0f;
	//}
	
	//Logger::DebugPrint("State Machine Name %s", playerManager.debugParamsTest.StateMachineName.ToString().c_str());
	//Logger::DebugPrint("Next State Name %s", playerManager.debugParamsTest.NextStateName.ToString().c_str());
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

	//if (state->Gamepad.sThumbRX != 0.0)
	//	Logger::DebugPrint("%d", state->Gamepad.sThumbRX);
	float rawRightXInput = (float)state->Gamepad.sThumbRX / 32768.0f * 2.0f; // Normalize to -1.0 to 1.0 and scale
	cameraController.YawInput = rawRightXInput > *settingsManager.CameraRotationDeadZonePtr || rawRightXInput < -*settingsManager.CameraRotationDeadZonePtr ? rawRightXInput : 0.0f;

	float rawRightYInput = (float)state->Gamepad.sThumbRY / 32768.0f * 2.0f; // Normalize to -1.0 to 1.0 and scale
	weaponManager.ZoomInput = rawRightYInput > 0.5f || rawRightYInput < -0.5f ? rawRightYInput : 0.0f;

	float triggerInput = (float)state->Gamepad.bRightTrigger / 255.0f; // Normalize to 0.0 to 1.0
	Inputs::LeftGripInputPressed = triggerInput > 0.5f ? true : false;

	Inputs::LeftGripInputPressed = state->Gamepad.wButtons == 256 ? true : false;

	//float rawLeftXInput = (float)state->Gamepad.sThumbLX / 32768.0f; // Normalize to -1.0 to 1.0
	//float rawLeftYInput = (float)state->Gamepad.sThumbLY / 32768.0f; // Normalize to -1.0 to 1.0

	//Logger::DebugPrint("Left Joystick X: %f", rawLeftXInput);
	//Logger::DebugPrint("Left Joystick Y: %f", rawLeftYInput);
	//Logger::DebugPrint("Total: %f", rawLeftXInput + rawLeftYInput);
}

void MGSDeltaPlugin::on_xinput_set_state(uint32_t* retval, uint32_t user_index, XINPUT_VIBRATION* vibration)
{
	PLUGIN_LOG_ONCE("XInput Set State");
}