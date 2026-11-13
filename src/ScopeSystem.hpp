#pragma once
#include "uevr/API.hpp"
#include "ue/Reflection.hpp"
#include "GameState.hpp"
#include "SettingsManager.hpp"

// The VR scopes of the Dragunov, Mosin Nagant and RPG: a scene capture rendered on a disk at the rear sight,
// with the game's sight widget in front of it
class ScopeSystem {
private:
	const GameState* const gameState;
	SettingsManager* const settingsManager;

	void spawnScope(ue::EGsrEquipId weaponId, ue::UObject* receiverMesh);
	void getRenderTarget(uevr::API::UObject* world);
	void spawnScopeStaticMeshComponent();
	void spawnSceneCaptureComponent();
	void spawnScopeWidget(ue::EGsrEquipId weaponId);
	void attachScopeToWeapon(ue::EGsrEquipId weaponId, ue::UObject* receiverMesh);

	ue::WeakObject ScopeActor; // AActor
	ue::WeakObject SourceDragnovScopeWidget; // UUserWidget
	ue::WeakObject DragnovScopeWidget; // UWidgetComponent
	ue::WeakObject SourceMosinScopeWidget; // UUserWidget
	ue::WeakObject MosinScopeWidget; // UWidgetComponent
	ue::WeakObject SourceRpgScopeWidget; // UUserWidget
	ue::WeakObject RpgScopeWidget; // UWidgetComponent

	ue::WeakObject ScopeStaticMeshComponent; // UStaticMeshComponent
	ue::WeakObject SceneCaptureComponent; // USceneCaptureComponent2D
	ue::WeakObject SceneCaptureUStaticMeshComponent; // UStaticMeshComponent

	ue::WeakObject ScopeRenderTarget; // UTextureRenderTarget2D

public:
	ScopeSystem(const GameState* gs, SettingsManager* sm) : gameState(gs), settingsManager(sm) {};

	// receiverMesh: the weapon's skeletal mesh, the scope is attached to its sockets
	void SpawnScope(ue::EGsrEquipId weaponId, ue::UObject* weaponRoot, ue::UObject* receiverMesh);
	void DestroyScope();
	void SwitchScopeState(ue::EGsrEquipId weaponId, bool enable);
	// Every frame while a scoped weapon is held: on while aiming, field of view and zoom
	void Update(ue::EGsrEquipId weaponId, float zoomInput);
};
