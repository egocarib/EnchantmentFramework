#include "EnchantmentInfo.h"
#include "skse/GameData.h"
#include "skse/GameObjects.h"



KnownBaseEnchantments::NamedEnchantMap KnownBaseEnchantments::discoveredEnchantments;

EnchantmentItem* KnownBaseEnchantments::LookupByName(const char* targetName, char hint) //'w' for weapon, 'a' for armor
{
	if (discoveredEnchantments.count(targetName) > 0)
		return discoveredEnchantments[targetName];

	if (hint != 'a')
		for (EnchantmentVec::iterator it = knownWeaponBaseEnchantments.begin(); it != knownWeaponBaseEnchantments.end(); ++it)
			if (strcmp(targetName, (DYNAMIC_CAST((*it), EnchantmentItem, TESFullName))->name.data) == 0)
				return discoveredEnchantments[targetName] = (*it);

	for (EnchantmentVec::iterator it = knownArmorBaseEnchantments.begin(); it != knownArmorBaseEnchantments.end(); ++it)
		if (strcmp(targetName, (DYNAMIC_CAST((*it), EnchantmentItem, TESFullName))->name.data) == 0)
			return discoveredEnchantments[targetName] = (*it);

	return NULL;
}

void KnownBaseEnchantments::Reset()
{
	knownWeaponBaseEnchantments.clear();
	knownArmorBaseEnchantments.clear();
}


bool PersistentWeaponEnchantments::bInitialized = false;

KnownBaseEnchantments* PersistentWeaponEnchantments::GetKnown(const bool &invalidate)
{
	static KnownBaseEnchantments known;
	if (invalidate || !bInitialized)
	{
		bInitialized = true;
		known.Reset();
		EnchantmentDataHandler::Visit(&known);
	}
	return &known;
}

//Only call this right after leaving the enchanting table
void PersistentWeaponEnchantments::PostCraftUpdate()
{
	newCraftedEnchantments.clear();
	PersistentFormManager* pPFM = PersistentFormManager::GetSingleton();
	bool bFirstLoopPass = true;
	for(UInt32 i = 0; i < pPFM->weaponEnchants.count; i++)
	{
		PersistentFormManager::EnchantData entryData;
		pPFM->weaponEnchants.GetNthItem(i, entryData);
		if (!entryData.enchantment)
			continue;
		else if (!(entryData.enchantment->data.calculations.flags & EnchantmentInfoEntry::kFlag_ManualCalc))
			continue; //Ignore Auto-Calc enchants (they are created before this plugin or via papyrus CreateEnchantment)
		else if (playerWeaponEnchants.find(entryData.enchantment) != playerWeaponEnchants.end())
			continue; //Already added to map

		FixIfChaosDamage(entryData.enchantment);

		UInt32 thisFormID = entryData.enchantment->formID;
		UInt32 thisFlags = EnchantmentInfoEntry::kFlag_ManualCalc;
		SInt32 thisEnchantmentCost = entryData.enchantment->data.calculations.cost;
		EnchantmentInfoEntry thisEnchantmentInfo(thisFormID, thisFlags, thisEnchantmentCost);

		//Rebuild list of known base enchantments during first loop (in case player learned more while crafting)
		GetKnown(bFirstLoopPass);
		bFirstLoopPass = false;

		playerWeaponEnchants[entryData.enchantment] = thisEnchantmentInfo;
		//store new enchantments along with iterator pointing to their EnchantmentInfoEntry data so we can update it elsewhere.
		//(using a direct pointer to the EnchantmentInfoEntry is unsafe, will be invalidated when the map above gets added to or altered.
		newCraftedEnchantments.push_back(playerWeaponEnchants.find(entryData.enchantment)); //find() returns the iterator which will remain valid
	}
}

void PersistentWeaponEnchantments::PostCraftUpdate(EnchantmentInfoReferenceVec* &newEnchantments)
{
	PostCraftUpdate();
	newEnchantments = &newCraftedEnchantments;
}

