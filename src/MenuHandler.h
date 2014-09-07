#pragma once

#include "skse/GameMenus.h"
#include "skse/GameEvents.h"
#include "skse/GameObjects.h"




struct TESTrackedStatsEvent
{
	BSFixedString	statName;
	UInt32			newValue;
};

template <>
class BSTEventSink <TESTrackedStatsEvent>
{
public:
	virtual ~BSTEventSink() {}
	virtual EventResult ReceiveEvent(TESTrackedStatsEvent * evn, EventDispatcher<TESTrackedStatsEvent> * dispatcher) = 0;
};

//Tracked Stats Event Handler
class TESTrackedStatsEventHandler : public BSTEventSink <TESTrackedStatsEvent> 
{
public:
	virtual	EventResult	ReceiveEvent(TESTrackedStatsEvent * evn, EventDispatcher<TESTrackedStatsEvent> * dispatcher);
};

extern	EventDispatcher<TESTrackedStatsEvent>*	g_trackedStatsEventDispatcher;
extern	TESTrackedStatsEventHandler				g_trackedStatsEventHandler;




//Menu Open/Close Event Handler
class LocalMenuHandler : public BSTEventSink <MenuOpenCloseEvent>
{
public:
	virtual EventResult		ReceiveEvent(MenuOpenCloseEvent * evn, EventDispatcher<MenuOpenCloseEvent> * dispatcher);
};





typedef std::map <MagicItem::EffectItem*, Condition*>		ConditionedEffectMap;
typedef std::map <EnchantmentItem*, ConditionedEffectMap>	EnchantmentConditionMap;

class ConditionedWeaponEnchantments : public EnchantmentConditionMap
{
  public:
	bool Accept(EnchantmentItem* pEnch);
};


typedef ConditionedWeaponEnchantments::iterator		CndEnchantIter;
typedef ConditionedEffectMap::iterator				CndEffectIter;


class MenuCore //Statics that don't need to change between loads in the same play session
{
public:
	void InitializeMenuMonitor();

	static LocalMenuHandler					thisMenu;
	static BSFixedString					enchantMenuString;
	static ConditionedWeaponEnchantments	cWeaponEnchants;
};


extern MenuCore menu;