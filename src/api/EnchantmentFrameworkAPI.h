#pragma once

#include <vector>

class EnchantmentItem;


struct EnchantmentFrameworkInterface
{
	enum { kInterfaceVersion = 1 };

	UInt32	version;

	//These functions currently only work for weapon enchantments. May add armor enchantment support later.
	std::vector<EnchantmentItem*>	(* GetAllCraftedEnchantments)(); //get all player-created persistent (weapon) enchantments
	std::vector<EnchantmentItem*>	(* GetCraftedEnchantmentParents)(EnchantmentItem* customEnchantment); //get base enchantments that were combined to craft this custom (weapon) enchantment
};