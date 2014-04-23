#include "MenuHandler.h"
#include "EnchantmentInfo.h"

LocalMenuHandler				MenuCore::thisMenu;
BSFixedString					MenuCore::enchantMenuString;
ConditionedWeaponEnchantments	MenuCore::cWeaponEnchants;

bool ConditionedWeaponEnchantments::Accept(EnchantmentItem* pEnch)
{
	if (pEnch->data.unk10 == 0x01) //Weapon enchantment (delivery type: 'contact')
	{
		ConditionedEffectMap conditionedEffects;
		for (UInt32 i = 0; i < pEnch->effectItemList.count; ++i)
		{
			MagicItem::EffectItem* pEffect = NULL;
			pEnch->effectItemList.GetNthItem(i, pEffect);
			if (pEffect && pEffect->condition)
				conditionedEffects[pEffect] = pEffect->condition; //('condition' renamed from 'unk14')
		}
		if (!conditionedEffects.empty())
			(*this)[pEnch] = conditionedEffects;
	}
	return true;
}


void MenuCore::InitializeMenuMonitor()
{
	static bool bOnce = true;
	if (bOnce)
	{
		bOnce = false;
		EnchantmentDataHandler::Visit(&cWeaponEnchants);
		enchantMenuString = UIStringHolder::GetSingleton()->craftingMenu;
		MenuManager::GetSingleton()->MenuOpenCloseEventDispatcher()->AddEventSink(&thisMenu);
	}
}


EventResult LocalMenuHandler::ReceiveEvent(MenuOpenCloseEvent * evn, EventDispatcher<MenuOpenCloseEvent> * dispatcher)
{
	if (!evn || (strcmp(evn->menuName.data, MenuCore::enchantMenuString.data) != 0))
		return kEvent_Continue;

	if (evn->opening) //Detach conditions from weapon enchantments
	{
		for (CndEnchantIter enchIt = MenuCore::cWeaponEnchants.begin(); enchIt != MenuCore::cWeaponEnchants.end(); ++enchIt)
			for(CndEffectIter effectIt = enchIt->second.begin(); effectIt != enchIt->second.end(); ++effectIt)
				effectIt->first->condition = NULL;
	}
	else //Reattach conditions
	{
		for (CndEnchantIter enchIt = MenuCore::cWeaponEnchants.begin(); enchIt != MenuCore::cWeaponEnchants.end(); ++enchIt)
			for(CndEffectIter effectIt = enchIt->second.begin(); effectIt != enchIt->second.end(); ++effectIt)
				effectIt->first->condition = effectIt->second;
	}

	if (!evn->opening)
		enchantTracker.Update();

	return kEvent_Continue;
}