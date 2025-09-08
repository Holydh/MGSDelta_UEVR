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

#include "Plugin.hpp"

using namespace uevr;

#define MAX_DEBUG_MESSAGE_SIZE 2048
bool g_DebugEnabled = false;
float g_AdjustValueV = 0;
float g_AdjustValueH = 0;

bool imguiEnabled = false;

static void DebugPrint(const char* Format, ...)
{
	char FormattedMessage[MAX_DEBUG_MESSAGE_SIZE];
	va_list ArgPtr = NULL;

	if (g_DebugEnabled)
	{
		/* Generate the formatted debug message. */
		va_start(ArgPtr, Format);
		vsprintf(FormattedMessage, Format, ArgPtr);
		va_end(ArgPtr);

		printf(FormattedMessage);
		API::get()->log_info(FormattedMessage);
	}
}

std::unique_ptr<MGSDeltaPlugin> g_plugin = std::make_unique<MGSDeltaPlugin>();

MGSDeltaPlugin::~MGSDeltaPlugin() {

}

void MGSDeltaPlugin::on_initialize() {
	// Logs to the appdata UnrealVRMod log.txt file
	API::get()->log_info("%s", "on_initialize");

	std::filesystem::path config_file = API::get()->get_persistent_dir() / "plugins" / "aimfix.txt";
	std::filesystem::path debug_file = API::get()->get_persistent_dir() / "debug";

	SetGunAdjustValuesFromConfig(config_file);
	API::get()->log_info("MGS3-AimFix.dll: using config file value for gun adjust v:%f h:%f", g_AdjustValueV, g_AdjustValueH);
	// Check if the file exists
	g_DebugEnabled = std::filesystem::exists(debug_file);

	if (g_DebugEnabled) {
		AllocConsole();
		freopen("CONOUT$", "w", stdout);
	}
	DebugPrint("Hello, console!\n");
	hook_fire_fn();
}

int32_t hook_fn(std::wstring_view class_name, std::wstring_view fn_name, void* destination, void** original) {
	auto obj = (API::UClass*)API::get()->find_uobject(class_name);

	if (obj == nullptr) {
		PLUGIN_LOG_ONCE_ERROR("Failed to find %ls", class_name.data());
		return -1;
	}

	auto fn = obj->find_function(fn_name);

	if (fn == nullptr) {
		PLUGIN_LOG_ONCE_ERROR("Failed to find %ls", fn_name.data());
		return -1;
	}

	auto native = fn->get_native_function();

	if (native == nullptr) {
		PLUGIN_LOG_ONCE_ERROR("Failed to get native function");
		return -1;
	}

	DebugPrint("%ls native: 0x%p", fn_name.data(), native);

	auto insn = utility::scan_disasm((uintptr_t)native, 0x1000, "E8 ? ? ? ?");

	if (!insn) {
		PLUGIN_LOG_ONCE_ERROR("Failed to find the instruction");
		DebugPrint("Not found");
		return -1;
	}

	uintptr_t insn_val = insn.value();

	auto relative_offset = *(int32_t*)(insn_val + 1);

	uintptr_t function_address = (insn_val + 5) + relative_offset;

	DebugPrint("Function address: 0x%p\n", (void*)function_address);

	return API::get()->param()->functions->register_inline_hook((void*)function_address, (void*)destination, original);
}

void MGSDeltaPlugin::hook_fire_fn() {
	m_fire_hook_id = hook_fn(L"Class /Script/Gsr.GsrEquipController", L"Fire", on_get_fire, (void**)&m_fire_hook_fn);
}

void* MGSDeltaPlugin::on_get_fire_internal(uevr::API::UObject* equipcontroller, FGsrFireRequest* FireRequest) {

	DebugPrint("Function hooked\n");

	struct {
		API::UObject* value{ nullptr };
	} obj;

	equipcontroller->call_function(L"GetCurrentWeapon", &obj);

	auto weapon = obj.value;

	if (weapon != nullptr) {
		const auto weapon_name = weapon->get_full_name();
		DebugPrint("Weapon: %ls\n", weapon_name.c_str());

		auto root_component = weapon->get_property<API::UObject*>(L"RootComponent");

		if (root_component != nullptr) {
			UEVR_Vector3d forward_params;

			root_component->call_function(L"GetForwardVector", &forward_params);
			DebugPrint("Forward params: %f, %f, %f\n", forward_params.x, forward_params.y, forward_params.z);

			FTransform muzzle_transform;

			weapon->call_function(L"GetMuzzleTransform", &muzzle_transform);
			auto location_params = muzzle_transform.Translation;
			DebugPrint("Location params: %f, %f, %f\n", location_params.x, location_params.y, location_params.z);

			FireRequest->FirePosition.x = location_params.x;
			FireRequest->FirePosition.y = location_params.y;
			FireRequest->FirePosition.z = location_params.z;

			float scaler = 8192.0;

			RotateAngleAxis rotate_params;
			static auto kismet_math_library_c = API::get()->find_uobject<API::UClass>(L"Class /Script/Engine.KismetMathLibrary");
			static auto kismet_math_library = kismet_math_library_c->get_class_default_object();

			rotate_params.InVect = forward_params;
			rotate_params.AngleDeg = 90.0f;
			root_component->call_function(L"GetUpVector", &rotate_params.Axis);
			DebugPrint("Up Vector: %f, %f, %f\n", rotate_params.Axis.x, rotate_params.Axis.y, rotate_params.Axis.z);

			kismet_math_library->call_function(L"RotateAngleAxis", &rotate_params);
			forward_params = rotate_params.Result;
			DebugPrint("Forward Params: %f, %f, %f\n", forward_params.x, forward_params.y, forward_params.z);

			DebugPrint("Old Aim Position: %f, %f, %f\n", FireRequest->AimPosition.x, FireRequest->AimPosition.y, FireRequest->AimPosition.z);
			FireRequest->AimPosition.x = location_params.x + (forward_params.x * scaler);
			FireRequest->AimPosition.y = location_params.y + (forward_params.y * scaler) - g_AdjustValueH;
			FireRequest->AimPosition.z = location_params.z + (forward_params.z * scaler) - g_AdjustValueV;
			DebugPrint("New Aim Position: %f, %f, %f\n", FireRequest->AimPosition.x, FireRequest->AimPosition.y, FireRequest->AimPosition.z);
		}
	}


	//Call original function with our replaced parameters
	auto result = m_fire_hook_fn(equipcontroller, FireRequest);
	//PLUGIN_LOG_ONCE("Result: 0x%p", result);

	return result;
}

