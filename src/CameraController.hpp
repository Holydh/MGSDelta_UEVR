#pragma once
#include "uevr/API.hpp"
#include "Utilities.hpp"
#include "Logger.hpp"
#include "magic_enum/magic_enum.hpp"
#include "ue/Reflection.hpp"

#include "PlayerManager.hpp"
#include "SettingsManager.hpp"

class CameraController {
private:
	PlayerManager* const playerManager;
	SettingsManager* const settingsManager;

	UEVR_Vector3d headColliderPosition{};
	float crawlOffset = 55.0f;
	float previousPawnPositionZ = 0.0f;

public:
	CameraController(PlayerManager* pm, SettingsManager* sm) : playerManager(pm), settingsManager(sm){};
	//virtual ~CameraController() = default;

	void ToggleCameraWobblingController(bool enabled);

	double ActualYaw = 0.0f;
	
	float YawInput = 0.0f;
	float YawProcessed = 0.0f;
	//UEVR_Vector3d playerPosition = { 0.0, 0.0, 0.0 };
	/*void SetCameraRotation();*/
	//void SetCameraPosition();
	float CameraHeightOffset = 0.0f;
	UEVR_Vector3d OriginalCameraPosition = { 0.0f, 0.0f, 0.0f };
	UEVR_Vector3d NewCameraPosition = { 0.0f, 0.0f, 0.0f };
	glm::fvec3 LastCameraPosition = { 0.0f, 0.0f, 0.0f };
	UEVR_Rotatord OriginalCameraRotation = { 0.0f, 0.0f, 0.0f };
	UEVR_Rotatord NewCameraRotation = { 0.0f, 0.0f, 0.0f };
	UEVR_Rotatord LastCameraRotation = { 0.0f, 0.0f, 0.0f };
	ue::FVector CameraForwardVector = { 1.0f, 0.0f, 0.0f };
	ue::FRotator ViewRotation{}; // rotation of the VR view rendered last frame

	ue::WeakObject CameraAttachementActor;
	bool SpawnCameraAttachementActor();
	void DestroyCameraAttachementActor();
	void AttachCameraToAttachementActor();
	void DetachCameraFromAttachementActor();
	bool IsCameraAttachedToAttachementActor = false;
	bool CameraAttachementActorAvailable = false;
	//bool CameraAttachementActorPositionSafeguard();

	void CalculateForwardOffset(const ue::FVector& location, const ue::FRotator& rotation, float fov, float& forwardOffset, glm::fvec3& forward);

	void ProcessCamera(float deltaTime);
	ue::EGsrItemId CurrentItemID = ue::EGsrItemId::IT_None;
	float InstantCameraPositionTimer = 0.25f;
	bool InstantCameraPositionNeeded = false;
	void ProcessCameraResetTimer(float delta);
	//void DisableGrassFading();

	void ProcessRoomscaleStance();

	void FixSurvivalViewerMenuRendering();
	void FixRadioMenuRendering();

	//void Apply2DScreenOverCutsceneType();


	enum CutsceneState
	{
		NotPlaying,
		Playing3D,
		Playing2D
	};

	CameraController::CutsceneState ActualCutsceneState = NotPlaying;
	CameraController::CutsceneState PreviousCutsceneState = NotPlaying;
	void CheckCutscenePlaying();
	bool CutsceneStarts();
	bool CutscenePlaying();
	bool CutsceneStops();
	void ApplyCutsceneSettings();
	void ShowMovieOnUI();
	void HideMovieUI();
	ue::WeakObject MovieWidget;
	ue::WeakObject MovieTexture;
	ue::WeakObject MoviePlayer;
	ue::EBinkMediaPlayerBinkDrawStyle MoviePlayerDrawStyle{};

	void SpawnCustom2DScreen();
	void ResizeBlackBars();
	void DestroyCustom2DScreen();
	ue::WeakObject Custom2DScreen;
	//void SetNearClipPlane(float nearClip);

	bool CardBoardEquipped = false;

	uevr::API::UObject* UIMenuBackgroundPanelSlot = nullptr;
};