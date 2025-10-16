#include "MGSDeltaPlugin.hpp"

using namespace uevr;

#define MAX_DEBUG_MESSAGE_SIZE 2048

bool g_DebugEnabled = true;
bool imguiEnabled = true;


std::unique_ptr<MGSDeltaPlugin> g_plugin = std::make_unique<MGSDeltaPlugin>();

MGSDeltaPlugin::MGSDeltaPlugin() : cameraController(&playerManager, &settingsManager), 
weaponController(&memoryManager, &settingsManager, &playerManager), 
memoryManager(),
hookManager(&weaponController, &cameraController, &playerManager), 
playerManager() {}; //Constructor
MGSDeltaPlugin::~MGSDeltaPlugin() {}//Destructor

void MGSDeltaPlugin::on_detach(HMODULE hmod)
{
	Toggle6DOF(false);
}

void MGSDeltaPlugin::on_initialize() {
	// Logs to the appdata UnrealVRMod log.txt file
	API::get()->log_info("%s", "on_initialize");
	std::filesystem::path config_file = API::get()->get_persistent_dir() / "plugins" / "aimfix.txt";
	std::filesystem::path debug_file = API::get()->get_persistent_dir() / "debug";
	std::filesystem::path uobjectHook_file = API::get()->get_persistent_dir() / "uobjecthook";
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
	settingsManager.InitSettingsManager();
	settingsManager.LoadAllWeaponConfigs(uobjectHook_file);
	hookManager.Initialize();
	hookManager.hook_bp_fn(L"Class /Script/Ics.IcsAnimInstance", L"OnEnteredState", NULL, (UEVR_UFunction_NativePostFn)HookManager::mod_OnEnteredState_post, true);
	hookManager.hook_bp_fn(L"AnimBlueprintGeneratedClass /Game/Gsr/Blueprints/Player/Animations/ABP_PlayerEludeAction.ABP_PlayerEludeAction_C", L"OnEnteredState", NULL, (UEVR_UFunction_NativePostFn)HookManager::mod_PlayerFalling_OnEnteredState_post, true);
	//weaponController.Initialize();
	memoryManager.InitMemoryManager();
	hookManager.HookFireFunction();
}

void MGSDeltaPlugin::on_present() {
	if (!imguiEnabled)
		return;
	std::scoped_lock _{ m_imgui_mutex };
	if (!m_initialized) {
		if (!initialize_imgui()) {
			API::get()->log_info("Failed to initialize imgui");
			return;
		}
		else {
			API::get()->log_info("Initialized imgui");
		}
	}

	const auto renderer_data = API::get()->param()->renderer;

	if (!API::get()->param()->vr->is_hmd_active()) {
		if (!m_was_rendering_desktop) {
			m_was_rendering_desktop = true;
			on_device_reset();
			return;
		}

		m_was_rendering_desktop = true;

		if (renderer_data->renderer_type == UEVR_RENDERER_D3D11) {
			ImGui_ImplDX11_NewFrame();
			g_d3d11.render_imgui();
		}
		else if (renderer_data->renderer_type == UEVR_RENDERER_D3D12) {
			auto command_queue = (ID3D12CommandQueue*)renderer_data->command_queue;

			if (command_queue == nullptr) {
				return;
			}

			ImGui_ImplDX12_NewFrame();
			g_d3d12.render_imgui();
		}
	}
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

void MGSDeltaPlugin::on_device_reset() {
	PLUGIN_LOG_ONCE("Example Device Reset");

	std::scoped_lock _{ m_imgui_mutex };

	const auto renderer_data = API::get()->param()->renderer;

	if (renderer_data->renderer_type == UEVR_RENDERER_D3D11) {
		ImGui_ImplDX11_Shutdown();
		g_d3d11 = {};
	}

	if (renderer_data->renderer_type == UEVR_RENDERER_D3D12) {
		g_d3d12.reset();
		ImGui_ImplDX12_Shutdown();
		g_d3d12 = {};
	}

	m_initialized = false;
}

void MGSDeltaPlugin::on_post_render_vr_framework_dx11(ID3D11DeviceContext* context, ID3D11Texture2D* texture, ID3D11RenderTargetView* rtv) {
	PLUGIN_LOG_ONCE("Post Render VR Framework DX11");

	const auto vr_active = API::get()->param()->vr->is_hmd_active();

	if (!m_initialized || !vr_active) {
		return;
	}

	if (m_was_rendering_desktop) {
		m_was_rendering_desktop = false;
		on_device_reset();
		return;
	}

	std::scoped_lock _{ m_imgui_mutex };

	ImGui_ImplDX11_NewFrame();
	g_d3d11.render_imgui_vr(context, rtv);
}

void MGSDeltaPlugin::on_post_render_vr_framework_dx12(ID3D12GraphicsCommandList* command_list, ID3D12Resource* rt, D3D12_CPU_DESCRIPTOR_HANDLE* rtv) {
	PLUGIN_LOG_ONCE("Post Render VR Framework DX12");
	
	if (!imguiEnabled)
		return;

	const auto vr_active = API::get()->param()->vr->is_hmd_active();

	if (!m_initialized || !vr_active) {
		return;
	}

	if (m_was_rendering_desktop) {
		m_was_rendering_desktop = false;
		on_device_reset();
		return;
	}

	std::scoped_lock _{ m_imgui_mutex };

	ImGui_ImplDX12_NewFrame();
	g_d3d12.render_imgui_vr(command_list, rtv);
}

bool MGSDeltaPlugin::on_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (API::get()->param()->functions->is_drawing_ui())
	{
		ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
		return false;
	}
	return true;
}

