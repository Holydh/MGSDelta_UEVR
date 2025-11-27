#include "PlayerManager.hpp"

void PlayerManager::ConvertAllRequiredStringToFnames()
{
	fNameBehind = SDK::UKismetStringLibrary::Conv_StringToName(L"Behind");
	fNamePeepStart = SDK::UKismetStringLibrary::Conv_StringToName(L"PeepStart");
    fNamePeepLoop = SDK::UKismetStringLibrary::Conv_StringToName(L"PeepLoop");
    fNamePeepEnd = SDK::UKismetStringLibrary::Conv_StringToName(L"PeepEnd");
	fNameAimStart = SDK::UKismetStringLibrary::Conv_StringToName(L"AimStart");
	fNameAimLoop = SDK::UKismetStringLibrary::Conv_StringToName(L"AimLoop");
	fNameBasic = SDK::UKismetStringLibrary::Conv_StringToName(L"Basic");
	fNameRolling = SDK::UKismetStringLibrary::Conv_StringToName(L"Rolling");
	fNameElude = SDK::UKismetStringLibrary::Conv_StringToName(L"Elude");
	fNameCombo = SDK::UKismetStringLibrary::Conv_StringToName(L"Combo");
	fNameCombo1 = SDK::UKismetStringLibrary::Conv_StringToName(L"Combo1");
	fNameCombo2 = SDK::UKismetStringLibrary::Conv_StringToName(L"Combo2");
	fNameCombo3 = SDK::UKismetStringLibrary::Conv_StringToName(L"Combo3");
	fNameStab1 = SDK::UKismetStringLibrary::Conv_StringToName(L"Stab1");
	fNameStab2 = SDK::UKismetStringLibrary::Conv_StringToName(L"Stab2");
	fNameEnemyCqc = SDK::UKismetStringLibrary::Conv_StringToName(L"SM_Cqc");
}

