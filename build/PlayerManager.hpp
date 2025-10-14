#pragma once

#include "uevr/API.hpp"
#include "Utilities.hpp"
#include "SDK/UMG_structs.hpp"
#include "SDK/Engine_classes.hpp"
#include "SDK/Gsr_structs.hpp"
#include "SDK/Gsr_classes.hpp"
#include "SDK/Ics_parameters.hpp"

class PlayerManager {
public:
	void ToggleMeshesVisibility(bool visible);
	void ToggleReticleVisibility(bool visible);

	bool WasPlayerControlled = false;
    bool IsPlayerControlled = false;
    void FetchGameState();

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
	SDK::EGsrPlayerStance CurrentStance = SDK::EGsrPlayerStance::Stand;
	SDK::EGsrPlayerStance PreviousStance = SDK::EGsrPlayerStance::Stand;
	float ChangeStanceTimer = 0.0f;

	SDK::Params::IcsAnimInstance_OnEnteredState debugParamsTest;

	void GetCurrentPlayerStance(float deltaTime);
	void ProcessPlayerCharacterState(const SDK::FName& StateName, const SDK::FName& StateMachineName);
	//EPlayerAnimState GetCurrentPlayerAnimState(const SDK::FName& State);
private:

};