void MGSDeltaPlugin::on_custom_event(const char* event_name, const char* event_data) {
	API::get()->log_info("Custom Event: %s %s", event_name, event_data);
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
		cameraController.ToggleCameraWobblingController(false);
		//memoryManager.ToggleAimRelatedMemoryInstructions(false);
		playerManager.ToggleMeshesVisibility(false);
		playerManager.ToggleReticleVisibility(false);
		cameraController.SpawnCameraAttachementActor();
		cameraController.AttachCameraToAttachementActor();
		//Utilities::DebugCigar0 = Utilities::SpawnDebugCigars(UEVR_Vector3d{114.0f, -3495.0f, 500.0f});
		//Utilities::DebugCigar1 = Utilities::SpawnDebugCigars(UEVR_Vector3d{114.0f, -3495.0f, 500.0f});
		vr_enabled = enable;
	}
	
	if(!enable && vr_enabled)
	{
		cameraController.ToggleCameraWobblingController(true);
		//memoryManager.ToggleAimRelatedMemoryInstructions(true);
		playerManager.ToggleMeshesVisibility(true);
		playerManager.ToggleReticleVisibility(true);
		cameraController.DetachCameraFromAttachementActor();
		cameraController.DestroyCameraAttachementActor();
	/*	Utilities::DestroyDebugCigars();*/
		vr_enabled = enable;
	}
}

