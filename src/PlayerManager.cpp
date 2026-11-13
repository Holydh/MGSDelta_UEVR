#include "PlayerManager.hpp"

void PlayerManager::ConvertAllRequiredStringToFnames()
{
	fNameBehind = ue::FName(L"Behind");
	fNamePeepStart = ue::FName(L"PeepStart");
    fNamePeepLoop = ue::FName(L"PeepLoop");
    fNamePeepEnd = ue::FName(L"PeepEnd");
	fNameAimStart = ue::FName(L"AimStart");
	fNameAimLoop = ue::FName(L"AimLoop");
	fNameBasic = ue::FName(L"Basic");
	fNameRolling = ue::FName(L"Rolling");
	fNameElude = ue::FName(L"Elude");
	fNameCombo = ue::FName(L"Combo");
	fNameCombo1 = ue::FName(L"Combo1");
	fNameCombo2 = ue::FName(L"Combo2");
	fNameCombo3 = ue::FName(L"Combo3");
	fNameStab1 = ue::FName(L"Stab1");
	fNameStab2 = ue::FName(L"Stab2");
	fNameEnemyCqc = ue::FName(L"SM_Cqc");
	fNameMain = ue::FName(L"Main");
	fNameHold = ue::FName(L"Hold");
	fNameCarry = ue::FName(L"CARRY");
}

void PlayerManager::ToggleMeshesVisibility(bool visible)
{
	auto pawn = uevr::API::get()->get_local_pawn(0);
	if (!pawn)
		return;

	static constexpr const wchar_t* playerSkeletalMeshes[] = {
		L"SkeletalMesh_Head",
		L"SkeletalMesh_Hair",
		L"SkeletalMesh_Hand",
		L"SkeletalMesh_Arm",
		L"SkeletalMesh_Body",
		L"SkeletalMesh_Backpack",
		L"SkeletalMesh_Bandana",
		L"SkeletalMesh_Lightning_Leg",
		L"SkeletalMesh_Lightning_Arm",
		L"SkeletalMesh_BeltLink",
		L"SkeletalMesh_Glass",
		L"SkeletalMesh_WaterBottle",
		L"SkeletalMesh_KnifeScabbard",
		L"SkeletalMesh_Knife",
		L"SkeletalMesh_HeadGear",
		L"SkeletalMesh_Hat",
		L"SkeletalMesh_EyePatch",
		L"SkeletalMesh_CqcKnifeScabbard",
		L"SkeletalMesh_CqcKnife",
	};

	const bool SetOnlyOwnerSee = visible ? false : true;
	for (const auto* meshName : playerSkeletalMeshes)
	{
		if (auto mesh = ue::get<ue::UObject*>(pawn, meshName))
			ue::call(mesh, L"SetOnlyOwnerSee", SetOnlyOwnerSee);
	}

	if (auto halfMK22 = ue::get<ue::UObject*>(pawn, L"S_mk22_half"))
	{
		ue::call(halfMK22, L"SetHiddenInGame", SetOnlyOwnerSee, true);
		ue::call(halfMK22, L"SetHiddenInSceneCapture", SetOnlyOwnerSee);
	}
}

void PlayerManager::ToggleReticleVisibility(bool visible)
{
	auto bp_GsrTpsSightWidget_c = uevr::API::get()->find_uobject<uevr::API::UClass>(L"Class /Script/Gsr.GsrTpsSightWidget");//pawn->get_property<uevr::API::UClass*>(L"WobblingControllerClass");
	//Logger::DebugPrint(bp_GsrTpsSightWidget_c->get_full_name());
	if (bp_GsrTpsSightWidget_c == nullptr)
	{
		Logger::DebugPrint("GsrTpsSightWidget not found");
		return;
	}

	auto bp_GsrTpsSightWidget = bp_GsrTpsSightWidget_c->get_objects_matching(false); //get_first_object_matching(false);
	for (size_t i = 0; i < bp_GsrTpsSightWidget.size(); i++)
	{
		//Logger::DebugPrint(bp_GsrTpsSightWidget[i]->get_full_name());
		const auto visibility = visible ? ue::ESlateVisibility::Visible : ue::ESlateVisibility::Hidden;
		ue::call(bp_GsrTpsSightWidget[i], L"SetVisibility", visibility);
	}
	Logger::DebugPrint("Crosshair visibility set to : %d", visible);
}

