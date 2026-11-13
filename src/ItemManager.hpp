#pragma once
#include "uevr/API.hpp"
#include "ue/Reflection.hpp"
#include "Tracked.hpp"
#include "GameState.hpp"
#include "SettingsManager.hpp"
#include "VRHands.hpp"

// The item equipped by Snake (goggles, stealth, scope, cardboard box...)
class ItemManager {
private:
	const GameState* const gameState;
	SettingsManager* const settingsManager;
	VRHands* const hands;

	void processItem();

	ue::WeakObject ItemController; // UGsrItemController
	ue::WeakObject CurrentItem; // UObject
	Tracked<ue::EGsrItemId> CurrentItemID = ue::EGsrItemId::IT_None;
	ue::WeakObject playerCameraManager; // BP_PlayerCameraManager_C
	// Retried every frame until the goggles' post process actor exists
	bool ToggleThermalVisionScanlineNextFrame = false;

public:
	ItemManager(const GameState* gs, SettingsManager* sm, VRHands* vh) : gameState(gs), settingsManager(sm), hands(vh) {};

	// Every frame while the player has control
	void Update();
	bool IsScopeEquipped() const;
	bool IsCardboardBoxEquipped() const;
	void ToggleThermalVisionScanline();

	// Commits this frame's tracked values: call last in the frame
	void EndFrame();
	void Cleanup();
};