void MGSDeltaPlugin::on_pre_engine_tick(API::UGameEngine* engine, float delta) {
	PLUGIN_LOG_ONCE("Pre Engine Tick: %f", delta);
	cameraController.DeltaTime = delta;
	playerManager.FetchGameState();

	if (playerManager.IsPlayerControlled && !playerManager.WasPlayerControlled)
	{
		Logger::DebugPrint("Player took control");
		Toggle6DOF(true);
	}

	if (!playerManager.IsPlayerControlled && playerManager.WasPlayerControlled)
	{
		Logger::DebugPrint("Player lost control");
		Toggle6DOF(false);
	}

	if (playerManager.IsPlayerControlled)
	{
		/*Logger::DebugPrint("Before ResolveAddresses");*/
		/*memoryManager.ResolveAddresses();*/
		/*Logger::DebugPrint("Before FetchEquipController");*/
		weaponController.FetchEquipController();
		/*Logger::DebugPrint("Before ProcessWeapon");*/
		weaponController.ProcessWeapon(delta);
		/*Logger::DebugPrint("Before FetchWeaponMeshAimData");*/
		weaponController.FetchWeaponMeshAimData();
		/*Logger::DebugPrint("Before ApplyAimDataToMemory");*/
		//weaponController.ApplyAimDataToMemory();
		/*Logger::DebugPrint("Before SetAimDirection");*/
		weaponController.SetAimDirection();
		/*Logger::DebugPrint("Before GetCurrentPlayerStance");*/
		playerManager.GetCurrentPlayerStance(delta);
		playerManager.UpdatePeepingState(delta);
	}

	cameraController.ProcessCameraResetTimer(delta);

	//Debug
	//if (playerManager.IsPlayerControlled)
	//{
	//	auto camActor = (SDK::AActor*)cameraController.CameraAttachementActor;
	//	auto cameraLocation = camActor->K2_GetActorLocation();
	//	//Logger::DebugPrint("Camera forward: %f, %f, %f", cameraController.CameraForwardVector.X, cameraController.CameraForwardVector.Y, cameraController.CameraForwardVector.Z);
	//	SDK::FHitResult hitResult;
	//	Utilities::DebugCigar0->K2_SetActorRelativeLocation(cameraLocation + cameraController.CameraForwardVector * 100.0f, false, &hitResult, true);
	//}

	if (m_initialized && API::get()->param()->functions->is_drawing_ui()) {
		std::scoped_lock _{ m_imgui_mutex };

		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		internal_frame();

		ImGui::EndFrame();
		ImGui::Render();
	}

	playerManager.WasPlayerControlled = playerManager.IsPlayerControlled;
	playerManager.WasPeepingInternal = playerManager.IsPeepingInternal;
	playerManager.PreviousStance = playerManager.CurrentStance;
}

void MGSDeltaPlugin::on_post_engine_tick(API::UGameEngine* engine, float delta) {
	PLUGIN_LOG_ONCE("Post Engine Tick: %f", delta);
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

	if (playerManager.IsPlayerControlled)
	{
		UEVR_Vector3d* pos = (UEVR_Vector3d*)position;
		UEVR_Rotatord* rot = (UEVR_Rotatord*)rotation;
		cameraController.ProcessCamera(*pos, *rot);
	}

	//Logger::DebugPrint("State Machine Name %s", playerManager.debugParamsTest.StateMachineName.ToString().c_str());
	//Logger::DebugPrint("Next State Name %s", playerManager.debugParamsTest.NextStateName.ToString().c_str());
}

void MGSDeltaPlugin::on_pre_calculate_stereo_view_offset(UEVR_StereoRenderingDeviceHandle, int view_index, float world_to_meters,
	UEVR_Vector3f* position, UEVR_Rotatorf* rotation, bool is_double)
{
	PLUGIN_LOG_ONCE("Pre Calculate Stereo View Offset");
	
	//auto rotationd = (UEVR_Rotatord*)rotation;

	//// Decoupled pitch.
	//if (!is_double) {
	//    rotation->pitch = 0.0f;
	//}
	//else {
	//    rotationd->pitch = 0.0;
	//}
	//if (cameraController.YawInput != 0.0)
	//	cameraController.actualYaw += cameraController.YawInput * cameraController.YawSpeed;
	///*Logger::DebugPrint("rotation->yaw: %f", rotation->yaw);*/
	//rotationd->yaw = cameraController.actualYaw;
	//if (playerManager.isPlayerControlled)
	//{
	//	cameraController.SetCameraRotation(rotationd->yaw);
	//}
	//Logger::DebugPrint("on_pre rotation->yaw: %f", rotation->yaw);


}