void PlayerManager::FetchGameState()
{
	static ue::SingletonRef gsrGameStateSubsystem_c{ L"Class /Script/Gsr.GsrGameStateSubsystem" };
	auto gsrGameStateSubsystem = gsrGameStateSubsystem_c.get();
	IsPlayerControlled = ue::call<bool>(gsrGameStateSubsystem, L"CheckGameStatus", CurrentGameStatus);

	// Some scripted sequences (e.g. the intro backpack drop) hide the player body, play a stand-in Snake and park
	// the hidden pawn underground while still reporting player control: hand the camera to the game meanwhile.
	// The plugin never hides the body this way (it uses SetOnlyOwnerSee); the game also hides it in first person view.
	if (IsPlayerControlled)
	{
		auto pawn = uevr::API::get()->get_local_pawn(0);
		auto body = ue::get<ue::UObject*>(pawn, L"SkeletalMesh_Body");
		IsPlayerControlled = ue::get<bool>(body, L"bVisible", true) ||
			ue::call<ue::EGsrPlayerCameraControllerCameraMode>(pawn, L"GetCameraMode") == ue::EGsrPlayerCameraControllerCameraMode::Subjective;
	}

	CurrentMenuType = ue::call<ue::EGsrUIMenuType>(gsrGameStateSubsystem, L"GetUIMenuType");
	//Logger::DebugPrint("IsInRadioCall: %d, IsInSurvivalViewer: %d, IsInPauseMenu: %d", isInRadioCall, isInSurvivalViewer, isInPauseMenu);
}

bool PlayerManager::PlayerGotControl()
{
	return IsPlayerControlled && !WasPlayerControlled;
}

bool PlayerManager::PlayerLostControl()
{
	return !IsPlayerControlled && WasPlayerControlled;
}

bool PlayerManager::IngameUI()
{
	return CurrentMenuType == ue::EGsrUIMenuType::None || (int)CurrentMenuType >= 4;
}

void PlayerManager::UpdatePeepingAndCqcGrabState(float deltaTime)
{
	if (IsPeepingInternal)
		IsPeeping = true;

	//Must give some more time for the camera to come back smoothly to position
	if (!IsPeepingInternal && WasPeepingInternal)
	{
		ExtendPeepingStateTimer = 2.0f;
	}

	if (!IsPeepingInternal)
		ExtendPeepingStateTimer -= deltaTime;

	if (ExtendPeepingStateTimer <= 0.0f)
	{
		IsPeeping = false;
		ExtendPeepingStateTimer = 0.0f;
	}

	if (IsPeepingInternal != WasPeepingInternal)
	{
		uevr::API::VR::set_mod_value("VR_RoomscaleMovement", !IsPeepingInternal);
	}

	if (IsCqcGrab != WasCqcGrab)
	{
		uevr::API::VR::set_mod_value("VR_RoomscaleMovement", !IsCqcGrab);
	}
}

void PlayerManager::GetCurrentPlayerStance(float deltaTime)
{
	// World->OwningGameInstance->LocalPlayers[0]->PlayerController->AcknowledgedPawn
	auto world = ue::get<ue::UObject*>(ue::get<ue::UObject*>(uevr::API::get()->get_engine(), L"GameViewport"), L"World");
	auto localPlayers = ue::get_ptr<ue::TArray<ue::UObject*>>(ue::get<ue::UObject*>(world, L"OwningGameInstance"), L"LocalPlayers");
	if (!localPlayers || localPlayers->empty())
		return;
	auto playerController = ue::get<ue::UObject*>(localPlayers->data[0], L"PlayerController");
	auto gsrPlayer = ue::get<ue::UObject*>(playerController, L"AcknowledgedPawn");
	if (!gsrPlayer)
		return;
	CurrentStance = ue::call<ue::EGsrPlayerStance>(gsrPlayer, L"GetCurrentStance");
	if (CurrentStance != PreviousStance)
	{
		ChangeStanceTimer = 0.0f;
		IsChangingStance = true;
	}

	if (IsChangingStance)
	{
		ChangeStanceTimer += deltaTime;
		if (ChangeStanceTimer >= 0.4f) //assuming the stance change animation takes 0.5 seconds
		{
			IsChangingStance = false;
		}
	}
	//Logger::DebugPrint("Current Stance: %d", (int)CurrentStance); //0=Stand, 1=Squat, 2=Crawl
}

