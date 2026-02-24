#pragma once
#include "Logger.hpp"
#include "uevr/API.hpp"
#include <Windows.h>


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
};