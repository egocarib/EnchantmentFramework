#include "MenuHandler.h"
#include "EnchantmentInfo.h"

#include "skse/GameRTTI.h" //remove this after debug, not needed

LocalMenuHandler				MenuCore::thisMenu;
BSFixedString					MenuCore::craftingMenu;
ConditionedWeaponEnchantments	MenuCore::cWeaponEnchants;

#define MARK true
#define ERASE false

EventDispatcher<TESTrackedStatsEvent>*	g_trackedStatsEventDispatcher = (EventDispatcher<TESTrackedStatsEvent>*) 0x012E5470;
TESTrackedStatsEventHandler				g_trackedStatsEventHandler;


typedef std::map<EnchantmentItem*, UInt32>	EnchantmentIntMap;

bool ConditionedWeaponEnchantments::Accept(EnchantmentItem* const pEnch)
{
	if (pEnch->data.deliveryType == 0x01) //Weapon enchantment (delivery type: 'contact')
	{
		ConditionedEffectMap conditionedEffects;
		for (UInt32 i = 0; i < pEnch->effectItemList.count; ++i)
		{
			MagicItem::EffectItem* pEffect = NULL;
			pEnch->effectItemList.GetNthItem(i, pEffect);
			if (pEffect && pEffect->unk14) //(unk14 == condition)
				conditionedEffects[pEffect] = reinterpret_cast<Condition*>(pEffect->unk14);
		}
		if (!conditionedEffects.empty())
			(*this)[pEnch] = conditionedEffects;
	}
	return true;
}





// namespace CraftHandler
// {

// 	void ReportEffects(tArray<MagicItem::EffectItem>* const effectArray)
// 	{
// 		// static std::map<EffectSetting*, UInt32> mgefs;
// 		// static MagEffVec mgefs;

// 		static std::list<EffectSetting*>	mgefs

// 		std::map<EffectSetting*, UInt32>	newMgefs;

// 		if (effectArray->count > mgefs.size())
// 			//dump into map with UInt32 count

// 		//then go through list and decrement counts/erase as found. then find what's left in each..

// 			// 1 3 6 7
// 			// 1 3 6 7 8 9

// 	}


// };






// void ResolveBaseEnchantments(EnchantmentInfoReferenceVec* const newEnchantments)
// {
// 	for (UInt32 h = 0; h < (*newEnchantments).size(); h++)
// 	{
// 		MagEffVec mgefs;
// 		EnchantmentItem* thisEnchantment = (*newEnchantments)[h]->first;

// 		for(UInt32 i = 0; i < thisEnchantment->effectItemList.count; i++)
// 		{
// 			MagicItem::EffectItem* effectItem = NULL;
// 			thisEnchantment->effectItemList.GetNthItem(i, effectItem);
// 			mgefs.push_back(effectItem->mgef);

// 			if (effectItem->area >= '@END') //end of this enchantment's effect list
// 			{
// 				effectItem->area -= '@END'; //restore to base enchantment's original value
// 				EnchantmentItem* baseEnchantment = FindBaseEnchantment(mgefs, thisEnchantment->data.deliveryType);
// 				_MESSAGE("Found a parent enchantment for player-made enchantment 0x%08X - [parent: 0x%08X  %s]", thisEnchantment->formID, (baseEnchantment) ? baseEnchantment->formID : 0, (baseEnchantment) ? (DYNAMIC_CAST(baseEnchantment, EnchantmentItem, TESFullName))->name.data : "<null>");
// 				if (baseEnchantment)
// 				{
// 					//Inherit conditions from parent enchantment
// 					for (UInt32 j = 0; j < baseEnchantment->effectItemList.count; ++j)
// 					{
// 						MagicItem::EffectItem* baseEffectItem = NULL;
// 						baseEnchantment->effectItemList.GetNthItem(j, baseEffectItem);
// 						if (baseEffectItem && baseEffectItem->unk14) //(unk14 == condition)
// 						{
// 							(*newEnchantments)[h]->second.attributes.hasConditions = true;
// 							MagicItem::EffectItem* pNew = NULL;
// 							thisEnchantment->effectItemList.GetNthItem(j, pNew);
// 							pNew->unk14 = baseEffectItem->unk14;
// 							// (weirdly enough, unlike the serialization load method, this
// 							//  doesn't cause any problems... I can reload the game many
// 							//  times and the condition stays valid and doesn't cause a crash)
// 						}
// 					}

