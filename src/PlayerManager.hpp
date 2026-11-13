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

	void ToggleGameHandsVisibility(bool visible);
	void ProcessGameHandsVisibility();
	void UpdateForkAndKnifeMeleeState(float delta);

	void SetPlayerStance(ue::EGsrPlayerStance NewStance);

private:
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