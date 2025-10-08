#pragma once

#include "uevr/API.hpp"
#include "Utilities.hpp"
#include "SDK/UMG_structs.hpp"
#include "SDK/Engine_classes.hpp"
#include "SDK/Gsr_structs.hpp"
#include "SDK/Gsr_classes.hpp"

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
	bool IsChangingStance = false;
	SDK::EGsrPlayerStance CurrentStance = SDK::EGsrPlayerStance::Stand;
	SDK::EGsrPlayerStance PreviousStance = SDK::EGsrPlayerStance::Stand;
	float ChangeStanceTime = 0.0f;

	void GetCurrentPlayerStance(float deltaTime);
	void SetIsPeeping(const SDK::FName& State);
	//EPlayerAnimState GetCurrentPlayerAnimState(const SDK::FName& State);
private:

};