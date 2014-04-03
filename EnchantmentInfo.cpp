#include "EnchantmentInfo.h"
#include "skse/GameData.h"
#include "skse/GameForms.h"
#include "skse/GameObjects.h"
#include "skse/GameRTTI.h"

using namespace EnchantmentInfoLib;

//these two will no longer be used soon, replacing with classes below:
EnchantmentInfoMap	EnchantmentInfoLib::_playerEnchantments;
EnchantmentVec		EnchantmentInfoLib::_knownBaseEnchantments;


//KnownBaseEnchants I don't think I need to keep around - b/c have to parse the whole list again anyway.
//playerEnchantments, though, is better to keep so I can parse only the difference and update info
//(like condition data, cost, etc) only for the new player enchantments after menu close.
//also - I think I should just ignore any auto-Calc player enchants now, since i cant fix them;
//all new enchants will be manual. So no need to save them or anything. (unless for conditions?)
class KnownBaseEnchantments : public EnchantmentInfoLib::EnchantmentVec
{
  public:
	bool Accept(EnchantmentItem* pEnch)
	{
		TESForm* pForm = DYNAMIC_CAST(pEnch, EnchantmentItem, TESForm);
		if (pForm && ((pForm->flags & TESForm::kFlagPlayerKnows) == TESForm::kFlagPlayerKnows))
			(*this).push_back(pEnch);
	}

	KnownBaseEnchantments() { EnchantmentDataHandler().Visit(this); }
	//KnownBaseEnchantments(EnchantmentDataHandler* enchantments) { enchantments->Visit(this); }
};


class PersistentFormWeaponEnchantments : public EnchantmentInfoLib::EnchantmentInfoMap
{
  public:
	static KnownBaseEnchantments* GetKnown(bool reevaluate = false)
	{
		static KnownBaseEnchantments known;
		static bool bStarted = false;
		if (bStarted)
			return (reevaluate) ? &(known = KnownBaseEnchantments()) : &known;
		else bStarted = true;
		return &known;
	}

	void Update()
	{
		//search persistentForms, ignoring any enchantments already recorded
		PersistentFormManager* pPFM = PersistentFormManager::GetSingleton();
		bool bFirst = true;
		for(UInt32 i = 0; i < pPFM->weaponEnchants.count; i++)
		{
			PersistentFormManager::EnchantData entryData;
			pPFM->weaponEnchants.GetNthItem(i, entryData);
			if (!entryData.enchantment)
				continue;
			else if (!(entryData.enchantment->data.unk00.unk04 & EnchantmentInfoEntry::kFlagManualCalc)) //if auto-calc, happened before this plugin (or via papyrus CreateEnchantment)
				continue;
			else if (this->find(entryData.enchantment) != this->end()) //already added to map
				continue;

			UInt32 thisFormID = entryData.enchantment->formID;
			UInt32 thisFlags = EnchantmentInfoEntry::kFlagManualCalc; //can probably delete this flag from struct, not needed anymore.
			SInt32 thisEnchantmentCost = entryData.enchantment->data.unk00.unk00;
			EnchantmentInfoEntry thisEnchantmentInfo(thisFormID, thisFlags, thisEnchantmentCost);

			KnownBaseEnchantments* knownBaseEnchants = GetKnown(bFirst); //force re-evaluate the first time through the loop (necessary before calling FindBaseEnchantment)
			bFirst = false;

			EnchantmentItem* baseEnchant = FindBaseEnchantment(entryData.enchantment);
			if (baseEnchant)
			{
				for (UInt32 j = 0; j < baseEnchant->effectItemList.count; ++j)
				{
					MagicItem::EffectItem* pEffectItem = NULL;
					baseEnchant->effectItemList.GetNthItem(j, pEffectItem);
					if (pEffectItem && pEffectItem->condition)
					{
						//NEED TO ACTUALLY APPLY THE NEW CONDITIONS TO THE ENCHANTMENT HERE TOO
						thisEnchantmentInfo.cData.hasConditions = true;
						thisEnchantmentInfo.cData.inheritFormFormID = baseEnchant->formID;
						break;
					}
				}
			}

			(*this)[entryData.enchantment] = thisEnchantmentInfo;
		}
	}

