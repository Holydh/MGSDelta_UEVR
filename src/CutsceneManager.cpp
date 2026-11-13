#include "CutsceneManager.hpp"
#include "Utilities.hpp"
#include "Logger.hpp"
#include "magic_enum/magic_enum.hpp"

void CutsceneManager::ApplyCutsceneSettings()
{
	if (gameState->CutsceneStarts())
	{
		uevr::API::VR::recenter_view();
		if (gameState->CurrentCutsceneState == GameState::Playing3D && !settingsManager->Settings.enableImmersiveMode)
		{
			SpawnCustom2DScreen();
		}
		Logger::DebugPrint("Cutscene State changed to : %s", std::string(magic_enum::enum_name(gameState->CurrentCutsceneState.Get())).c_str());
	}
	if (gameState->CutsceneStops())
	{
		DestroyCustom2DScreen();
		Logger::DebugPrint("Cutscene State changed to : %s", std::string(magic_enum::enum_name(gameState->CurrentCutsceneState.Get())).c_str());
	}

	if (gameState->CurrentCutsceneState == GameState::Playing2D)
		ShowMovieOnUI();
	else
		HideMovieUI();

	if (gameState->CutscenePlaying())
	{
		static ue::SingletonRef cobraCutsceneCameraCtrlMngr_c{ L"Class /Script/CobraCutScene.CobraCutsceneCameraControlManagerComponent" };
		auto cobraCutsceneCameraCtrlMngr = cobraCutsceneCameraCtrlMngr_c.get();
		if (!cobraCutsceneCameraCtrlMngr)
		{
			Logger::DebugPrint("CobraCutsceneCameraCtrlMngr not found");
			return;
		}

		if (!settingsManager->Settings.enableImmersiveMode)
		{
			if (ue::get<bool>(cobraCutsceneCameraCtrlMngr, L"bIsFpv"))
				DestroyCustom2DScreen();
			else
				SpawnCustom2DScreen();
		}
	}
}

// 2D cutscenes are Bink overlays drawn across the whole double-wide stereo render target, which splits them
// between the eyes. Render them to a texture instead, shown by a full-screen widget: UEVR draws the UI whole in each eye.
void CutsceneManager::ShowMovieOnUI()
{
	static ue::ClassRef binkMediaPlayer_c{ L"Class /Script/BinkMediaPlayer.BinkMediaPlayer" };
	if (!binkMediaPlayer_c.get())
		return;

	for (auto player : binkMediaPlayer_c.get()->get_objects_matching(false))
	{
		// Radio, food, cure... movies already render to textures shown by the game's widgets
		const auto drawStyle = ue::get<ue::EBinkMediaPlayerBinkDrawStyle>(player, L"BinkDrawStyle");
		if (drawStyle == ue::EBinkMediaPlayerBinkDrawStyle::BMASM_Bink_DS_RenderToTexture)
			continue;

		if (MovieWidget == nullptr)
		{
			static ue::ClassRef widgetBlueprintLibrary_c{ L"Class /Script/UMG.WidgetBlueprintLibrary" };
			static ue::ClassRef commonUserWidget_c{ L"Class /Script/CommonUI.CommonUserWidget" }; // UUserWidget is abstract
			static ue::ClassRef image_c{ L"Class /Script/UMG.Image" };
			static ue::ClassRef binkMediaTexture_c{ L"Class /Script/BinkMediaPlayer.BinkMediaTexture" };

			auto widget = ue::call_static<ue::UObject*>(widgetBlueprintLibrary_c.get(), L"Create",
				ue::get_world(), commonUserWidget_c.get(), nullptr);
			auto widgetTree = ue::get<ue::UObject*>(widget, L"WidgetTree");
			if (!widgetTree || !image_c.get() || !binkMediaTexture_c.get())
			{
				Logger::DebugPrint("Failed to create the 2D cutscene widget");
				return;
			}

			auto image = uevr::API::get()->spawn_object(image_c.get(), widgetTree);
			auto texture = uevr::API::get()->spawn_object(binkMediaTexture_c.get(), widget);
			if (!image || !texture)
			{
				Logger::DebugPrint("Failed to create the 2D cutscene image or texture");
				return;
			}
			ue::set(widgetTree, L"RootWidget", image);
			ue::call(image, L"SetBrushResourceObject", texture);
			ue::call(widget, L"AddToViewport", -100); // below the game's widgets (subtitles...)

			MovieWidget = widget;
			MovieTexture = texture;
		}

		ue::set(player, L"BinkDrawStyle", ue::EBinkMediaPlayerBinkDrawStyle::BMASM_Bink_DS_RenderToTexture);
		ue::call(MovieTexture, L"SetMediaPlayer", player);
		MoviePlayer = player;
		MoviePlayerDrawStyle = drawStyle;
	}
}

