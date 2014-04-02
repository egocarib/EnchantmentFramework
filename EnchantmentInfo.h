#pragma once

#include "skse/GameData.h"
#include "skse/GameObjects.h"
#include <vector>
#include <list>
#include <map>

struct EnchantmentInfoEntry
{
	enum
	{
		kFlagManualCalc = 0x01
	};

	UInt32	formID;
	UInt32	flags;
	SInt32	enchantmentCost;

	EnchantmentInfoEntry(UInt32 a1 = 0, UInt32 a2 = 0, SInt32 a3 = -1) : formID(a1), flags(a2), enchantmentCost(a3) {}
};

struct EnchantmentInfoUnion
{
	EnchantmentItem*		enchantment;
	EnchantmentInfoEntry	entry;

	EnchantmentInfoUnion(EnchantmentItem* a1, EnchantmentInfoEntry a2) : enchantment(a1), entry(a2) {}
};

class EnchantmentDataHandler
{
  private:
	DataHandler* data;

  public:
	template <class Visitor>
	void Visit(Visitor* visitor)
	{
		bool bContinue = true;
		for(UInt32 i = 0; (i < data->enchantments.count) && bContinue; i++)
		{
			EnchantmentItem* pEnch = NULL;
			data->enchantments.GetNthItem(i, pEnch);
			if (pEnch)
				bContinue = visitor->Accept(pEnch);
		}
	}

	EnchantmentDataHandler() : data(DataHandler::GetSingleton()) {}
};


namespace EnchantmentInfoLib
{
	EnchantmentInfoUnion GetNthPersistentEnchantmentInfo(PersistentFormManager* pPFM, UInt32 idx);


	typedef std::vector <EnchantmentItem*>						EnchantmentVec;
	//typedef std::list <MagicItem::EffectItem*>					LinkedEffectList; //depricated, replaced by ConditionedEffectMap
	typedef std::map <MagicItem::EffectItem*, Condition*>		ConditionedEffectMap;
	typedef std::map <EnchantmentItem*, EnchantmentInfoEntry>	EnchantmentInfoMap;
	//typedef std::map <EnchantmentItem*, LinkedEffectList>		EnchantmentEffectMap; //depricated, replaced by EnchantmentConditionMap
	typedef std::map <EnchantmentItem*, ConditionedEffectMap>		EnchantmentConditionMap;

	extern EnchantmentInfoMap		_playerEnchantments;
	extern EnchantmentVec			_knownBaseEnchantments;

	bool BuildKnownBaseEnchantmentVec();
	EnchantmentVec* BuildFullWeaponEnchantmentsList();
	bool BuildPersistentFormsEnchantmentMap();
	EnchantmentItem* FindBaseEnchantment(EnchantmentItem* pEnch);
	void RunFirstLoadEnchantmentFix();
}