void PlayerManager::ProcessPlayerCharacterState(const ue::FName& StateName, const ue::FName& StateMachineName)
{
	if (StateName == fNamePeepStart || 
		StateName == fNamePeepLoop || 
		StateName == fNamePeepEnd ||
		StateName == fNameAimStart && StateMachineName == fNameBehind || 
		StateName == fNameAimLoop && StateMachineName == fNameBehind)
		IsPeepingInternal = true;
	else
		IsPeepingInternal = false;

	if (StateMachineName == fNameElude)
		IsHanging = true;
	else
		IsHanging = false;

	if (StateMachineName == fNameRolling)
		IsRolling = true;
	else
		IsRolling = false;

	if (StateMachineName == fNameCarry)
		IsCarrying = true;
	else
		IsCarrying = false;

	if (MeleeAttackRunning && IsKnifeOrFork && IsAiming)
	{
		MeleeAttackRunning = StateMachineName == fNameMain && StateName == fNameHold ? false : true;
		IsKnifeOrFork = MeleeAttackRunning ? true : false;
		//if (!MeleeAttackRunning)
		//{
		//	Logger::DebugPrint("Fork and knife melee ended");
		//}
	}
	else
	{
		MeleeAttackRunning = 
		StateMachineName == fNameCombo ||
		StateMachineName == fNameEnemyCqc && !StateName.ToWString().starts_with(L"Gun") ||
		StateName == fNameCombo1 || 
		StateName == fNameCombo2 || 
		StateName == fNameCombo3 ||
		StateName == fNameStab1 || 
		StateName == fNameStab2 ? true : false;
		//if (!MeleeAttackRunning && WasMeleeAttackRunning)
		//	Logger::DebugPrint("Melee ended");
	}
	if (MeleeAttackRunning && StateMachineName == fNameMain)
	{
		//Logger::DebugPrint("Fork and knife melee started");
		IsKnifeOrFork = true;
	}

	IsCqcGrab = StateMachineName == fNameEnemyCqc ? true : false; //need check

	
	//Logger::DebugPrint("StateMachineName: %s, StateName: %s", StateMachineName.ToString().c_str(), StateName.ToString().c_str());
}

void PlayerManager::UpdateForkAndKnifeMeleeState(float delta)
{
	//Logger::DebugPrint("CurrentAimingStatus : %d, IsAiming : %d", CurrentAimingStatus, IsAiming);
	if (MeleeAttackRunning && IsKnifeOrFork && IsFirstPersonView && !IsAiming)
	{
		forkAndKnifeTimer += delta;
		if (forkAndKnifeTimer >= forkAndKnifeDuration) //give some time to finish the attack animation
		{
			forkAndKnifeTimer = 0.0f;
			//Logger::DebugPrint("stop melee because not aiming. CurrentAimingStatus : %d, IsAiming : %d ", CurrentAimingStatus,IsAiming);
			MeleeAttackRunning = false;
			IsKnifeOrFork = false;
		}
	}
}

void PlayerManager::SetPlayerStance(ue::EGsrPlayerStance NewStance)
{
	static ue::ClassRef gsrPlayerActionCore_C{ L"Class /Script/Gsr.GsrPlayerActionCore" };
	auto gsrPlayerActionCore = gsrPlayerActionCore_C.get() ? gsrPlayerActionCore_C.get()->get_first_object_matching(false) : nullptr;
	ue::call(gsrPlayerActionCore, L"RequestChangeStance", NewStance);
}

void PlayerManager::GetCurrentAimStateAndCameraMode()
{
	auto player = uevr::API::get()->get_local_pawn(0);
	if (!player)
		return;
	CurrentCameraMode = ue::call<ue::EGsrPlayerCameraControllerCameraMode>(player, L"GetCameraMode");
	IsFirstPersonView = CurrentCameraMode == ue::EGsrPlayerCameraControllerCameraMode::Subjective;

	auto snakeStatus = ue::call<ue::UObject*>(player, L"GetSnakeStatus");
	CurrentAimingStatus = ue::call<ue::EAimingStatus>(snakeStatus, L"GetAimingStatus");
	IsAiming = CurrentAimingStatus == ue::EAimingStatus::Aiming_None ? false : true;
}

