#include "PlayerManager.hpp"

void PlayerManager::ConvertAllRequiredStringToFnames()
{
	fNameBehind = ue::FName(L"Behind");
	fNamePeepStart = ue::FName(L"PeepStart");
    fNamePeepLoop = ue::FName(L"PeepLoop");
    fNamePeepEnd = ue::FName(L"PeepEnd");
	fNameAimStart = ue::FName(L"AimStart");
	fNameAimLoop = ue::FName(L"AimLoop");
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

void PlayerManager::SetMeshesVisible(bool visible)
{
	auto pawn = gameState->Pawn;
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

void PlayerManager::SetReticleVisible(bool visible)
{
	auto bp_GsrTpsSightWidget_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Gsr.GsrTpsSightWidget");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
	if (bp_GsrTpsSightWidget_c == nullptr)
	{
		Logger::DebugPrint("GsrTpsSightWidget not found");
		return;
	}

	auto bp_GsrTpsSightWidget = bp_GsrTpsSightWidget_c->get_objects_matching(false); //get_first_object_matching(false);
	for (size_t i = 0; i < bp_GsrTpsSightWidget.size(); i++)
	{
		const auto visibility = visible ? ue::ESlateVisibility::Visible : ue::ESlateVisibility::Hidden;
		ue::call(bp_GsrTpsSightWidget[i], L"SetVisibility", visibility);
	}
	Logger::DebugPrint("Crosshair visibility set to : %d", visible);
}

void PlayerManager::EndFrame()
{
	IsPeepingInternal.Commit();
	MeleeAttackRunning.Commit();
	IsCarrying.Commit();
}

void PlayerManager::UpdatePeepingAndCqcGrabState(float deltaTime)
{
	if (IsPeepingInternal)
		IsPeeping = true;

	//Must give some more time for the camera to come back smoothly to position
	if (IsPeepingInternal.Fell())
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
}

void PlayerManager::UpdateStanceChange(float deltaTime)
{
	if (gameState->CurrentStance.Changed())
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

	if (MeleeAttackRunning && IsKnifeOrFork && gameState->IsAiming)
	{
		MeleeAttackRunning = StateMachineName == fNameMain && StateName == fNameHold ? false : true;
		IsKnifeOrFork = MeleeAttackRunning ? true : false;
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
	}
	if (MeleeAttackRunning && StateMachineName == fNameMain)
	{
		IsKnifeOrFork = true;
	}

	IsCqcGrab = StateMachineName == fNameEnemyCqc ? true : false; //need check

	
}

void PlayerManager::UpdateForkAndKnifeMeleeState(float delta)
{
	if (MeleeAttackRunning && IsKnifeOrFork && gameState->IsFirstPersonView && !gameState->IsAiming)
	{
		forkAndKnifeTimer += delta;
		if (forkAndKnifeTimer >= forkAndKnifeDuration) //give some time to finish the attack animation
		{
			forkAndKnifeTimer = 0.0f;
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

void PlayerManager::SetGameHandsVisible(bool visible)
{
	if (auto handsSkeletalMesh = ue::get<ue::UObject*>(gameState->Pawn, L"SkeletalMesh_Hand"))
		ue::call(handsSkeletalMesh, L"SetOnlyOwnerSee", !visible);
}

void PlayerManager::ProcessGameHandsVisibility()
{
	// The game's hands replace the VR hands (hidden meanwhile) during melee attacks and while carrying
	if (MeleeAttackRunning.Changed() || IsCarrying.Changed())
		SetGameHandsVisible(MeleeAttackRunning || IsCarrying);
}
