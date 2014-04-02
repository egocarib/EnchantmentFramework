#include "EnchantmentInfo.h"
#include "skse/GameData.h"
#include "skse/GameForms.h"
#include "skse/GameObjects.h"
#include "skse/GameRTTI.h"

using namespace EnchantmentInfoLib;

EnchantmentInfoMap	EnchantmentInfoLib::_playerEnchantments;
EnchantmentVec		EnchantmentInfoLib::_knownBaseEnchantments;


EnchantmentInfoUnion EnchantmentInfoLib::GetNthPersistentEnchantmentInfo(PersistentFormManager* pPFM, UInt32 idx)
{
	PersistentFormManager::EnchantData entryData;
	pPFM->weaponEnchants.GetNthItem(idx, entryData);
	if (!entryData.enchantment)
		return EnchantmentInfoUnion(NULL, EnchantmentInfoEntry());

	UInt32 thisFormID = entryData.enchantment->formID;
	UInt32 thisFlags = entryData.enchantment->data.unk00.unk04 & EnchantmentInfoEntry::kFlagManualCalc;
	SInt32 thisEnchantmentCost = entryData.enchantment->data.unk00.unk00;

	EnchantmentInfoEntry thisEnchantmentInfo(thisFormID, thisFlags, thisEnchantmentCost);
	return EnchantmentInfoUnion(entryData.enchantment, thisEnchantmentInfo);
}


bool EnchantmentInfoLib::BuildKnownBaseEnchantmentVec()
{
	_knownBaseEnchantments.clear();

	DataHandler* dh = DataHandler::GetSingleton();
	for(UInt32 i = 0; i < dh->enchantments.count; i++)
	{
		EnchantmentItem* pEI = NULL;
		dh->enchantments.GetNthItem(i, pEI);
		TESForm* pForm = DYNAMIC_CAST(pEI, EnchantmentItem, TESForm);
		if (pForm)
		{
			bool playerKnows = ((pForm->flags & TESForm::kFlagPlayerKnows) == TESForm::kFlagPlayerKnows);
			if (playerKnows && (pForm->formID != 0x000FC05B)) // exclude EnchArmorResistMagic01, so we don't use it for calculations (shouldn't be knowable, vanilla error)
				_knownBaseEnchantments.push_back(pEI);

			// {
			// 	MagicItem::EffectItem* pFirstEffect = NULL;
			// 	pEI->effectItemList.GetNthItem(0, pFirstEffect);
			// 	if (pFirstEffect)
			// 		_knownBaseEnchantments[pEI] = pFirstEffect->mgef;

				//DEBUG ONLY:
				 // TESFullName* pFN = DYNAMIC_CAST(pEI, EnchantmentItem, TESFullName);
				 // TESFullName* pFN2 = DYNAMIC_CAST(_knownBaseEnchantments[pEI], EffectSetting, TESFullName);
				 // _MESSAGE("    KnownEnchantment[%3u]:  %s (0x%08X) MGEF = %s", i, pFN ? pFN->name.data : "NO NAME", pEI->formID, pFN2 ? pFN2->name.data : "NO NAME");
			// }
			// else
			// {
			// 	 TESFullName* pFN = DYNAMIC_CAST(pEI, EnchantmentItem, TESFullName);
			// 	 TESFullName* pFN2 = DYNAMIC_CAST(_knownBaseEnchantments[pEI], EffectSetting, TESFullName);
			// 	 _MESSAGE("    UnknownEnchantment[%3u]:  %s (0x%08X) MGEF = %s", i, pFN ? pFN->name.data : "NO NAME", pEI->formID, pFN2 ? pFN2->name.data : "NO NAME");
			// }
		}
	}
	return (_knownBaseEnchantments.size() > 0);
}



bool EnchantmentInfoLib::BuildPersistentFormsEnchantmentMap()
{
	_playerEnchantments.clear();

	PersistentFormManager* pPersistentForms = PersistentFormManager::GetSingleton();
	for(UInt32 i = 0; i < pPersistentForms->weaponEnchants.count; i++)
	{
		EnchantmentInfoUnion eU = GetNthPersistentEnchantmentInfo(pPersistentForms, i);
		if (eU.enchantment)
		{
			_playerEnchantments[eU.enchantment] = eU.entry;
			_MESSAGE("Read data from PersistentFormManager: [Enchantment: 0x%08X] [Flags: %s] [Cost: %d]"
				,eU.entry.formID
				,eU.entry.flags ? "MANUAL" : "AUTO"
				,eU.entry.enchantmentCost);
		}
	}
	return (_playerEnchantments.size() > 0);
}


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
	for (EnchantmentVec::iterator baseEnchIt = _knownBaseEnchantments.begin(); baseEnchIt != _knownBaseEnchantments.end(); ++baseEnchIt)
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

