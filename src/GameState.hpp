#pragma once
#include "uevr/API.hpp"
#include "ue/Reflection.hpp"
#include "Tracked.hpp"

// What the game is doing this frame. Read once by Update() at the start of the engine tick,
// read-only for everything else.
class GameState {
public:
	enum CutsceneState
	{
		NotPlaying,
		Playing3D,
		Playing2D
	};

	ue::UObject* World = nullptr;
	ue::UObject* Pawn = nullptr; // local player's pawn, valid until the end of the tick

	Tracked<bool> IsPlayerControlled = false;
	Tracked<ue::EGsrUIMenuType> CurrentMenuType = ue::EGsrUIMenuType::None;

	// Only read while the player has control
	Tracked<ue::EGsrPlayerStance> CurrentStance = ue::EGsrPlayerStance::Stand;
	ue::EGsrPlayerCameraControllerCameraMode CurrentCameraMode = ue::EGsrPlayerCameraControllerCameraMode::Around;
	bool IsFirstPersonView = false;
	bool IsAiming = false;

	// Only read while the player has no control
	Tracked<CutsceneState> CurrentCutsceneState = NotPlaying;

	void Update();
	// Commits this frame's tracked values: call last in the frame
	void EndFrame();

	bool PlayerGotControl() const;
	bool PlayerLostControl() const;
	bool IngameUI() const;
	bool CutsceneStarts() const;
	bool CutscenePlaying() const;
	bool CutsceneStops() const;
};
