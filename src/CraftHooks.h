#pragma once

#include "skse/GameObjects.h"
#include <queue>

class EnchantmentItem;
class MagicItem;


class EnchantCraftMonitor
{
private:
	EnchantmentItem*				stagedNewEnchantment;
	UInt32							stagedNewEnchantmentType;
	std::queue<EnchantmentItem*>	stagedBaseEnchantments;
	bool 							isCommitted;

public:
	enum
	{
		kEnchantmentType_Armor,
		kEnchantmentType_Weapon
	};

	EnchantCraftMonitor() : stagedNewEnchantment(NULL), stagedBaseEnchantments(), isCommitted(false) {}

	void Push(EnchantmentItem* enchantment)
	{
		if (isCommitted)
		{	//Clear the queue
			isCommitted = false;
			stagedBaseEnchantments = std::queue<EnchantmentItem*>();
		}
		stagedBaseEnchantments.push(enchantment);
	}

	EnchantmentItem* Pop()
	{
		if (stagedBaseEnchantments.empty())
			return NULL;
		EnchantmentItem* enchantment = stagedBaseEnchantments.front();
		stagedBaseEnchantments.pop();
		return enchantment;
	}

	void Commit(EnchantmentItem* enchantment, UInt32 type)
	{
		stagedNewEnchantment = enchantment;
		stagedNewEnchantmentType = type;
		isCommitted = true;
	}

	EnchantmentItem* GetStagedNewEnchantment() { return stagedNewEnchantment; }
	UInt32 GetStagedNewEnchantmentType() { return stagedNewEnchantmentType; }

	template <class Visitor>
	void Visit(Visitor* visitor)
	{
		while (EnchantmentItem* e = this->Pop())
			if (!visitor->Accept(e))
				break;
		stagedNewEnchantment = NULL;
	}
};


class GetCostliestEffectItemHook : public MagicItem
{
public:
	MagicItem::EffectItem* CostliestEffect_Hook(UInt32 arg1, bool arg2);
	static void Hook_Commit(void);
};


extern	EnchantCraftMonitor		g_craftData;

void CreateEnchantmentHook_Commit(void);
