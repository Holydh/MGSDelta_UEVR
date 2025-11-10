local uevrUtils = require('libs/uevr_utils')
local hands = require('libs/hands')
local controllers = require('libs/controllers')
local handsCreated = false
local handsRequired = true
local isHoldingWeapon = false
local attachementName = "WP_None"

function on_level_change(level)
	controllers.createController(0)
	controllers.createController(1)
	hands.reset()

	
	local paramsFile = 'hands_parameters' -- found in the [game profile]/data directory
	local configName = 'Main' -- the name you gave your config
	local animationName = 'Shared' -- the name you gave your animation
	hands.createFromConfig(paramsFile, configName, animationName)

	if (hands.exists()) then
		uevr.api:dispatch_custom_event("HandsCreated", "1")
		handsCreated = true
	end
end

uevr.sdk.callbacks.on_pre_engine_tick(function(engine, delta)
	if hands.exists() and not handsRequired then
		hands.destroyHands()
		uevr.api:dispatch_custom_event("HandsCreated", "0")
		handsCreated = false
		return
	end

	if hands.exists() and handsRequired then
		handsCreated = true
		return
	end

	if handsRequired and not handsCreated then
		controllers.createController(0)
		controllers.createController(1)
		hands.reset()


		local paramsFile = 'hands_parameters' -- found in the [game profile]/data directory
		local configName = 'Main'      -- the name you gave your config
		local animationName = 'Shared' -- the name you gave your animation
		hands.createFromConfig(paramsFile, configName, animationName)
		if (hands.exists()) then
			uevr.api:dispatch_custom_event("HandsCreated", "1")
			-- print(hands.getHandComponent(Handed.Right).get_full_name())
			handsCreated = true
		end
	end
end)

uevr.sdk.callbacks.on_lua_event(function(event_name, event_string)
    if event_name == "PlayerTookControl" then
		if event_string =="1" then
			print("Player took Control lua")
			handsRequired = true
		else
			print("Player lost Control lua")
			handsRequired = false
		end
	end

	if event_name == "CurrentWeapon" then
		if hands.exists() then
			attachementName = event_string
			if event_string == "wp_none" then
				isHoldingWeapon = false
				hands.handleInputIndividualHand(true, false, Handed.Right, false, false)
				hands.handleInputIndividualHand(true, false, Handed.Left, false, false)
			else
				isHoldingWeapon = true
				hands.handleInputIndividualHand(false, attachementName, Handed.Right, false, false)
				if attachementName == "wp_gove" then
					hands.handleInputIndividualHand(false, attachementName, Handed.Left, false, false)
				else
					hands.handleInputIndividualHand(true, false, Handed.Left, false, false)
				end
			end
		end
	end
end)

function on_xinput_get_state(retval, user_index, state)
	if hands.exists() then
		if isHoldingWeapon then
			if attachementName == "wp_gove" then
				hands.handleInputIndividualHand(state, attachementName, Handed.Left, false, false)
			else
				hands.handleInputIndividualHand(state, false, Handed.Left, false, false)
			end
			hands.handleInputIndividualHand(state, attachementName, Handed.Right, false, false)
		else
			hands.handleInputIndividualHand(state, false, Handed.Right, false, false)
			hands.handleInputIndividualHand(state, false, Handed.Left, false, false)
		end
	end
end
