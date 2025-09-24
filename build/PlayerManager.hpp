#pragma once

#include "uevr/API.hpp"
#include "Utilities.hpp"

class PlayerManager {
public:
	void ToggleMeshesVisibility(bool visible);
	void ToggleReticleVisibility(bool visible);

	bool wasPlayerControlled = false;
    bool isPlayerControlled = false;
    void FetchGameState();
private:

};