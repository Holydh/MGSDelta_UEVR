#pragma once
#include "Logger.hpp"
#include "uevr/API.hpp"
#include <Windows.h>
#include <Xinput.h>
#include <cstdint>


class Inputs {
private:
	static float secondHandGripTimer;
	static float secondHandGripDelay;
	static bool isSecondHandGripInDelay;

public:
	static bool IsActionActive(bool leftController, const char* action);
	static void CheckSecondHandGrip(float delta);

	static bool LeftGripInputPressed;
	static bool WasLeftGripInputPressed;

	
	static bool SecondHandGripTriggeredBeforeDelay;

	static bool IsKeyPressed(int vKey);

	// Workaround for UEVR running its XInput processing twice per game call when the
	// Microsoft GameInput "XInput forwarder" replaces System32\XInput1_3.dll (it forwards to
	// XInput1_4.dll, and UEVR hooks both). Without it, UEVR's one-shot inputs are lost:
	// short menu press (START / pause menu) and gesture + joystick D-pad.
	// Must be called first in on_xinput_get_state. See Inputs.cpp for details.
	static void FixNestedXInputCall(uint32_t* retval, uint32_t user_index, XINPUT_STATE* state);
};