void PlayerManager::FetchMotionControllers()
{
	static ue::ClassRef motionControllerComponent_c{ L"Class /Script/HeadMountedDisplay.MotionControllerComponent" };
	static const ue::FName leftSource{ L"Left" };
	static const ue::FName rightSource{ L"Right" };
	if (!motionControllerComponent_c.get())
		return;

	LeftController = nullptr;
	LeftHandMesh = nullptr;
	RightController = nullptr;
	RightHandMesh = nullptr;
	for (auto motionControllerComponent : motionControllerComponent_c.get()->get_objects_matching(false))
	{
		// Skip the previous hands' controllers: destroyed by the Lua script, but not garbage collected yet
		if (!ue::is_valid(motionControllerComponent))
			continue;
		const auto motionSource = ue::get<ue::FName>(motionControllerComponent, L"MotionSource");
		if (motionSource != leftSource && motionSource != rightSource)
			continue;

		auto handMesh = ue::call<ue::UObject*>(motionControllerComponent, L"GetChildComponent", 0);
		if (motionSource == leftSource)
		{
			LeftController = motionControllerComponent;
			LeftHandMesh = handMesh;
			Logger::DebugPrint(L"Found Left Motion Controller: " + motionControllerComponent->get_full_name());
			if (handMesh)
				Logger::DebugPrint(L"Found Left Hand Mesh: " + handMesh->get_full_name());
		}
		else
		{
			RightController = motionControllerComponent;
			RightHandMesh = handMesh;
			Logger::DebugPrint(L"Found Right Motion Controller: " + motionControllerComponent->get_full_name());
			if (handMesh)
				Logger::DebugPrint(L"Found Right Hand Mesh: " + handMesh->get_full_name());
		}
			
		
	}
}

void PlayerManager::ToggleVRHandsVisibility(bool enable)
{
	bool visibility = enable;
	auto pawn = uevr::API::get()->get_local_pawn(0);
	if (auto handsSkeletalMesh = ue::get<ue::UObject*>(pawn, L"SkeletalMesh_Hand")) {
		ue::call(handsSkeletalMesh, L"SetOnlyOwnerSee", visibility);
	}
	uevr::API::get()->dispatch_lua_event("HideHands", !visibility ? "1" : "0");
}

void PlayerManager::ProcessVRHandsVisibility()
{
	if (MeleeAttackRunning != WasMeleeAttackRunning || IsCarrying != WasCarrying)
		ToggleVRHandsVisibility(MeleeAttackRunning || IsCarrying ? false : true);
}

void PlayerManager::FindBasicAction(float deltaTime)
{
	auto pawn = uevr::API::get()->get_local_pawn(0);
	if (BasicAction && ue::call<ue::UObject*>(BasicAction, L"GetOwnerActor") == pawn)
		return;

	BasicAction = nullptr;
	rootMotionCallbackEnabledByUs = false;
	// The engine's object list is scanned at most once per second while the action isn't found
	basicActionSearchTimer -= deltaTime;
	if (!pawn || basicActionSearchTimer > 0.0f || !Utilities::GsrPlayerBasicAction_c.get())
		return;
	basicActionSearchTimer = 1.0f;

	for (auto action : Utilities::GsrPlayerBasicAction_c.get()->get_objects_matching(false))
	{
		if (ue::is_valid(action) && ue::call<ue::UObject*>(action, L"GetOwnerActor") == pawn)
		{
			BasicAction = action;
			Logger::DebugPrint(L"Found player basic action: " + action->get_full_name());
			Logger::DebugPrint("Root motion override callback enabled by the game: %d",
				ue::call<bool>(action, L"IsSetOverrideRootMotionVelocityCallbackState"));			return;
		}
	}
}

