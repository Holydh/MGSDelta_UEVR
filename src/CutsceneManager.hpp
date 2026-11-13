#pragma once
#include "uevr/API.hpp"
#include "ue/Reflection.hpp"
#include "GameState.hpp"
#include "SettingsManager.hpp"

// Cutscenes outside immersive mode (black bars box around the view) and 2D movies (shown on the UI)
class CutsceneManager {
private:
	const GameState* const gameState;
	SettingsManager* const settingsManager;

	void ShowMovieOnUI();
	void HideMovieUI();
	ue::WeakObject MovieWidget;
	ue::WeakObject MovieTexture;
	ue::WeakObject MoviePlayer;
	ue::EBinkMediaPlayerBinkDrawStyle MoviePlayerDrawStyle{};

	ue::WeakObject Custom2DScreen;

public:
	CutsceneManager(const GameState* gs, SettingsManager* sm) : gameState(gs), settingsManager(sm) {};

	void ApplyCutsceneSettings();
	void SpawnCustom2DScreen();
	void ResizeBlackBars();
	void DestroyCustom2DScreen();
};
