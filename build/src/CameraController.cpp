#include "CameraController.hpp"


void CameraController::ToggleGameCameraController(bool enabled)
{
	auto cameraController_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Gsr.GsrPlayerCameraController");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
	auto cameraController = cameraController_c->get_first_object_matching(false);
	Utilities::ParameterSingleBool singleBoolParams{
	enabled ? true : false
	};
	cameraController->call_function(L"SetActive", &singleBoolParams);
}

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



void CameraController::SetCameraRotation(double yaw)
{
	auto cameraManager_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Engine.PlayerCameraManager");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
	auto cameraManager = cameraManager_c->get_first_object_matching(false);

	auto test = (reinterpret_cast<Utilities::FCameraCacheEntry*>(uevr::API::get()->sdk()->uobject->get_property_data(cameraManager->to_handle(), L"CameraCachePrivate")));
	if (test != NULL) //*test = 0.8f;
	{
		Logger::DebugPrint("before overwrite : %f", test->POV.Rotation.yaw);
		test->POV.Rotation.yaw = yaw;
		Logger::DebugPrint("after : %f", test->POV.Rotation.yaw);
	}
}
