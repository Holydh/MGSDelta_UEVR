#include "Inputs.hpp"
#include <chrono>
#include <intrin.h>

bool Inputs::isSecondHandGripInDelay = false;
float Inputs::secondHandGripTimer = 0.0f;
float Inputs::secondHandGripDelay = 0.2f;

bool Inputs::LeftGripInputPressed = false;
bool Inputs::WasLeftGripInputPressed = false;
bool Inputs::SecondHandGripTriggeredBeforeDelay = false;

bool Inputs::IsActionActive(bool leftController, const char* action)
{
	auto vr = uevr::API::get()->param()->vr;
    UEVR_ActionHandle actionHandle = vr->get_action_handle(action);
    UEVR_InputSourceHandle source = leftController ? vr->get_left_joystick_source() : vr->get_right_joystick_source();

    if (!actionHandle || !source)
        return false;

    return vr->is_action_active(actionHandle, source);
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

// ---------------------------------------------------------------------------------------------
// Nested XInputGetState workaround
// ---------------------------------------------------------------------------------------------
// The problem:
//   Microsoft GameInput (installed by the Xbox app, some games, etc.) replaces
//   C:\Windows\System32\XInput1_3.dll with an "XInput forwarder" whose XInputGetState simply
//   calls XInput1_4.dll's XInputGetState. UEVR hooks XInputGetState in both DLLs, so a single
//   game call goes through both hooks, nested:
//
//     game -> UEVR hook (1_3) -> forwarder -> UEVR hook (1_4) -> real XInput
//                 ^ outer pass                     ^ inner pass
//
//   UEVR's VR::on_xinput_get_state runs in both passes (inner first) and rebuilds the gamepad
//   state from scratch each time. Most inputs are recomputed every pass so they survive, but
//   two are one-shot and consumed by the inner pass:
//     - short press on the menu button: START is injected for a single call (pause menu),
//     - gesture + joystick D-pad: the D-pad direction is injected once, and the stick zeroed.
//   The outer pass, which is what the game actually receives, then has neither, so the pause
//   menu never opens and the gesture D-pad acts like a normal stick move.
//
// The fix:
//   Plugin callbacks run after UEVR's VR code in each pass, so the state we see in the inner
//   pass is the complete, correct VR state. We remember it, and when the outer pass of the same
//   call arrives we copy it over the outer state.
//
//   The outer pass is recognized as: same thread, same user index, shortly after the previous
//   call, and with our callback higher on the stack (shallower) than in the previous call,
//   i.e. the previous call happened *inside* this one. Normal back-to-back calls from the game
//   have the same stack depth, so without nesting this does nothing.
//
// This can be removed once UEVR guards its XInput hooks against re-entrancy.
// ---------------------------------------------------------------------------------------------
namespace {
	struct XInputCallSnapshot {
		bool valid = false;
		uint32_t userIndex = 0;
		uintptr_t stackAddress = 0;
		std::chrono::steady_clock::time_point time{};
		uint32_t retval = 0;
		XINPUT_STATE state{};
	};

	// Per thread: nested passes always happen on the calling thread, and this avoids locking.
	thread_local XInputCallSnapshot t_previousXInputCall{};

	// The inner pass returns into the outer one within microseconds; this only guards
	// against matching an unrelated older call.
	constexpr auto kMaxNestedCallDelay = std::chrono::milliseconds(2);
}

__declspec(noinline) void Inputs::FixNestedXInputCall(uint32_t* retval, uint32_t user_index, XINPUT_STATE* state)
{
	if (retval == nullptr || state == nullptr)
		return;

	// Address of our return address = how deep on the stack this callback is running.
	// The stack grows down, so a higher address means a shallower (outer) call.
	const auto stackAddress = (uintptr_t)_AddressOfReturnAddress();
	const auto now = std::chrono::steady_clock::now();
	auto& previous = t_previousXInputCall;

	const bool isOuterPassOfNestedCall =
		previous.valid &&
		previous.userIndex == user_index &&
		stackAddress > previous.stackAddress &&
		now - previous.time <= kMaxNestedCallDelay;

	if (isOuterPassOfNestedCall)
	{
		PLUGIN_LOG_ONCE("[MGSDelta] Nested XInputGetState detected (GameInput XInput forwarder?), using the inner call state to keep pause and gesture D-pad inputs");

		// Give the game the state UEVR built in the inner pass (with START / gesture D-pad).
		*state = previous.state;
		*retval = previous.retval;

		// The pair is handled; the next game call starts fresh.
		previous.valid = false;
		return;
	}

	// Remember this call; if it turns out to be an inner pass, the outer pass will use it.
	previous.valid = true;
	previous.userIndex = user_index;
	previous.stackAddress = stackAddress;
	previous.time = now;
	previous.retval = *retval;
	previous.state = *state;
}