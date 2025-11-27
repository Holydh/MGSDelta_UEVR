#pragma once
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
    void FetchGameState();

	SDK::EGsrGameStatus CurrentGameStatus = SDK::EGsrGameStatus::InGame;

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
	float ExtendPeepingStateTimer = 0.0f;
	void UpdatePeepingState(float deltaTime);

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
	bool HandAttachedToWeapon = false;

	void UpdateHandVisibility();
	void UpdateForkAndKnifeMeleeState(float delta);
	//EPlayerAnimState GetCurrentPlayerAnimState(const SDK::FName& State);
private:
	uevr::API::UObject* handsSkeletalMesh;
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
	float forkAndKnifeTimer = 0.0f;
	float forkAndKnifeDuration = 0.1f;
};