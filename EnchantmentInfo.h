#pragma once

#include "skse/GameData.h"
#include "skse/GameObjects.h"
#include "skse/GameRTTI.h"
#include <vector>
#include <map>


typedef std::vector<EffectSetting*> 		MagEffVec;
typedef std::vector<MagicItem::EffectItem*> EffectItemVec;


class EnchantmentInfoEntry
{
public:
	enum { kFlagManualCalc = 0x01 };

	EnchantmentInfoEntry(UInt32 idArg = 0, UInt32 flagArg = 0, SInt32 costArg = -1)
		: formID(idArg), flags(flagArg), enchantmentCost(costArg), cData() {}

	struct ConditionData
	{
		bool 	hasConditions;
		UInt32	parentFormID; //Base enchantment that conditions should be inherited from
	};

	UInt32			formID;
	UInt32			flags;
	SInt32			enchantmentCost;
	ConditionData	cData;
};


typedef std::vector <EnchantmentItem*>						EnchantmentVec;
typedef std::map <EnchantmentItem*, EnchantmentInfoEntry>	EnchantmentInfoMap;

class KnownBaseEnchantments
{
public:
	bool Accept(EnchantmentItem* pEnch)
	{
		TESForm* pForm = DYNAMIC_CAST(pEnch, EnchantmentItem, TESForm);
		if (pForm && ((pForm->flags & TESForm::kFlagPlayerKnows) == TESForm::kFlagPlayerKnows))
		{
			if (pEnch->data.unk10 == 0x01) //Delivery type: 'contact'
				knownWeaponBaseEnchantments.push_back(pEnch);
			else
				knownArmorBaseEnchantments.push_back(pEnch);
		}
		return true;
	}

	//Find known base enchantment from targetName:
	EnchantmentItem* LookupByName(const char* targetName, char hint = 'w');
	//Clear vectors:
	void Reset();

	EnchantmentVec knownWeaponBaseEnchantments;
	EnchantmentVec knownArmorBaseEnchantments;

	typedef std::map<const char*, EnchantmentItem*> NamedEnchantMap;
	static NamedEnchantMap discoveredEnchantments;
};


#define INVALIDATE true

class PersistentWeaponEnchantments : public EnchantmentInfoMap
{
public:
	//Get all base enchantments currently known by the player:
	static KnownBaseEnchantments* GetKnown(const bool &invalidate = false);
	//Record new custom enchantments crafted at the enchantment table:
	void Update();
	//Reset info for new game or new save load:
	void Reset();

private:
  	static bool bInitialized;
};


class EnchantmentDataHandler //Exposes list of all loaded enchantment forms
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


EnchantmentItem* FindBaseEnchantment(EnchantmentItem* pEnch);

bool IsChaosDamageEffect(EffectSetting* mgef);

void FixIfChaosDamage(EnchantmentItem* pEnch);

extern PersistentWeaponEnchantments		enchantTracker;