void MGSDeltaPlugin::on_post_calculate_stereo_view_offset(UEVR_StereoRenderingDeviceHandle, int view_index, float world_to_meters,
	UEVR_Vector3f* position, UEVR_Rotatorf* rotation, bool is_double)
{
	PLUGIN_LOG_ONCE("Post Calculate Stereo View Offset");
	//auto rotationd = (UEVR_Rotatord*)rotation;
	//rotationd->yaw = 90.0f;
	//Logger::DebugPrint("on_post rotation->yaw: %f", rotation->yaw);
	//auto positiond = (UEVR_Vector3d*)position;
	//if (playerManager.isPlayerControlled)
	//{
	//	auto pawn = uevr::API::get()->get_local_pawn(0);
	//	UEVR_Vector3d pawnPosition;
	//	pawn->call_function(L"K2_GetActorLocation", &pawnPosition);
	//	//Logger::DebugPrint("Pawn Position : %f, %f, %f", pawnPosition.x, pawnPosition.y, pawnPosition.z);
	//	auto grassVolumeCollisionHead = pawn->get_property<uevr::API::UObject*>(L"GrassVolumeCollisionHead");
	//	UEVR_Vector3d headColliderPosition;
	//	grassVolumeCollisionHead->call_function(L"K2_GetComponentLocation", &headColliderPosition);
	//	auto newPosition = UEVR_Vector3d(pawnPosition.x, pawnPosition.y, headColliderPosition.z);
	//	cameraController.playerPosition = newPosition;
	//	positiond->x = newPosition.x;
	//	positiond->y = newPosition.y;
	//	positiond->z = newPosition.z;
	//}
}

void MGSDeltaPlugin::on_pre_viewport_client_draw(UEVR_UGameViewportClientHandle viewport_client, UEVR_FViewportHandle viewport, UEVR_FCanvasHandle canvas) {
	PLUGIN_LOG_ONCE("Pre Viewport Client Draw");
}

void MGSDeltaPlugin::on_post_viewport_client_draw(UEVR_UGameViewportClientHandle viewport_client, UEVR_FViewportHandle viewport, UEVR_FCanvasHandle canvas) {
	PLUGIN_LOG_ONCE("Post Viewport Client Draw");
}

void MGSDeltaPlugin::on_xinput_get_state(uint32_t* retval, uint32_t user_index, XINPUT_STATE* state)
{
	PLUGIN_LOG_ONCE("XInput Get State");
	//if (state->Gamepad.sThumbRX != 0.0)
	//	Logger::DebugPrint("%d", state->Gamepad.sThumbRX);
	float rawYawInput = (float)state->Gamepad.sThumbRX / 32768.0f * 2.0f; // Normalize to -1.0 to 1.0 and scale
	cameraController.YawInput = rawYawInput > 0.2f || rawYawInput < -0.2f ? rawYawInput : 0.0f;
}

void MGSDeltaPlugin::on_xinput_set_state(uint32_t* retval, uint32_t user_index, XINPUT_VIBRATION* vibration)
{
	PLUGIN_LOG_ONCE("XInput Set State");
}

bool MGSDeltaPlugin::initialize_imgui() {
	if (m_initialized) {
		return true;
	}

	std::scoped_lock _{ m_imgui_mutex };

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	static const auto imgui_ini = API::get()->get_persistent_dir(L"imgui_mgsDeltaVR_plugin.ini").string();
	ImGui::GetIO().IniFilename = imgui_ini.c_str();

	const auto renderer_data = API::get()->param()->renderer;

	DXGI_SWAP_CHAIN_DESC swap_desc{};
	auto swapchain = (IDXGISwapChain*)renderer_data->swapchain;
	swapchain->GetDesc(&swap_desc);

	m_wnd = swap_desc.OutputWindow;

	if (!ImGui_ImplWin32_Init(m_wnd)) {
		return false;
	}

	if (renderer_data->renderer_type == UEVR_RENDERER_D3D11) {
		if (!g_d3d11.initialize()) {
			return false;
		}
	}
	else if (renderer_data->renderer_type == UEVR_RENDERER_D3D12) {
		if (!g_d3d12.initialize()) {
			return false;
		}
	}

	m_initialized = true;
	return true;
}

