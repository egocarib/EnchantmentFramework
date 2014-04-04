#include "MenuHandler.h"
#include "EnchantmentInfo.h"
#include "common/ICriticalSection.h"

#include "DumpDebug.h" //DEBUG

using namespace EnchantmentInfoLib;

MenuManager*		LocalMenuManager;
BSFixedString		EnchantMenuString;
LocalMenuHandler	g_menuHandler;
ICriticalSection	conditionEditLock;

class ConditionedWeaponEnchantments : public EnchantmentConditionMap
{
  public:
	bool Accept(EnchantmentItem* pEnch)
	{
		if (pEnch->data.unk10 == 0x01) //delivery type: 'Contact' (weapon enchantment)
		{
			ConditionedEffectMap conditionedEffects;
			for (UInt32 i = 0; i < pEnch->effectItemList.count; ++i)
			{
				MagicItem::EffectItem* pEffect = NULL;
				pEnch->effectItemList.GetNthItem(i, pEffect);
				if (pEffect && pEffect->condition)
					conditionedEffects[pEffect] = pEffect->condition; // 'condition' renamed from 'unk14'
			}
			if (!conditionedEffects.empty()) //add to EnchantmentConditionMap
				(*this)[pEnch] = conditionedEffects;
		}
		return true;
	}

	ConditionedWeaponEnchantments() {}
	//ConditionedWeaponEnchantments(EnchantmentDataHandler* enchantments) { enchantments->Visit(this); }
};

ConditionedWeaponEnchantments		cWeaponEnchants;


void MenuHandler::InitializeMenuMonitor()
{
	static bool firstLoad = true;
	if (firstLoad)
	{
		firstLoad = false;
		cWeaponEnchants = ConditionedWeaponEnchantments();
		EnchantmentDataHandler::Visit(&cWeaponEnchants);
		LocalMenuManager = MenuManager::GetSingleton();
		EnchantMenuString = UIStringHolder::GetSingleton()->craftingMenu;
		LocalMenuManager->MenuOpenCloseEventDispatcher()->AddEventSink(&g_menuHandler);
	}
}

EventResult LocalMenuHandler::ReceiveEvent(MenuOpenCloseEvent * evn, EventDispatcher<MenuOpenCloseEvent> * dispatcher)
{
	if (evn->menuName.data != EnchantMenuString.data)
		return kEvent_Continue;

	conditionEditLock.Enter();
	//technically this still probably isn't threadsafe, if threads pile up and enter
	//in wrong order - however, this is also pretty fast so I don't see how the user could
	//conceivably open/close the crafting menu fast enough to generate any problems..

	if (evn->opening) //detach conditions from weapon enchantments
	{
		for (ConditionedWeaponEnchantments::iterator enchIt = cWeaponEnchants.begin(); enchIt != cWeaponEnchants.end(); ++enchIt)
			for(ConditionedEffectMap::iterator effectIt = enchIt->second.begin(); effectIt != enchIt->second.end(); ++effectIt)
				effectIt->first->condition = NULL;
	}
	else //re-attach conditions
	{
		for (ConditionedWeaponEnchantments::iterator enchIt = cWeaponEnchants.begin(); enchIt != cWeaponEnchants.end(); ++enchIt)
			for(ConditionedEffectMap::iterator effectIt = enchIt->second.begin(); effectIt != enchIt->second.end(); ++effectIt)
				effectIt->first->condition = effectIt->second;
	}

	conditionEditLock.Leave();

	if (!evn->opening)
		//ConditionalizeNewPlayerEnchantments() //vanilla bug results in base enchantment conditions being stripped from all player-enchanted items
		//NEED TO REPLACE THIS FUNCTION

	return kEvent_Continue;
}