#pragma once
#include "uevr/API.hpp"
#include "ue/Reflection.hpp"

// The VR hands created by the Lua hands script, and their attachment to the held weapon
class VRHands {
private:
	bool mainHandAttachedToWeapon = false;
	bool secondHandAttachedToWeapon = false;
	// Also tells the Lua hands script (TwoHandsHandling)
	void setSecondHandAttachedToWeapon(bool attached);

public:
	ue::WeakObject LeftController;  // MotionControllerComponent
	ue::WeakObject LeftHandMesh;    // PoseableMeshComponent
	ue::WeakObject RightController; // MotionControllerComponent
	ue::WeakObject RightHandMesh;   // PoseableMeshComponent

	void FetchMotionControllers();
	// The hands are gone (destroyed by the Lua script, or with the level): forget them and their weapon attachment
	void Cleanup();

	bool MainHandAttachedToWeapon() const { return mainHandAttachedToWeapon; }
	bool SecondHandAttachedToWeapon() const { return secondHandAttachedToWeapon; }
	void AttachMainHand(ue::UObject* parent, const ue::FVector& location, const ue::FRotator& rotation);
	// Back on the controller. False if it wasn't attached.
	bool DetachMainHand();
	void AttachSecondHand(ue::UObject* parent, const ue::FVector& location, const ue::FRotator& rotation);
	bool DetachSecondHand();

	// The VR hands get the materials of Snake's hands (camouflage, stealth)
	void ApplyPawnHandMaterial();
};