void PersistentWeaponEnchantments::Reset()
{
	playerWeaponEnchants.clear();
	bInitialized = false;
}


EnchantmentItem* FindBaseEnchantment(MagEffVec mgefs, UInt32 deliveryType) //Base enchantment data is not stored on player-crafted enchantments
{
	//Locate known base enchantment with matching effect list
	EnchantmentVec* known = NULL;
	if (deliveryType == 0x01) //Weapon enchantment (delivery type: 'contact')
		known = &PersistentWeaponEnchantments::GetKnown()->knownWeaponBaseEnchantments;
	else if (deliveryType == 0x00) //Armor enchantment (delivery type: 'self')
		known = &PersistentWeaponEnchantments::GetKnown()->knownArmorBaseEnchantments;

	for (EnchantmentVec::iterator baseEnchIt = known->begin(); baseEnchIt != known->end(); ++baseEnchIt)
	{
		if ((*baseEnchIt)->effectItemList.count != mgefs.size()) //Compare effect list length
			continue;
		for (MagEffVec::iterator mgefIt = mgefs.begin(); mgefIt != mgefs.end(); ++mgefIt)
		{
			MagicItem::EffectItem* pBaseEnchEffectItem = NULL;
			EffectSetting* pBaseEnchMGEF = NULL;
			(*baseEnchIt)->effectItemList.GetNthItem(mgefIt - mgefs.begin(), pBaseEnchEffectItem);
			pBaseEnchMGEF = pBaseEnchEffectItem->mgef;
			if (pBaseEnchMGEF ? pBaseEnchMGEF != *mgefIt : true)
				break;
			if (mgefIt == (mgefs.end() - 1)) //Confirmed match
				return (*baseEnchIt);
		}
	}
	return NULL;
}


bool IsChaosDamageEffect(EffectSetting* mgef)
{
	static DataHandler* data = DataHandler::GetSingleton();
	static const char * dragonborn = "Dragonborn.esm";
	static UInt32 lowBoundFormID = (data) ? ((data->GetModIndex(dragonborn)) << 24) | 0x02C46B : 0;
	return (mgef) ? (mgef->formID >= lowBoundFormID) && (mgef->formID <= (lowBoundFormID + 0x02)) : false;
}

void FixIfChaosDamage(EnchantmentItem* pEnch) //Detect Chaos Damage and fix its non-scaling effects
{
	if (!pEnch || (pEnch->effectItemList.count < 3))
		return;

	EffectItemVec effects;

	for(UInt32 i = 0; i < pEnch->effectItemList.count; ++i)
	{
		MagicItem::EffectItem* pEffectItem = NULL;
		pEnch->effectItemList.GetNthItem(i, pEffectItem);
		if (pEffectItem && IsChaosDamageEffect(pEffectItem->mgef))
			effects.push_back(pEffectItem);
	}

	if (effects.size() != 3)
		return;
	
	//Technically this might not always be "correct" -- for instance, it ignores elemental Enchanting perks.
	if (effects[0]->magnitude == effects[1]->magnitude)
		effects[0]->magnitude = effects[1]->magnitude = effects[2]->magnitude;
	else if (effects[0]->magnitude == effects[2]->magnitude)
		effects[0]->magnitude = effects[2]->magnitude = effects[1]->magnitude;
	else if (effects[1]->magnitude == effects[2]->magnitude)
		effects[1]->magnitude = effects[2]->magnitude = effects[0]->magnitude;

	//Update new effect costs
	float newEnchantmentCost = 0.0;
	for (UInt32 i = 0; i < 3; ++i)
	{
		float thisCost = effects[i]->mgef->properties.baseCost * pow((effects[i]->magnitude * effects[i]->duration / 10.0), 1.1);
		effects[i]->cost = thisCost;
		newEnchantmentCost += thisCost;
	}

	//Although updating the cost would be technically correct, it also throws off the price of the enchanted item
	//compared to what was displayed at the enchanting table, which seems to go against the spirit of this patch.

	// pEnch->data.calculations.cost = (UInt32)newEnchantmentCost;
}