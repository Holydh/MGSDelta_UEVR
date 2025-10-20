#include "Inputs.hpp"

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
