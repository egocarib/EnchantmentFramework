#include "EnchantmentInfo.h"
#include "MenuHandler.h"
#include "CraftHooks.h"


PersistentWeaponEnchantments	g_enchantTracker;


void EnchantmentInfoEntry::EvaluateConditions()
{
	//Inherit conditions from parent enchantments
	EnchantmentItem* thisEnchant = DYNAMIC_CAST(LookupFormByID(formID), TESForm, EnchantmentItem);
	for (UInt32 enchantNum = 0, effectNum = 0; enchantNum < parentForms.data.size(); enchantNum++)
	{
		EnchantmentItem* parentEnchant = DYNAMIC_CAST(LookupFormByID(parentForms.data[enchantNum]), TESForm, EnchantmentItem);

		if (!g_weaponEnchantmentConditions.HasIndexed(parentEnchant))
		{
			effectNum += parentEnchant->effectItemList.count;
			continue;
		}
		else //Parent has conditions
		{
			attributes.hasConditions = true;
			for (UInt32 i = 0; i < parentEnchant->effectItemList.count; i++)
			{
				MagicItem::EffectItem* parentEffectItem = NULL;
				parentEnchant->effectItemList.GetNthItem(i, parentEffectItem);
				if (parentEffectItem)
				{
					MagicItem::EffectItem* pNew = NULL;
					thisEnchant->effectItemList.GetNthItem(effectNum, pNew);
					pNew->unk14 = reinterpret_cast<UInt32>(g_weaponEnchantmentConditions.GetCondition(parentEnchant, parentEffectItem));
					// (weirdly enough, unlike the serialization load method, this
					//  doesn't cause any problems... I can reload the game many
					//  times and the condition stays valid and doesn't cause a crash)
				}
				effectNum++; //total effect counter for this entire custom enchantment
			}
		}
	}
}


void PersistentWeaponEnchantments::PostCraftUpdate()
{
	//armor enchantments not supported at the moment (no real need to support them)
	if (g_craftData.GetStagedNewEnchantmentType() == EnchantCraftMonitor::kEnchantmentType_Armor)
		return;

	EnchantmentItem* newEnchantment = g_craftData.GetStagedNewEnchantment(); //Enchantment that was just created
	if (!newEnchantment)
		return;
	else if (playerWeaponEnchants.find(newEnchantment) != playerWeaponEnchants.end()) //Ignore duplicates
		return;

	FixIfChaosDamage(newEnchantment);

	UInt32 thisFormID = newEnchantment->formID;
	UInt32 thisFlags = EnchantmentInfoEntry::kFlag_ManualCalc;
	SInt32 thisEnchantmentCost = newEnchantment->data.calculations.cost;
	EnchantmentInfoEntry thisEnchantmentInfo(thisFormID, thisFlags, thisEnchantmentCost);

	g_craftData.Visit(&thisEnchantmentInfo.parentForms); //Get all parent enchantments combined to make this one

	thisEnchantmentInfo.EvaluateConditions(); //Inherit conditions from parent enchantments

	playerWeaponEnchants[newEnchantment] = thisEnchantmentInfo; //Update array of all player enchantments
}


bool PersistentWeaponEnchantments::IsChaosDamageEffect(EffectSetting* mgef)
{
	static DataHandler* data = DataHandler::GetSingleton();
	static const char * dragonborn = "Dragonborn.esm";
	static UInt32 lowBoundFormID = (data) ? ((data->GetModIndex(dragonborn)) << 24) | 0x02C46B : 0;
	return (mgef) ? (mgef->formID >= lowBoundFormID) && (mgef->formID <= (lowBoundFormID + 0x02)) : false;
}

void PersistentWeaponEnchantments::FixIfChaosDamage(EnchantmentItem* pEnch) //Detect Chaos Damage and fix its non-scaling effects
{
	if (!pEnch || (pEnch->effectItemList.count < 3))
		return;

	EffectItemVec effects;

	for(UInt32 i = 0; i < pEnch->effectItemList.count; ++i)
	{
		MagicItem::EffectItem* pEffectItem = NULL;
		pEnch->effectItemList.GetNthItem(i, pEffectItem);
		if (pEffectItem && IsChaosDamageEffect(pEffectItem->mgef))
			effects.push_back(pEffectItem);
	}

	if (effects.size() != 3)
		return;
	
	//Technically this might not always be "correct" -- for instance, it ignores elemental Enchanting perks.
	if (effects[0]->magnitude == effects[1]->magnitude)
		effects[0]->magnitude = effects[1]->magnitude = effects[2]->magnitude;
	else if (effects[0]->magnitude == effects[2]->magnitude)
		effects[0]->magnitude = effects[2]->magnitude = effects[1]->magnitude;
	else if (effects[1]->magnitude == effects[2]->magnitude)
		effects[1]->magnitude = effects[2]->magnitude = effects[0]->magnitude;

	//See early commits for formula to update enchant cost too, now removed
}



//Enchantment Framework Methods:
namespace EnchantmentFramework
{

EnchantmentVec GetAllCraftedEnchantments()
{
	struct
	{
		EnchantmentVec eVec;
		bool Accept(EnchantmentItem* e, EnchantmentInfoEntry ei)
		{
			eVec.push_back(e);
			return true;
		}
	} persistentEnchantments;

	g_enchantTracker.Visit(&persistentEnchantments);

	return persistentEnchantments.eVec;
}


EnchantmentVec GetCraftedEnchantmentParents(EnchantmentItem* customEnchantment)
{
	return g_enchantTracker.GetParents(customEnchantment);
}

};


EnchantmentFrameworkInterface g_enchantmentFrameworkInterface =
{
	EnchantmentFrameworkInterface::kInterfaceVersion,
	EnchantmentFramework::GetAllCraftedEnchantments,
	EnchantmentFramework::GetCraftedEnchantmentParents
};