void PlayerManager::UpdateCompactCollision(bool enabled, float deltaTime)
{
	// Blocked: the capsule moved much slower than the velocity we gave it last frame. With root
	// motion the movement component keeps reporting the requested velocity, so the actual
	// displacement is measured instead.
	constexpr double minRequestedSpeed = 50.0; // cm/s
	constexpr double blockedSpeedRatio = 0.5;
	// Long enough that walking up to something and stopping against it stays compact
	constexpr float blockedDelay = 0.5f;       // s pushing before the body collision comes back
	constexpr float fullBodyHoldTime = 1.0f;   // s moving freely before going compact again

	auto pawn = uevr::API::get()->get_local_pawn(0);
	const auto location = ue::call<ue::FVector>(pawn, L"K2_GetActorLocation");
	const double movedX = location.X - lastPawnLocation.X;
	const double movedY = location.Y - lastPawnLocation.Y;
	const double actualSpeed = lastDeltaTime > 0.0f ? std::sqrt(movedX * movedX + movedY * movedY) / lastDeltaTime : 0.0;
	const bool blocked = lastRequestedSpeed > minRequestedSpeed && actualSpeed < lastRequestedSpeed * blockedSpeedRatio;

	// Prone always stays compact, even against walls
	const bool prone = CurrentStance == ue::EGsrPlayerStance::Crawl;
	blockedTime = blocked && !prone ? blockedTime + deltaTime : 0.0f;
	if (blockedTime >= blockedDelay)
	{
		if (fullBodyCollisionTimer <= 0.0f)
			Logger::DebugPrint("Compact collision: pushing into something, body spheres needed");
		fullBodyCollisionTimer = fullBodyHoldTime;
	}
	else
		fullBodyCollisionTimer = prone ? 0.0f : std::max(0.0f, fullBodyCollisionTimer - deltaTime);

	if (!subCollisionController)
		subCollisionController = ue::call<ue::UObject*>(pawn, L"GetSubCollisionController");

	lastPawnLocation = location;
	lastDeltaTime = deltaTime;
	lastRequestedSpeed = LinearLocomotionActive ?
		std::sqrt(LinearLocomotionVelocity.X * LinearLocomotionVelocity.X + LinearLocomotionVelocity.Y * LinearLocomotionVelocity.Y) : 0.0;

	// Standing and crouched: every action (wall-hug, ladders...). Prone: only the prone locomotion,
	// the prone actions keep the game's collision. FPV sets its own compact collision.
	const bool compact = enabled && !IsFirstPersonView && (!prone || ue::call<bool>(BasicAction, L"IsActive"));
	// The body spheres come back while pushing into something (wall-hug, step over), never the
	// 40 cm wall capsule: it would push Snake away from the wall as the wall-hug starts.
	const bool bodyCompact = compact && fullBodyCollisionTimer <= 0.0f;

	// Master switch, left alone by the game: it rewrites the body spheres itself every frame
	if (bodyCompact)
		ue::call(subCollisionController, L"SetActive", false);
	else if (bodyCollisionOff)
		ue::call(subCollisionController, L"SetActive", true);
	if (bodyCompact != bodyCollisionOff)
		Logger::DebugPrint("Compact collision: body collision %s", bodyCompact ? "turned off" : "back on");
	bodyCollisionOff = bodyCompact;

	if (compact && !compactCollisionApplied)
	{
		// Before compactCollisionApplied: FilterWallCollision lets this call through
		wallCollisionWantedByGame = ue::call<bool>(pawn, L"IsEnableWallCollision");
		ue::call(pawn, L"SetEnableWallCollision", false);
		compactCollisionApplied = true;
		Logger::DebugPrint("Compact collision: wall collision turned off");
	}
	else if (!compact && compactCollisionApplied)
	{
		compactCollisionApplied = false;
		if (wallCollisionWantedByGame)
			ue::call(pawn, L"SetEnableWallCollision", true);
		Logger::DebugPrint("Compact collision: wall collision back on");
	}
}

bool PlayerManager::FilterWallCollision(ue::UObject* character, bool enable)
{
	if (!compactCollisionApplied || character != uevr::API::get()->get_local_pawn(0))
		return enable;

	// Kept for when the game's collision comes back
	wallCollisionWantedByGame = enable;
	return false;
}

