#pragma once

#include "EnchantmentInfo.h"


class EnchantmentDataPluginInterface
{
	void* GetEnchantmentTracker()	{ return &g_enchantTracker; }
};

EnchantmentDataPluginInterface	g_enchantmentDataInterface;