// 					//Add to parent forms list, and clear vector to continue search for next mgef set on this enchantment
// 					(*newEnchantments)[h]->second.parentForms.data.push_back(baseEnchantment->formID);
// 				}
// 				mgefs.clear();
// 			}
// 		}
// 	}
// }


// class MarkAllWeaponEnchantmentForms
// {
// private:
// 	const bool shouldMark;
// 	enum { kDeliveryType_Contact = 0x01 /* weapon enchantment */ };

// public:
// 	bool Accept(EnchantmentItem* const pEnch)
// 	{
// 		if (pEnch->data.deliveryType == kDeliveryType_Contact)
// 		{
// 			UInt32 finalIndex = (pEnch->effectItemList.count) - 1;
// 			MagicItem::EffectItem* effectItem = NULL;
// 			pEnch->effectItemList.GetNthItem(finalIndex, effectItem);
			
// 			//Cache enchantment's final effect area and replace with a unique identifier
// 			//(FYI, Dawnguard Rune Axe is the only vanilla enchant with area in effects)
// 			if (shouldMark) //{
// 				effectItem->area += '@END';
// 				//_MESSAGE("area for enchantment 0x%08X increased by '@END' to   %u", pEnch->formID, effectItem->area); }
// 			else //{
// 				effectItem->area -= '@END';
// 				//_MESSAGE("area for enchantment 0x%08X decreased by '@END' to   %u", pEnch->formID, effectItem->area); }
// 		}
// 		return true;
// 	}

// 	MarkAllWeaponEnchantmentForms(bool b) : shouldMark(b) {}
// };


void MenuCore::InitializeMenuMonitor()
{
	static bool bOnce = true;
	if (bOnce)
	{
		bOnce = false;
		EnchantmentDataHandler::Visit(&cWeaponEnchants);
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
		//Detach conditions from weapon enchantments
		for (CndEnchantIter enchIt = MenuCore::cWeaponEnchants.begin(); enchIt != MenuCore::cWeaponEnchants.end(); ++enchIt)
			for(CndEffectIter effectIt = enchIt->second.begin(); effectIt != enchIt->second.end(); ++effectIt)
				effectIt->first->unk14 = NULL; //(unk14 == condition)

		//Mark final effect area to allow distinguishing among multiple base enchantments used to craft a custom enchantment
		// MarkAllWeaponEnchantmentForms marker(MARK);
		// EnchantmentDataHandler::Visit(&marker);
	}

	else
	{
		//Reattach conditions
		for (CndEnchantIter enchIt = MenuCore::cWeaponEnchants.begin(); enchIt != MenuCore::cWeaponEnchants.end(); ++enchIt)
			for(CndEffectIter effectIt = enchIt->second.begin(); effectIt != enchIt->second.end(); ++effectIt)
				effectIt->first->unk14 = reinterpret_cast<UInt32>(effectIt->second);





		// EnchantmentInfoReferenceVec* pNewCraftedEnchantmentInfo;
		//enchantTracker.PostCraftUpdate(); //Patch crafted enchantments

		// ResolveBaseEnchantments(pNewCraftedEnchantmentInfo); //Determine base enchantments combined during crafting. record & restore their area

		// MarkAllWeaponEnchantmentForms marker(ERASE); //Restore area values for all loaded weapon enchantment forms
		// EnchantmentDataHandler::Visit(&marker);
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