#include "EnchantmentInfo.h"
#include "skse/GameData.h"
#include "skse/GameObjects.h"


bool PersistentWeaponEnchantments::bInitialized = false;

KnownBaseEnchantments* PersistentWeaponEnchantments::GetKnown(const bool &reevaluate)
{
	static KnownBaseEnchantments known;
	if (reevaluate || !bInitialized)
	{
		bInitialized = true;
		known.clear();
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
				thisEnchantmentInfo.cData.inheritFormFormID = baseEnchant->formID;
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
	typedef std::vector<EffectSetting*> MagEffVec;
	MagEffVec mgefs(pEnch->effectItemList.count);

	//Insert MGEFs from passed enchantment into vector
	for(UInt32 i = 0; i < pEnch->effectItemList.count; ++i)
	{
		MagicItem::EffectItem* pEffectItem = NULL;
		EffectSetting* pMGEF = NULL;
		pEnch->effectItemList.GetNthItem(i, pEffectItem);
		pMGEF = pEffectItem->mgef;
		mgefs.push_back(pMGEF);
	}

	//Locate known base enchantment with matching effect list
	KnownBaseEnchantments* known = PersistentWeaponEnchantments::GetKnown();
	for (KnownBaseEnchantments::iterator baseEnchIt = known->begin(); baseEnchIt != known->end(); ++baseEnchIt)
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