void MGSDeltaPlugin::SetGunAdjustValuesFromConfig(const std::filesystem::path& ConfigFile) {
	// Check if the file exists
	if (!std::filesystem::exists(ConfigFile)) {
		g_AdjustValueV = 0.0f;
		g_AdjustValueH = 0.0f;
		return;
	}

	std::ifstream file(ConfigFile);
	std::string line;

	while (std::getline(file, line)) {
		// Trim leading and trailing spaces
		line.erase(0, line.find_first_not_of(" \t"));
		line.erase(line.find_last_not_of(" \t") + 1);

		// Skip comments and blank lines
		if (line.empty() || line[0] == '#') {
			continue;
		}

		// Look for the "GunAdjustV=" prefix
		if (line.rfind("GunAdjustV=", 0) == 0) {
			std::string value_str = line.substr(11); // Extract the value as a string
			char* end_ptr = nullptr;
			float value = std::strtof(value_str.c_str(), &end_ptr);

			// Validate conversion
			if (end_ptr != value_str.c_str() && *end_ptr == '\0') {
				g_AdjustValueV = value; // Successfully parsed float
			}
			else {
				g_AdjustValueV = 0.0f; // Invalid float
			}
		}

		// Look for the "GunAdjustH=" prefix
		if (line.rfind("GunAdjustH=", 0) == 0) {
			std::string value_str = line.substr(11); // Extract the value as a string
			char* end_ptr = nullptr;
			float value = std::strtof(value_str.c_str(), &end_ptr);

			// Validate conversion
			if (end_ptr != value_str.c_str() && *end_ptr == '\0') {
				g_AdjustValueH = value; // Successfully parsed float
			}
			else {
				g_AdjustValueH = 0.0f; // Invalid float
			}
		}
	}
}

void MGSDeltaPlugin::on_present() {
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
	ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);

	return !ImGui::GetIO().WantCaptureMouse && !ImGui::GetIO().WantCaptureKeyboard;
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

void MGSDeltaPlugin::on_pre_engine_tick(API::UGameEngine* engine, float delta) {
	PLUGIN_LOG_ONCE("Pre Engine Tick: %f", delta);

	static bool once = true;

	// Unit tests for the API basically.
	if (once) {
		once = false;

		API::get()->log_info("Running once on pre engine tick");
		API::get()->execute_command(L"stat fps");

		API::FName test_name{ L"Left" };
		std::string name_narrow{ std::wstring_convert<std::codecvt_utf8<wchar_t>>{}.to_bytes(test_name.to_string()) };
		API::get()->log_info("Test FName: %s", name_narrow.c_str());

		//print_all_objects();
		//test_console_manager();
		//test_engine(engine);
	}

	if (m_initialized) {
		std::scoped_lock _{ m_imgui_mutex };

		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		internal_frame();

		ImGui::EndFrame();
		ImGui::Render();
	}
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
}

void MGSDeltaPlugin::on_post_calculate_stereo_view_offset(UEVR_StereoRenderingDeviceHandle, int view_index, float world_to_meters,
	UEVR_Vector3f* position, UEVR_Rotatorf* rotation, bool is_double)
{
	PLUGIN_LOG_ONCE("Post Calculate Stereo View Offset");
}

void MGSDeltaPlugin::on_pre_viewport_client_draw(UEVR_UGameViewportClientHandle viewport_client, UEVR_FViewportHandle viewport, UEVR_FCanvasHandle canvas) {
	PLUGIN_LOG_ONCE("Pre Viewport Client Draw");
}

void MGSDeltaPlugin::on_post_viewport_client_draw(UEVR_UGameViewportClientHandle viewport_client, UEVR_FViewportHandle viewport, UEVR_FCanvasHandle canvas) {
	PLUGIN_LOG_ONCE("Post Viewport Client Draw");
}

bool MGSDeltaPlugin::initialize_imgui() {
	if (!imguiEnabled)
		return false;

	if (m_initialized) {
		return true;
	}

	std::scoped_lock _{ m_imgui_mutex };

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	static const auto imgui_ini = API::get()->get_persistent_dir(L"imgui_template_plugin.ini").string();
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

	if (ImGui::Begin("Very Cool Template Plugin")) {
		ImGui::Text("Hello from the very cool plugin!");
		ImGui::Text("Decoupled pitch: %i", API::VR::is_decoupled_pitch_enabled());

		if (ImGui::Button("Toggle decoupled pitch")) {
			API::VR::set_decoupled_pitch_enabled(!API::VR::is_decoupled_pitch_enabled());
		}

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
}