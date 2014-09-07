#pragma once

#include "EnchantmentInfo.h"
#include "MenuHandler.h"


class EnchantmentDataPluginInterface
{
	void* GetMenuHandler()			{ return &menu; }
	void* GetEnchantmentTracker()	{ return &enchantTracker; }
};

EnchantmentDataPluginInterface g_enchantmentDataInterface;