void CutsceneManager::HideMovieUI()
{
	if (MovieWidget == nullptr)
		return;

	ue::call(MovieWidget, L"RemoveFromParent");
	MovieWidget = nullptr;
	MovieTexture = nullptr;

	// In case the game reuses this player for its next movie
	if (MoviePlayer)
		ue::set(MoviePlayer, L"BinkDrawStyle", MoviePlayerDrawStyle);
	MoviePlayer = nullptr;
}

void CutsceneManager::SpawnCustom2DScreen()
{
	if (!gameState->CutscenePlaying() || Custom2DScreen != nullptr)
		return;

	static ue::ClassRef staticMesh_c{ L"Class /Script/Engine.StaticMesh" };
	static ue::ClassRef staticMeshComponent_c{ L"Class /Script/Engine.StaticMeshComponent" };

    // 1. Find the assets already loaded in memory. Checked before spawning the actor: this runs every
	// frame of a cutscene until it succeeds, so a failure must not leave an actor behind.
	auto staticMesh = ue::find_object(L"StaticMesh /Engine/BasicShapes/Plane.Plane");
    if (ue::is_a(staticMesh, staticMesh_c))
    {
		Logger::DebugPrint(L"Found static mesh : " + staticMesh->get_full_name());
	}
	else
	{
		Logger::DebugPrint("Asset found is not a StaticMesh or SkeletalMesh");
		return;
	}

	auto translucentMat = ue::find_object(L"Material /Engine/EngineDebugMaterials/DebugMeshMaterial.DebugMeshMaterial");
	if (!translucentMat)
	{
		Logger::DebugPrint("Failed to find material for 2D screen");
		return;
	}

	UEVR_Vector3d uevrVector = { 0.0f, 0.0f, 0.0f };
	auto actor = Utilities::SpawnActor(uevrVector, nullptr);
	if (!actor)
	{
		return;
	}

	ue::set(translucentMat, L"TwoSided", true);
	ue::set(translucentMat, L"bDisableDepthTest", true);
	ue::set(translucentMat, L"BlendMode", ue::EBlendMode::BLEND_Translucent);
	ue::set(translucentMat, L"ShadingModel", ue::EMaterialShadingModel::MSM_Unlit);

	// create 5 planes for the box
	for (int i = 0; i < 5; i++)
	{
		auto staticMeshComponent = ue::call<ue::UObject*>(actor, L"AddComponentByClass",
			staticMeshComponent_c.get(),
			false,            // bManualAttachment
			ue::FTransform{}, // RelativeTransform
			false);           // bDeferredFinish

		if (!staticMeshComponent)
		{
			Logger::DebugPrint("Failed to create UStaticMeshComponent");
			ue::call(actor, L"K2_DestroyActor");
			return;
		}
		Logger::DebugPrint(L"Created StaticMeshComponent: " + staticMeshComponent->get_full_name());
		// 3. Assign the mesh asset to our new component instance
		ue::call(staticMeshComponent, L"SetStaticMesh", staticMesh);
		Logger::DebugPrint("Assigned mesh to StaticMeshComponent");

		// 4. Attach the new component to the Actor's root component
		ue::call(staticMeshComponent, L"K2_AttachToComponent",
			ue::call<ue::UObject*>(actor, L"K2_GetRootComponent"), // Parent
			ue::FName{},                                          // SocketName (none)
			ue::EAttachmentRule::KeepRelative,                    // Use the relative location we just set
			ue::EAttachmentRule::KeepRelative,                    // Use the relative rotation we just set
			ue::EAttachmentRule::KeepRelative,                    // Use the relative scale we just set
			false);                                               // bWeldSimulatedBodies
		Logger::DebugPrint("Attached StaticMeshComponent to Actor's root component");

		// 6. Register the component to make it "live" and renderable in the world
		ue::call(staticMeshComponent, L"Activate", true);
		Logger::DebugPrint("Activated StaticMeshComponent");

		// Optional: Disable collision if it's just for visuals
		ue::call(staticMeshComponent, L"SetCollisionEnabled", ue::ECollisionEnabled::NoCollision);
		ue::call(staticMeshComponent, L"Activate", true);
		ue::call(staticMeshComponent, L"SetVisibility", true, false);
		ue::call(staticMeshComponent, L"SetHiddenInGame", false, false);
		ue::call(staticMeshComponent, L"SetWorldScale3D", ue::FVector{ 1, 1, 1 });
		ue::call(staticMeshComponent, L"SetMobility", ue::EComponentMobility::Movable);

		ue::call(staticMeshComponent, L"SetRenderCustomDepth", true);

		ue::set<int32_t>(staticMeshComponent, L"TranslucencySortPriority", 1000);
		ue::call(staticMeshComponent, L"SetTranslucencySortDistanceOffset", 1000.0f);

		auto dynamicMaterial = ue::call<ue::UObject*>(staticMeshComponent, L"CreateAndSetMaterialInstanceDynamicFromMaterial", 0, translucentMat);
		ue::call(dynamicMaterial, L"SetVectorParameterValue",
			ue::FName(L"Color"),
			ue::FLinearColor{ 0.0f, 0.0f, 0.0f, 9999.0f }
		);

	}

	Custom2DScreen = actor;

	ResizeBlackBars();

	static ue::ClassRef cameraManager_c{ L"BlueprintGeneratedClass /Game/Blueprints/Camera/BP_PlayerCameraManager.BP_PlayerCameraManager_C" };
	auto cameraManagerActor = cameraManager_c.get() ? cameraManager_c.get()->get_first_object_matching(false) : nullptr;

	ue::call(Custom2DScreen, L"K2_AttachToActor", cameraManagerActor,
				ue::FName{},                                      // SocketName (none)
				ue::EAttachmentRule::KeepRelative,             // Use the relative location we just set
				ue::EAttachmentRule::KeepRelative,             // Use the relative rotation we just set
				ue::EAttachmentRule::KeepRelative,             // Use the relative scale we just set
				false                                            // bWeldSimulatedBodies
			);
	if (auto parentActor = ue::call<ue::UObject*>(actor, L"GetParentActor"))
		Logger::DebugPrint(L"custom box parent : " + parentActor->get_full_name());
}

