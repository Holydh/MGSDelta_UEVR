#include "PlayerManager.hpp"

void PlayerManager::ConvertAllRequiredStringToFnames()
{
	fNameBehind = ue::FName(L"Behind");
	fNamePeepStart = ue::FName(L"PeepStart");
    fNamePeepLoop = ue::FName(L"PeepLoop");
    fNamePeepEnd = ue::FName(L"PeepEnd");
	fNameAimStart = ue::FName(L"AimStart");
	fNameAimLoop = ue::FName(L"AimLoop");
	fNameBasic = ue::FName(L"Basic");
	fNameRolling = ue::FName(L"Rolling");
	fNameElude = ue::FName(L"Elude");
	fNameCombo = ue::FName(L"Combo");
	fNameCombo1 = ue::FName(L"Combo1");
	fNameCombo2 = ue::FName(L"Combo2");
	fNameCombo3 = ue::FName(L"Combo3");
	fNameStab1 = ue::FName(L"Stab1");
	fNameStab2 = ue::FName(L"Stab2");
	fNameEnemyCqc = ue::FName(L"SM_Cqc");
	fNameMain = ue::FName(L"Main");
	fNameHold = ue::FName(L"Hold");
	fNameCarry = ue::FName(L"CARRY");
}

void PlayerManager::ToggleMeshesVisibility(bool visible)
{
	auto pawn = uevr::API::get()->get_local_pawn(0);
	if (!pawn)
		return;

	static constexpr const wchar_t* playerSkeletalMeshes[] = {
		L"SkeletalMesh_Head",
		L"SkeletalMesh_Hair",
		L"SkeletalMesh_Hand",
		L"SkeletalMesh_Arm",
		L"SkeletalMesh_Body",
		L"SkeletalMesh_Backpack",
		L"SkeletalMesh_Bandana",
		L"SkeletalMesh_Lightning_Leg",
		L"SkeletalMesh_Lightning_Arm",
		L"SkeletalMesh_BeltLink",
		L"SkeletalMesh_Glass",
		L"SkeletalMesh_WaterBottle",
		L"SkeletalMesh_KnifeScabbard",
		L"SkeletalMesh_Knife",
		L"SkeletalMesh_HeadGear",
		L"SkeletalMesh_Hat",
		L"SkeletalMesh_EyePatch",
		L"SkeletalMesh_CqcKnifeScabbard",
		L"SkeletalMesh_CqcKnife",
	};

	const bool SetOnlyOwnerSee = visible ? false : true;
	for (const auto* meshName : playerSkeletalMeshes)
	{
		if (auto mesh = ue::get<ue::UObject*>(pawn, meshName))
			ue::call(mesh, L"SetOnlyOwnerSee", SetOnlyOwnerSee);
	}

	if (auto halfMK22 = ue::get<ue::UObject*>(pawn, L"S_mk22_half"))
	{
		ue::call(halfMK22, L"SetHiddenInGame", SetOnlyOwnerSee, true);
		ue::call(halfMK22, L"SetHiddenInSceneCapture", SetOnlyOwnerSee);
	}
}

void PlayerManager::ToggleReticleVisibility(bool visible)
{
	auto bp_GsrTpsSightWidget_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Gsr.GsrTpsSightWidget");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
	//Logger::DebugPrint(bp_GsrTpsSightWidget_c->get_full_name());
	if (bp_GsrTpsSightWidget_c == nullptr)
	{
		Logger::DebugPrint("GsrTpsSightWidget not found");
		return;
	}

	auto bp_GsrTpsSightWidget = bp_GsrTpsSightWidget_c->get_objects_matching(false); //get_first_object_matching(false);
	for (size_t i = 0; i < bp_GsrTpsSightWidget.size(); i++)
	{
		//Logger::DebugPrint(bp_GsrTpsSightWidget[i]->get_full_name());
		const auto visibility = visible ? ue::ESlateVisibility::Visible : ue::ESlateVisibility::Hidden;
		ue::call(bp_GsrTpsSightWidget[i], L"SetVisibility", visibility);
	}
	Logger::DebugPrint("Crosshair visibility set to : %d", visible);
}

void PlayerManager::FetchGameState()
{
	static ue::ClassRef gsrGameStateSubsystem_c{ L"Class /Script/Gsr.GsrGameStateSubsystem" };
	auto gsrGameStateSubsystem = gsrGameStateSubsystem_c.get() ? gsrGameStateSubsystem_c.get()->get_first_object_matching(false) : nullptr;
	IsPlayerControlled = ue::call<bool>(gsrGameStateSubsystem, L"CheckGameStatus", CurrentGameStatus);

	CurrentMenuType = ue::call<ue::EGsrUIMenuType>(gsrGameStateSubsystem, L"GetUIMenuType");
	//Logger::DebugPrint("IsInRadioCall: %d, IsInSurvivalViewer: %d, IsInPauseMenu: %d", isInRadioCall, isInSurvivalViewer, isInPauseMenu);
}

