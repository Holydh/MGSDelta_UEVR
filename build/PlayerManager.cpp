#include "PlayerManager.hpp"

void PlayerManager::ToggleMeshesVisibility(bool visible)
{
	auto pawn = uevr::API::get()->get_local_pawn(0);
	auto headSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Head");
	auto hairSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Hair");
	auto handsSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Hand");
	auto armsSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Arm");
	auto bodySkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Body");
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
	isPlayerControlled = playerControlled;
	//Logger::DebugPrint("IsPlayerControlled: %d", singleBoolParams);
}

void PlayerManager::GetCurrentPlayerStance(float deltaTime)
{
	SDK::UWorld* World = SDK::UWorld::GetWorld();
	SDK::AGsrPlayer* gsrPlayer = (SDK::AGsrPlayer*)World->OwningGameInstance->LocalPlayers[0]->PlayerController->AcknowledgedPawn;
	CurrentStance = gsrPlayer->GetCurrentStance();
	if (CurrentStance != PreviousStance)
	{
		ChangeStanceTime = 0.0f;
		IsChangingStance = true;
	}

	if (IsChangingStance)
	{
		ChangeStanceTime += deltaTime;
		if (ChangeStanceTime >= 1.0f) //assuming the stance change animation takes 0.5 seconds
		{
			IsChangingStance = false;
		}
	}
	//Logger::DebugPrint("Current Stance: %d", (int)CurrentStance); //0=Stand, 1=Squat, 2=Crawl
}

void PlayerManager::SetIsPeeping(const SDK::FName& State)
{
	static const SDK::FName PeepStart = SDK::UKismetStringLibrary::Conv_StringToName(L"PeepStart");
    static const SDK::FName PeepLoop = SDK::UKismetStringLibrary::Conv_StringToName(L"PeepLoop");
    static const SDK::FName PeepEnd = SDK::UKismetStringLibrary::Conv_StringToName(L"PeepEnd");

	if (State == PeepStart || State == PeepLoop || State == PeepEnd)
		IsPeeping = true;
	else
		IsPeeping = false;
}

//PlayerManager::EPlayerAnimState PlayerManager::GetCurrentPlayerAnimState(const SDK::FName& State)
//{
//
//}
