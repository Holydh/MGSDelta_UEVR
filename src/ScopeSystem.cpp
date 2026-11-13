#include "ScopeSystem.hpp"
#include "Utilities.hpp"
#include "Logger.hpp"

using ue::EGsrEquipId;

namespace {
constinit ue::ClassRef StaticMeshComponent_c{ L"Class /Script/Engine.StaticMeshComponent" };
constinit ue::ClassRef SceneCaptureComponent2D_c{ L"Class /Script/Engine.SceneCaptureComponent2D" };
constinit ue::ClassRef WidgetComponent_c{ L"Class /Script/UMG.WidgetComponent" };
constinit ue::ClassRef Image_c{ L"Class /Script/UMG.Image" };
constinit ue::ClassRef CSightDragnovWidget_c{ L"Class /Script/CobraUI.CSightDragnovWidget" };
constinit ue::ClassRef CSightMosinWidget_c{ L"Class /Script/CobraUI.CSightMosinWidget" };
constinit ue::ClassRef CSightRpg7Widget_c{ L"Class /Script/CobraUI.CSightRpg7Widget" };

void hideWidget(ue::UObject* widget)
{
	ue::call(widget, L"SetVisibility", ue::ESlateVisibility::Hidden);
}

// Image->Brush.TintColor.SpecifiedColor = tint, then Image->SetBrush(Brush)
void setImageTint(ue::UObject* image, const ue::FLinearColor& tint)
{
	auto brush = ue::get_struct(image, L"Brush");
	if (!brush)
		return;
	brush.sub(L"TintColor").set(L"SpecifiedColor", tint);
	ue::call(image, L"SetBrush", brush);
}
}

void ScopeSystem::SpawnScope(ue::EGsrEquipId weaponId, ue::UObject* weaponRoot, ue::UObject* receiverMesh)
{
	for (auto child : ue::array_snapshot(weaponRoot, L"AttachChildren"))
	{
		if (!child) continue;
		//Destroy any previous cylinder mesh if still attached. Only ours (owned by the scope actor):
		//the weapon's own meshes belong to the game, which may still use them.
		if (ScopeActor && ue::is_a(child, StaticMeshComponent_c) && ue::call<ue::UObject*>(child, L"GetOwner") == ScopeActor) {
			ue::call(child, L"K2_DestroyComponent", child);
			ScopeStaticMeshComponent = nullptr;
		}
	}

	spawnScope(weaponId, receiverMesh);
	attachScopeToWeapon(weaponId, receiverMesh);
}

void ScopeSystem::Update(ue::EGsrEquipId weaponId, float zoomInput)
{
	SwitchScopeState(weaponId, gameState->IsAiming);


	auto scopeMagnification = ue::call<float>(gameState->Pawn, L"GetScopeMagnification");
	float scopeFov = scopeMagnification == 3.0f ? 10.0f : 3.0f;
	scopeFov = weaponId == EGsrEquipId::WP_Rpg ? 16.0f : scopeFov;
	ue::set(SceneCaptureComponent, L"FOVAngle", scopeFov);
	//Toggle zoom :
	if (zoomInput > 0.0f && scopeMagnification == 3.0f)
	{
		if (weaponId == EGsrEquipId::WP_Dragnov)
		{
			ue::call(SourceDragnovScopeWidget, L"SightZoomIn");
			ue::call(SourceDragnovScopeWidget, L"SetCamZoomToggle");
		}
		if (weaponId == EGsrEquipId::WP_MosinNagant)
		{
			ue::call(SourceMosinScopeWidget, L"SightZoomIn");
			ue::call(SourceMosinScopeWidget, L"SetCamZoomToggle");
		}
	}
	if (zoomInput < 0.0f && scopeMagnification == 10.0f)
	{
		if (weaponId == EGsrEquipId::WP_Dragnov)
		{
			ue::call(SourceDragnovScopeWidget, L"SightZoomOut");
			ue::call(SourceDragnovScopeWidget, L"SetCamZoomToggle");
		}
		if (weaponId == EGsrEquipId::WP_MosinNagant)
		{
			ue::call(SourceMosinScopeWidget, L"SightZoomOut");
			ue::call(SourceMosinScopeWidget, L"SetCamZoomToggle");
		}
	}
}