bool PlayerManager::PlayerGotControl()
{
	return IsPlayerControlled && !WasPlayerControlled;
}

bool PlayerManager::PlayerLostControl()
{
	return !IsPlayerControlled && WasPlayerControlled;
}

bool PlayerManager::IngameUI()
{
	return CurrentMenuType == ue::EGsrUIMenuType::None || (int)CurrentMenuType >= 4;
}

void PlayerManager::UpdatePeepingAndCqcGrabState(float deltaTime)
{
	if (IsPeepingInternal)
		IsPeeping = true;

	//Must give some more time for the camera to come back smoothly to position
	if (!IsPeepingInternal && WasPeepingInternal)
	{
		ExtendPeepingStateTimer = 2.0f;
	}

	if (!IsPeepingInternal)
		ExtendPeepingStateTimer -= deltaTime;

	if (ExtendPeepingStateTimer <= 0.0f)
	{
		IsPeeping = false;
		ExtendPeepingStateTimer = 0.0f;
	}

	if (IsPeepingInternal != WasPeepingInternal)
	{
		uevr::API::VR::set_mod_value("VR_RoomscaleMovement", !IsPeepingInternal);
	}

	if (IsCqcGrab != WasCqcGrab)
	{
		uevr::API::VR::set_mod_value("VR_RoomscaleMovement", !IsCqcGrab);
	}
}

void PlayerManager::GetCurrentPlayerStance(float deltaTime)
{
	// World->OwningGameInstance->LocalPlayers[0]->PlayerController->AcknowledgedPawn
	auto world = ue::get<ue::UObject*>(ue::get<ue::UObject*>(uevr::API::get()->get_engine(), L"GameViewport"), L"World");
	auto localPlayers = ue::get_ptr<ue::TArray<ue::UObject*>>(ue::get<ue::UObject*>(world, L"OwningGameInstance"), L"LocalPlayers");
	if (!localPlayers || localPlayers->empty())
		return;
	auto playerController = ue::get<ue::UObject*>(localPlayers->data[0], L"PlayerController");
	auto gsrPlayer = ue::get<ue::UObject*>(playerController, L"AcknowledgedPawn");
	if (!gsrPlayer)
		return;
	CurrentStance = ue::call<ue::EGsrPlayerStance>(gsrPlayer, L"GetCurrentStance");
	if (CurrentStance != PreviousStance)
	{
		ChangeStanceTimer = 0.0f;
		IsChangingStance = true;
	}

	if (IsChangingStance)
	{
		ChangeStanceTimer += deltaTime;
		if (ChangeStanceTimer >= 0.4f) //assuming the stance change animation takes 0.5 seconds
		{
			IsChangingStance = false;
		}
	}
	//Logger::DebugPrint("Current Stance: %d", (int)CurrentStance); //0=Stand, 1=Squat, 2=Crawl
}

void PlayerManager::ProcessPlayerCharacterState(const ue::FName& StateName, const ue::FName& StateMachineName)
{
	if (StateName == fNamePeepStart || 
		StateName == fNamePeepLoop || 
		StateName == fNamePeepEnd ||
		StateName == fNameAimStart && StateMachineName == fNameBehind || 
		StateName == fNameAimLoop && StateMachineName == fNameBehind)
		IsPeepingInternal = true;
	else
		IsPeepingInternal = false;

	if (StateMachineName == fNameElude)
		IsHanging = true;
	else
		IsHanging = false;

	if (StateMachineName == fNameRolling)
		IsRolling = true;
	else
		IsRolling = false;

	if (StateMachineName == fNameCarry)
		IsCarrying = true;
	else
		IsCarrying = false;

	if (MeleeAttackRunning && IsKnifeOrFork && IsAiming)
	{
		MeleeAttackRunning = StateMachineName == fNameMain && StateName == fNameHold ? false : true;
		IsKnifeOrFork = MeleeAttackRunning ? true : false;
		//if (!MeleeAttackRunning)
		//{
		//	Logger::DebugPrint("Fork and knife melee ended");
		//}
	}
	else
	{
		MeleeAttackRunning = 
		StateMachineName == fNameCombo ||
		StateMachineName == fNameEnemyCqc && !StateName.ToWString().starts_with(L"Gun") ||
		StateName == fNameCombo1 || 
		StateName == fNameCombo2 || 
		StateName == fNameCombo3 ||
		StateName == fNameStab1 || 
		StateName == fNameStab2 ? true : false;
		//if (!MeleeAttackRunning && WasMeleeAttackRunning)
		//	Logger::DebugPrint("Melee ended");
	}
	if (MeleeAttackRunning && StateMachineName == fNameMain)
	{
		//Logger::DebugPrint("Fork and knife melee started");
		IsKnifeOrFork = true;
	}

	IsCqcGrab = StateMachineName == fNameEnemyCqc ? true : false; //need check

	
	//Logger::DebugPrint("StateMachineName: %s, StateName: %s", StateMachineName.ToString().c_str(), StateName.ToString().c_str());
}

