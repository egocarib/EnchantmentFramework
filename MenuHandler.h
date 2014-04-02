#pragma once

#include "skse/GameMenus.h"
#include "skse/GameEvents.h"

class LocalMenuHandler : public BSTEventSink <MenuOpenCloseEvent>
{
public:
	virtual EventResult		ReceiveEvent(MenuOpenCloseEvent * evn, EventDispatcher<MenuOpenCloseEvent> * dispatcher);
};

extern MenuManager*			LocalMenuManager;
extern BSFixedString		EnchantMenuString;
extern LocalMenuHandler		g_menuHandler;

namespace MenuHandler
{
	void InitializeMenuMonitor();
}