	void Reset()
	{
		//for new revert/load, clear map and rebuild
		//does this need to be integrated into the Serialization_Load method?
		//probably not, just reset here and read persistentForms, then overwrite from loaded data
	}

	PersistentFormWeaponEnchantments() {/*Do Stuff*/}
};


EnchantmentItem* EnchantmentInfoLib::FindBaseEnchantment(EnchantmentItem* pEnch) //base enchantment data is not stored on player-crafted enchantments
{
	typedef std::vector<EffectSetting*> MagEffVec;
	MagEffVec mgefs(pEnch->effectItemList.count);

	//insert mgefs from target player enchantment into vector
	for(UInt32 i = 0; i < pEnch->effectItemList.count; ++i)
	{
		MagicItem::EffectItem* pEffectItem = NULL;
		EffectSetting* pMGEF = NULL;
		pEnch->effectItemList.GetNthItem(i, pEffectItem);
		pMGEF = pEffectItem->mgef;
		mgefs.push_back(pMGEF);
	}

	//locate known base enchantment with same mgefs
	KnownBaseEnchantments* known = PersistentFormWeaponEnchantments::GetKnown();
	for (KnownBaseEnchantments::iterator baseEnchIt = known->begin(); baseEnchIt != known->end(); ++baseEnchIt)
	{
		for (MagEffVec::iterator mgefIt = mgefs.begin(); mgefIt != mgefs.end(); ++mgefIt)
		{
			MagicItem::EffectItem* pBaseEnchEffectItem = NULL;
			EffectSetting* pBaseEnchMGEF = NULL;
			(*baseEnchIt)->effectItemList.GetNthItem(mgefIt - mgefs.begin(), pBaseEnchEffectItem);
			pBaseEnchMGEF = pBaseEnchEffectItem->mgef;
			if (pBaseEnchMGEF ? pBaseEnchMGEF != *mgefIt : true)
				break;
			if (mgefIt == (mgefs.end() - 1))
				return (*baseEnchIt);
		}
	}
	return NULL;
}




// bool EnchantmentInfoLib::BuildPersistentFormsEnchantmentMap()
// {
// 	_playerEnchantments.clear();

// 	PersistentFormManager* pPersistentForms = PersistentFormManager::GetSingleton();
// 	for(UInt32 i = 0; i < pPersistentForms->weaponEnchants.count; i++)
// 	{
// 		EnchantmentInfoUnion eU = GetNthPersistentEnchantmentInfo(pPersistentForms, i);
// 		if (eU.enchantment)
// 		{
// 			_playerEnchantments[eU.enchantment] = eU.entry;
// 			_MESSAGE("Read data from PersistentFormManager: [Enchantment: 0x%08X] [Flags: %s] [Cost: %d]"
// 				,eU.entry.formID
// 				,eU.entry.flags ? "MANUAL" : "AUTO"
// 				,eU.entry.enchantmentCost);
// 		}
// 	}
// 	return (_playerEnchantments.size() > 0);
// }




// EnchantmentInfoUnion EnchantmentInfoLib::GetNthPersistentEnchantmentInfo(PersistentFormManager* pPFM, UInt32 idx)
// {
// 	PersistentFormManager::EnchantData entryData;
// 	pPFM->weaponEnchants.GetNthItem(idx, entryData);
// 	if (!entryData.enchantment)
// 		return EnchantmentInfoUnion(NULL);
// 	else if (!(entryData.enchantment->data.unk00.unk04 & EnchantmentInfoEntry::kFlagManualCalc)) //if auto-calc, happened before this plugin (or via papyrus CreateEnchantment)
// 		return EnchantmentInfoUnion(NULL);

// 	UInt32 thisFormID = entryData.enchantment->formID;
// 	UInt32 thisFlags = EnchantmentInfoEntry::kFlagManualCalc;
// 	SInt32 thisEnchantmentCost = entryData.enchantment->data.unk00.unk00;

// 	EnchantmentInfoEntry thisEnchantmentInfo(thisFormID, thisFlags, thisEnchantmentCost);
// 	return EnchantmentInfoUnion(entryData.enchantment, thisEnchantmentInfo);
// }