void PlayerManager::UpdateForkAndKnifeMeleeState(float delta)
{
	//Logger::DebugPrint("CurrentAimingStatus : %d, IsAiming : %d", CurrentAimingStatus, IsAiming);
	if (MeleeAttackRunning && IsKnifeOrFork && IsFirstPersonView && !IsAiming)
	{
		forkAndKnifeTimer += delta;
		if (forkAndKnifeTimer >= forkAndKnifeDuration) //give some time to finish the attack animation
		{
			forkAndKnifeTimer = 0.0f;
			//Logger::DebugPrint("stop melee because not aiming. CurrentAimingStatus : %d, IsAiming : %d ", CurrentAimingStatus,IsAiming);
			MeleeAttackRunning = false;
			IsKnifeOrFork = false;
		}
	}
}

void PlayerManager::SetPlayerStance(ue::EGsrPlayerStance NewStance)
{
	static ue::ClassRef gsrPlayerActionCore_C{ L"Class /Script/Gsr.GsrPlayerActionCore" };
	auto gsrPlayerActionCore = gsrPlayerActionCore_C.get() ? gsrPlayerActionCore_C.get()->get_first_object_matching(false) : nullptr;
	ue::call(gsrPlayerActionCore, L"RequestChangeStance", NewStance);
}

void PlayerManager::GetCurrentAimStateAndCameraMode()
{
	auto player = uevr::API::get()->get_local_pawn(0);
	if (!player)
		return;
	CurrentCameraMode = ue::call<ue::EGsrPlayerCameraControllerCameraMode>(player, L"GetCameraMode");
	IsFirstPersonView = CurrentCameraMode == ue::EGsrPlayerCameraControllerCameraMode::Subjective;

	auto snakeStatus = ue::call<ue::UObject*>(player, L"GetSnakeStatus");
	CurrentAimingStatus = ue::call<ue::EAimingStatus>(snakeStatus, L"GetAimingStatus");
	IsAiming = CurrentAimingStatus == ue::EAimingStatus::Aiming_None ? false : true;
}

void PlayerManager::FetchMotionControllers()
{
	static auto motionControllerComponent_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/HeadMountedDisplay.MotionControllerComponent");
	auto motionControllerComponents = motionControllerComponent_c->get_objects_matching(false);
	
	for (size_t i = 0; i < motionControllerComponents.size(); i++)
	{
		auto motionControllerComponent = motionControllerComponents[i];
		auto handMesh = ue::call<ue::UObject*>(motionControllerComponent, L"GetChildComponent", 0);
		if (ue::get<ue::FName>(motionControllerComponent, L"MotionSource") == ue::FName(L"Left"))
		{
			LeftController = (SDK::UMotionControllerComponent*)motionControllerComponent;
			LeftHandMesh = (SDK::UPoseableMeshComponent*)handMesh;
			Logger::DebugPrint(L"Found Left Motion Controller: " + motionControllerComponent->get_full_name());
			if (handMesh)
				Logger::DebugPrint(L"Found Left Hand Mesh: " + handMesh->get_full_name());
		}
		else
		{
			RightController = (SDK::UMotionControllerComponent*)motionControllerComponent;
			RightHandMesh = (SDK::UPoseableMeshComponent*)handMesh;
			Logger::DebugPrint(L"Found Right Motion Controller: " + motionControllerComponent->get_full_name());
			if (handMesh)
				Logger::DebugPrint(L"Found Right Hand Mesh: " + handMesh->get_full_name());
		}
			
		
	}
}

void PlayerManager::ToggleVRHandsVisibility(bool enable)
{
	bool visibility = enable;
	auto pawn = uevr::API::get()->get_local_pawn(0);
	if (auto handsSkeletalMesh = ue::get<ue::UObject*>(pawn, L"SkeletalMesh_Hand")) {
		ue::call(handsSkeletalMesh, L"SetOnlyOwnerSee", visibility);
	}
	uevr::API::get()->dispatch_lua_event("HideHands", !visibility ? "1" : "0");
}

void PlayerManager::ProcessVRHandsVisibility()
{
	if (MeleeAttackRunning != WasMeleeAttackRunning || IsCarrying != WasCarrying)
		ToggleVRHandsVisibility(MeleeAttackRunning || IsCarrying ? false : true);
}

void PlayerManager::Cleanup()
{
	LeftHandMesh = nullptr;
	RightHandMesh = nullptr;
}