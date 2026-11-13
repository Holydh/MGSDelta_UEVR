#include "MenuFixes.hpp"
#include "ue/Reflection.hpp"
#include "Logger.hpp"

void MenuFixes::FixSurvivalViewerMenuRendering()
{
	static ue::ClassRef sv_main_c{ L"WidgetBlueprintGeneratedClass /CobraUI/Blueprint/sv_main.sv_main_C" };
	auto mainWidget = sv_main_c.get() ? sv_main_c.get()->get_first_object_matching(false) : nullptr;
	if (!mainWidget)
		return;

	ue::call(ue::get<ue::UObject*>(mainWidget, L"ModelCaptureCanvas"), L"SetVisibility", ue::ESlateVisibility::Hidden);
	// Get sub menu type : mainWidget->GetSvStep();

	auto rootWidgetCanvasPanel = ue::get<ue::UObject*>(ue::get<ue::UObject*>(mainWidget, L"WidgetTree"), L"RootWidget");
	auto rootSlots = ue::get_ptr<ue::TArray<ue::UObject*>>(rootWidgetCanvasPanel, L"Slots");
	if (!rootSlots)
		return;
	Logger::DebugPrint(rootWidgetCanvasPanel->get_full_name());

	// Loop through slots of the root panel
	Logger::DebugPrint("Root widget slots:");
	for (auto slot : *rootSlots)
	{
		if (!slot)
			continue;
		Logger::DebugPrint(slot->get_full_name());

		// Access the widget contained in this slot
		auto containedWidget = ue::get<ue::UObject*>(slot, L"Content");
		if (containedWidget) {
			Logger::DebugPrint(L"  Contains: " + containedWidget->get_full_name());
		}
	}

	// If you want to access a specific child panel's slots
	// Find a specific child widget by name or index
	if (!rootSlots->empty()) {
		static ue::ClassRef panelWidget_c{ L"Class /Script/UMG.PanelWidget" };

		// Get the first slot's content (which might be another panel)
		auto firstChildWidget = ue::get<ue::UObject*>(rootSlots->data[0], L"Content");

		// If it's a panel, you can access its slots
		if (ue::is_a(firstChildWidget, panelWidget_c)) {
			auto childSlots = ue::get_ptr<ue::TArray<ue::UObject*>>(firstChildWidget, L"Slots");
			for (int i = 0; childSlots && i < childSlots->count; i++) {
				auto slot = childSlots->data[i];
				if (!slot)
					continue;
				const std::wstring slotName = slot->get_fname()->to_string(); // Get just the name without path

				if (slotName == L"CanvasPanelSlot_10") {
					ue::call(ue::get<ue::UObject*>(slot, L"Content"), L"SetVisibility", ue::ESlateVisibility::Hidden);
					Logger::DebugPrint("Found target slot at index: %i", i);
				}
				if (slotName == L"CanvasPanelSlot_1")
				{
					ue::call(ue::get<ue::UObject*>(slot, L"Content"), L"SetVisibility", ue::ESlateVisibility::Hidden);
					Logger::DebugPrint("Found target slot at index: %i", i);
				}
			}
		}
	}
}

void MenuFixes::FixRadioMenuRendering()
{
	static ue::ClassRef Radio_c{ L"WidgetBlueprintGeneratedClass /CobraUI/Blueprint/Radio/Radio.Radio_C" };
	auto mainWidget = Radio_c.get() ? Radio_c.get()->get_first_object_matching(false) : nullptr;
	ue::call(ue::get<ue::UObject*>(mainWidget, L"ModelBg"), L"SetVisibility", ue::ESlateVisibility::Hidden);
}
