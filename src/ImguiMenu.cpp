#include "ImGuiMenu.hpp"

void ImGuiMenu::InitializeImGuiSettings()
{
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(imguiSize); // Scale padding, spacing, borders, etc.
	ImGui::GetIO().FontGlobalScale = imguiSize; // Also scale fonts
}

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

	if (!uevr::API::get()->param()->vr->is_hmd_active() && !m_was_rendering_desktop) {
		m_was_rendering_desktop = true;
		onDeviceReset();
		return;
	}

	// Drawn to the backbuffer in VR too, so the menu shows on the spectator window and works with the mouse
	if (!uevr::API::get()->param()->functions->is_drawing_ui())
		return;

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
	{
		return;
	}

	std::scoped_lock _{ m_imgui_mutex };
	//ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

	//ImGui::GetIO().MousePos = ImVec2(100, 100);
	checkMouseFocus(deltaTime);

	ImGui_ImplWin32_NewFrame();
	handleVRcursor();
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
	auto test = io.MousePos;
	///*Logger::DebugPrint("Mouse has focus : %i", MouseHasFocus);*/
	//if (MouseHasFocus)
	//	return;

 //   // Retrieve controller and HMD transforms
 //   UEVR_Matrix4x4f controllerPoseMatrix;
 //   UEVR_Matrix4x4f hmdPoseMatrix;
 //   vr->get_grip_transform(rightControllerIndex, &controllerPoseMatrix);
 //   vr->get_transform(hmdIndex, &hmdPoseMatrix);
 //   
 //   // Convert to GLM matrices
 //   glm::mat4 controllerMatrix = glm::make_mat4(&controllerPoseMatrix.m[0][0]);
 //   glm::mat4 hmdMatrix = glm::make_mat4(&hmdPoseMatrix.m[0][0]);
 //   
 //   // Extract controller position and calculate pointing direction
 //   glm::vec3 controllerPosition = glm::vec3(controllerMatrix[3]);
 //   glm::vec3 controllerForward = -glm::vec3(controllerMatrix[1]);
 //   glm::vec3 controllerUp = -glm::vec3(controllerMatrix[2]);
 //   glm::vec3 controllerDirection = glm::normalize(controllerForward * 0.5f + controllerUp * 0.3f);
 //   
 //   // Get VR play space origin and rotation
 //   UEVR_Vector3f standingOriginRaw;
 //   UEVR_Quaternionf rotationOffsetRaw;
 //   vr->get_standing_origin(&standingOriginRaw);
 //   vr->get_rotation_offset(&rotationOffsetRaw);
 //   
 //   glm::vec3 playSpaceOrigin(standingOriginRaw.x, standingOriginRaw.y, standingOriginRaw.z);
 //   glm::quat playSpaceRotation(rotationOffsetRaw.w, rotationOffsetRaw.x, 
 //                               rotationOffsetRaw.y, rotationOffsetRaw.z);
 //   
 //   // Calculate UI plane orientation
 //   glm::vec3 uiRightAxis = playSpaceRotation * glm::vec3(1, 0, 0);
 //   glm::vec3 uiUpAxis = playSpaceRotation * glm::vec3(0, 1, 0);
 //   glm::vec3 uiNormalAxis = playSpaceRotation * glm::vec3(0, 0, -1);
 //   
 //   // Position UI plane at configured distance from origin
 //   float uiDistance = uevr::API::VR::get_mod_value<float>(Framework_UI_Distance);
 //   glm::vec3 uiPlaneCenter = playSpaceOrigin + uiNormalAxis * uiDistance;
 //   
 //   // Perform ray-plane intersection
 //   float rayDotNormal = glm::dot(uiNormalAxis, controllerDirection);
 //   
 //   if (fabs(rayDotNormal) > 1e-6f)  // If ray not parallel to plane
 //   {
 //       float intersectionDistance = glm::dot(uiPlaneCenter - controllerPosition, uiNormalAxis) / rayDotNormal;
 //       
 //       if (intersectionDistance > 0.0f)  // Intersection in front of controller
 //       {
 //           // Calculate intersection point
 //           glm::vec3 intersectionPoint = controllerPosition + controllerDirection * intersectionDistance;
 //           
 //           // Project intersection to UI plane local coordinates
 //           glm::vec3 localOffset = intersectionPoint - uiPlaneCenter;
 //           float localX = glm::dot(localOffset, uiRightAxis);
 //           float localY = glm::dot(localOffset, uiUpAxis);
 //           
 //           // Normalize by UI size and apply cursor speed scaling
 //           float uiSize = uevr::API::VR::get_mod_value<float>(Framework_UI_Size);
 //           float normalizedX = (localX / uiSize) * 0.565f;
 //           float normalizedY = localY / uiSize;
 //           
 //           // Convert to ImGui screen coordinates
 //           ImVec2 targetMousePosition(
 //               (normalizedX + 0.5f) * io.DisplaySize.x,
 //               (0.5f - normalizedY) * io.DisplaySize.y
 //           );
 //           
 //           // Initialize last position if needed
 //           if (LastMousePosition.x < 0.0f)
 //           {
 //               LastMousePosition = targetMousePosition;
 //           }
 //           
 //           // Apply smoothing to cursor movement
 //           const float smoothingFactor = 0.2f;
 //           LastMousePosition.x = glm::mix(LastMousePosition.x, targetMousePosition.x, smoothingFactor);
 //           LastMousePosition.y = glm::mix(LastMousePosition.y, targetMousePosition.y, smoothingFactor);
 //           
 //           // Update ImGui input state
            io.MousePos = LastMousePosition;
			const bool controllerInput = Inputs::IsActionActive(false, "/actions/default/in/AButtonRight");
			//io.AddMouseButtonEvent(0, io.MouseClicked);
			// Only send changes: sending it every frame would release the real mouse button held from on_message
			if (controllerInput != ControllerInput)
				io.AddMouseButtonEvent(0, controllerInput);
			ControllerInput = controllerInput;
    //    }
    //}
}

