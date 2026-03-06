#pragma once
#include "magic_enum/magic_enum.hpp"
#include "uevr/API.hpp"
#include "Utilities.hpp"
#include "SDK/UMG_structs.hpp"
#include "SDK/Engine_classes.hpp"
#include "SDK/Gsr_structs.hpp"
#include "SDK/Gsr_classes.hpp"
#include "SDK/Ics_parameters.hpp"
#include "SDK/BP_Player_classes.hpp"
#include "SDK/HeadMountedDisplay_classes.hpp"
#include "SDK/BP_SnakeStatus_classes.hpp"


class PlayerManager {
public:
	void ConvertAllRequiredStringToFnames();
	void ToggleMeshesVisibility(bool visible);
	void ToggleReticleVisibility(bool visible);

	bool WasPlayerControlled = false;
    bool IsPlayerControlled = false;
	bool IsPlayerControlForceDisabled = false;
	bool WasPlayerControlForceDisabled = false;
	bool PlayerControlOverride = false;
    void FetchGameState();

	bool PlayerGotControl();
	bool PlayerLostControl();

	SDK::EGsrGameStatus CurrentGameStatus = SDK::EGsrGameStatus::InGame;
	SDK::EGsrUIMenuType CurrentMenuType = SDK::EGsrUIMenuType::None;
	SDK::EGsrUIMenuType PreviousMenuType = SDK::EGsrUIMenuType::None;
	bool IngameUI();

	enum EPlayerAnimState
	{
		Default,
		Stand2Squat,
		Squat2Crawl,
		Crawl2Squat,
		Squat2Stand,
		StanceChange,
		//CrawlIdle,
		PeepStart,
		PeepLoop,
		PeepEnd
	};
	EPlayerAnimState CurrentAnimState = Default;

	bool IsPeepingInternal = false;
	bool WasPeepingInternal = false;
	bool IsPeeping = false;
	bool WasPeeping = false;
	float ExtendPeepingStateTimer = 0.0f;
	void UpdatePeepingAndCqcGrabState(float deltaTime);

	bool IsHanging = false;
	bool IsChangingStance = false;
	bool IsRolling = false;
	bool IsAiming = false;
	bool WasAiming = false;
	bool IsFirstPersonView = false;
	bool WasFirstPersonView = false;
	bool MeleeAttackRunning = false;
	bool WasMeleeAttackRunning = false;
	bool IsKnifeOrFork = false;
	bool IsCqcGrab = false;
	bool WasCqcGrab = false;
	bool IsCarrying = false;
	bool WasCarrying = false;

	SDK::EGsrPlayerStance CurrentStance = SDK::EGsrPlayerStance::Stand;
	SDK::EGsrPlayerStance PreviousStance = SDK::EGsrPlayerStance::Stand;
	float ChangeStanceTimer = 0.0f;

	SDK::Params::IcsAnimInstance_OnEnteredState debugParamsTest;

	void GetCurrentPlayerStance(float deltaTime);
	void ProcessPlayerCharacterState(const SDK::FName& StateName, const SDK::FName& StateMachineName);
	void GetCurrentAimStateAndCameraMode();

	SDK::EGsrPlayerCameraControllerCameraMode CurrentCameraMode = SDK::EGsrPlayerCameraControllerCameraMode::Around;
	SDK::EGsrPlayerCameraControllerCameraMode PreviousCameraMode = SDK::EGsrPlayerCameraControllerCameraMode::Around;
	SDK::EAimingStatus CurrentAimingStatus = SDK::EAimingStatus::Aiming_None;

	SDK::UMotionControllerComponent* LeftController;
	SDK::UPoseableMeshComponent* LeftHandMesh;
	SDK::UMotionControllerComponent* RightController;
	SDK::UPoseableMeshComponent* RightHandMesh;

	void FetchMotionControllers();
	bool MainHandAttachedToWeapon = false;
	bool SecondHandAttachedToWeapon = false;
	bool WasSecondHandAttachedToWeapon = false;

	void ToggleVRHandsVisibility(bool enable);
	void ProcessVRHandsVisibility();
	void UpdateForkAndKnifeMeleeState(float delta);

	void SetPlayerStance(SDK::EGsrPlayerStance NewStance);
	void Cleanup();
	//EPlayerAnimState GetCurrentPlayerAnimState(const SDK::FName& State);
private:
	//Peep State Machine Name
	SDK::FName fNameBehind;
	//Peep States
	SDK::FName fNamePeepStart;
	SDK::FName fNamePeepLoop;
	SDK::FName fNamePeepEnd;
	SDK::FName fNameAimStart;
	SDK::FName fNameAimLoop;
	//Rolling State Machine Name
	SDK::FName fNameBasic;
	//Rolling States
	SDK::FName fNameRolling;
	//Falling from edge State Machine Name
	SDK::FName fNameElude;
	//Melee attack
	SDK::FName fNameEnemyCqc;
	SDK::FName fNameCombo;
	SDK::FName fNameCombo1;
	SDK::FName fNameCombo2;
	SDK::FName fNameCombo3;
	SDK::FName fNameStab1;
	SDK::FName fNameStab2;
	//Fork and knife StateMachine Name
	SDK::FName fNameMain;
	//Fork and knife not attacking state
	SDK::FName fNameHold;
	//Carry State Machine Name
	SDK::FName fNameCarry;
	float forkAndKnifeTimer = 0.0f;
	float forkAndKnifeDuration = 0.1f;
};