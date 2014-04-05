#pragma once

#include "skse/GameMenus.h"
#include "skse/GameEvents.h"
#include "skse/GameObjects.h"

typedef std::map <MagicItem::EffectItem*, Condition*>		ConditionedEffectMap;
typedef std::map <EnchantmentItem*, ConditionedEffectMap>	EnchantmentConditionMap;


class LocalMenuHandler : public BSTEventSink <MenuOpenCloseEvent>
{
public:
	virtual EventResult		ReceiveEvent(MenuOpenCloseEvent * evn, EventDispatcher<MenuOpenCloseEvent> * dispatcher);
};


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

