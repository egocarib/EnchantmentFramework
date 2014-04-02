#pragma once

#include "skse/GameData.h"
#include "skse/GameObjects.h"
#include <vector>
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


namespace EnchantmentLib
{
	EnchantmentInfoUnion GetNthPersistentEnchantmentInfo(PersistentFormManager* pPFM, UInt32 idx);

	typedef std::map <EnchantmentItem*, EnchantmentInfoEntry>	EnchantmentInfoMap;
	typedef std::vector <EnchantmentItem*>	KnownEnchantmentsVec;

	EnchantmentInfoMap		_playerEnchantments;
	KnownEnchantmentsVec	_knownBaseEnchantments;

	bool BuildKnownBaseEnchantmentVec();
	bool BuildPersistentFormsEnchantmentMap();
	EnchantmentItem* FindBaseEnchantment(EnchantmentItem* pEnch);
	void RunFirstLoadEnchantmentFix();
}