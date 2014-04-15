#pragma once

#include "skse/ScaleformCallbacks.h"
#include "../EnchantmentInfo.h"


//Scalable effect archetypes
enum MgefArchetypes
{
	kValueMod     = 0,
	kScript       = 1,
	kAbsorb       = 4,
	kDualValueMod = 5,
	kCalm         = 6,
	kDemoralize   = 7,
	kFrenzy       = 8,
	kInvisibility = 11,
	kParalysis    = 21,
	kSoulTrap     = 23,
	kTurnUndead   = 24,
	kPeakValueMod = 34,
	kSlowTime     = 37,
	kBanish       = 42
};

//Avoid scaling these AVs when found on
//secondary effects (potentially buggy)
enum ActorValues
{
	kSpeedMult       = 30,
	kWeaponSpeedMult = 85
};

//Determine whether safe to scale the effect
bool IsScalableEffect(EffectSetting* mgef);


enum MgefScalingFlags
{
	kEffectFlag_Magnitude = EffectSetting::Properties::kEffectType_Magnitude,
	kEffectFlag_Duration = EffectSetting::Properties::kEffectType_Duration
};


struct EditEnchantment
{
	enum ScaleType
	{
		kScaleNothing,
		kScaleMagnitude,
		kScaleDuration
	};

	struct BaseInfo
	{
		ScaleType	scaleType;

		//Ratio of this effect's scaling base value to
		//the costliest effect's scaling base value
		float		scaleRatio;

		//Base data (to restore after enchant)
		float		baseMagnitude;
		UInt32		baseDuration;
		float       baseMgefCost;
		float		baseEffectCost;
	};

	typedef std::vector<BaseInfo> BaseInfoVec;

	EnchantmentItem*	enchantment;
	EffectItemVec		effects;
	BaseInfoVec			effectsInfo;
	float				costliestEffectBaseValue;
	UInt32				costliestEffectIndex;
};


class Scaleform_IdentifySelectedEnchantment : public GFxFunctionHandler //IdentifySliderEnchantment
{
public:
	virtual void Invoke(Args* args);
};


class Scaleform_UpdateSecondaryEffectValues : public GFxFunctionHandler
{
public:
	virtual void Invoke(Args* args);
};

class ResetCurrentEffect {}; //or just do this from above, with parameter of -1 or something.


// };