void PlayerManager::ToggleMeshesVisibility(bool visible)
{
	auto pawn = uevr::API::get()->get_local_pawn(0);
	auto headSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Head");
	auto hairSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Hair");
	handsSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Hand");
	auto armsSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Arm");
	auto bodySkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Body");
	auto backpackSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Backpack");
	auto bandanaSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Bandana");
	auto lightning_legSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Lightning_Leg");
	auto lightning_armSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Lightning_Arm");
	auto beltLinkSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_BeltLink");
	auto glassSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Glass");
	auto waterBottleSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_WaterBottle");
	auto knifeScabbardSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_KnifeScabbard");
	auto knifeSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Knife");
	auto headGearSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_HeadGear");
	auto hatSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Hat");
	auto eyePatchSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_EyePatch");
	auto cqcKnifeScabbardSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_CqcKnifeScabbard");
	auto cqcKnifeSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_CqcKnife");
	auto halfMK22 = pawn->get_property<uevr::API::UObject*>(L"S_mk22_half");

	bool SetOnlyOwnerSee = visible ? false : true;
	if (headSkeletalMesh) {
		headSkeletalMesh->call_function(L"SetOnlyOwnerSee", &SetOnlyOwnerSee);
	}
	if (hairSkeletalMesh) {
		hairSkeletalMesh->call_function(L"SetOnlyOwnerSee", &SetOnlyOwnerSee);
	}
	if (handsSkeletalMesh) {
		handsSkeletalMesh->call_function(L"SetOnlyOwnerSee", &SetOnlyOwnerSee);
	}
	if (armsSkeletalMesh) {
		armsSkeletalMesh->call_function(L"SetOnlyOwnerSee", &SetOnlyOwnerSee);
	}
	if (bodySkeletalMesh) {
		bodySkeletalMesh->call_function(L"SetOnlyOwnerSee", &SetOnlyOwnerSee);
	}
	if (backpackSkeletalMesh) {
		backpackSkeletalMesh->call_function(L"SetOnlyOwnerSee", &SetOnlyOwnerSee);
	}
	if (bandanaSkeletalMesh) {
		bandanaSkeletalMesh->call_function(L"SetOnlyOwnerSee", &SetOnlyOwnerSee);
	}
	if (lightning_legSkeletalMesh) {
		lightning_legSkeletalMesh->call_function(L"SetOnlyOwnerSee", &SetOnlyOwnerSee);
	}
	if (lightning_armSkeletalMesh) {
		lightning_armSkeletalMesh->call_function(L"SetOnlyOwnerSee", &SetOnlyOwnerSee);
	}
	if (beltLinkSkeletalMesh) {
		beltLinkSkeletalMesh->call_function(L"SetOnlyOwnerSee", &SetOnlyOwnerSee);
	}
	if (glassSkeletalMesh) {
		glassSkeletalMesh->call_function(L"SetOnlyOwnerSee", &SetOnlyOwnerSee);
	}
	if (waterBottleSkeletalMesh) {
		waterBottleSkeletalMesh->call_function(L"SetOnlyOwnerSee", &SetOnlyOwnerSee);
	}
	if (knifeScabbardSkeletalMesh) {
		knifeScabbardSkeletalMesh->call_function(L"SetOnlyOwnerSee", &SetOnlyOwnerSee);
	}
	if (knifeSkeletalMesh) {
		knifeSkeletalMesh->call_function(L"SetOnlyOwnerSee", &SetOnlyOwnerSee);
	}
	if (headGearSkeletalMesh) {
		headGearSkeletalMesh->call_function(L"SetOnlyOwnerSee", &SetOnlyOwnerSee);
	}
	if (hatSkeletalMesh) {
		hatSkeletalMesh->call_function(L"SetOnlyOwnerSee", &SetOnlyOwnerSee);
	}
	if (eyePatchSkeletalMesh) {
		eyePatchSkeletalMesh->call_function(L"SetOnlyOwnerSee", &SetOnlyOwnerSee);
	}
	if (cqcKnifeScabbardSkeletalMesh) {
		cqcKnifeScabbardSkeletalMesh->call_function(L"SetOnlyOwnerSee", &SetOnlyOwnerSee);
	}
	if (cqcKnifeSkeletalMesh) {
		cqcKnifeSkeletalMesh->call_function(L"SetOnlyOwnerSee", &SetOnlyOwnerSee);
	}
	if (halfMK22) {
		auto staticMeshComponent = (SDK::UStaticMeshComponent*)halfMK22;
		staticMeshComponent->SetHiddenInGame(SetOnlyOwnerSee, true);
		staticMeshComponent->SetHiddenInSceneCapture(SetOnlyOwnerSee);
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
		SDK::ESlateVisibility visibility;
		visibility = visible ? SDK::ESlateVisibility::Visible : SDK::ESlateVisibility::Hidden;

		bp_GsrTpsSightWidget[i]->call_function(L"SetVisibility", &visibility);
	}
	Logger::DebugPrint("Crosshair visibility set to : %d", visible);
}

void PlayerManager::FetchGameState()
{
	auto gsrGameStateSubsystem_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Gsr.GsrGameStateSubsystem");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
	auto gsrGameStateSubsystem = (SDK::UGsrGameStateSubsystem*)gsrGameStateSubsystem_c->get_first_object_matching(false);
	IsPlayerControlled = gsrGameStateSubsystem->CheckGameStatus(CurrentGameStatus);
}

void PlayerManager::UpdatePeepingState(float deltaTime)
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
}