void ScopeSystem::SwitchScopeState(ue::EGsrEquipId weaponId, bool enable)
{
	// Hiding the capture isn't guaranteed to stop it from rendering the scene every frame
	if (SceneCaptureComponent) ue::set(SceneCaptureComponent, L"bCaptureEveryFrame", enable);
	if (SceneCaptureComponent) ue::call(SceneCaptureComponent, L"SetVisibility", enable, false);
	if (SceneCaptureUStaticMeshComponent) ue::call(SceneCaptureUStaticMeshComponent, L"SetVisibility", enable, false);
	if (ScopeStaticMeshComponent) ue::call(ScopeStaticMeshComponent, L"SetVisibility", enable, false);

	// Off: every sight widget, weaponId may already be the next weapon (no scope, or none)
	if (!enable)
	{
		if (SourceDragnovScopeWidget) hideWidget(SourceDragnovScopeWidget);
		if (SourceMosinScopeWidget) hideWidget(SourceMosinScopeWidget);
		if (SourceRpgScopeWidget) hideWidget(SourceRpgScopeWidget);
		return;
	}

	const auto visibility = ue::ESlateVisibility::Visible;
	switch (weaponId)
	{
		case EGsrEquipId::WP_Dragnov:
			if (SourceDragnovScopeWidget) ue::call(SourceDragnovScopeWidget, L"SetVisibility", visibility);
			if (SourceMosinScopeWidget) hideWidget(SourceMosinScopeWidget);
			if (SourceRpgScopeWidget) hideWidget(SourceRpgScopeWidget);
		break;
		case EGsrEquipId::WP_MosinNagant:
			if (SourceMosinScopeWidget) ue::call(SourceMosinScopeWidget, L"SetVisibility", visibility);
			if (SourceDragnovScopeWidget) hideWidget(SourceDragnovScopeWidget);
			if (SourceRpgScopeWidget) hideWidget(SourceRpgScopeWidget);
			break;
		case EGsrEquipId::WP_Rpg :
			if (SourceRpgScopeWidget) ue::call(SourceRpgScopeWidget, L"SetVisibility", visibility);
			if (SourceDragnovScopeWidget) hideWidget(SourceDragnovScopeWidget);
			if (SourceMosinScopeWidget) hideWidget(SourceMosinScopeWidget);
			break;
	}
}

void ScopeSystem::DestroyScope()
{
	if (ScopeStaticMeshComponent) ue::call(ScopeStaticMeshComponent, L"K2_DestroyComponent", ScopeStaticMeshComponent);
	if (SceneCaptureComponent) ue::call(SceneCaptureComponent, L"K2_DestroyComponent", SceneCaptureComponent);
	if (SceneCaptureUStaticMeshComponent) ue::call(SceneCaptureUStaticMeshComponent, L"K2_DestroyComponent", SceneCaptureUStaticMeshComponent);
	if (DragnovScopeWidget) ue::call(DragnovScopeWidget, L"K2_DestroyComponent", DragnovScopeWidget);
	if (MosinScopeWidget) ue::call(MosinScopeWidget, L"K2_DestroyComponent", MosinScopeWidget);
	if (RpgScopeWidget) ue::call(RpgScopeWidget, L"K2_DestroyComponent", RpgScopeWidget);
	if (ScopeActor) ue::call(ScopeActor, L"K2_DestroyActor");
	ScopeStaticMeshComponent = nullptr;
	SceneCaptureComponent = nullptr;
	SceneCaptureUStaticMeshComponent = nullptr;
	ScopeActor = nullptr;
	ScopeRenderTarget = nullptr;
	SourceDragnovScopeWidget = nullptr;
	DragnovScopeWidget = nullptr;
	SourceMosinScopeWidget = nullptr;
	MosinScopeWidget = nullptr;
	SourceRpgScopeWidget = nullptr;
	RpgScopeWidget = nullptr;
}

