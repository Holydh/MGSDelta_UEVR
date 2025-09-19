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
