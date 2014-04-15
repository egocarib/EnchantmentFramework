#include "ScaleformExtensions.h"
#include <sstream>


namespace
{
	EditEnchantment selectedEnchantment;
}

bool IsScalableArchetype(UInt32 arg)
{
	return (arg == kValueMod || arg == kPeakValueMod || arg == kDualValueMod || arg == kAbsorb ||
			arg == kBanish || arg == kScript || arg == kSoulTrap || arg == kTurnUndead || arg == kDemoralize ||
			arg == kParalysis || arg == kFrenzy || arg == kCalm || arg == kInvisibility || arg == kSlowTime);
}

bool IsScalableActorValue(UInt32 arg)
{
	return (arg != kSpeedMult && arg != kWeaponSpeedMult);
}

bool IsScalableEffect(EffectSetting* mgef)
{
	return (IsScalableArchetype(mgef->properties.archetype)
			&& IsScalableActorValue(mgef->properties.primaryValue)
			&& IsScalableActorValue(mgef->properties.secondaryValue));
}



void Scaleform_IdentifySelectedEnchantment::Invoke(Args* args)
{
	_MESSAGE("Scaleform_IdentifySelectedEnchantment::Invoke (arg = %s)", args->args[0].GetString());
	ASSERT(args->numArgs >= 1);

	const char* enchantmentName = args->args[0].GetString();
	KnownBaseEnchantments* known = PersistentWeaponEnchantments::GetKnown(REEVALUATE);
	EnchantmentItem* enchantment = known->LookupByName(enchantmentName);

	_MESSAGE("    (enchantment = %08X) effects: %u", (enchantment) ? enchantment->formID : 0, (enchantment) ? enchantment->effectItemList.count : 999);

	if (!enchantment || enchantment->effectItemList.count <= 1)
	{
		selectedEnchantment.enchantment = NULL;
		return;
	}

	selectedEnchantment.enchantment = enchantment;
	selectedEnchantment.effects.clear();
	selectedEnchantment.effectsInfo.clear();

	//Get costliest effect index and base value
	MagicItem::EffectItem* costliest = CALL_MEMBER_FN(enchantment, GetCostliestEffectItem)(5, false);
	selectedEnchantment.costliestEffectIndex = (costliest) ? enchantment->effectItemList.GetItemIndex(costliest) : 0;
	_MESSAGE("    costliest Effect Index = %u (%s)", selectedEnchantment.costliestEffectIndex, (costliest) ? "TRUE" : "FALSE");

	float costliestEffectBaseValue = 0.0;
	if ((costliest->mgef->properties.flags & kEffectFlag_Magnitude) == kEffectFlag_Magnitude)
		{costliestEffectBaseValue = costliest->magnitude; _MESSAGE("    kEffectFlag_Magnitude");}
	else if ((costliest->mgef->properties.flags & kEffectFlag_Duration) == kEffectFlag_Duration)
		{costliestEffectBaseValue = (float)costliest->duration; _MESSAGE("    kEffectFlag_Duration");}

	if (costliestEffectBaseValue == 0.0)
		return;

	selectedEnchantment.costliestEffectBaseValue = costliestEffectBaseValue;

	//Build effect vector in same order as effectItemList
	for(UInt32 i = 0; i < enchantment->effectItemList.count; ++i)
	{
		EditEnchantment::BaseInfo thisInfo;

		//Will indicate that we should ignore this effect
		thisInfo.scaleType = EditEnchantment::kScaleNothing;

		//Record info for secondary effects
		if (i == selectedEnchantment.costliestEffectIndex)
			selectedEnchantment.effects.push_back(costliest);
		else
		{
			_MESSAGE("    checking effect #%u", i);
			MagicItem::EffectItem* effectItem = NULL;
			enchantment->effectItemList.GetNthItem(i, effectItem);
			selectedEnchantment.effects.push_back(effectItem);

			if (effectItem && IsScalableEffect(effectItem->mgef))
			{
				_MESSAGE("    IsScalableEffect");
				//Only magnitude OR duration of the costliest MagicEffect will scale on an enchantment by
				//default, depending on that MagicEffect's flags. The "Power Affects Magnitude" flag takes
				//precedence. When that is absent, "Power Affects Duration" flag causes duration scaling.
				//If neither flag is checked in the MagicEffect, then no scaling of effect will occur.
				//Using the same logic, we will force scaling of secondary (non-costliest) Magic Effects.

				//Record whether we should scale magnitude or duration for each secondary effect:
				if ((effectItem->mgef->properties.flags & kEffectFlag_Magnitude) == kEffectFlag_Magnitude)
				{   _MESSAGE("      1");
					thisInfo.scaleType = EditEnchantment::kScaleMagnitude;
					thisInfo.scaleRatio = effectItem->magnitude / costliestEffectBaseValue;
				}
				else if ((effectItem->mgef->properties.flags & kEffectFlag_Duration) == kEffectFlag_Duration)
				{   _MESSAGE("      2");
					thisInfo.scaleType = EditEnchantment::kScaleDuration;
					thisInfo.scaleRatio = (float)effectItem->duration / costliestEffectBaseValue;
				}
  				 _MESSAGE("      3");
				//Record base effect values which will need to be restored after enchantment slider closes:
				thisInfo.baseMagnitude = effectItem->magnitude; _MESSAGE("    thisInfo.baseMagnitude = effectItem->magnitude  [%g]", effectItem->magnitude);
				thisInfo.baseDuration = effectItem->duration; _MESSAGE("    thisInfo.baseDuration = effectItem->duration  [%u]", effectItem->duration);
				thisInfo.baseEffectCost = effectItem->cost; _MESSAGE("    thisInfo.baseEffectCost = effectItem->cost  [%g]", effectItem->cost);
				thisInfo.baseMgefCost = effectItem->mgef->properties.baseCost; _MESSAGE("    thisInfo.baseMgefCost = effectItem->mgef->properties.baseCost  [%g]", effectItem->mgef->properties.baseCost);
			}
		}

		selectedEnchantment.effectsInfo.push_back(thisInfo);
	}
}


