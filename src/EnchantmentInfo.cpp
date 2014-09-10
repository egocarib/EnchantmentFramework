#include "EnchantmentInfo.h"
#include "CraftHooks.h"
#include "skse/GameData.h"
#include "skse/GameObjects.h"




//Called each time an item is crafted at the enchanting table
void PersistentWeaponEnchantments::PostCraftUpdate()
{
	EnchantmentItem* newEnchantment = g_craftData.GetStagedNewEnchantment();
	if (!newEnchantment)
		return;
	else if (!(newEnchantment->data.calculations.flags & EnchantmentInfoEntry::kFlag_ManualCalc))
		return;	//Ignore Auto-Calc enchants (they are created before this plugin or via papyrus CreateEnchantment)
				//(enchants are set to manual calc when crafted until game reload, then they revert to auto without this plugin's presence)
	else if (playerWeaponEnchants.find(newEnchantment) != playerWeaponEnchants.end())
		return; //Already added to map

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

	//Update new effect costs
		// float newEnchantmentCost = 0.0;
		// for (UInt32 i = 0; i < 3; ++i)
		// {
		// 	float thisCost = effects[i]->mgef->properties.baseCost * pow((effects[i]->magnitude * effects[i]->duration / 10.0), 1.1);
		// 	effects[i]->cost = thisCost;
		// 	newEnchantmentCost += thisCost;
		// }

		//Although updating the cost would be technically correct, it also throws off the price of the enchanted item
		//compared to what was displayed at the enchanting table, which seems to go against the spirit of this patch.

		// pEnch->data.calculations.cost = (UInt32)newEnchantmentCost;
}