void MGSDeltaPlugin::internal_frame() {
	if (!API::get()->param()->functions->is_drawing_ui())
		return;
	//ImGui::GetIO().WantCaptureMouse = false; //API::get()->param()->functions->is_drawing_ui;
	static auto spot_c = API::get()->find_uobject<API::UClass>(L"Class /Script/Engine.SpotLight");
	auto kismet_system_library = API::get()->find_uobject<API::UClass>(L"Class /Script/Engine.KismetSystemLibrary")->get_class_default_object();
	auto kismet_math_library = API::get()->find_uobject<API::UClass>(L"Class /Script/Engine.KismetMathLibrary")->get_class_default_object();
	auto kismet_rendering_library = API::get()->find_uobject<API::UClass>(L"Class /Script/Engine.KismetRenderingLibrary")->get_class_default_object();
	auto kismet_material_library = API::get()->find_uobject<API::UClass>(L"Class /Script/Engine.KismetMaterialLibrary")->get_class_default_object();
	auto kismet_input_library = API::get()->find_uobject<API::UClass>(L"Class /Script/Engine.KismetInputLibrary")->get_class_default_object();
	auto kismet_string_library = API::get()->find_uobject<API::UClass>(L"Class /Script/Engine.KismetStringLibrary")->get_class_default_object();
	auto kismet_array_library = API::get()->find_uobject<API::UClass>(L"Class /Script/Engine.KismetArrayLibrary")->get_class_default_object();
	auto kismet_text_library = API::get()->find_uobject<API::UClass>(L"Class /Script/Engine.KismetTextLibrary")->get_class_default_object();
	auto statics = API::get()->find_uobject<API::UClass>(L"Class /Script/Engine.GameplayStatics")->get_class_default_object();
	
	if (ImGui::Begin("MGS Delta VR Plugin")) {
		ImGui::Text("--- Camera Settings ---");
	/*	ImGui::Text("Decoupled pitch: %i", API::VR::is_decoupled_pitch_enabled());

		if (ImGui::Button("Toggle decoupled pitch")) {
			API::VR::set_decoupled_pitch_enabled(!API::VR::is_decoupled_pitch_enabled());
		}*/

		if (ImGui::Checkbox("Smooth Camera Rotation", settingsManager.SmoothCameraRotationPtr))
			settingsManager.SettingsModifiedThisFrame = true;

		if(ImGui::SliderFloat("Rotation Smoothness", settingsManager.CameraYawLerpPtr, 1.0f, 10.0f))
			settingsManager.SettingsModifiedThisFrame = true;
		
			
		std::wstring object_path;
		static char obj_path[256]{};
		ImGui::Text("Enter an object to spawn");
		ImGui::SameLine();
		if (ImGui::InputText("objectspawner", obj_path, sizeof(obj_path)))
			object_path = widen(obj_path);

		if (ImGui::Button("Spawn Object")) {
			auto obj_res = API::get()->find_uobject<API::UObject>(object_path);
			kismet_system_library->call_function(L"IsValid", obj_res);
			auto spawned_component = API::get()->spawn_object(obj_res->get_class(), obj_res->get_outer());
		}


		static char lua_event_name[256]{};
		ImGui::Text("Send Event to Lua");
		ImGui::InputText("LuaEvent", lua_event_name, sizeof(lua_event_name));


		static char lua_event_data[256]{};
		ImGui::InputText("LuaData", lua_event_data, sizeof(lua_event_data));

		if (ImGui::Button("Send Lua Event")) {
			if (lua_event_name[0] != '\0') {
				dispatch_lua_event(lua_event_name, lua_event_data);
			}
		}
		static char input[256]{};
		std::string mod_value = API::VR::get_mod_value<std::string>(input);
		ImGui::Text("Mod value: %s", mod_value.c_str());

		if (ImGui::Button("Save Config")) {
			API::VR::save_config();
		}

		if (ImGui::Button("Reload Config")) {
			API::VR::reload_config();
		}

		if (ImGui::Button("Toggle UObjectHook disabled")) {
			const auto value = API::UObjectHook::is_disabled();

			API::UObjectHook::set_disabled(!value);
		}

	}
	ImGui::End();

	if (settingsManager.SettingsModifiedThisFrame)
	{
		settingsManager.WriteSettingsToPluginConfigFile();
		settingsManager.SettingsModifiedThisFrame = false;
	}
}