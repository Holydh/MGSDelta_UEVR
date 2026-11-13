#include "Locomotion.hpp"
#include "Utilities.hpp"
#include "Logger.hpp"

void Locomotion::FindBasicAction(float deltaTime)
{
	auto pawn = gameState->Pawn;
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

void Locomotion::UpdateCompactCollision(bool enabled, float deltaTime)
{
	// Blocked: the capsule moved much slower than the velocity we gave it last frame. With root
	// motion the movement component keeps reporting the requested velocity, so the actual
	// displacement is measured instead.
	constexpr double minRequestedSpeed = 50.0; // cm/s
	constexpr double blockedSpeedRatio = 0.5;
	// Long enough that walking up to something and stopping against it stays compact
	constexpr float blockedDelay = 0.5f;       // s pushing before the body collision comes back
	constexpr float fullBodyHoldTime = 1.0f;   // s moving freely before going compact again

	auto pawn = gameState->Pawn;
	const auto location = ue::call<ue::FVector>(pawn, L"K2_GetActorLocation");
	const double movedX = location.X - lastPawnLocation.X;
	const double movedY = location.Y - lastPawnLocation.Y;
	const double actualSpeed = lastDeltaTime > 0.0f ? std::sqrt(movedX * movedX + movedY * movedY) / lastDeltaTime : 0.0;
	const bool blocked = lastRequestedSpeed > minRequestedSpeed && actualSpeed < lastRequestedSpeed * blockedSpeedRatio;

	// Prone always stays compact, even against walls
	const bool prone = gameState->CurrentStance == ue::EGsrPlayerStance::Crawl;
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
	const bool compact = enabled && !gameState->IsFirstPersonView && (!prone || ue::call<bool>(BasicAction, L"IsActive"));
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

bool Locomotion::FilterWallCollision(ue::UObject* character, bool enable)
{
	if (!compactCollisionApplied || character != uevr::API::get()->get_local_pawn(0))
		return enable;

	// Kept for when the game's collision comes back
	wallCollisionWantedByGame = enable;
	return false;
}

void Locomotion::UpdateLinearLocomotion(bool enabled, bool gameTakesControl, double viewYaw, float deltaTime)
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

	if (!enabled || gameState->CurrentMenuType != ue::EGsrUIMenuType::None)
		return;

	// Wall-hugging, climbing, rolling... are other action modules, and falling, swimming or ladders
	// aren't ground movement: the game keeps its own motion there. The game also walks Snake itself
	// through the area transitions and other scripted moves, unless the player disabled it.
	auto pawn = gameState->Pawn;
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
	switch (gameState->CurrentStance)
	{
	case ue::EGsrPlayerStance::Stand: maxSpeed = gameState->IsFirstPersonView ? 121.0 : stalking ? 70.0 : 390.0; break;
	case ue::EGsrPlayerStance::Squat: maxSpeed = gameState->IsFirstPersonView ? 72.0 : stalking ? 70.0 : 320.0; break;
	case ue::EGsrPlayerStance::Crawl: maxSpeed = gameState->IsFirstPersonView ? 110.0 : stalking ? 70.0 : 122.0; break;
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

bool Locomotion::GameDrivesMoveInput(ue::UObject* pawn, float deltaTime)
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

void Locomotion::LogCollisionAndRotation(double viewYaw)
{
	auto pawn = gameState->Pawn;
	if (!pawn)
		return;

	auto capsule = ue::get<ue::UObject*>(pawn, L"CapsuleComponent");
	Logger::DebugPrint("[Collision] camera mode %d, stance %d | capsule radius %.1f, half height %.1f | actor yaw %.1f, view yaw %.1f",
		(int)gameState->CurrentCameraMode, (int)gameState->CurrentStance.Get(),
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
