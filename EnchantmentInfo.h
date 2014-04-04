#pragma once

#include "skse/GameData.h"
#include "skse/GameObjects.h"
#include "skse/GameRTTI.h"
#include <vector>
#include <map>


class EnchantmentInfoEntry
{
public:
	enum { kFlagManualCalc = 0x01 };

	EnchantmentInfoEntry(UInt32 idArg = 0, UInt32 flagArg = 0, SInt32 costArg = -1)
		: formID(idArg), flags(flagArg), enchantmentCost(costArg), cData() {}

	struct ConditionData
	{
		bool 	hasConditions;
		UInt32	inheritFormFormID; //(base enchantment) will need to double-check in case mod removes conditions from inheriting form.
	};

	UInt32			formID;
	UInt32			flags;
	SInt32			enchantmentCost;
	ConditionData	cData;
};


typedef std::vector <EnchantmentItem*>						EnchantmentVec;
typedef std::map <EnchantmentItem*, EnchantmentInfoEntry>	EnchantmentInfoMap;

class KnownBaseEnchantments : public EnchantmentVec
{
public:
	bool Accept(EnchantmentItem* pEnch)
	{
		TESForm* pForm = DYNAMIC_CAST(pEnch, EnchantmentItem, TESForm);
		if (pForm && ((pForm->flags & TESForm::kFlagPlayerKnows) == TESForm::kFlagPlayerKnows))
			(*this).push_back(pEnch);
	}
};


class PersistentWeaponEnchantments : public EnchantmentInfoMap
{
public:
	static KnownBaseEnchantments* GetKnown(const bool &reevaluate = false);
	void Update();
	void Reset();

private:
  	static bool bInitialized;
};


class EnchantmentDataHandler
{
public:
	template <class Visitor>
	static void Visit(Visitor* visitor)
	{
		static DataHandler* data = DataHandler::GetSingleton();
		bool bContinue = true;
		for(UInt32 i = 0; (i < data->enchantments.count) && bContinue; i++)
		{
			EnchantmentItem* pEnch = NULL;
			data->enchantments.GetNthItem(i, pEnch);
			if (pEnch)
				bContinue = visitor->Accept(pEnch);
		}
	}

private:
	EnchantmentDataHandler() {}
};


namespace EnchantmentInfoLib
{
	EnchantmentItem* FindBaseEnchantment(EnchantmentItem* pEnch);
}