void PlayerManager::UpdateLinearLocomotion(bool enabled, bool gameTakesControl, double viewYaw, float deltaTime)
{
	LinearLocomotionActive = false;
	FindBasicAction(deltaTime);
	if (!BasicAction)
		return;

	// The callback is only asked for when enabled. Never disabled if the game enabled it itself.
	const bool callbackEnabled = ue::call<bool>(BasicAction, L"IsSetOverrideRootMotionVelocityCallbackState");
	if (enabled && !callbackEnabled)
	{
		ue::call(BasicAction, L"SetOverrideRootMotionVelocityCallbackState", true);
		rootMotionCallbackEnabledByUs = true;
	}
	else if (!enabled && rootMotionCallbackEnabledByUs)
	{
		ue::call(BasicAction, L"SetOverrideRootMotionVelocityCallbackState", false);
		rootMotionCallbackEnabledByUs = false;
	}

	if (!enabled || CurrentMenuType != ue::EGsrUIMenuType::None)
		return;

	// Wall-hugging, climbing, rolling... are other action modules, and falling, swimming or ladders
	// aren't ground movement: the game keeps its own motion there. The game also walks Snake itself
	// through the area transitions and other scripted moves, unless the player disabled it.
	auto pawn = uevr::API::get()->get_local_pawn(0);
	auto movement = ue::get<ue::UObject*>(pawn, L"CharacterMovement");
	const bool gameDrivesInput = gameTakesControl && (GameDrivesMoveInput(pawn, deltaTime) ||
		ue::call<bool>(ue::get<ue::UObject*>(BasicAction, L"m_actionDesc"), L"IsForceMoveBetweenAreaState"));
	if (!ue::call<bool>(BasicAction, L"IsActive") || !ue::call<bool>(movement, L"IsMovingOnGround") || gameDrivesInput)
		return;

	// Body faces the view, like in FPV: melee, CQC and throws start from the actor's facing
	ue::call<bool>(pawn, L"K2_SetActorRotation", ue::FRotator{ 0.0, viewYaw, 0.0 }, false);

	// Top speeds of the game's own locomotion, measured in cm/s. FPV keeps its speeds while stalking.
	const bool stalking = ue::call<bool>(ue::call<ue::UObject*>(pawn, L"GetInputContainerBP"), L"IsStalking");
	double maxSpeed;
	switch (CurrentStance)
	{
	case ue::EGsrPlayerStance::Stand: maxSpeed = IsFirstPersonView ? 121.0 : stalking ? 70.0 : 390.0; break;
	case ue::EGsrPlayerStance::Squat: maxSpeed = IsFirstPersonView ? 72.0 : stalking ? 70.0 : 320.0; break;
	case ue::EGsrPlayerStance::Crawl: maxSpeed = IsFirstPersonView ? 110.0 : stalking ? 70.0 : 122.0; break;
	default: return;
	}

	constexpr double deadZone = 0.1;
	const double stickX = MoveStickX;
	const double stickY = MoveStickY;
	const double magnitude = std::sqrt(stickX * stickX + stickY * stickY);
	const double rate = magnitude > deadZone ? (std::min(magnitude, 1.0) - deadZone) / (1.0 - deadZone) : 0.0;

	// Unreal: X forward, Y right, positive yaw turns right
	const double yaw = glm::radians(viewYaw);
	const ue::FVector forward{ std::cos(yaw), std::sin(yaw), 0.0 };
	const ue::FVector right{ -std::sin(yaw), std::cos(yaw), 0.0 };
	const ue::FVector direction = rate > 0.0 ? (forward * stickY + right * stickX) / magnitude : ue::FVector{};

	LinearLocomotionVelocity = direction * (rate * maxSpeed);
	LinearLocomotionActive = true;
}

