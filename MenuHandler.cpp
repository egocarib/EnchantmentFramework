#include "MenuHandler.h"

MenuManager*		LocalMenuManager;
BSFixedString		EnchantMenuString;
LocalMenuHandler	g_menuHandler;

void MenuHandler::InitializeMenuMonitor()
{
	LocalMenuManager = MenuManager::GetSingleton();
	EnchantMenuString = UIStringHolder::GetSingleton()->craftingMenu;
	LocalMenuManager->MenuOpenCloseEventDispatcher()->AddEventSink(&g_menuHandler);
}

EventResult LocalMenuHandler::ReceiveEvent(MenuOpenCloseEvent * evn, EventDispatcher<MenuOpenCloseEvent> * dispatcher)
{
	//_MESSAGE("MENU EVENT TRIGGERED");

	if (evn->menuName.data != EnchantMenuString.data)
		return kEvent_Continue;

	if (evn->opening)
	{
		//detach all pointers
	}
	else
	{
		//re-attach all pointers
	}

	return kEvent_Continue;
}