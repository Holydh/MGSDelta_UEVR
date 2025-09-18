#include "CameraController.hpp"


void CameraController::ToggleCameraWobblingController(bool enabled) {
	//static auto pawn = uevr::API::get()->get_local_pawn(0);
	//Logger::DebugPrint("Camera Recoil : " + enabled);
	auto wobblingController_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Gsr.GsrWobblingController");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
	auto wobblingController = wobblingController_c->get_first_object_matching(false);
	//Logger::DebugPrint(wobblingController->get_full_name());
	Utilities::ParameterSingleBool singleBoolParams{
		enabled ? true : false
	};
	wobblingController->call_function(L"SetEnabled", &singleBoolParams);
}

void CameraController::ToggleMeshesVisibility(bool visible)
{
	static auto pawn = uevr::API::get()->get_local_pawn(0);
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
