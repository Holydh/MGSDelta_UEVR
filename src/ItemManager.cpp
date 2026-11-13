#include "ItemManager.hpp"
#include "Utilities.hpp"
#include "Logger.hpp"

namespace {
constinit ue::ClassRef PlayerCameraManagerBP_c{ L"BlueprintGeneratedClass /Game/Blueprints/Camera/BP_PlayerCameraManager.BP_PlayerCameraManager_C" };
}

void ItemManager::Update()
{
	ItemController = ue::call<ue::UObject*>(gameState->Pawn, L"GetItemControllerBP");
	if (ItemController == nullptr)
	{
		Logger::DebugPrint("No itemController found");
	}

	processItem();

	if (thermalScanlinePending)
	{
		ApplyThermalVisionScanlineSetting();
	}
}

void ItemManager::processItem()
{
	if (ItemController == nullptr)
	{
		CurrentItemID = ue::EGsrItemId::IT_None;
		return;
	}

	auto newItemID = ue::call<ue::EGsrItemId>(ItemController, L"GetItemId");
	if (CurrentItemID != newItemID)
	{
		CurrentItemID = newItemID;
		Logger::DebugPrint("Item changed. New Item ID : %d", (int)CurrentItemID.Get());

		CurrentItem = ue::get<ue::UObject*>(ItemController, L"m_currentItem");
		if (CurrentItem)
			Logger::DebugPrint(CurrentItem->get_full_name());

		if (CurrentItemID == ue::EGsrItemId::IT_NightVision)
		{
			ue::call(ue::get<ue::UObject*>(CurrentItem, L"SKM_nvgo_dmo_hd"), L"SetOnlyOwnerSee", true);

		}
		if (CurrentItemID == ue::EGsrItemId::IT_Thermal)
		{
			ue::call(ue::get<ue::UObject*>(CurrentItem, L"SKM_thgo_dmo_hd"), L"SetOnlyOwnerSee", true);
			playerCameraManager = Utilities::FirstObjectOf(PlayerCameraManagerBP_c);
			if (playerCameraManager == nullptr)
			{
				Logger::DebugPrint("No player camera manager found");
				return;
			}
			thermalScanlinePending = true;
		}

		if (CurrentItemID.Entered(ue::EGsrItemId::IT_Stealth))
		{
			hands->ApplyPawnHandMaterial();
		}
		if (CurrentItemID.Left(ue::EGsrItemId::IT_Stealth))
		{
			hands->ApplyPawnHandMaterial();
		}
	}
}

bool ItemManager::IsScopeEquipped() const
{
	return CurrentItemID == ue::EGsrItemId::IT_Scope;
}

bool ItemManager::IsCardboardBoxEquipped() const
{
	return CurrentItemID == ue::EGsrItemId::IT_CBoxA || CurrentItemID == ue::EGsrItemId::IT_CBoxB ||
		CurrentItemID == ue::EGsrItemId::IT_CBoxC || CurrentItemID == ue::EGsrItemId::IT_CBoxD;
}

void ItemManager::EndFrame()
{
	CurrentItemID.Commit();
}

void ItemManager::Cleanup()
{
	playerCameraManager = nullptr;
}

void ItemManager::ApplyThermalVisionScanlineSetting()
{
	if (playerCameraManager == nullptr)
	{
		Logger::DebugPrint("No player camera manager found");
		return;
	}
	auto thermalPostProcessActor = ue::get<ue::UObject*>(playerCameraManager, L"ThermalPostProcessActor");
	if (thermalPostProcessActor == nullptr)
	{
		return;
	}
	Logger::DebugPrint("Toggle Thermal Vision Scanline");
	ue::call(ue::get<ue::UObject*>(thermalPostProcessActor, L"Scanline"), L"SetVisibility", !settingsManager->Settings.disableThermalVisionScanline, true);
	thermalScanlinePending = false;
}