bool PlayerManager::GameDrivesMoveInput(ue::UObject* pawn, float deltaTime)
{
	// In normal play the game's move input follows the stick. When the game takes control it feeds its
	// own: Snake moves with the stick released, or keeps a direction the stick doesn't point to.
	auto input = ue::call<ue::UObject*>(pawn, L"GetInputContainerBP");
	const double stick = std::sqrt(MoveStickX * MoveStickX + MoveStickY * MoveStickY);
	const float gameRate = ue::call<float>(input, L"GetMoveSpeedRawRate");
	bool mismatch = gameRate > 0.5f && stick < 0.2;
	if (gameRate > 0.5f && stick > 0.5)
	{
		const double stickAngle = glm::degrees(std::atan2(MoveStickX, MoveStickY));
		mismatch = std::abs(std::remainder(stickAngle - ue::call<float>(input, L"GetInputDirectionDegree"), 360.0)) > 45.0;
	}

	// The game's input can lag a frame or two behind a fast stick move
	constexpr float holdTime = 0.2f;
	gameInputMismatchTime = mismatch ? gameInputMismatchTime + deltaTime : 0.0f;
	const bool drives = gameInputMismatchTime >= holdTime;
	if (drives != gameDrivingMoveInput)
		Logger::DebugPrint("Linear locomotion: %s", drives ? "the game drives Snake, its own motion kept" : "back to the player's input");
	gameDrivingMoveInput = drives;
	return drives;
}

void PlayerManager::LogCollisionAndRotation(double viewYaw)
{
	auto pawn = uevr::API::get()->get_local_pawn(0);
	if (!pawn)
		return;

	auto capsule = ue::get<ue::UObject*>(pawn, L"CapsuleComponent");
	Logger::DebugPrint("[Collision] camera mode %d, stance %d | capsule radius %.1f, half height %.1f | actor yaw %.1f, view yaw %.1f",
		(int)CurrentCameraMode, (int)CurrentStance,
		ue::call<float>(capsule, L"GetScaledCapsuleRadius"), ue::call<float>(capsule, L"GetScaledCapsuleHalfHeight"),
		ue::call<ue::FRotator>(pawn, L"K2_GetActorRotation").Yaw, viewYaw);
	Logger::DebugPrint("[Collision] wall collision enabled %d, radius %.1f, half height %.1f",
		ue::call<bool>(pawn, L"IsEnableWallCollision"), ue::call<float>(pawn, L"GetWallCollisionRadius"),
		ue::call<float>(pawn, L"GetWallCollisionHalfHeight"));

	// EGsrPlayerSubCollisionIndex
	static constexpr const char* subCollisionNames[] = {
		"Head", "RightArm", "LeftArm", "RightLeg", "LeftLeg", "RightFoot", "LeftFoot", "Root", "RightArmUpper", "LeftArmUpper"
	};
	auto subCollisions = ue::call<ue::UObject*>(pawn, L"GetSubCollisionController");
	Logger::DebugPrint("[Collision] sub-collisions active %d", ue::call<bool>(subCollisions, L"IsActive"));
	for (uint8_t i = 0; i < std::size(subCollisionNames); i++)
	{
		const auto position = ue::call<ue::FVector>(subCollisions, L"GetCalculatedLocalPosition", i);
		const auto setPosition = ue::call<ue::FVector>(subCollisions, L"GetLocalPosition", i);
		Logger::DebugPrint("[Collision]   %s: enabled %d, radius %.1f, position (%.1f, %.1f, %.1f) | set (%.1f, %.1f, %.1f), socket %s, from offset Z %.1f, adjust Z to base %d",
			subCollisionNames[i], ue::call<bool>(subCollisions, L"IsEnable", i), ue::call<float>(subCollisions, L"GetRadius", i),
			position.X, position.Y, position.Z, setPosition.X, setPosition.Y, setPosition.Z,
			ue::call<ue::FName>(subCollisions, L"GetBaseSocketName", i).ToString().c_str(),
			ue::call<float>(subCollisions, L"GetFromOffsetZ", i), ue::call<bool>(subCollisions, L"IsAdjustZToActorBase", i));
	}
}

void PlayerManager::Cleanup()
{
	// The VR hands are gone (destroyed by the Lua script, or with the level): forget them and their
	// weapon attachment, so the hands get attached again once new ones are created.
	LeftController = nullptr;
	LeftHandMesh = nullptr;
	RightController = nullptr;
	RightHandMesh = nullptr;
	MainHandAttachedToWeapon = false;
	if (SecondHandAttachedToWeapon)
		uevr::API::get()->dispatch_lua_event("TwoHandsHandling", "0");
	SecondHandAttachedToWeapon = false;
}