void ScopeSystem::spawnScope(ue::EGsrEquipId weaponId, ue::UObject* receiverMesh)
{
	auto world = ue::get_world();

	getRenderTarget(world);

	if (!ScopeRenderTarget)
	{
		Logger::DebugPrint("Failed to get render target, destroying Scope actor");
		if (ScopeActor)
			ue::call(ScopeActor, L"K2_DestroyActor");
		ScopeStaticMeshComponent = nullptr;
		SceneCaptureComponent = nullptr;
		SceneCaptureUStaticMeshComponent = nullptr;
		return;
	}

	if (!ScopeActor)
	{
		ScopeStaticMeshComponent = nullptr;
		SceneCaptureComponent = nullptr;
		SceneCaptureUStaticMeshComponent = nullptr;
		DragnovScopeWidget = nullptr;
		MosinScopeWidget = nullptr;
		RpgScopeWidget = nullptr;
		ScopeActor = Utilities::SpawnActor(UEVR_Vector3d{0.0f, 0.0f, 0.0f}, ue::call<ue::UObject*>(receiverMesh, L"GetOwner"));
	}
	if (!ScopeStaticMeshComponent)
		spawnScopeStaticMeshComponent();
	if (!SceneCaptureComponent)
		spawnSceneCaptureComponent();
	spawnScopeWidget(weaponId);
}

void ScopeSystem::getRenderTarget(uevr::API::UObject* world)
{
	ue::FLinearColor linearColor{};
	if (ScopeRenderTarget)
		Logger::DebugPrint(ScopeRenderTarget->get_full_name());
	if (!ScopeRenderTarget)
	{
		ScopeRenderTarget = ue::call_static<ue::UObject*>(ue::lib::KismetRendering, L"CreateRenderTarget2D",
			world, settingsManager->Settings.scopeResolution, settingsManager->Settings.scopeResolution,
			ue::ETextureRenderTargetFormat::RTF_RGBA16f, linearColor, false, false);
		if (ScopeRenderTarget)
			Logger::DebugPrint(L"Render target created : " + ScopeRenderTarget->get_full_name());
	}
}

void ScopeSystem::spawnScopeStaticMeshComponent()
{
	auto scopeStaticMeshComponent = ue::call<ue::UObject*>(ScopeActor, L"AddComponentByClass", StaticMeshComponent_c.get(), false, ue::FTransform{}, false);
	if (!scopeStaticMeshComponent/* || !scopeWidgetStaticMeshComponent*/)
	{
		Logger::DebugPrint("Failed to spawn scope mesh");
		return;
	}

	auto uDiskMaterial = ue::find_object(L"Material /Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial");

	if (!uDiskMaterial)
	{
		Logger::DebugPrint("Failed to find scopeDiskMaterialUObject");
		return;
	}

	ue::set(uDiskMaterial, L"TwoSided", false);
	ue::set(uDiskMaterial, L"BlendMode", ue::EBlendMode::BLEND_Opaque);
	ue::set(uDiskMaterial, L"bDisableDepthTest", true);
	ue::set(uDiskMaterial, L"ShadingModel", ue::EMaterialShadingModel::MSM_DefaultLit);


	auto cylinderMesh = ue::find_object(L"StaticMesh /Game/Art/VFX/Game_HD_VFX/Fire/Fire/TheFury_Fire/Mesh/SM_VFX_Disk20edge.SM_VFX_Disk20edge");

	if (!cylinderMesh)
	{
		Logger::DebugPrint("Can't find cylinder mesh");
		return;
	}
	ue::call(scopeStaticMeshComponent, L"SetStaticMesh", cylinderMesh);
	ue::call(scopeStaticMeshComponent, L"SetVisibility", false, false);
	ue::call(scopeStaticMeshComponent, L"SetCollisionEnabled", ue::ECollisionEnabled::NoCollision);
	ue::call(scopeStaticMeshComponent, L"SetMobility", ue::EComponentMobility::Movable);

	auto scopeDynamicMaterial = ue::call<ue::UObject*>(scopeStaticMeshComponent, L"CreateAndSetMaterialInstanceDynamicFromMaterial", 0, uDiskMaterial);
	//Applying the render texture to the material :
	ue::call(scopeDynamicMaterial, L"SetTextureParameterValue", ue::FName(L"LinearColor"), ScopeRenderTarget);

	ScopeStaticMeshComponent = scopeStaticMeshComponent;
}