void ResetSecondaryEffectValues()
{
	_MESSAGE("ResetSecondaryEffectValues");
	for (UInt32 i = 0; i < selectedEnchantment.effects.size(); ++i)
	{
		if (selectedEnchantment.effectsInfo[i].scaleType == EditEnchantment::kScaleNothing)
			continue;
		
		selectedEnchantment.effects[i]->magnitude = selectedEnchantment.effectsInfo[i].baseMagnitude; _MESSAGE("    selectedEnchantment.effects[i]->magnitude = selectedEnchantment.effectsInfo[i].baseMagnitude  [%g]", selectedEnchantment.effectsInfo[i].baseMagnitude);
		selectedEnchantment.effects[i]->duration = selectedEnchantment.effectsInfo[i].baseDuration; _MESSAGE("    selectedEnchantment.effects[i]->duration = selectedEnchantment.effectsInfo[i].baseDuration  [%u]", selectedEnchantment.effectsInfo[i].baseDuration);
		selectedEnchantment.effects[i]->cost = selectedEnchantment.effectsInfo[i].baseEffectCost; _MESSAGE("    selectedEnchantment.effects[i]->cost = selectedEnchantment.effectsInfo[i].baseEffectCost  [%g]", selectedEnchantment.effectsInfo[i].baseEffectCost);
	}
}

void UpdateSecondaryEffectValues(float newVal)
{
	_MESSAGE("UpdateSecondaryEffectValues");
	for (UInt32 i = 0; i < selectedEnchantment.effects.size(); ++i)
	{
		if (selectedEnchantment.effectsInfo[i].scaleType == EditEnchantment::kScaleNothing)
			continue;
		else if (selectedEnchantment.effectsInfo[i].scaleType == EditEnchantment::kScaleMagnitude)
			selectedEnchantment.effects[i]->magnitude = newVal * selectedEnchantment.effectsInfo[i].scaleRatio;
		else //scaleType == kScaleDuration
			selectedEnchantment.effects[i]->duration = (UInt32)(newVal * selectedEnchantment.effectsInfo[i].scaleRatio);

		//Update effect cost using game formula
		//TEMPORARILY DISABLED:
		// float mag = (selectedEnchantment.effects[i]->magnitude > 1.0) ? (selectedEnchantment.effects[i]->magnitude) : 1.0;
		// float dur = (selectedEnchantment.effects[i]->duration > 0) ? (float)(selectedEnchantment.effects[i]->duration) : 10.0;
		// selectedEnchantment.effects[i]->cost = selectedEnchantment.effectsInfo[i].baseMgefCost * pow((mag * dur / 10.0), 1.1);
	}
}

void Scaleform_UpdateSecondaryEffectValues::Invoke(Args* args)
{
	// const char* charNum = args->args[0].GetString();
	// SInt32 num = 0;
	// std::istringstream in(charNum);
	// in >> num;

	SInt32 num = (SInt32)args->args[0].GetNumber();

	_MESSAGE("Scaleform_UpdateSecondaryEffectValues::Invoke (arg = %d)", num);
	if (!selectedEnchantment.enchantment)
		return;


	_MESSAGE("Scaleform_UpdateSecondaryEffectValues::Invoke cntd");


	ASSERT(args->numArgs >= 1);



	//float numArg = args->args[0].GetNumber();

	if (num < 0.0) //Will receive -1 when slider is closed
		ResetSecondaryEffectValues();
	else
		UpdateSecondaryEffectValues(num);
}