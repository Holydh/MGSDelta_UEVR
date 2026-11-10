#pragma once
#include "magic_enum/magic_enum.hpp"
#include "uevr/API.hpp"
#include "Utilities.hpp"
#include "ue/Reflection.hpp"

// TEMPORARY: the hand components below keep their SDK pointer types until WeaponManager
// (their main user) is ported. Forward declarations only, no SDK include needed here.
namespace SDK {
	class UMotionControllerComponent;
	class UPoseableMeshComponent;
}


class PlayerManager {
public:
	void ConvertAllRequiredStringToFnames();
	void ToggleMeshesVisibility(bool visible);
	void ToggleReticleVisibility(bool visible);

	bool WasPlayerControlled = false;
    bool IsPlayerControlled = false;
    void FetchGameState();

	bool PlayerGotControl();
	bool PlayerLostControl();

	ue::EGsrGameStatus CurrentGameStatus = ue::EGsrGameStatus::InGame;
	ue::EGsrUIMenuType CurrentMenuType = ue::EGsrUIMenuType::None;
	ue::EGsrUIMenuType PreviousMenuType = ue::EGsrUIMenuType::None;
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

	ue::EGsrPlayerStance CurrentStance = ue::EGsrPlayerStance::Stand;
	ue::EGsrPlayerStance PreviousStance = ue::EGsrPlayerStance::Stand;
	float ChangeStanceTimer = 0.0f;

	void GetCurrentPlayerStance(float deltaTime);
	void ProcessPlayerCharacterState(const ue::FName& StateName, const ue::FName& StateMachineName);
	void GetCurrentAimStateAndCameraMode();

	ue::EGsrPlayerCameraControllerCameraMode CurrentCameraMode = ue::EGsrPlayerCameraControllerCameraMode::Around;
	ue::EGsrPlayerCameraControllerCameraMode PreviousCameraMode = ue::EGsrPlayerCameraControllerCameraMode::Around;
	ue::EAimingStatus CurrentAimingStatus = ue::EAimingStatus::Aiming_None;

	SDK::UMotionControllerComponent* LeftController = nullptr;
	SDK::UPoseableMeshComponent* LeftHandMesh = nullptr;
	SDK::UMotionControllerComponent* RightController = nullptr;
	SDK::UPoseableMeshComponent* RightHandMesh = nullptr;

	void FetchMotionControllers();
	bool MainHandAttachedToWeapon = false;
	bool SecondHandAttachedToWeapon = false;
	bool WasSecondHandAttachedToWeapon = false;

	void ToggleVRHandsVisibility(bool enable);
	void ProcessVRHandsVisibility();
	void UpdateForkAndKnifeMeleeState(float delta);

	void SetPlayerStance(ue::EGsrPlayerStance NewStance);
	void Cleanup();
	//EPlayerAnimState GetCurrentPlayerAnimState(const ue::FName& State);
private:
	//Peep State Machine Name
	ue::FName fNameBehind;
	//Peep States
	ue::FName fNamePeepStart;
	ue::FName fNamePeepLoop;
	ue::FName fNamePeepEnd;
	ue::FName fNameAimStart;
	ue::FName fNameAimLoop;
	//Rolling State Machine Name
	ue::FName fNameBasic;
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