void ScopeSystem::spawnSceneCaptureComponent()
{
	SceneCaptureUStaticMeshComponent = ue::call<ue::UObject*>(ScopeActor, L"AddComponentByClass", StaticMeshComponent_c.get(), false, ue::FTransform{}, false);
	SceneCaptureComponent = ue::call<ue::UObject*>(ScopeActor, L"AddComponentByClass", SceneCaptureComponent2D_c.get(), false, ue::FTransform{}, false);

	if (!SceneCaptureUStaticMeshComponent || !SceneCaptureComponent)
	{
		Logger::DebugPrint("failed to spawn scene capture mesh and component");
		return;
	}
	ue::set(SceneCaptureComponent, L"TextureTarget", ScopeRenderTarget);
	ue::set(SceneCaptureComponent, L"bCaptureOnMovement", false);
	ue::set(SceneCaptureComponent, L"bCaptureEveryFrame", false); // enabled while aiming, see SwitchScopeState
	ue::set(SceneCaptureComponent, L"bUseRayTracingIfEnabled", false);
	ue::set(SceneCaptureComponent, L"bAlwaysPersistRenderingState", true);
	const auto postProcess = ue::get_struct(SceneCaptureComponent, L"PostProcessSettings");
	postProcess.set(L"bOverride_MotionBlurAmount", true);
	postProcess.set(L"MotionBlurAmount", 0.0f); //Disable motion blur
	postProcess.set(L"bOverride_ScreenSpaceReflectionIntensity", true);
	postProcess.set(L"ScreenSpaceReflectionIntensity", 0.0f); //Disable screen space reflections
	postProcess.set(L"bOverride_AmbientOcclusionIntensity", true);
	postProcess.set(L"AmbientOcclusionIntensity", 0.0f); //Disable ambient occlusion
	postProcess.set(L"bOverride_BloomIntensity", true);
	postProcess.set(L"BloomIntensity", 0.0f);
	postProcess.set(L"bOverride_LensFlareIntensity", true);
	postProcess.set(L"LensFlareIntensity", 0.0f); //Disable lens flares
	postProcess.set(L"bOverride_VignetteIntensity", true);
	postProcess.set(L"VignetteIntensity", 0.0f);//Disable vignette
	// Lumen in the scope view is a second full Lumen update each frame
	postProcess.set(L"bOverride_DynamicGlobalIlluminationMethod", true);
	postProcess.set(L"DynamicGlobalIlluminationMethod", ue::EDynamicGlobalIlluminationMethod::None);
	postProcess.set(L"bOverride_ReflectionMethod", true);
	postProcess.set(L"ReflectionMethod", ue::EReflectionMethod::None);
	ue::call(SceneCaptureComponent, L"SetVisibility", false, false);
	ue::call(SceneCaptureComponent, L"SetMobility", ue::EComponentMobility::Movable);

	ue::call(SceneCaptureUStaticMeshComponent, L"SetVisibility", false, false);
	ue::call(SceneCaptureUStaticMeshComponent, L"SetMobility", ue::EComponentMobility::Movable);

}

void ProcessAllChildWidgets(ue::UObject* ParentWidget, const ue::FLinearColor& NewTint)
{
	if (!ParentWidget) return;

	// Process this widget if it's an image
	if (ParentWidget->get_full_name().find(L"Image") != std::wstring::npos && ue::is_a(ParentWidget, Image_c))
	{
		setImageTint(ParentWidget, NewTint);
	}

	// Recurse into children (only panels have some)
	int ChildCount = ue::try_call<int32_t>(ParentWidget, L"GetChildrenCount");

	for (int i = 0; i < ChildCount; i++)
	{
		auto Child = ue::call<ue::UObject*>(ParentWidget, L"GetChildAt", i);
		if (Child)
		{
			ProcessAllChildWidgets(Child, NewTint);
		}
	}
}

void SetAllImageTints(ue::UObject* rootWidget, const ue::FLinearColor& newTint)
{
	if (!rootWidget) return;
	ProcessAllChildWidgets(rootWidget, newTint);
}

