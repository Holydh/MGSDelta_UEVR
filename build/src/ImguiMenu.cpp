#include "ImGuiMenu.hpp"

void ImGuiMenu::RenderImGuiFrame()
{
	std::scoped_lock _{ m_imgui_mutex };
	if (!m_initialized) {
		if (!initializeImGui()) {
			uevr::API::get()->log_info("Failed to initialize imgui");
			return;
		}
		else {
			uevr::API::get()->log_info("Initialized imgui");
		}
	}

	const auto renderer_data = uevr::API::get()->param()->renderer;

	if (!uevr::API::get()->param()->vr->is_hmd_active()) {
		if (!m_was_rendering_desktop) {
			m_was_rendering_desktop = true;
			onDeviceReset();
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

void ImGuiMenu::RenderImGuiVRFrameDX11(ID3D11DeviceContext* context, ID3D11Texture2D* texture, ID3D11RenderTargetView* rtv)
{
	if (!ImguiEnabled)
		return;
	const auto vr_active = uevr::API::get()->param()->vr->is_hmd_active();

	if (!m_initialized || !vr_active) {
		return;
	}

	if (m_was_rendering_desktop) {
		m_was_rendering_desktop = false;
		onDeviceReset();
		return;
	}

	std::scoped_lock _{ m_imgui_mutex };

	ImGui_ImplDX11_NewFrame();
	g_d3d11.render_imgui_vr(context, rtv);
}

void ImGuiMenu::RenderImGuiVRFrameDX12(ID3D12GraphicsCommandList* command_list, ID3D12Resource* rt, D3D12_CPU_DESCRIPTOR_HANDLE* rtv)
{
	if (!ImguiEnabled)
		return;

	const auto vr_active = uevr::API::get()->param()->vr->is_hmd_active();

	if (!m_initialized || !vr_active) {
		return;
	}

	if (m_was_rendering_desktop) {
		m_was_rendering_desktop = false;
		onDeviceReset();
		return;
	}

	std::scoped_lock _{ m_imgui_mutex };

	ImGui_ImplDX12_NewFrame();
	g_d3d12.render_imgui_vr(command_list, rtv);
}

void ImGuiMenu::ProcessImGui(float deltaTime)
{
	if (!m_initialized || !uevr::API::get()->param()->functions->is_drawing_ui())
		return;

	std::scoped_lock _{ m_imgui_mutex };
	//ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

	//ImGui::GetIO().MousePos = ImVec2(100, 100);
	checkMouseFocus(deltaTime);
	handleVRcursor();
	
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	internalFrame();

	ImGui::EndFrame();
	ImGui::Render();
}

void ImGuiMenu::handleVRcursor()
{
    // Get VR interface and device indices
    auto vr = uevr::API::get()->param()->vr;
    auto rightControllerIndex = vr->get_right_controller_index();
    auto hmdIndex = vr->get_hmd_index();
    ImGuiIO& io = ImGui::GetIO();
	io.MouseDrawCursor = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);

	if (MouseHasFocus)
		return;

    // Retrieve controller and HMD transforms
    UEVR_Matrix4x4f controllerPoseMatrix;
    UEVR_Matrix4x4f hmdPoseMatrix;
    vr->get_grip_transform(rightControllerIndex, &controllerPoseMatrix);
    vr->get_transform(hmdIndex, &hmdPoseMatrix);
    
    // Convert to GLM matrices
    glm::mat4 controllerMatrix = glm::make_mat4(&controllerPoseMatrix.m[0][0]);
    glm::mat4 hmdMatrix = glm::make_mat4(&hmdPoseMatrix.m[0][0]);
    
    // Extract controller position and calculate pointing direction
    glm::vec3 controllerPosition = glm::vec3(controllerMatrix[3]);
    glm::vec3 controllerForward = -glm::vec3(controllerMatrix[1]);
    glm::vec3 controllerUp = -glm::vec3(controllerMatrix[2]);
    glm::vec3 controllerDirection = glm::normalize(controllerForward * 0.5f + controllerUp * 0.3f);
    
    // Get VR play space origin and rotation
    UEVR_Vector3f standingOriginRaw;
    UEVR_Quaternionf rotationOffsetRaw;
    vr->get_standing_origin(&standingOriginRaw);
    vr->get_rotation_offset(&rotationOffsetRaw);
    
    glm::vec3 playSpaceOrigin(standingOriginRaw.x, standingOriginRaw.y, standingOriginRaw.z);
    glm::quat playSpaceRotation(rotationOffsetRaw.w, rotationOffsetRaw.x, 
                                rotationOffsetRaw.y, rotationOffsetRaw.z);
    
    // Calculate UI plane orientation
    glm::vec3 uiRightAxis = playSpaceRotation * glm::vec3(1, 0, 0);
    glm::vec3 uiUpAxis = playSpaceRotation * glm::vec3(0, 1, 0);
    glm::vec3 uiNormalAxis = playSpaceRotation * glm::vec3(0, 0, -1);
    
    // Position UI plane at configured distance from origin
    float uiDistance = uevr::API::VR::get_mod_value<float>(Framework_UI_Distance);
    glm::vec3 uiPlaneCenter = playSpaceOrigin + uiNormalAxis * uiDistance;
    
    // Perform ray-plane intersection
    float rayDotNormal = glm::dot(uiNormalAxis, controllerDirection);
    
    if (fabs(rayDotNormal) > 1e-6f)  // If ray not parallel to plane
    {
        float intersectionDistance = glm::dot(uiPlaneCenter - controllerPosition, uiNormalAxis) / rayDotNormal;
        
        if (intersectionDistance > 0.0f)  // Intersection in front of controller
        {
            // Calculate intersection point
            glm::vec3 intersectionPoint = controllerPosition + controllerDirection * intersectionDistance;
            
            // Project intersection to UI plane local coordinates
            glm::vec3 localOffset = intersectionPoint - uiPlaneCenter;
            float localX = glm::dot(localOffset, uiRightAxis);
            float localY = glm::dot(localOffset, uiUpAxis);
            
            // Normalize by UI size and apply cursor speed scaling
            float uiSize = uevr::API::VR::get_mod_value<float>(Framework_UI_Size);
            float normalizedX = (localX / uiSize) * 0.565f;
            float normalizedY = localY / uiSize;
            
            // Convert to ImGui screen coordinates
            ImVec2 targetMousePosition(
                (normalizedX + 0.5f) * io.DisplaySize.x,
                (0.5f - normalizedY) * io.DisplaySize.y
            );
            
            // Initialize last position if needed
            if (LastMousePosition.x < 0.0f)
            {
                LastMousePosition = targetMousePosition;
            }
            
            // Apply smoothing to cursor movement
            const float smoothingFactor = 0.2f;
            LastMousePosition.x = glm::mix(LastMousePosition.x, targetMousePosition.x, smoothingFactor);
            LastMousePosition.y = glm::mix(LastMousePosition.y, targetMousePosition.y, smoothingFactor);
            
            // Update ImGui input state
            io.MousePos = LastMousePosition;
            io.AddMouseButtonEvent(0, Inputs::IsActionActive(false, "/actions/default/in/AButtonRight"));
        }
    }
}

void ImGuiMenu::checkMouseFocus(float delta)
{
	//Logger::DebugPrint("MouseMovedThisFrame : %i", MouseMovedThisFrame);
	//Logger::DebugPrint("MouseHasFocus : %i", MouseHasFocus);
	//Logger::DebugPrint("mouseUseTimer : %f", mouseUseTimer);
	auto io = ImGui::GetIO();

	if (!MouseMovedThisFrame && !io.MouseDown[0])
	{
		mouseUseTimer -= delta;
	}
	else
	{
		mouseUseTimer = mouseUseTime;
		MouseHasFocus = true;
	}
	if (mouseUseTimer <= 0)
	{
		MouseHasFocus = false;
	}
}

void ImGuiMenu::onDeviceReset()
{
	std::scoped_lock _{ m_imgui_mutex };

	const auto renderer_data = uevr::API::get()->param()->renderer;

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

void ImGuiMenu::internalFrame()
{
	if (!uevr::API::get()->param()->functions->is_drawing_ui())
		return;
	//ImGui::GetIO().WantCaptureMouse = false; //API::get()->param()->functions->is_drawing_ui;
	static auto spot_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Engine.SpotLight");
	auto kismet_system_library = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Engine.KismetSystemLibrary")->get_class_default_object();
	auto kismet_math_library = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Engine.KismetMathLibrary")->get_class_default_object();
	auto kismet_rendering_library = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Engine.KismetRenderingLibrary")->get_class_default_object();
	auto kismet_material_library = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Engine.KismetMaterialLibrary")->get_class_default_object();
	auto kismet_input_library = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Engine.KismetInputLibrary")->get_class_default_object();
	auto kismet_string_library = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Engine.KismetStringLibrary")->get_class_default_object();
	auto kismet_array_library = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Engine.KismetArrayLibrary")->get_class_default_object();
	auto kismet_text_library = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Engine.KismetTextLibrary")->get_class_default_object();
	auto statics = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Engine.GameplayStatics")->get_class_default_object();
	
	if (ImGui::Begin("MGS Delta VR Plugin")) {
		ImGui::Text("--- Camera Settings ---");
	/*	ImGui::Text("Decoupled pitch: %i", API::VR::is_decoupled_pitch_enabled());

		if (ImGui::Button("Toggle decoupled pitch")) {
			API::VR::set_decoupled_pitch_enabled(!API::VR::is_decoupled_pitch_enabled());
		}*/

		if (ImGui::Checkbox("Attach To Snake Head", settingsManager->AttachCameraToSnakeHeadPtr))
			settingsManager->SettingsModifiedThisFrame = true;

		float available = ImGui::GetContentRegionAvail().x;
		ImGui::PushItemWidth(available * 0.5f); 
		if(ImGui::SliderFloat("Rotation Speed", settingsManager->CameraRotationSpeedPtr, 0.1f, 50.0f))
			settingsManager->SettingsModifiedThisFrame = true;

		
		if (ImGui::Checkbox("Smooth Camera Rotation", settingsManager->SmoothCameraRotationPtr))
			settingsManager->SettingsModifiedThisFrame = true;

		if(ImGui::SliderFloat("Rotation Smoothness", settingsManager->CameraYawLerpPtr, 0.0f, 10.0f))
			settingsManager->SettingsModifiedThisFrame = true;
		
		if (ImGui::Checkbox("Smooth Camera Position", settingsManager->SmoothCameraPositionPtr))
			settingsManager->SettingsModifiedThisFrame = true;

		if(ImGui::SliderFloat("Horizontal Smoothness (X&Y Axis)", settingsManager->CameraPositionXYLerpPtr, 0.0f, 10.0f))
			settingsManager->SettingsModifiedThisFrame = true;

		if(ImGui::SliderFloat("Vertical Smoothness (Z Axis)", settingsManager->CameraPositionZLerpPtr, 0.0f, 10.0f))
			settingsManager->SettingsModifiedThisFrame = true;
		ImGui::PopItemWidth();
		/*	
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
		}*/

	}
	ImGui::End();

	if (settingsManager->SettingsModifiedThisFrame)
	{
		settingsManager->WriteSettingsToPluginConfigFile();
		settingsManager->SettingsModifiedThisFrame = false;
	}
}

bool ImGuiMenu::initializeImGui() {
	if (m_initialized) {
		return true;
	}

	std::scoped_lock _{ m_imgui_mutex };

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	static const auto imgui_ini = uevr::API::get()->get_persistent_dir(L"imgui_mgsDeltaVR_plugin.ini").string();
	ImGui::GetIO().IniFilename = imgui_ini.c_str();
	//ImGui::GetIO().MouseDrawCursor = true;
	const auto renderer_data = uevr::API::get()->param()->renderer;

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