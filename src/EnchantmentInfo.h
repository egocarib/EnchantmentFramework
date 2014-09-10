#pragma once

#include "skse/GameData.h"
#include "skse/GameForms.h"
#include "skse/GameObjects.h"
#include "skse/GameRTTI.h"
#include <vector>
#include <map>

#include "[PluginLibrary]/SerializeForm.h"

#define INVALIDATE true


typedef std::vector<EffectSetting*> 						MagEffVec;
typedef std::vector<MagicItem::EffectItem*> 				EffectItemVec;
typedef std::vector <EnchantmentItem*>						EnchantmentVec;


class EnchantmentInfoEntry
{
public:
	enum { kFlag_ManualCalc = 0x01 };

	EnchantmentInfoEntry(UInt32 idArg = 0, UInt32 flagArg = 0, SInt32 costArg = -1, bool condArg = false)
		: formID(idArg), attributes(flagArg, costArg, condArg), parentForms() {}

	struct Attributes
	{
		UInt32			flags;
		SInt32			enchantmentCost;
		bool			hasConditions;
		Attributes(UInt32 flagArg = 0, SInt32 costArg = -1, bool condArg = false)
			: flags(flagArg), enchantmentCost(costArg), hasConditions(condArg) {}
	};

	struct ParentForms
	{
		std::vector<UInt32>	data;
		bool Accept(EnchantmentItem* e) { data.push_back(e->formID); return true; }
		ParentForms() : data() {}
	};


	//Members
	UInt32				formID;
	Attributes			attributes;
	ParentForms			parentForms;


	void EvaluateConditions()
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
			else
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
						pNew->unk14 = g_weaponEnchantmentConditions.GetCondition(parentEnchant, parentEffectItem);
						// (weirdly enough, unlike the serialization load method, this
						//  doesn't cause any problems... I can reload the game many
						//  times and the condition stays valid and doesn't cause a crash)
					}
					effectNum++; //total effect counter for this entire custom enchantment
				}
			}
		}
	}

	template <typename SerializeInterface_T>
	void Serialize(SerializeInterface_T* const intfc)
	{
		SerialFormData enchantmentForm(formID);
		intfc->WriteRecordData(&enchantmentForm, sizeof(enchantmentForm));
		intfc->WriteRecordData(&attributes, sizeof(EnchantmentInfoEntry::Attributes));
		UInt32 numberOfParentForms = parentForms.data.size();
		intfc->WriteRecordData(&numberOfParentForms, sizeof(UInt32)); //Number of forms to follow
		for (UInt32 i = 0; i < parentForms.data.size(); i++)
		{
			SerialFormData parentForm(parentForms.data[i]);
			intfc->WriteRecordData(&parentForm, sizeof(parentForm));
		}
	}

	template <typename SerializeInterface_T>
	void Deserialize(SerializeInterface_T* const intfc, UInt32* const sizeRead, UInt32* const sizeExpected)
	{
		(*sizeRead) = (*sizeExpected) = 0;
		SerialFormData enchantmentForm;
		(*sizeRead) += intfc->ReadRecordData(&enchantmentForm, sizeof(SerialFormData));
		(*sizeExpected) += sizeof(SerialFormData);
		if (*sizeRead != *sizeExpected)
			return;

		UInt32 result = enchantmentForm.Deserialize(&formID);
		if (result != SerialFormData::kResult_Succeeded)
			SerialFormData::OutputError(result);

		(*sizeRead) += intfc->ReadRecordData(&attributes, sizeof(EnchantmentInfoEntry::Attributes));
		(*sizeExpected) += sizeof(EnchantmentInfoEntry::Attributes);
		if (*sizeRead != *sizeExpected)
			return;

		UInt32 numParents;
		(*sizeRead) += intfc->ReadRecordData(&numParents, sizeof(UInt32));
		(*sizeExpected) += sizeof(UInt32);
		if (*sizeRead != *sizeExpected)
			return;

		for (UInt32 i = 0; i < numParents; i++)
		{
			SerialFormData parentForm;
			(*sizeRead) += intfc->ReadRecordData(&parentForm, sizeof(SerialFormData));
			(*sizeExpected) += sizeof(SerialFormData);
			if (*sizeRead != *sizeExpected)
				return;

			UInt32 thisFormID;
			UInt32 result = parentForm.Deserialize(&thisFormID);

			if (result != SerialFormData::kResult_Succeeded)
				SerialFormData::OutputError(result);
			else
				parentForms.data.push_back(thisFormID);
		}
	}
};


typedef std::map <EnchantmentItem*, EnchantmentInfoEntry>	EnchantmentInfoMap;




class PersistentWeaponEnchantments
{
private:
	EnchantmentInfoMap	playerWeaponEnchants;

public:
	void Reset() { playerWeaponEnchants.clear(); } //Reset info for new game or new save load

	void Push(EnchantmentItem* enchantment, EnchantmentInfoEntry &info) //Rebuild map during load
	{
		playerWeaponEnchants[enchantment] = info;
	}

	void PostCraftUpdate(); //Fixes new crafted enchantments and adds them to the tracker

	bool IsChaosDamageEffect(EffectSetting* mgef); //Secondary enchantment effects don't scale with slider
	void FixIfChaosDamage(EnchantmentItem* pEnch); //(should try to fix this for all enchantments eventually)

	template <typename SerializeInterface_T>
	void Serialize(SerializeInterface_T* const intfc)
	{
		for (EnchantmentInfoMap::iterator it = playerWeaponEnchants.begin(); it != playerWeaponEnchants.end(); ++it)
			it->second.Serialize(intfc);
	}
};


class EnchantmentDataHandler //Exposes list of all loaded enchantment forms
{
public:
	template <class Visitor>
	static void Visit(Visitor* visitor)
	{
		static DataHandler* data = DataHandler::GetSingleton();
		bool bContinue = true;
		for(UInt32 i = 0; (i < data->enchantments.count) && bContinue; i++)
		{
			EnchantmentItem* pEnch = NULL;
			data->enchantments.GetNthItem(i, pEnch);
			if (pEnch)
				bContinue = visitor->Accept(pEnch);
		}
	}

private:
	EnchantmentDataHandler() {}
};



extern PersistentWeaponEnchantments		enchantTracker;