void ScopeSystem::spawnScopeWidget(ue::EGsrEquipId weaponId)
{
	//Fetch existing widget :
	ue::UObject* sourceWidget = nullptr;
	switch (weaponId)
	{
	case EGsrEquipId::WP_Dragnov:
	{
		if (DragnovScopeWidget) return;
		const auto widgets = CSightDragnovWidget_c.get() ? CSightDragnovWidget_c.get()->get_objects_matching() : std::vector<ue::UObject*>{};
		sourceWidget = widgets.size() > 1 ? widgets[1] : nullptr; //check
		break;
	}
	case EGsrEquipId::WP_MosinNagant:
		if (MosinScopeWidget) return;
		sourceWidget = Utilities::FirstObjectOf(CSightMosinWidget_c); //check if false works here and below
		break;

	case EGsrEquipId::WP_Rpg:
		if (RpgScopeWidget) return;
		sourceWidget = Utilities::FirstObjectOf(CSightRpg7Widget_c);
		break;
	}



	const ue::FVector2D drawSize = {2100.0f, 2100.0f}; //The original size of the widget found ingame

	if (!sourceWidget)
	{
		Logger::DebugPrint("Failed to find the existing widget");
		return;
	}

	auto widgetComponent = ue::call<ue::UObject*>(ScopeActor, L"AddComponentByClass", WidgetComponent_c.get(), false, ue::FTransform{}, false);

	if (widgetComponent == nullptr)
	{
		Logger::DebugPrint("Failed to create actor for widget creation");
		return;
	}

	ue::call(sourceWidget, L"RemoveFromViewport");
	ue::call(widgetComponent, L"SetWidget", sourceWidget);
	ue::call(widgetComponent, L"SetTwoSided", false);
	ue::call(widgetComponent, L"SetDrawSize", drawSize);
	ue::set(widgetComponent, L"BlendMode", ue::EWidgetBlendMode::Transparent);

	//Set arrows to white for better visibility
	const ue::FLinearColor color = { 1.0f, 1.0f, 1.0f, 1.0f };
	ue::UObject* arrowGroup = ue::get<ue::UObject*>(sourceWidget, L"ArrowGroup");

	// Every sight widget has these three boxes
	hideWidget(ue::get<ue::UObject*>(sourceWidget, L"BoxBg"));
	hideWidget(ue::get<ue::UObject*>(sourceWidget, L"RightBox"));
	hideWidget(ue::get<ue::UObject*>(sourceWidget, L"LeftBox"));

	switch (weaponId)
	{
	case EGsrEquipId::WP_Dragnov:
	{
		SourceDragnovScopeWidget = sourceWidget;
		DragnovScopeWidget = widgetComponent;
		break;
	}
	case EGsrEquipId::WP_MosinNagant:
	{
		const ue::FLinearColor mosinColor = { 1.0f, 1.0f, 1.0f, 0.0f };
		auto root = ue::get<ue::UObject*>(ue::get<ue::UObject*>(sourceWidget, L"WidgetTree"), L"RootWidget");
		auto ImageWidget = ue::call<ue::UObject*>(root, L"GetChildAt", 0);
		if (ImageWidget)
		{
			Logger::DebugPrint(ImageWidget->get_full_name());
			setImageTint(ImageWidget, mosinColor);
		}
		SourceMosinScopeWidget = sourceWidget;
		MosinScopeWidget = widgetComponent;
		break;
	}
	case EGsrEquipId::WP_Rpg:
	{
		SourceRpgScopeWidget = sourceWidget;
		RpgScopeWidget = widgetComponent;
		break;
	}
	}

	SetAllImageTints(arrowGroup, color);


}

