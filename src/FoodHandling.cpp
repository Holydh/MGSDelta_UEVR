#include "FoodHandling.hpp"
#include "Inputs.hpp"
#include "Logger.hpp"

namespace {
constinit ue::ClassRef StaticMeshComponent_c{ L"Class /Script/Engine.StaticMeshComponent" };
}

void FoodHandling::Update(ue::UObject* weaponRoot, ue::UObject* leftHandMesh, float cameraYawDelta, float delta)
{
	//Food floating animation
	ue::UObject* foodMesh = nullptr;
	for (auto child : ue::array_snapshot(weaponRoot, L"AttachChildren"))
	{
		if (!child) continue;
		if (ue::is_a(child, StaticMeshComponent_c)) {
			const std::wstring meshName = child->get_fname()->to_string();
			if (meshName.find(L"ThrowableStaticMesh") != std::wstring::npos)
			{
				foodMesh = child;
				break;
			}
		}
	}

	if (!foodMesh)
	{
		Logger::DebugPrint("No food mesh found");
		return;
	}

	ue::FHitResult hitResult;

	if (Inputs::LeftGripInputPressed.Fell())
	{
		// Re-sync to current world rotation
		auto loc = ue::call<ue::FVector>(foodMesh, L"K2_GetComponentLocation");
		FoodIdleStart = glm::vec3(loc.X, loc.Y, loc.Z);
		FoodIdlePosition = FoodIdleStart;

		auto rotQuat = ue::call<ue::FTransform>(foodMesh, L"K2_GetComponentToWorld").Rotation;
		auto rot = ue::call<ue::FRotator>(foodMesh, L"K2_GetComponentRotation");
		FoodCurrentLocalRotation = {(float)rotQuat.W, (float)rotQuat.X, (float)rotQuat.Y, (float)rotQuat.Z};
		CurrentFoodRotation = rot.Yaw;
	}

	if (Inputs::LeftGripInputPressed.Rose())
	{
		auto foodT = ue::call<ue::FTransform>(foodMesh, L"K2_GetComponentToWorld");
		auto handT = ue::call<ue::FTransform>(leftHandMesh, L"K2_GetComponentToWorld");

		glm::quat foodRot(foodT.Rotation.W, foodT.Rotation.X, foodT.Rotation.Y, foodT.Rotation.Z);
		glm::quat handRot(handT.Rotation.W, handT.Rotation.X, handT.Rotation.Y, handT.Rotation.Z);

		glm::vec3 foodPos(foodT.Translation.X, foodT.Translation.Y, foodT.Translation.Z);
		glm::vec3 handPos(handT.Translation.X, handT.Translation.Y, handT.Translation.Z);

		glm::quat handRotInv = glm::inverse(handRot);

		FoodGrabRelativeRotation = handRotInv * foodRot;
		FoodGrabRelativePosition = handRotInv * (foodPos - handPos);
	}

	if (!Inputs::LeftGripInputPressed)
	{

		CurrentFoodRotation -= delta * 20.0f;
		if (CurrentFoodRotation <= -360.0f)
			CurrentFoodRotation += 360.0f;

		CurrentFoodRotation += cameraYawDelta;

		float radians = glm::radians(CurrentFoodRotation);

		// Apply the float
		float positionOffset = sinf(radians * 2.0f) * 0.01f; // frequency * amplitude
		auto rotationOffset = sinf(radians * 4.0f) * 2.0f;

		// Build target rotation properly
		glm::vec3 targetEulerDeg(-rotationOffset, -rotationOffset, CurrentFoodRotation);
		glm::vec3 targetRad = glm::radians(targetEulerDeg);

		glm::quat qTarget = glm::quat(glm::mat3(
			glm::rotate(glm::mat4(1.0f), targetRad.y, glm::vec3(0, 1, 0)) * // Yaw
			glm::rotate(glm::mat4(1.0f), targetRad.x, glm::vec3(1, 0, 0)) * // Pitch
			glm::rotate(glm::mat4(1.0f), targetRad.z, glm::vec3(0, 0, 1))   // Roll
		));


		// slerp from our *previous* smoothed value
		FoodCurrentLocalRotation = glm::slerp(FoodCurrentLocalRotation, qTarget, delta * 2.0f);

		// Target idle pos
		auto weaponRootPos = ue::call<ue::FVector>(weaponRoot, L"K2_GetComponentLocation");
		glm::vec3 targetPos(
			weaponRootPos.X,
			weaponRootPos.Y,
			weaponRootPos.Z + positionOffset
		);


		auto test = ue::call<ue::FVector>(foodMesh, L"K2_GetComponentLocation");
		FoodIdlePosition = glm::mix({test.X, test.Y, test.Z}, targetPos, delta * 8.0f);

		// apply
		ue::FTransform finalTsf = {};
		finalTsf.Rotation = {FoodCurrentLocalRotation.x, FoodCurrentLocalRotation.y, FoodCurrentLocalRotation.z, FoodCurrentLocalRotation.w};
		finalTsf.Translation = {FoodIdlePosition.x, FoodIdlePosition.y, FoodIdlePosition.z};
		finalTsf.Scale3D = ue::call<ue::FVector>(foodMesh, L"K2_GetComponentScale");

		

		ue::call(foodMesh, L"K2_SetWorldTransform", finalTsf, false, &hitResult, false);
	}
	else
	{
		// Follow left hand position 
		auto handT = ue::call<ue::FTransform>(leftHandMesh, L"K2_GetComponentToWorld");

		glm::quat handRot(handT.Rotation.W, handT.Rotation.X, handT.Rotation.Y, handT.Rotation.Z);
		glm::vec3 handPos(handT.Translation.X, handT.Translation.Y, handT.Translation.Z);

		// world transform reconstruction with the offset
		glm::quat newFoodRot = handRot * FoodGrabRelativeRotation;
		glm::vec3 newFoodPos = handPos + (handRot * FoodGrabRelativePosition);

		ue::FTransform out;
		out.Rotation = { newFoodRot.x, newFoodRot.y, newFoodRot.z, newFoodRot.w };
		out.Translation = { newFoodPos.x, newFoodPos.y, newFoodPos.z };
		out.Scale3D = ue::call<ue::FVector>(foodMesh, L"K2_GetComponentScale");

		ue::call(foodMesh, L"K2_SetWorldTransform", out, false, &hitResult, false);
	}
}
