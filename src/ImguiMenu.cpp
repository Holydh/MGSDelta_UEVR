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

void ImGuiMenu::ProcessImGui()
{
	if (!m_initialized || !uevr::API::get()->param()->functions->is_drawing_ui())
	{
		return;
	}

	std::scoped_lock _{ m_imgui_mutex };

	ImGui_ImplWin32_NewFrame();
	handleVRcursor();
	ImGui::NewFrame();

	internalFrame();

	ImGui::EndFrame();
	ImGui::Render();
}

void ImGuiMenu::handleVRcursor()
{
	// The cursor position comes from the Lua script (MouseX / MouseY events), the click from the right A button
	ImGuiIO& io = ImGui::GetIO();
	io.MouseDrawCursor = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
	io.MousePos = LastMousePosition;
	const bool controllerInput = Inputs::IsActionActive(false, "/actions/default/in/AButtonRight");
	// Only send changes: sending it every frame would release the real mouse button held from on_message
	if (controllerInput != ControllerInput)
		io.AddMouseButtonEvent(0, controllerInput);
	ControllerInput = controllerInput;
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
			ImGui::SeparatorText("Rotation");

			if (ImGui::SliderFloat("Rotation speed", &settingsManager->Settings.cameraRotationSpeed, 0.1f, 50.0f))
				settingsManager->SettingsModified = true;
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Controls how fast the camera rotates when turning");

			if (ImGui::SliderFloat("Rotation deadzone", &settingsManager->Settings.cameraRotationDeadZone, 0.0f, 1.0f))
				settingsManager->SettingsModified = true;
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Thumbstick input below this threshold will be ignored");

			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			ImGui::SeparatorText("Movement smoothing");

			if (ImGui::Checkbox("Smooth camera rotation", &settingsManager->Settings.smoothCameraRotation))
				settingsManager->SettingsModified = true;

			ImGui::BeginDisabled(!settingsManager->Settings.smoothCameraRotation);
			{
				ImGui::Indent();
				if (ImGui::SliderFloat("Rotation smoothness", &settingsManager->Settings.cameraYawLerp, 0.0f, 10.0f))
					settingsManager->SettingsModified = true;
				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::Text("Higher values = smoother but slower rotation");
					ImGui::Text("Lower values = snappier but less smooth");
					ImGui::EndTooltip();
				}
				ImGui::Unindent();
			}
			ImGui::EndDisabled();

			if (ImGui::Checkbox("Smooth camera position", &settingsManager->Settings.smoothCameraPosition))
				settingsManager->SettingsModified = true;

			ImGui::BeginDisabled(!settingsManager->Settings.smoothCameraPosition);
			{
				ImGui::Indent();
				if (ImGui::SliderFloat("Horizontal smoothness (X&Y Axis)", &settingsManager->Settings.cameraPositionXYLerp, 0.0f, 10.0f))
					settingsManager->SettingsModified = true;
				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::Text("Higher values = smoother but slower movement");
					ImGui::Text("Lower values = snappier but less smooth");
					ImGui::EndTooltip();
				}

				if (ImGui::SliderFloat("Vertical smoothness (Z Axis)", &settingsManager->Settings.cameraPositionZLerp, 0.0f, 10.0f))
					settingsManager->SettingsModified = true;
				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::Text("Higher values = smoother but slower movement");
					ImGui::Text("Lower values = snappier but less smooth");
					ImGui::EndTooltip();
				}
				ImGui::Unindent();

				ImGui::Dummy(ImVec2(0.0f, 10.0f));
				ImGui::SeparatorText("Misc.");

				if (ImGui::Checkbox("Attach camera to snake head", &settingsManager->Settings.attachCameraToSnakeHead))
					settingsManager->SettingsModified = true;
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Not recommended for comfort");

				ImGui::Dummy(ImVec2(0.0f, 10.0f));
			}
			ImGui::EndDisabled();
		}
		
		if (ImGui::CollapsingHeader("Gameplay"))
		{
			ImGui::SeparatorText("Locomotion");
			if (ImGui::Checkbox("Linear head-relative locomotion (experimental)", &settingsManager->Settings.linearLocomotion))
				settingsManager->SettingsModified = true;
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Constant speed in the direction of your head, instead of following Snake's animations.");
			ImGui::BeginDisabled(!settingsManager->Settings.linearLocomotion);
			if (ImGui::Checkbox("Let the game take control of Snake's movement", &settingsManager->Settings.gameTakesMovementControl))
				settingsManager->SettingsModified = true;
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
				ImGui::SetTooltip("Area transitions and scripted moves use the game's own movement.\nDisabled: Snake always follows your stick.");
			ImGui::EndDisabled();

			ImGui::SeparatorText("RoomScale");
			if (ImGui::Checkbox("Enable RoomScale Prone And Crouch", &settingsManager->Settings.enableRoomscaleProneAndCrouch))
				settingsManager->SettingsModified = true;
			if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::Text("Allows the game to set Snake's Stance from your own IRL stance");
				ImGui::EndTooltip();
			}

			ImGui::BeginDisabled(!settingsManager->Settings.enableRoomscaleProneAndCrouch);
			{
				ImGui::Indent();
				if (ImGui::SliderFloat("Squat under this value", &settingsManager->Settings.squatUnderThisValue, -1.0f, 0.0f))
					settingsManager->SettingsModified = true;
				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::Text("In meter. The hmd's own height after recentering is at 0.");
					ImGui::Text("The game will set Snake to Squat when your hmd's height is below this value.");
					ImGui::EndTooltip();
				}

				if (ImGui::SliderFloat("Crawl under this value", &settingsManager->Settings.crawlUnderThisValue, -2.0f, 0.0f))
					settingsManager->SettingsModified = true;
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
			if (ImGui::Checkbox("Enable VR scopes", &settingsManager->Settings.enableWeaponsScopes))
			{
				settingsManager->SettingsModified = true;
				if (!settingsManager->Settings.enableWeaponsScopes)
					weaponManager->DestroyScope();
				else if (gameState->IsPlayerControlled)
					weaponManager->SpawnScope();
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("VR scopes have an impact on framerate");

			ImGui::BeginDisabled(!settingsManager->Settings.enableWeaponsScopes);
			{
				ImGui::Indent();
				const int resolution = settingsManager->Settings.scopeResolution;

				// The slider moves in 64 px steps, from 64 to 2048 px
				const int step = 64;
				int sliderIndex = resolution / step;
				if (ImGui::SliderInt("Scopes resolution", &sliderIndex, 64 / step, 2048 / step))
				{
					const int newResolution = sliderIndex * step;
					if (newResolution != settingsManager->Settings.scopeResolution)
					{
						settingsManager->Settings.scopeResolution = newResolution;
						settingsManager->SettingsModified = true;

						if (gameState->IsPlayerControlled && settingsManager->Settings.enableWeaponsScopes)
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


			if (ImGui::Checkbox("Disable all recoil", &settingsManager->Settings.disableAllRecoil))
			{
				settingsManager->SettingsModified = true;
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Disable recoil for all weapons");

			if (ImGui::Checkbox("Enable two handed grip pivot", &settingsManager->Settings.enableTwoHandsPivot))
			{
				settingsManager->SettingsModified = true;
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Allow the weapon to align with both hands when gripped");

			if (ImGui::Checkbox("Enable two handed realistic recoil", &settingsManager->Settings.enableTwoHandsRealisticRecoil))
			{
				settingsManager->SettingsModified = true;
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Weapon recoil will be stronger when gripped with single hand. Lighter when gripped with both hands.");

			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			ImGui::SeparatorText("Items");
			if (ImGui::Checkbox("Disable thermal goggles scanline effect", &settingsManager->Settings.disableThermalVisionScanline))
			{
				settingsManager->SettingsModified = true;
				weaponManager->ToggleThermalVisionScanline();
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Removes the aggressive scanline effect from the thermal vision");
			ImGui::Dummy(ImVec2(0.0f, 10.0f));
		}

		if (ImGui::CollapsingHeader("Rendering Settings"))
		{
			ImGui::SeparatorText("Cutscenes");
			if (ImGui::Checkbox("Enable Immersive mode for cutscenes", &settingsManager->Settings.enableImmersiveMode)) {
				if (settingsManager->Settings.enableImmersiveMode)
				{
					cameraController->InstantCameraPositionNeeded = true;
					cameraController->InstantCameraPositionTimer = 0.25f;
					cutsceneManager->DestroyCustom2DScreen();
				}

				else
					cutsceneManager->SpawnCustom2DScreen();
				settingsManager->SettingsModified = true;
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Text("Cutscenes camera movements can be intense. Enabling this can cause motion sickness.");
				ImGui::Text("Black bars will spawn for a comfortable experience if disabled.");
				ImGui::EndTooltip();
			}

			ImGui::BeginDisabled(settingsManager->Settings.enableImmersiveMode);
			{
				ImGui::Indent();
				if (ImGui::SliderFloat("Black Bars Size", &settingsManager->Settings.blackBarsSize, 1.0f, 10.0f, "%.1f"))
				{
					if (gameState->CutscenePlaying())
						cutsceneManager->ResizeBlackBars();
					settingsManager->SettingsModified = true;
				}
					
				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::Text("Enlarge or reduce the black bars.");
					ImGui::EndTooltip();
				}
				ImGui::Unindent();
			}
			ImGui::EndDisabled();

			ImGui::BeginDisabled(!settingsManager->Settings.enableImmersiveMode);
			{
				ImGui::Indent();
				if (ImGui::Checkbox("Smooth Camera Movements", &settingsManager->Settings.smoothCameraMovementsInImmersiveCutscenes)) {
					settingsManager->SettingsModified = true;
				}

				ImGui::BeginDisabled(!settingsManager->Settings.smoothCameraMovementsInImmersiveCutscenes);
				{
					ImGui::Indent();
					if (ImGui::SliderFloat("Position Smoothness", &settingsManager->Settings.cutsceneCameraPositionLerp, 0.0f, 10.0f))
						settingsManager->SettingsModified = true;
					if (ImGui::IsItemHovered()) {
						ImGui::BeginTooltip();
						ImGui::Text("Higher values = smoother but slower movement");
						ImGui::Text("Lower values = snappier but less smooth");
						ImGui::EndTooltip();
					}
					if (ImGui::SliderFloat("Rotation Smoothness", &settingsManager->Settings.cutsceneCameraRotationLerp, 0.0f, 10.0f))
						settingsManager->SettingsModified = true;
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

			if (ImGui::Checkbox("Enable 3D in Cutscenes", &settingsManager->Settings.enable3DCutscenes))
				settingsManager->SettingsModified = true;

			if (ImGui::Checkbox("Enable 3D in Radio window", &settingsManager->Settings.enable3DRadio))
				settingsManager->SettingsModified = true;
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Can make text more difficult to read, disabled by default");

			if (ImGui::Checkbox("Enable 3D in Survival Menu", &settingsManager->Settings.enable3DSurvivalView))
				settingsManager->SettingsModified = true;
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Works perfectly, enabled by default");
		}

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

	// Saved once the edit ends, not on every frame of a slider drag
	if (settingsManager->SettingsModified && !ImGui::IsAnyItemActive())
	{
		settingsManager->Save();
		settingsManager->SettingsModified = false;
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