void ScopeSystem::attachScopeToWeapon(ue::EGsrEquipId weaponId, ue::UObject* receiverMesh)
{
	if (!receiverMesh){	Logger::DebugPrint("attachScopeToWeapon() : receiverMesh not found"); return; }
	if (!SceneCaptureComponent){	Logger::DebugPrint("attachScopeToWeapon() : SceneCaptureComponent not found"); return; }
	if (!SceneCaptureUStaticMeshComponent){	Logger::DebugPrint("attachScopeToWeapon() : SceneCaptureComponentMesh not found"); return; }
	if (!ScopeStaticMeshComponent){	Logger::DebugPrint("attachScopeToWeapon() : ScopeCylinderComponent not found"); return; }

	Logger::DebugPrint("Attaching scope to weapon");
	ue::call(SceneCaptureUStaticMeshComponent, L"K2_AttachToComponent",
		receiverMesh,
		ue::FName(L"CNP_MUZZLE_FLASH"),
		ue::EAttachmentRule::SnapToTarget,
		ue::EAttachmentRule::SnapToTarget,
		ue::EAttachmentRule::KeepRelative,
		true
	);

	ue::call(SceneCaptureComponent, L"K2_AttachToComponent",
		SceneCaptureUStaticMeshComponent,
		ue::FName(L"None"),
		ue::EAttachmentRule::SnapToTarget,
		ue::EAttachmentRule::SnapToTarget,
		ue::EAttachmentRule::KeepRelative,
		true
	);

	ue::call(ScopeStaticMeshComponent, L"K2_AttachToComponent",
		receiverMesh,
		ue::FName(L"CNP_REAR_SIGHT"),
		ue::EAttachmentRule::SnapToTarget,
		ue::EAttachmentRule::SnapToTarget,
		ue::EAttachmentRule::KeepRelative,
		true
	);

	ue::FHitResult hitResult;
	ue::call(SceneCaptureComponent, L"SetVisibility", true, true);
	ue::call(SceneCaptureComponent, L"SetMobility", ue::EComponentMobility::Movable);
	ue::call(SceneCaptureComponent, L"Activate", true);
	ue::call(SceneCaptureComponent, L"K2_SetRelativeRotation", ue::FRotator{0.0f, 90.0f, 0.0f}, false, &hitResult, true);

	// Scope widget component, its socket offsets and scales per weapon
	bool isScopedWeapon = true;
	ue::UObject* scopeWidget = nullptr;
	ue::FVector captureMeshLocation{};
	ue::FVector scopeMeshLocation{};
	double scopeMeshScale = 1.0;
	ue::FVector widgetLocation{};
	double widgetScale = 1.0;
	switch (weaponId)
	{
	case EGsrEquipId::WP_Dragnov:
		scopeWidget = DragnovScopeWidget;
		captureMeshLocation = { 0.0f, 0.0f, 0.0f };
		scopeMeshLocation = { 0.0f, -30.9, 0.0f };
		scopeMeshScale = 0.15f;
		widgetLocation = { 0.0f, 0.0f, 0.05f };
		widgetScale = 0.025f;
		break;
	case EGsrEquipId::WP_MosinNagant:
		scopeWidget = MosinScopeWidget;
		captureMeshLocation = { 0.0f, 0.0f, 0.0f };
		scopeMeshLocation = { 0.0f, -18.5f, 0.0f };
		scopeMeshScale = 0.12f;
		widgetLocation = { 0.0f, 0.0f, 0.05f };
		widgetScale = 0.023f;
		break;
	case EGsrEquipId::WP_Rpg:
		scopeWidget = RpgScopeWidget;
		captureMeshLocation = { 0.0f, 26.0f, 0.0f };
		scopeMeshLocation = { 0.0f, 1.33f, 0.0f };
		scopeMeshScale = 0.185f;
		widgetLocation = { 0.0f, 0.0f, 0.1f };
		widgetScale = 0.012f;
		break;
	default:
		isScopedWeapon = false;
		break;
	}

	if (isScopedWeapon)
	{
		ue::call(scopeWidget, L"K2_AttachToComponent",
			ScopeStaticMeshComponent,
			ue::FName(L"None"),
			ue::EAttachmentRule::SnapToTarget,
			ue::EAttachmentRule::SnapToTarget,
			ue::EAttachmentRule::KeepRelative,
			true
		);
		ue::call(SceneCaptureUStaticMeshComponent, L"K2_SetRelativeLocation", captureMeshLocation, false, &hitResult, true);
		ue::call(ScopeStaticMeshComponent, L"K2_SetRelativeLocation", scopeMeshLocation, false, &hitResult, true);
		ue::call(ScopeStaticMeshComponent, L"K2_SetRelativeRotation", ue::FRotator{ 180.0f, 0.0f, -90.0f }, false, &hitResult, true);
		ue::call(ScopeStaticMeshComponent, L"SetRelativeScale3D", ue::FVector{ scopeMeshScale, scopeMeshScale, scopeMeshScale });
		ue::call(scopeWidget, L"K2_SetRelativeLocation", widgetLocation, false, &hitResult, true);
		ue::call(scopeWidget, L"K2_SetRelativeRotation", ue::FRotator{ 90.0f, 0.0f, -90.0f }, false, &hitResult, true);
		ue::call(scopeWidget, L"SetRelativeScale3D", ue::FVector{ widgetScale, widgetScale, widgetScale });
	}

	ue::call(ScopeStaticMeshComponent, L"SetVisibility", false, false);
}
