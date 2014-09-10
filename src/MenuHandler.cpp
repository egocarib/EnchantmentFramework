#include "MenuHandler.h"
#include "EnchantmentInfo.h"

#include "skse/GameRTTI.h" //remove this after debug, not needed

LocalMenuHandler				MenuCore::thisMenu;
BSFixedString					MenuCore::craftingMenu;

ConditionedWeaponEnchantments	g_weaponEnchantmentConditions;

#define MARK true
#define ERASE false

EventDispatcher<TESTrackedStatsEvent>*	g_trackedStatsEventDispatcher = (EventDispatcher<TESTrackedStatsEvent>*) 0x012E5470;
TESTrackedStatsEventHandler				g_trackedStatsEventHandler;






void MenuCore::InitializeMenuMonitor()
{
	static bool bOnce = true;
	if (bOnce)
	{
		bOnce = false;
		EnchantmentDataHandler::Visit(&g_weaponEnchantmentConditions);
		craftingMenu = UIStringHolder::GetSingleton()->craftingMenu;
		MenuManager::GetSingleton()->MenuOpenCloseEventDispatcher()->AddEventSink(&thisMenu);
	}
}


EventResult LocalMenuHandler::ReceiveEvent(MenuOpenCloseEvent* evn, EventDispatcher<MenuOpenCloseEvent>* dispatcher)
{
	if (!evn || (strcmp(evn->menuName.data, MenuCore::craftingMenu.data) != 0))
		return kEvent_Continue;

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

	return kEvent_Continue;
}



EventResult TESTrackedStatsEventHandler::ReceiveEvent(TESTrackedStatsEvent * evn, EventDispatcher<TESTrackedStatsEvent> * dispatcher)
{
	//New enchanted item crafted (maybe staff too? haven't tried)

	//should also disable this when not at the enchanting table...
		//REMOVE EVENT SINKS WHEN MENU NOT OPEN, or set bool (since hooks should be deactivated too)
	if (strcmp(evn->statName.data, TrackedStatsStringHolder::GetSingleton()->magicItemsMade.data) == 0)
	{

		enchantTracker.PostCraftUpdate(); //Adds this crafted (weapon) enchantment to tracker and updates necessary data

		_MESSAGE("Received magicItemMade event!");
	}

	return kEvent_Continue;
}