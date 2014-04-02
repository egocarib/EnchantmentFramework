#include "MenuHandler.h"
#include "EnchantmentInfo.h"
#include "common/ICriticalSection.h"

#include "DumpDebug.h" //DEBUG

using namespace EnchantmentInfoLib;

MenuManager*		LocalMenuManager;
BSFixedString		EnchantMenuString;
LocalMenuHandler	g_menuHandler;
ICriticalSection	conditionEditLock;

class ConditionedWeaponEnchantments : public EnchantmentInfoLib::EnchantmentConditionMap
{
  public:
	bool Accept(EnchantmentItem* pEnch)
	{
		//_MESSAGE("Accepting enchantment 0x%08X (unk08: %u unk0C: %u unk10: %u unk14: %u unk18: %u)", pEnch->formID, pEnch->unk08, pEnch->unk0C, pEnch->unk10, pEnch->unk14, pEnch->unk18);
		if (pEnch->data.unk10 == 0x01) //delivery type: 'Contact' (weapon enchantment)
		{
			_MESSAGE("    Weapon Enchantment detected.... %s [%08X]", (DYNAMIC_CAST(pEnch, EnchantmentItem, TESFullName))->name.data, pEnch->formID);
			ConditionedEffectMap conditionedEffects;
			for (UInt32 i = 0; i < pEnch->effectItemList.count; ++i)
			{
				MagicItem::EffectItem* pEffect = NULL;
				pEnch->effectItemList.GetNthItem(i, pEffect);
				_MESSAGE("          Checking EffectItem #%u  (%s [%08X])", i + 1, (pEffect) ? (DYNAMIC_CAST(pEffect->mgef, EffectSetting, TESFullName))->name.data : "INVALID", (pEffect) ? pEffect->mgef->formID : 0);
				if (pEffect && pEffect->condition)
				{
					_MESSAGE("          Has Condition!   <-----------------------------------------------------------------");
					conditionedEffects[pEffect] = pEffect->condition;
				}
			}
			_MESSAGE("          Enchantment has %u conditioned effects.", conditionedEffects.size());
			if (!conditionedEffects.empty()) //add to EnchantmentConditionMap
				(*this)[pEnch] = conditionedEffects;
		}
		return true;
	}

	ConditionedWeaponEnchantments() {}
	ConditionedWeaponEnchantments(EnchantmentDataHandler* enchantments) { enchantments->Visit(this); }
};

ConditionedWeaponEnchantments		cWeaponEnchants;


void MenuHandler::InitializeMenuMonitor()
{
	_MESSAGE("Initializing Menu Monitor...");
	static EnchantmentDataHandler	eData;
	cWeaponEnchants = ConditionedWeaponEnchantments(&eData);
	LocalMenuManager = MenuManager::GetSingleton();
	EnchantMenuString = UIStringHolder::GetSingleton()->craftingMenu;
	LocalMenuManager->MenuOpenCloseEventDispatcher()->AddEventSink(&g_menuHandler);
}

EventResult LocalMenuHandler::ReceiveEvent(MenuOpenCloseEvent * evn, EventDispatcher<MenuOpenCloseEvent> * dispatcher)
{
	//_MESSAGE("MENU EVENT TRIGGERED");

	if (evn->menuName.data != EnchantMenuString.data)
		return kEvent_Continue;

	conditionEditLock.Enter(); //technically this still probably isn't threadsafe, if threads pile up and enter
						//in wrong order - however, this is also pretty fast so I don't see how the user could
						//conceivably open/close the crafting menu fast enough to generate any problems..

	if (evn->opening)
	{
		_MESSAGE("ConditionedWeaponEnchantments Map Size: %u", cWeaponEnchants.size());
		_MESSAGE("    Detaching Condition Pointers...");

		//detach conditions from weapon enchantments
		for (ConditionedWeaponEnchantments::iterator enchIt = cWeaponEnchants.begin(); enchIt != cWeaponEnchants.end(); ++enchIt)
			for(ConditionedEffectMap::iterator effectIt = enchIt->second.begin(); effectIt != enchIt->second.end(); ++effectIt)
				effectIt->first->condition = NULL;
	}
	else
	{
		//_MESSAGE("ConditionedWeaponEnchantments Map Size: %u", cWeaponEnchants.size());
		_MESSAGE("    Re-attaching Condition Pointers...");

		//re-attach conditions
		for (ConditionedWeaponEnchantments::iterator enchIt = cWeaponEnchants.begin(); enchIt != cWeaponEnchants.end(); ++enchIt)
		{
			for(ConditionedEffectMap::iterator effectIt = enchIt->second.begin(); effectIt != enchIt->second.end(); ++effectIt)
				effectIt->first->condition = effectIt->second;
			DumpEnchantmentData(enchIt->first); //DEBUG
		}
	}

	conditionEditLock.Leave();

	return kEvent_Continue;
}