void ImGuiMenu::checkMouseFocus(float delta)
{
	//Logger::DebugPrint("MouseMovedThisFrame : %i", MouseMovedThisFrame);
	//Logger::DebugPrint("MouseHasFocus : %i", MouseHasFocus);
	//Logger::DebugPrint("mouseUseTimer : %f", mouseUseTimer);
	auto io = ImGui::GetIO();


	MouseHasFocus = uevr::API::get()->param()->vr->is_hmd_active() ? false : true;
	return;

	//need rework :
	if (ControllerInput)
	{
		MouseHasFocus = false;
		return;
	}

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
	
	if (!imguiSettingsInitialized)
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(imguiSize); // Scale padding, spacing, borders, etc.
		ImGui::GetIO().FontGlobalScale = imguiSize;
		imguiSettingsInitialized = true;
	}

	if (ImGui::Begin("MGS Delta VR Plugin Settings")) {
		float available = ImGui::GetContentRegionAvail().x;
		ImGui::PushItemWidth(available * 0.4f);
		if (ImGui::CollapsingHeader("Camera"))
		{
			/*	ImGui::Text("Decoupled pitch: %i", API::VR::is_decoupled_pitch_enabled());

				if (ImGui::Button("Toggle decoupled pitch")) {
					API::VR::set_decoupled_pitch_enabled(!API::VR::is_decoupled_pitch_enabled());
				}*/
			ImGui::SeparatorText("Rotation");

			if (ImGui::SliderFloat("Rotation speed", settingsManager->CameraRotationSpeedPtr, 0.1f, 50.0f))
				settingsManager->SettingsModifiedThisFrame = true;
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Controls how fast the camera rotates when turning");

			if (ImGui::SliderFloat("Rotation deadzone", settingsManager->CameraRotationDeadZonePtr, 0.0f, 1.0f))
				settingsManager->SettingsModifiedThisFrame = true;
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Thumbstick input below this threshold will be ignored");

			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			ImGui::SeparatorText("Movement smoothing");

			if (ImGui::Checkbox("Smooth camera rotation", settingsManager->SmoothCameraRotationPtr))
				settingsManager->SettingsModifiedThisFrame = true;

			ImGui::BeginDisabled(!*settingsManager->SmoothCameraRotationPtr);
			{
				ImGui::Indent();
				if (ImGui::SliderFloat("Rotation smoothness", settingsManager->CameraYawLerpPtr, 0.0f, 10.0f))
					settingsManager->SettingsModifiedThisFrame = true;
				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::Text("Higher values = smoother but slower rotation");
					ImGui::Text("Lower values = snappier but less smooth");
					ImGui::EndTooltip();
				}
				ImGui::Unindent();
			}
			ImGui::EndDisabled();

			if (ImGui::Checkbox("Smooth camera position", settingsManager->SmoothCameraPositionPtr))
				settingsManager->SettingsModifiedThisFrame = true;

			ImGui::BeginDisabled(!*settingsManager->SmoothCameraPositionPtr);
			{
				ImGui::Indent();
				if (ImGui::SliderFloat("Horizontal smoothness (X&Y Axis)", settingsManager->CameraPositionXYLerpPtr, 0.0f, 10.0f))
					settingsManager->SettingsModifiedThisFrame = true;
				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::Text("Higher values = smoother but slower movement");
					ImGui::Text("Lower values = snappier but less smooth");
					ImGui::EndTooltip();
				}

				if (ImGui::SliderFloat("Vertical smoothness (Z Axis)", settingsManager->CameraPositionZLerpPtr, 0.0f, 10.0f))
					settingsManager->SettingsModifiedThisFrame = true;
				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::Text("Higher values = smoother but slower movement");
					ImGui::Text("Lower values = snappier but less smooth");
					ImGui::EndTooltip();
				}
				ImGui::Unindent();

				ImGui::Dummy(ImVec2(0.0f, 10.0f));
				ImGui::SeparatorText("Misc.");

				if (ImGui::Checkbox("Attach camera to snake head", settingsManager->AttachCameraToSnakeHeadPtr))
					settingsManager->SettingsModifiedThisFrame = true;
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Not recommended for comfort");

				ImGui::Dummy(ImVec2(0.0f, 10.0f));
			}
			ImGui::EndDisabled();
		}
		
		if (ImGui::CollapsingHeader("Gameplay"))
		{
			ImGui::SeparatorText("RoomScale");
			if (ImGui::Checkbox("Enable RoomScale Prone And Crouch", settingsManager->EnableRoomscaleProneAndCrouchPtr))
				settingsManager->SettingsModifiedThisFrame = true;
			if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::Text("Allows the game to set Snake's Stance from your own IRL stance");
				ImGui::EndTooltip();
			}

			ImGui::BeginDisabled(!*settingsManager->EnableRoomscaleProneAndCrouchPtr);
			{
				ImGui::Indent();
				if (ImGui::SliderFloat("Squat under this value", settingsManager->SquatUnderThisValuePtr, -1.0f, 0.0f))
					settingsManager->SettingsModifiedThisFrame = true;
				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::Text("In meter. The hmd's own height after recentering is at 0.");
					ImGui::Text("The game will set Snake to Squat when your hmd's height is below this value.");
					ImGui::EndTooltip();
				}

				if (ImGui::SliderFloat("Crawl under this value", settingsManager->CrawlUnderThisValuePtr, -2.0f, 0.0f))
					settingsManager->SettingsModifiedThisFrame = true;
				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::Text("In meter. The hmd's own height after recentering is at 0.");
					ImGui::Text("The game will set Snake to Crawl when your hmd's height is below this value.");
					ImGui::EndTooltip();
				}
				ImGui::Unindent();
			}
			ImGui::EndDisabled();

			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			ImGui::SeparatorText("Weapons");
			if (ImGui::Checkbox("Enable VR scopes", settingsManager->EnableWeaponsScopesPtr))
			{
				settingsManager->SettingsModifiedThisFrame = true;
				if (!*settingsManager->EnableWeaponsScopesPtr)
					weaponManager->DestroyScope();
				else if (playerManager->IsPlayerControlled)
					weaponManager->SpawnScope();
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("VR scopes have an impact on framerate");

			ImGui::BeginDisabled(!*settingsManager->EnableWeaponsScopesPtr);
			{
				ImGui::Indent();
				int resolution = *settingsManager->ScopeResolutionPtr;

				// Your constants
				const int step = 64;
				const int minRes = 64;
				const int maxRes = 2048;

				int sliderMin = minRes / step;  // 1
				int sliderMax = maxRes / step;  // 32

				// Create slider index from actual resolution (only used for initialization)
				int sliderIndex = resolution / step;

				// Make sure the index pointer starts correct (optional safety)
				*settingsManager->ScopeResolutionSliderIndexPtr = sliderIndex;

				if (ImGui::SliderInt("Scopes resolution",
					settingsManager->ScopeResolutionSliderIndexPtr,
					sliderMin,
					sliderMax))
				{
					// Use the pointer that ImGui updated
					int newIndex = *settingsManager->ScopeResolutionSliderIndexPtr;

					// Convert to actual resolution
					int newResolution = newIndex * step;

					// Write resolution back to your real pointer
					if (newResolution != *settingsManager->ScopeResolutionPtr)
					{
						*settingsManager->ScopeResolutionPtr = newResolution;
						settingsManager->SettingsModifiedThisFrame = true;

						if (playerManager->IsPlayerControlled && *settingsManager->EnableWeaponsScopesPtr)
						{
							weaponManager->DestroyScope();
							weaponManager->SpawnScope();
						}
					}
				}
				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::Text("Change the quality of the Scope view. Can help with performance.");
					ImGui::EndTooltip();
				}
				ImGui::SameLine();
				ImGui::Text("%d x %d px", resolution, resolution);
				ImGui::Unindent();
			}
			ImGui::EndDisabled();


			if (ImGui::Checkbox("Disable all recoil", settingsManager->DisableAllRecoilPtr))
			{
				settingsManager->SettingsModifiedThisFrame = true;
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Disable recoil for all weapons");

			if (ImGui::Checkbox("Enable two handed grip pivot", settingsManager->EnableTwoHandsPivotPtr))
			{
				settingsManager->SettingsModifiedThisFrame = true;
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Allow the weapon to align with both hands when gripped");

			if (ImGui::Checkbox("Enable two handed realistic recoil", settingsManager->EnableTwoHandsRealisticRecoilPtr))
			{
				settingsManager->SettingsModifiedThisFrame = true;
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Weapon recoil will be stronger when gripped with single hand. Lighter when gripped with both hands.");

			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			ImGui::SeparatorText("Items");
			if (ImGui::Checkbox("Disable thermal goggles scanline effect", settingsManager->DisableThermalVisionScanlinePtr))
			{
				settingsManager->SettingsModifiedThisFrame = true;
				weaponManager->ToggleThermalVisionScanline();
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Removes the aggressive scanline effect from the thermal vision");
			ImGui::Dummy(ImVec2(0.0f, 10.0f));
		}

		if (ImGui::CollapsingHeader("Rendering Settings"))
		{
			ImGui::SeparatorText("Cutscenes");
			if (ImGui::Checkbox("Enable Immersive mode for cutscenes", settingsManager->EnableImmersiveModePtr)) {
				if (*settingsManager->EnableImmersiveModePtr)
				{
					cameraController->InstantCameraPositionNeeded = true;
					cameraController->InstantCameraPositionTimer = 0.25f;
					cameraController->DestroyCustom2DScreen();
				}

				else
					cameraController->SpawnCustom2DScreen();
				uevr::API::VR::set_mod_value("VR_DecoupledPitch", *settingsManager->EnableImmersiveModePtr);
				settingsManager->SettingsModifiedThisFrame = true;
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Text("Cutscenes camera movements can be intense. Enabling this can cause motion sickness.");
				ImGui::Text("Black bars will spawn for a comfortable experience if disabled.");
				ImGui::EndTooltip();
			}

			ImGui::BeginDisabled(*settingsManager->EnableImmersiveModePtr);
			{
				ImGui::Indent();
				if (ImGui::SliderFloat("Black Bars Size", settingsManager->BlackBarsSizePtr, 1.0f, 10.0f, "%.1f"))
				{
					if (cameraController->CutscenePlaying())
						cameraController->ResizeBlackBars();
					settingsManager->SettingsModifiedThisFrame = true;
				}
					
				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::Text("Enlarge or reduce the black bars.");
					ImGui::Text("Default value (1.0) is how the cutscenes are intended to be seen in flatscreen");
					ImGui::EndTooltip();
				}
				ImGui::Unindent();
			}
			ImGui::EndDisabled();

			ImGui::BeginDisabled(!*settingsManager->EnableImmersiveModePtr);
			{
				ImGui::Indent();
				if (ImGui::Checkbox("Smooth Camera Movements", settingsManager->SmoothCameraMovementsInImmersiveCutscenesPtr)) {
					settingsManager->SettingsModifiedThisFrame = true;
				}

				ImGui::BeginDisabled(!*settingsManager->SmoothCameraMovementsInImmersiveCutscenesPtr);
				{
					ImGui::Indent();
					//ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f); // Make sliders a bit shorter
					if (ImGui::SliderFloat("Position Smoothness", settingsManager->CutsceneCameraPositionLerpPtr, 0.0f, 10.0f))
						settingsManager->SettingsModifiedThisFrame = true;
					if (ImGui::IsItemHovered()) {
						ImGui::BeginTooltip();
						ImGui::Text("Higher values = smoother but slower movement");
						ImGui::Text("Lower values = snappier but less smooth");
						ImGui::EndTooltip();
					}
					if (ImGui::SliderFloat("Rotation Smoothness", settingsManager->CutsceneCameraRotationLerpPtr, 0.0f, 10.0f))
						settingsManager->SettingsModifiedThisFrame = true;
					if (ImGui::IsItemHovered()) {
						ImGui::BeginTooltip();
						ImGui::Text("Higher values = smoother but slower movement");
						ImGui::Text("Lower values = snappier but less smooth");
						ImGui::EndTooltip();
					}
					ImGui::Unindent();
				}
				ImGui::Unindent();
				ImGui::EndDisabled();
			}
			ImGui::EndDisabled();

			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			ImGui::SeparatorText("3D Rendering");

			if (ImGui::Checkbox("Enable 3D in Cutscenes", settingsManager->Enable3DCutscenesPtr))
			{
				settingsManager->SettingsModifiedThisFrame = true;
				if (cameraController->CutscenePlaying())
					uevr::API::VR::set_mod_value("VR_WorldScale", *settingsManager->Enable3DCutscenesPtr ? 1.0f : 0.0f);
			}
			//if (ImGui::IsItemHovered())
			//	ImGui::SetTooltip("Some link softwares (SteamVR and VD) can have issues when both Cut Out screen and 3D cutscenes are enabled. Disable one or the other if you have back screen or double vision issues.");

			if (ImGui::Checkbox("Enable 3D in Radio window", settingsManager->Enable3DRadioPtr))
			{
				settingsManager->SettingsModifiedThisFrame = true;
				if (playerManager->CurrentMenuType == ue::EGsrUIMenuType::Radio)
				{
					uevr::API::VR::set_mod_value("VR_WorldScale", *settingsManager->Enable3DRadioPtr ? 1.0f : 0.0f);
				}
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Can make text more difficult to read, disabled by default");

			if (ImGui::Checkbox("Enable 3D in Survival Menu", settingsManager->Enable3DSurvivalViewPtr))
			{
				settingsManager->SettingsModifiedThisFrame = true;
				if (playerManager->CurrentMenuType == ue::EGsrUIMenuType::SV)
					uevr::API::VR::set_mod_value("VR_WorldScale", *settingsManager->Enable3DSurvivalViewPtr ? 1.0f : 0.0f);
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Works perfectly, enabled by default");
		}

		BodyIK::DrawMenu();

		if (ImGui::CollapsingHeader("Performance"))
		{
			ImGui::TextWrapped("Checked settings are forced: the game would otherwise reset them on each level and cutscene. Unchecking one gives it back to the game.");
			const char* group = nullptr;
			for (auto& o : performanceManager->Overrides)
			{
				if (group == nullptr || strcmp(group, o.group) != 0)
				{
					group = o.group;
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SeparatorText(group);
				}
				ImGui::PushID(&o);
				if (ImGui::Checkbox("##enabled", &o.enabled))
				{
					performanceManager->RequestApply();
					performanceManager->Save();
				}
				ImGui::SameLine();
				ImGui::BeginDisabled(!o.enabled || o.missing);
				const std::string name = ue::narrow(o.name);
				bool changed = false;
				if (o.isInt)
				{
					int value = (int)std::lround(o.value);
					changed = ImGui::SliderInt(name.c_str(), &value, (int)o.min, (int)o.max);
					if (changed)
						o.value = (float)value;
				}
				else
					changed = ImGui::SliderFloat(name.c_str(), &o.value, o.min, o.max, "%.2f");
				if (changed)
					performanceManager->RequestApply();
				if (ImGui::IsItemDeactivatedAfterEdit())
					performanceManager->Save();
				ImGui::EndDisabled();
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
					ImGui::SetTooltip("%s", o.missing ? "Not found in this game" : o.tooltip);
				ImGui::PopID();
			}

			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			ImGui::SeparatorText("Measure");
			bool monitoring = performanceMonitor->Enabled;
			if (ImGui::Checkbox("Measure performance", &monitoring))
				performanceMonitor->SetEnabled(monitoring);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Measures framerate, GPU and thread usage every 2 s. Off by default");
			if (monitoring)
			{
				const auto stats = performanceMonitor->GetSnapshot();
				if (!stats.valid)
					ImGui::Text("Measuring...");
				else
				{
					ImGui::Text("%.0f fps, frame %.1f ms (max %.1f)", stats.fps, stats.frameMs, stats.frameMaxMs);
					ImGui::Text("Game thread tick %.1f ms (max %.1f), plugin %.2f ms", stats.gameTickMs, stats.gameTickMaxMs, stats.pluginMs);
					if (stats.gpuPercent >= 0.0f)
					{
						ImGui::Text("GPU %.0f%% (game %.0f%%), VRAM %.0f MB", stats.gpuPercent, stats.gpuGamePercent, stats.vramMB);
						ImGui::Text("Game GPU engines: %s", stats.gpuGameEngines.c_str());
					}
					else
						ImGui::Text("GPU usage unavailable");
					ImGui::Text("Busiest threads (%% of one core):");
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("A thread close to 100%% (GameThread, RenderThread, RHIThread) limits the framerate.\nGPU close to 100%% = the GPU limits it.");
					for (const auto& thread : stats.busiestThreads)
						ImGui::BulletText("%s  %.0f%%", thread.name.c_str(), thread.cpuPercent);
				}
				bool logToFile = performanceMonitor->LogToFile;
				if (ImGui::Checkbox("Write these stats to log.txt every 2 s", &logToFile))
					performanceMonitor->LogToFile = logToFile;
			}
		}

		ImGui::PopItemWidth();
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

	// Initialization runs again after each device reset (desktop <-> VR): keep the same context
	// instead of leaking a new one every time
	if (ImGui::GetCurrentContext() == nullptr)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		static const auto imgui_ini = uevr::API::get()->get_persistent_dir(L"imgui_mgsDeltaVR_plugin.ini").string();
		ImGui::GetIO().IniFilename = imgui_ini.c_str();
	}
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