void CutsceneManager::ResizeBlackBars()
{
	if (Custom2DScreen == nullptr)
		return;

	// Dimensions (Half-extents)
    float H = 250.0f; 
    float W = H * (16.0f / 9.0f);
    float D = 2000.0f; // How "deep" the cinema box is
    
    float planeUnit = 100.0f;
	auto root = ue::call<ue::UObject*>(Custom2DScreen, L"K2_GetRootComponent");
	ue::call(root, L"K2_SetRelativeLocation", ue::FVector{settingsManager->Settings.blackBarsSize * 100.0f -1000.0f , 0.0f, 0.0f }, false, nullptr, true);

    auto children = ue::get_ptr<ue::TArray<ue::UObject*>>(root, L"AttachChildren");
    for (int i = 0; children && i < children->count; i++)
    {
        auto smc = children->data[i];
        switch (i)
        {
            case 0: // TOP (Ceiling)
                ue::call(smc, L"K2_SetRelativeLocation", ue::FVector{ 0.0f, 0.0f, H }, false, nullptr, true);
                ue::call(smc, L"K2_SetRelativeRotation", ue::FRotator{ 180.0f, 0.0f, 0.0f }, false, nullptr, true);
                ue::call(smc, L"SetWorldScale3D", ue::FVector{ D/planeUnit, (W*2)/planeUnit, 1.0f });
                break;

            case 1: // BOTTOM (Floor)
                ue::call(smc, L"K2_SetRelativeLocation", ue::FVector{ 0.0f, 0.0f, -H }, false, nullptr, true);
                ue::call(smc, L"K2_SetRelativeRotation", ue::FRotator{ 0.0f, 0.0f, 0.0f }, false, nullptr, true);
                ue::call(smc, L"SetWorldScale3D", ue::FVector{ D/planeUnit, (W*2)/planeUnit, 1.0f });
                break;

			case 2: // BACK (The wall behind the screen)
				// Move it back by the full depth D
				ue::call(smc, L"K2_SetRelativeLocation", ue::FVector{ -D * 0.5f, 0.0f, 0.0f }, false, nullptr, true);
				ue::call(smc, L"K2_SetRelativeRotation", ue::FRotator{ 90.0f, 0.0f, 0.0f }, false, nullptr, true);
				ue::call(smc, L"SetWorldScale3D", ue::FVector{ (H * 2) / planeUnit, (W * 2) / planeUnit, 1.0f });
				break;

			case 3: // LEFT (Side wall)
				ue::call(smc, L"K2_SetRelativeLocation", ue::FVector{ 0.0f, -W, 0.0f }, false, nullptr, true);
				ue::call(smc, L"K2_SetRelativeRotation", ue::FRotator{ 90.0f, 0.0f, 90.0f }, false, nullptr, true);
				ue::call(smc, L"SetWorldScale3D", ue::FVector{ (H * 2) / planeUnit, D / planeUnit, 1.0f });
				break;

			case 4: // RIGHT (Side wall)
				ue::call(smc, L"K2_SetRelativeLocation", ue::FVector{ 0.0f, W, 0.0f }, false, nullptr, true);
				ue::call(smc, L"K2_SetRelativeRotation", ue::FRotator{ 90.0f, 0.0f, 90.0f }, false, nullptr, true);
				ue::call(smc, L"SetWorldScale3D", ue::FVector{ (H * 2) / planeUnit, D / planeUnit, 1.0f });
				break;
		}
	}
}

void CutsceneManager::DestroyCustom2DScreen()
{
	if (Custom2DScreen)
	{
		ue::call(Custom2DScreen, L"K2_DestroyActor");
		Custom2DScreen = nullptr;
	}
}
