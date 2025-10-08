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

	bool wasPlayerControlled = false;
    bool isPlayerControlled = false;
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

	bool IsPeeping = false;
	bool IsHanging = false;
	bool IsChangingStance = false;
	SDK::EGsrPlayerStance CurrentStance = SDK::EGsrPlayerStance::Stand;
	SDK::EGsrPlayerStance PreviousStance = SDK::EGsrPlayerStance::Stand;
	float ChangeStanceTime = 0.0f;

	/*SDK::Params::IcsAnimInstance_OnEnteredState debugParamsTest;*/

	void GetCurrentPlayerStance(float deltaTime);
	void SetIsPeaking(const SDK::FName& StateName, const SDK::FName& StateMachineName);
	//EPlayerAnimState GetCurrentPlayerAnimState(const SDK::FName& State);
private:

};