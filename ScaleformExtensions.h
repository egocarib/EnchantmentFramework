#include "skse/ScaleformCallbacks.h"
#include "EnchantmentInfo.h"


class Scaleform_IdentifySliderEnchantment : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args* args)
	{
		ASSERT(args->numArgs >= 1);

		const char* enchantmentName = args->args[0].GetString();
		KnownBaseEnchantments* known = PersistentWeaponEnchantments::GetKnown(REEVALUATE);
		EnchantmentItem* enchantment = known->LookupByName(enchantmentName);
	}
};


class Scaleform_SetSecondaryEffectValues : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args* args)
	{
		ASSERT(args->numArgs >= 1);
		_MESSAGE("SetSecondaryEffectValues call received: %s", args->args[0].GetString());
	}
};