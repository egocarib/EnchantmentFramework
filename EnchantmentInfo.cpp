#include "EnchantmentInfo.h"
#include "skse/GameData.h"
#include "skse/GameObjects.h"


bool PersistentWeaponEnchantments::bInitialized = false;


EnchantmentItem* KnownBaseEnchantments::LookupByName(const char* targetName)
{
	for (EnchantmentVec::iterator it = knownWeaponBaseEnchantments.begin(); it != knownWeaponBaseEnchantments.end(); ++it)
		if (strcmp(targetName, (DYNAMIC_CAST((*it), EnchantmentItem, TESFullName))->name.data) == 0)
			return (*it);

	for (EnchantmentVec::iterator it = knownArmorBaseEnchantments.begin(); it != knownArmorBaseEnchantments.end(); ++it)
		if (strcmp(targetName, (DYNAMIC_CAST((*it), EnchantmentItem, TESFullName))->name.data) == 0)
			return (*it);

	return NULL;
}

void KnownBaseEnchantments::Reset()
{
	knownWeaponBaseEnchantments.clear();
	knownArmorBaseEnchantments.clear();
}


KnownBaseEnchantments* PersistentWeaponEnchantments::GetKnown(const bool &reevaluate)
{
	static KnownBaseEnchantments known;
	if (reevaluate || !bInitialized)
	{
		bInitialized = true;
		known.Reset();
		EnchantmentDataHandler::Visit(&known);
	}
	return &known;
}

void PersistentWeaponEnchantments::Update()
{
	PersistentFormManager* pPFM = PersistentFormManager::GetSingleton();
	bool bFirstLoopPass = true;
	for(UInt32 i = 0; i < pPFM->weaponEnchants.count; i++)
	{
		PersistentFormManager::EnchantData entryData;
		pPFM->weaponEnchants.GetNthItem(i, entryData);
		if (!entryData.enchantment)
			continue;
		else if (!(entryData.enchantment->data.unk00.unk04 & EnchantmentInfoEntry::kFlagManualCalc))
			continue; //Ignore Auto-Calc enchants (they are created before this plugin or via papyrus CreateEnchantment)
		else if (this->find(entryData.enchantment) != this->end())
			continue; //Already added to map

		FixIfChaosDamage(entryData.enchantment);

		UInt32 thisFormID = entryData.enchantment->formID;
		UInt32 thisFlags = EnchantmentInfoEntry::kFlagManualCalc;
		SInt32 thisEnchantmentCost = entryData.enchantment->data.unk00.unk00;
		EnchantmentInfoEntry thisEnchantmentInfo(thisFormID, thisFlags, thisEnchantmentCost);

		//Rebuild list of known base enchantments during first loop
		GetKnown(bFirstLoopPass);
		bFirstLoopPass = false;

		//Inherit conditions from base enchantment, if any
		EnchantmentItem* baseEnchant = FindBaseEnchantment(entryData.enchantment);
		if (baseEnchant)
		{
			for (UInt32 j = 0; j < baseEnchant->effectItemList.count; ++j)
			{
				MagicItem::EffectItem* pEffectItem = NULL;
				baseEnchant->effectItemList.GetNthItem(j, pEffectItem);
				if (pEffectItem && pEffectItem->condition)
				{
					thisEnchantmentInfo.cData.hasConditions = true;
					MagicItem::EffectItem* pNew = NULL;
					entryData.enchantment->effectItemList.GetNthItem(j, pNew);
					pNew->condition = pEffectItem->condition;
				}
			}
			if (thisEnchantmentInfo.cData.hasConditions)
				thisEnchantmentInfo.cData.parentFormID = baseEnchant->formID;
		}

		(*this)[entryData.enchantment] = thisEnchantmentInfo;
	}
}

void PersistentWeaponEnchantments::Reset()
{
	clear();
	bInitialized = false;
}


EnchantmentItem* FindBaseEnchantment(EnchantmentItem* pEnch) //Base enchantment data is not stored on player-crafted enchantments
{
	MagEffVec mgefs;

	//Insert MGEFs from passed enchantment into vector
	for(UInt32 i = 0; i < pEnch->effectItemList.count; ++i)
	{
		MagicItem::EffectItem* pEffectItem = NULL;
		pEnch->effectItemList.GetNthItem(i, pEffectItem);
		mgefs.push_back(pEffectItem->mgef);
	}

	//Locate known base enchantment with matching effect list
	EnchantmentVec* known = NULL;
	if (pEnch->data.unk10 == 0x01) //Weapon enchantment (delivery type: 'contact')
		known = &PersistentWeaponEnchantments::GetKnown()->knownWeaponBaseEnchantments;
	else
		known = &PersistentWeaponEnchantments::GetKnown()->knownArmorBaseEnchantments;

	for (EnchantmentVec::iterator baseEnchIt = known->begin(); baseEnchIt != known->end(); ++baseEnchIt)
	{
		if ((*baseEnchIt)->effectItemList.count != pEnch->effectItemList.count) //Compare effect list length
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

	//I've decided _NOT_ to update the enchantment cost right now. The reason being, it completely throws off
	//the price of the item, making it much higher than displayed at the enchanting table. Even thhough this is
	//technically the correct price, it kind of goes against the spirit of the fix to make items have different
	//price & charges than are displayed at the enchanting table. Maybe eventually, that can be fixed too.
	
	// pEnch->data.unk00.unk00 = (UInt32)newEnchantmentCost;
}