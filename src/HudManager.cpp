#include "HudManager.hpp"

#include <format>

#include "Logger.hpp"
#include "ue/Reflection.hpp"

namespace {

constinit ue::ClassRef CHUDState_c{ L"Class /Script/CobraUI.CHUDState" };
constinit ue::ClassRef PanelWidget_c{ L"Class /Script/UMG.PanelWidget" };
constinit ue::ClassRef CanvasPanelSlot_c{ L"Class /Script/UMG.CanvasPanelSlot" };

constinit ue::ClassRef HudElementClasses[] = {
	ue::ClassRef{ L"Class /Script/CobraUI.CHudEntryWidget" }, // life, camo, phase, call, timer, boss gauges
	ue::ClassRef{ L"Class /Script/CobraUI.CCompassWidget" },
	ue::ClassRef{ L"Class /Script/CobraUI.CQuickSwitchMenuItemWidget" }, // equipped weapon / item
	ue::ClassRef{ L"Class /Script/CobraUI.CActionIconWidget" },
	ue::ClassRef{ L"Class /Script/CobraUI.CPhaseIndicatorWidget" },
	ue::ClassRef{ L"Class /Script/CobraUI.CBatteryInfoWidget" },
	ue::ClassRef{ L"Class /Script/CobraUI.CItemBoxWidget" },
};

std::wstring describeWidget(ue::UObject* widget)
{
	const auto desired = ue::call<ue::FVector2D>(widget, L"GetDesiredSize");
	const auto visibility = ue::call<ue::ESlateVisibility>(widget, L"GetVisibility");
	auto text = widget->get_full_name() + std::format(L" | desired {:.0f}x{:.0f} | visibility {}", desired.X, desired.Y, (int)visibility);

	const auto slot = ue::get<ue::UObject*>(widget, L"Slot");
	if (ue::is_a(slot, CanvasPanelSlot_c))
	{
		const auto position = ue::call<ue::FVector2D>(slot, L"GetPosition");
		const auto size = ue::call<ue::FVector2D>(slot, L"GetSize");
		const auto alignment = ue::call<ue::FVector2D>(slot, L"GetAlignment");
		text += std::format(L" | canvas pos {:.0f},{:.0f} size {:.0f}x{:.0f} align {:.2f},{:.2f} autosize {} z {}",
			position.X, position.Y, size.X, size.Y, alignment.X, alignment.Y,
			ue::call<bool>(slot, L"GetAutoSize"), ue::call<int32_t>(slot, L"GetZOrder"));
	}
	return text;
}

// Recurses through panels only: user widgets (the HUD elements) are leaves
void logWidgetTree(ue::UObject* widget, int depth)
{
	if (!widget)
		return;
	Logger::DebugPrint(std::wstring(depth * 2, L' ') + describeWidget(widget));
	if (!ue::is_a(widget, PanelWidget_c))
		return;
	const int32_t count = ue::call<int32_t>(widget, L"GetChildrenCount");
	for (int32_t i = 0; i < count; i++)
		logWidgetTree(ue::call<ue::UObject*>(widget, L"GetChildAt", i), depth + 1);
}

}

void HudManager::LogHudWidgets()
{
	Logger::DebugPrint("===== HUD widget tree =====");
	if (CHUDState_c.get())
	{
		for (auto hud : CHUDState_c.get()->get_objects_matching(false))
		{
			Logger::DebugPrint(L"HUD root (in viewport: " + std::to_wstring(ue::call<bool>(hud, L"IsInViewport")) + L"): " + describeWidget(hud));
			logWidgetTree(ue::get<ue::UObject*>(ue::get<ue::UObject*>(hud, L"WidgetTree"), L"RootWidget"), 1);
		}
	}

	Logger::DebugPrint("===== HUD element instances =====");
	for (const auto& elementClass : HudElementClasses)
	{
		if (!elementClass.get())
			continue;
		for (auto element : elementClass.get()->get_objects_matching(false))
		{
			const auto parent = ue::call<ue::UObject*>(element, L"GetParent");
			Logger::DebugPrint(describeWidget(element) + L" | parent " + (parent ? parent->get_full_name() : L"none"));
		}
	}
}
