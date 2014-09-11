#include "MenuHandler.h"
#include "EnchantmentInfo.h"


ConditionedWeaponEnchantments			g_weaponEnchantmentConditions;

EventDispatcher<TESTrackedStatsEvent>*	g_trackedStatsEventDispatcher = (EventDispatcher<TESTrackedStatsEvent>*) 0x012E5470;
TESTrackedStatsEventHandler				g_trackedStatsEventHandler;
LocalMenuHandler						g_menuEventHandler;


EventResult LocalMenuHandler::ReceiveEvent(MenuOpenCloseEvent* evn, EventDispatcher<MenuOpenCloseEvent>* dispatcher)
{
	if (strcmp(evn->menuName.data, UIStringHolder::GetSingleton()->craftingMenu.data) == 0)
	{
		if (evn->opening)
		{
			g_weaponEnchantmentConditions.Detach(); //Detach conditions from weapon enchantments
			g_trackedStatsEventDispatcher->AddEventSink(&g_trackedStatsEventHandler);
		}
		else
		{
			g_weaponEnchantmentConditions.Reattach(); //Reattach conditions
			g_trackedStatsEventDispatcher->RemoveEventSink(&g_trackedStatsEventHandler);
		}
	}

	return kEvent_Continue;
}

EventResult TESTrackedStatsEventHandler::ReceiveEvent(TESTrackedStatsEvent * evn, EventDispatcher<TESTrackedStatsEvent> * dispatcher)
{
	//TODO: Does this event fire when staff is crafted? When stat is increased from papyrus script?
	if (strcmp(evn->statName.data, TrackedStatsStringHolder::GetSingleton()->magicItemsMade.data) == 0)
		g_enchantTracker.PostCraftUpdate(); //Adds this crafted (weapon) enchantment to tracker and updates necessary data

	return kEvent_Continue;
}