void PlayerManager::GetCurrentPlayerStance(float deltaTime)
{
	SDK::UWorld* World = SDK::UWorld::GetWorld();
	SDK::AGsrPlayer* gsrPlayer = (SDK::AGsrPlayer*)World->OwningGameInstance->LocalPlayers[0]->PlayerController->AcknowledgedPawn;
	CurrentStance = gsrPlayer->GetCurrentStance();
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

void PlayerManager::ProcessPlayerCharacterState(const SDK::FName& StateName, const SDK::FName& StateMachineName)
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

	MeleeAttackRunning = 
		StateMachineName == fNameCombo ||
		StateMachineName == fNameEnemyCqc && !StateName.StartsWith("Gun") ||
		StateName == fNameCombo1 || 
		StateName == fNameCombo2 || 
		StateName == fNameCombo3 ||
		StateName == fNameStab1 || 
		StateName == fNameStab2 ? true : false;

	//Logger::DebugPrint("MeleeAttackRunning: %d", MeleeAttackRunning);
	//Logger::DebugPrint("IsRolling: %d", IsRolling);
	//Logger::DebugPrint("IsHanging: %d", IsHanging);
	//Logger::DebugPrint("IsPeepingInternal: %d", IsPeepingInternal);
	//Logger::DebugPrint("IsPeeping: %d", IsPeeping);
	
	//Logger::DebugPrint("StateMachineName: %s, StateName: %s", StateMachineName.ToString().c_str(), StateName.ToString().c_str());
}

void PlayerManager::GetCurrentAimStateAndCameraMode()
{
	auto playerController = (SDK::ABP_Player_C*)uevr::API::get()->get_local_pawn(0);
	CurrentCameraMode = playerController->GetCameraMode();
	IsFirstPersonView = CurrentCameraMode == SDK::EGsrPlayerCameraControllerCameraMode::Subjective;

	auto snakeStatus = (SDK::UBP_SnakeStatus_C*)playerController->GetSnakeStatus();
	CurrentAimingStatus = snakeStatus->GetAimingStatus();
	if (CurrentAimingStatus != SDK::EAimingStatus::Aiming_None) IsAiming = true;
	IsAiming = CurrentAimingStatus == SDK::EAimingStatus::Aiming_None ? false : true;
}

void PlayerManager::FetchMotionControllers()
{
	static auto motionControllerComponent_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/HeadMountedDisplay.MotionControllerComponent");
	auto motionControllerComponents = motionControllerComponent_c->get_objects_matching(false);
	
	for (size_t i = 0; i < motionControllerComponents.size(); i++)
	{
		auto motionControllerComponent = (SDK::UMotionControllerComponent*)motionControllerComponents[i];
		if (motionControllerComponent->MotionSource == SDK::UKismetStringLibrary::Conv_StringToName(L"Left"))
		{
			LeftController = motionControllerComponent;
			LeftHandMesh = (SDK::UPoseableMeshComponent*)motionControllerComponent->GetChildComponent(0);
			Logger::DebugPrint("Found Left Motion Controller: %s", motionControllerComponent->GetFullName().c_str());
			Logger::DebugPrint("Found Left Hand Mesh: %s", LeftHandMesh->GetFullName().c_str());
		}
		else
		{
			RightController = motionControllerComponent;
			RightHandMesh = (SDK::UPoseableMeshComponent*)motionControllerComponent->GetChildComponent(0);
			Logger::DebugPrint("Found Right Motion Controller: %s", motionControllerComponent->GetFullName().c_str());
			Logger::DebugPrint("Found Right Hand Mesh: %s", RightHandMesh->GetFullName().c_str());
		}
			
		
	}
}

void PlayerManager::UpdateHandVisibility()
{
	if (MeleeAttackRunning != WasMeleeAttackRunning)
	{
		Logger::DebugPrint("hello %d", MeleeAttackRunning);
		bool visibility = !MeleeAttackRunning;
		if (handsSkeletalMesh) {
			handsSkeletalMesh->call_function(L"SetOnlyOwnerSee", &visibility);
		}
		if (RightHandMesh)
			RightHandMesh->SetVisibility(visibility, visibility);
		if (LeftHandMesh)
			LeftHandMesh->SetVisibility(visibility, visibility);
	}
}
