#include "PlayerManager.hpp"

void PlayerManager::ToggleMeshesVisibility(bool visible)
{
	auto pawn = uevr::API::get()->get_local_pawn(0);
	auto headSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Head");
	auto hairSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Hair");
	auto handsSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Hand");
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




	bool SetOnlyOwnerSee = !visible ? true : false;
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
	auto pawn = uevr::API::get()->get_local_pawn(0);
	bool playerControlled = {};
	if (pawn) {
		pawn->call_function(L"IsPlayerControlled", &playerControlled);
	}
	IsPlayerControlled = playerControlled;
	//Logger::DebugPrint("IsPlayerControlled: %d", singleBoolParams);
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
		if (ChangeStanceTimer >= 1.0f) //assuming the stance change animation takes 0.5 seconds
		{
			IsChangingStance = false;
		}
	}
	//Logger::DebugPrint("Current Stance: %d", (int)CurrentStance); //0=Stand, 1=Squat, 2=Crawl
}

void PlayerManager::ProcessPlayerCharacterState(const SDK::FName& StateName, const SDK::FName& StateMachineName)
{
	//Peep State Machine Name
	static const SDK::FName Behind = SDK::UKismetStringLibrary::Conv_StringToName(L"Behind");
	//Peep States
	static const SDK::FName PeepStart = SDK::UKismetStringLibrary::Conv_StringToName(L"PeepStart");
    static const SDK::FName PeepLoop = SDK::UKismetStringLibrary::Conv_StringToName(L"PeepLoop");
    static const SDK::FName PeepEnd = SDK::UKismetStringLibrary::Conv_StringToName(L"PeepEnd");
	static const SDK::FName AimStart = SDK::UKismetStringLibrary::Conv_StringToName(L"AimStart");
	static const SDK::FName AimLoop = SDK::UKismetStringLibrary::Conv_StringToName(L"AimLoop");

	//Rolling State Machine Name
	static const SDK::FName Basic = SDK::UKismetStringLibrary::Conv_StringToName(L"Basic");
	//Rolling States
	static const SDK::FName Rolling = SDK::UKismetStringLibrary::Conv_StringToName(L"Rolling");

	//Aiming States
	static const SDK::FName ParallelIdle = SDK::UKismetStringLibrary::Conv_StringToName(L"ParallelIdle");
	static const SDK::FName ParallelMoveStart = SDK::UKismetStringLibrary::Conv_StringToName(L"ParallelMoveStart");
	static const SDK::FName ParallelMoveCycle = SDK::UKismetStringLibrary::Conv_StringToName(L"ParallelMoveCycle");
	static const SDK::FName ParallelMoveStop = SDK::UKismetStringLibrary::Conv_StringToName(L"ParallelMoveStop");

	//Falling from edge State Machine Name
	static const SDK::FName Elude = SDK::UKismetStringLibrary::Conv_StringToName(L"Elude");
	

	
	if (StateName == PeepStart || 
		StateName == PeepLoop || 
		StateName == PeepEnd ||
		StateName == AimStart && StateMachineName == Behind || 
		StateName == AimLoop && StateMachineName == Behind)
		IsPeepingInternal = true;
	else
		IsPeepingInternal = false;

	if (StateMachineName == Elude)
		IsHanging = true;
	else
		IsHanging = false;

	if (StateMachineName == Rolling)
		IsRolling = true;
	else
		IsRolling = false;

	if (StateName == ParallelIdle || StateName == ParallelMoveStart || StateName == ParallelMoveCycle || StateName == ParallelMoveStop)
		IsAiming = true;
	else
		IsAiming = false;
	//Logger::DebugPrint("IsRolling: %d", IsRolling);
	//Logger::DebugPrint("IsHanging: %d", IsHanging);
	//Logger::DebugPrint("IsPeepingInternal: %d", IsPeepingInternal);
	//Logger::DebugPrint("IsPeeping: %d", IsPeeping);

	//Logger::DebugPrint("StateMachineName: %s, StateName: %s", StateMachineName.ToString().c_str(), StateName.ToString().c_str());
}

//PlayerManager::EPlayerAnimState PlayerManager::GetCurrentPlayerAnimState(const SDK::FName& State)
//{
//
//}
