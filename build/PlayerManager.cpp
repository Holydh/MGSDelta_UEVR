#include "PlayerManager.hpp"

void PlayerManager::ToggleMeshesVisibility(bool visible)
{
	auto pawn = uevr::API::get()->get_local_pawn(0);
	auto headSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Head");
	auto hairSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Hair");
	auto handsSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Hand");
	auto armsSkeletalMesh = pawn->get_property<uevr::API::UObject*>(L"SkeletalMesh_Arm");
	Utilities::ParameterSingleBool singleBoolParams{
	!visible ? true : false
	};
	if (headSkeletalMesh) {
		headSkeletalMesh->call_function(L"SetOnlyOwnerSee", &singleBoolParams);
	}
	if (hairSkeletalMesh) {
		hairSkeletalMesh->call_function(L"SetOnlyOwnerSee", &singleBoolParams);
	}
	if (handsSkeletalMesh) {
		handsSkeletalMesh->call_function(L"SetOnlyOwnerSee", &singleBoolParams);
	}
	if (armsSkeletalMesh) {
		armsSkeletalMesh->call_function(L"SetOnlyOwnerSee", &singleBoolParams);
	}
}

void PlayerManager::FetchGameState()
{
	auto pawn = uevr::API::get()->get_local_pawn(0);

	Utilities::ParameterSingleBool singleBoolParams{
		false
	};
	if (pawn) {
		pawn->call_function(L"IsPlayerControlled", &singleBoolParams);
	}
	isPlayerControlled = singleBoolParams.boolValue;
	//Logger::DebugPrint("IsPlayerControlled: %d", singleBoolParams);
}
