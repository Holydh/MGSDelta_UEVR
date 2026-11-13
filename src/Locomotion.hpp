#pragma once
#include "uevr/API.hpp"
#include "ue/Reflection.hpp"
#include "GameState.hpp"

// Snake's movement and collision changes for VR
class Locomotion {
private:
	const GameState* const gameState;

	void FindBasicAction(float deltaTime);
	float basicActionSearchTimer = 0.0f;
	bool rootMotionCallbackEnabledByUs = false;
	ue::WeakObject subCollisionController; // the local player's IcsSubCollisionController
	bool compactCollisionApplied = false; // wall capsule kept off
	bool bodyCollisionOff = false;
	bool GameDrivesMoveInput(ue::UObject* pawn, float deltaTime);
	float gameInputMismatchTime = 0.0f;
	bool gameDrivingMoveInput = false;
	ue::FVector lastPawnLocation{};
	float lastDeltaTime = 0.0f;
	double lastRequestedSpeed = 0.0;
	float blockedTime = 0.0f;
	float fullBodyCollisionTimer = 0.0f;
	bool wallCollisionWantedByGame = false;

public:
	Locomotion(const GameState* gs) : gameState(gs) {};

	// Linear head-relative locomotion: replaces the root motion velocity of the basic action
	float MoveStickX = 0.0f; // left stick, -1..1, written by the XInput callback
	float MoveStickY = 0.0f;
	ue::WeakObject BasicAction; // the local player's GsrPlayerBasicAction
	bool LinearLocomotionActive = false;
	ue::FVector LinearLocomotionVelocity{};
	// gameTakesControl: the game's own motion is kept while it drives Snake (area transitions, scripted moves)
	void UpdateLinearLocomotion(bool enabled, bool gameTakesControl, double viewYaw, float deltaTime);
	// Only the root capsule collides (close to the compact collision of the game's FPV) instead of the
	// whole body and the wall capsule. Every frame; enabled = false gives the game's collision back.
	void UpdateCompactCollision(bool enabled, float deltaTime);
	// Called by the native SetEnableWallCollision hook, returns the value to apply. The game turns the
	// wall capsule back on at each state change, which pushes Snake out of walls: kept off while compact.
	bool FilterWallCollision(ue::UObject* character, bool enable);
	// Debug (F9): capsule, sub-collisions and facing, to compare TPS and FPV
	void LogCollisionAndRotation(double viewYaw);
};
