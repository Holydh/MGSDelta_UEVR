#pragma once
#include "magic_enum/magic_enum.hpp"
#include "uevr/API.hpp"
#include "Utilities.hpp"
#include "ue/Reflection.hpp"
#include "Tracked.hpp"
#include "GameState.hpp"


class PlayerManager {
private:
	const GameState* const gameState;
public:
	PlayerManager(const GameState* gs) : gameState(gs) {};
	void ConvertAllRequiredStringToFnames();
	void ToggleMeshesVisibility(bool visible);
	void ToggleReticleVisibility(bool visible);

    // Commits this frame's tracked values: call last in the frame
    void EndFrame();

	Tracked<bool> IsPeepingInternal = false;
	bool IsPeeping = false;
	float ExtendPeepingStateTimer = 0.0f;
	void UpdatePeepingAndCqcGrabState(float deltaTime);

	bool IsHanging = false;
	bool IsChangingStance = false;
	bool IsRolling = false;
	Tracked<bool> MeleeAttackRunning = false;
	bool IsKnifeOrFork = false;
	bool IsCqcGrab = false;
	Tracked<bool> IsCarrying = false;

	float ChangeStanceTimer = 0.0f;

	void UpdateStanceChange(float deltaTime);
	void ProcessPlayerCharacterState(const ue::FName& StateName, const ue::FName& StateMachineName);

	ue::WeakObject LeftController;  // MotionControllerComponent
	ue::WeakObject LeftHandMesh;    // PoseableMeshComponent
	ue::WeakObject RightController; // MotionControllerComponent
	ue::WeakObject RightHandMesh;   // PoseableMeshComponent

	void FetchMotionControllers();
	bool MainHandAttachedToWeapon = false;
	bool SecondHandAttachedToWeapon = false;

	void ToggleVRHandsVisibility(bool enable);
	void ProcessVRHandsVisibility();
	void UpdateForkAndKnifeMeleeState(float delta);

	void SetPlayerStance(ue::EGsrPlayerStance NewStance);
	void Cleanup();

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
private:
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

	//Peep State Machine Name
	ue::FName fNameBehind;
	//Peep States
	ue::FName fNamePeepStart;
	ue::FName fNamePeepLoop;
	ue::FName fNamePeepEnd;
	ue::FName fNameAimStart;
	ue::FName fNameAimLoop;
	//Rolling States
	ue::FName fNameRolling;
	//Falling from edge State Machine Name
	ue::FName fNameElude;
	//Melee attack
	ue::FName fNameEnemyCqc;
	ue::FName fNameCombo;
	ue::FName fNameCombo1;
	ue::FName fNameCombo2;
	ue::FName fNameCombo3;
	ue::FName fNameStab1;
	ue::FName fNameStab2;
	//Fork and knife StateMachine Name
	ue::FName fNameMain;
	//Fork and knife not attacking state
	ue::FName fNameHold;
	//Carry State Machine Name
	ue::FName fNameCarry;
	float forkAndKnifeTimer = 0.0f;
	float forkAndKnifeDuration = 0.1f;
};