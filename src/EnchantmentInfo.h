#pragma once

#include "skse/GameData.h"
#include "skse/GameForms.h"
#include "skse/GameObjects.h"
#include "skse/GameRTTI.h"
#include <vector>
#include <map>

#include "[PluginLibrary]/SerializeForm.h"

#define INVALIDATE true


typedef std::vector<EffectSetting*> 		MagEffVec;
typedef std::vector<MagicItem::EffectItem*> EffectItemVec;


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


	void EvaluateConditions() //TODO - this wont work currently since conditions are detached while in the enchanting menu. need to queue these or read from saved data.
	{
		//Inherit conditions from parent enchantments
		EnchantmentItem* thisEnchant = DYNAMIC_CAST(LookupFormByID(formID), TESForm, EnchantmentItem);
		for (UInt32 enchantNum = 0, effectNum = 0; enchantNum < parentForms.data.size(); enchantNum++)
		{
			EnchantmentItem* parentEnchant = DYNAMIC_CAST(LookupFormByID(parentForms.data[enchantNum]), TESForm, EnchantmentItem);
			for (UInt32 i = 0; i < parentEnchant->effectItemList.count; i++)
			{
				MagicItem::EffectItem* parentEffectItem = NULL;
				parentEnchant->effectItemList.GetNthItem(i, parentEffectItem);
				if (parentEffectItem && parentEffectItem->unk14) //(unk14 == condition)
				{
					attributes.hasConditions = true;
					MagicItem::EffectItem* pNew = NULL;
					thisEnchant->effectItemList.GetNthItem(effectNum, pNew);
					pNew->unk14 = parentEffectItem->unk14;
					// (weirdly enough, unlike the serialization load method, this
					//  doesn't cause any problems... I can reload the game many
					//  times and the condition stays valid and doesn't cause a crash)
				}
				effectNum++; //total effect counter for this entire custom enchantment
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


typedef std::vector <EnchantmentItem*>						EnchantmentVec;
typedef std::map <EnchantmentItem*, EnchantmentInfoEntry>	EnchantmentInfoMap;
//typedef std::vector <EnchantmentInfoMap::iterator>			EnchantmentInfoReferenceVec;


// class KnownBaseEnchantments
// {
// public:
// 	bool Accept(EnchantmentItem* pEnch)
// 	{
// 		TESForm* pForm = DYNAMIC_CAST(pEnch, EnchantmentItem, TESForm);
// 		if (pForm && ((pForm->flags & TESForm::kFlagPlayerKnows) == TESForm::kFlagPlayerKnows))
// 		{
// 			if (pEnch->data.deliveryType == 0x01) //Delivery type: 'contact'
// 				knownWeaponBaseEnchantments.push_back(pEnch);
// 			else if (pEnch->data.deliveryType == 0x00) //Delivery type: 'self'
// 				knownArmorBaseEnchantments.push_back(pEnch);
// 		}
// 		return true;
// 	}

// 	//Find known base enchantment from targetName:
// 	EnchantmentItem* LookupByName(const char* targetName, char hint = 'w');
// 	//Clear vectors:
// 	void Reset();

// 	EnchantmentVec knownWeaponBaseEnchantments;
// 	EnchantmentVec knownArmorBaseEnchantments;

// 	typedef std::map<const char*, EnchantmentItem*> NamedEnchantMap;
// 	static NamedEnchantMap discoveredEnchantments;
// };


class PersistentWeaponEnchantments
{
private:
	EnchantmentInfoMap			playerWeaponEnchants;
	static bool bInitialized;

public:
	template <typename SerializeInterface_T>
	void Serialize(SerializeInterface_T* const intfc)
	{
		for (EnchantmentInfoMap::iterator it = playerWeaponEnchants.begin(); it != playerWeaponEnchants.end(); ++it)
			it->second.Serialize(intfc);
	}
	void Add(EnchantmentItem* enchantment, EnchantmentInfoEntry &info)
	{	//used during serialization load
		playerWeaponEnchants[enchantment] = info; //Add to main tracker
	}
	UInt32 GetSize() { return playerWeaponEnchants.size(); }
	// static KnownBaseEnchantments* GetKnown(const bool &invalidate = false); //Get all base enchantments currently known by the player
	void PostCraftUpdate(); //Fixes any new custom enchantments crafted at the enchantment table & vectorizes an iterator to each new enchantment
	void Reset(); //Reset info for new game or new save load

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


EnchantmentItem* FindBaseEnchantment(MagEffVec mgefs, UInt32 deliveryType);

bool IsChaosDamageEffect(EffectSetting* mgef);

void FixIfChaosDamage(EnchantmentItem* pEnch);

extern PersistentWeaponEnchantments		enchantTracker;