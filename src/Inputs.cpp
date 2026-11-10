#include "Inputs.hpp"

bool Inputs::isSecondHandGripInDelay = false;
float Inputs::secondHandGripTimer = 0.0f;
float Inputs::secondHandGripDelay = 0.2f;

bool Inputs::LeftGripInputPressed = false;
bool Inputs::WasLeftGripInputPressed = false;
bool Inputs::SecondHandGripTriggeredBeforeDelay = false;

bool Inputs::IsActionActive(bool leftController, const char* action)
{
	auto vr = uevr::API::get()->param()->vr;
    static UEVR_ActionHandle triggerAction = vr->get_action_handle(action);
    UEVR_InputSourceHandle source = leftController ? vr->get_left_joystick_source() : vr->get_right_joystick_source();

    if (!triggerAction || !source)
        return false;

    return vr->is_action_active(triggerAction, source);
	return false;
}

void Inputs::CheckSecondHandGrip(float delta)
{
    SecondHandGripTriggeredBeforeDelay = false;
    
    if (!isSecondHandGripInDelay && LeftGripInputPressed)
    {
        isSecondHandGripInDelay = true;
        secondHandGripTimer = 0.0f;
    }

    // If grip is held, accumulate duration
    if (isSecondHandGripInDelay && LeftGripInputPressed)
    {
        secondHandGripTimer += delta;
    }

    // grip just released this frame
    if (isSecondHandGripInDelay && !LeftGripInputPressed)
    {
        isSecondHandGripInDelay = false;
		
        // Short press check
        if (secondHandGripTimer <= secondHandGripDelay)
        {
			Logger::DebugPrint("Second hand grip triggered before delay");
            SecondHandGripTriggeredBeforeDelay = true;    // only true for this frame
        }
    }
}

bool Inputs::IsKeyPressed(int vKey) 
{
	return (GetAsyncKeyState(vKey) & 0x8000) != 0;
}