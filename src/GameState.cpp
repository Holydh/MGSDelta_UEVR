#include "GameState.hpp"

void GameState::Update()
{
	World = ue::get_world();
	Pawn = uevr::API::get()->get_local_pawn(0);

	static ue::SingletonRef gsrGameStateSubsystem_c{ L"Class /Script/Gsr.GsrGameStateSubsystem" };
	auto gsrGameStateSubsystem = gsrGameStateSubsystem_c.get();
	CurrentMenuType = ue::call<ue::EGsrUIMenuType>(gsrGameStateSubsystem, L"GetUIMenuType");

	bool playerControlled = ue::call<bool>(gsrGameStateSubsystem, L"CheckGameStatus", ue::EGsrGameStatus::InGame);
	// Some scripted sequences (e.g. the intro backpack drop) hide the player body, play a stand-in Snake and park
	// the hidden pawn underground while still reporting player control: hand the camera to the game meanwhile.
	// The plugin never hides the body this way (it uses SetOnlyOwnerSee); the game also hides it in first person view.
	if (playerControlled)
	{
		auto body = ue::get<ue::UObject*>(Pawn, L"SkeletalMesh_Body");
		playerControlled = ue::get<bool>(body, L"bVisible", true) ||
			ue::call<ue::EGsrPlayerCameraControllerCameraMode>(Pawn, L"GetCameraMode") == ue::EGsrPlayerCameraControllerCameraMode::Subjective;
	}
	IsPlayerControlled = playerControlled && IngameUI();

	if (IsPlayerControlled)
	{
		if (!Pawn)
			return;
		CurrentCameraMode = ue::call<ue::EGsrPlayerCameraControllerCameraMode>(Pawn, L"GetCameraMode");
		IsFirstPersonView = CurrentCameraMode == ue::EGsrPlayerCameraControllerCameraMode::Subjective;
		auto snakeStatus = ue::call<ue::UObject*>(Pawn, L"GetSnakeStatus");
		IsAiming = ue::call<ue::EAimingStatus>(snakeStatus, L"GetAimingStatus") != ue::EAimingStatus::Aiming_None;
		CurrentStance = ue::call<ue::EGsrPlayerStance>(Pawn, L"GetCurrentStance");
	}
	else
	{
		const bool cutscene = ue::call<bool>(gsrGameStateSubsystem, L"CheckGameStatus", ue::EGsrGameStatus::Cutscene);
		const bool movie = ue::call<bool>(gsrGameStateSubsystem, L"CheckGameStatus", ue::EGsrGameStatus::Movie);
		if (cutscene || movie)
			CurrentCutsceneState = cutscene ? Playing3D : Playing2D;
		else
			CurrentCutsceneState = NotPlaying;
	}
}

void GameState::EndFrame()
{
	IsPlayerControlled.Commit();
	CurrentMenuType.Commit();
	CurrentStance.Commit();
	CurrentCutsceneState.Commit();
}

bool GameState::PlayerGotControl() const
{
	return IsPlayerControlled.Rose();
}

bool GameState::PlayerLostControl() const
{
	return IsPlayerControlled.Fell();
}

bool GameState::IngameUI() const
{
	return CurrentMenuType == ue::EGsrUIMenuType::None || (int)CurrentMenuType.Get() >= 4;
}

bool GameState::CutsceneStarts() const
{
	return CurrentCutsceneState.Previous() == NotPlaying && CutscenePlaying();
}

bool GameState::CutscenePlaying() const
{
	return CurrentCutsceneState == Playing3D || CurrentCutsceneState == Playing2D;
}

bool GameState::CutsceneStops() const
{
	return CurrentCutsceneState == NotPlaying && CurrentCutsceneState.Previous() != NotPlaying;
}
