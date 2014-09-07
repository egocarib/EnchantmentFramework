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

	EnchantmentInfoEntry(UInt32 idArg = 0, UInt32 flagArg = 0, SInt32 costArg = -1) : formID(idArg), parentForms()
	{
		data.flags = flagArg;
		data.enchantmentCost = costArg;
		data.hasConditions = false;
	}

	struct CoreData
	{
		UInt32			flags;
		SInt32			enchantmentCost;
		bool			hasConditions;
	};

	UInt32					formID;
	CoreData				data;
	std::vector<UInt32>		parentForms;

	template <typename SerializeInterface_T>
	void Serialize(SerializeInterface_T* const intfc)
	{
		SerialFormData enchantmentForm(formID);
		intfc->WriteRecordData(&enchantmentForm, sizeof(enchantmentForm));
		intfc->WriteRecordData(&data, sizeof(CoreData));
		UInt32 numberOfParentForms = parentForms.size();
		intfc->WriteRecordData(&numberOfParentForms, sizeof(UInt32)); //Number of forms to follow
		for (UInt32 i = 0; i < parentForms.size(); i++)
		{
			SerialFormData parentForm(parentForms[i]);
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

		(*sizeRead) += intfc->ReadRecordData(&data, sizeof(CoreData));
		(*sizeExpected) += sizeof(CoreData);
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
				parentForms.push_back(thisFormID);
		}
	}
};


typedef std::vector <EnchantmentItem*>						EnchantmentVec;
typedef std::map <EnchantmentItem*, EnchantmentInfoEntry>	EnchantmentInfoMap;
typedef std::vector <EnchantmentInfoMap::iterator>			EnchantmentInfoReferenceVec;


class KnownBaseEnchantments
{
public:
	bool Accept(EnchantmentItem* pEnch)
	{
		TESForm* pForm = DYNAMIC_CAST(pEnch, EnchantmentItem, TESForm);
		if (pForm && ((pForm->flags & TESForm::kFlagPlayerKnows) == TESForm::kFlagPlayerKnows))
		{
			if (pEnch->data.deliveryType == 0x01) //Delivery type: 'contact'
				knownWeaponBaseEnchantments.push_back(pEnch);
			else if (pEnch->data.deliveryType == 0x00) //Delivery type: 'self'
				knownArmorBaseEnchantments.push_back(pEnch);
		}
		return true;
	}

	//Find known base enchantment from targetName:
	EnchantmentItem* LookupByName(const char* targetName, char hint = 'w');
	//Clear vectors:
	void Reset();

	EnchantmentVec knownWeaponBaseEnchantments;
	EnchantmentVec knownArmorBaseEnchantments;

	typedef std::map<const char*, EnchantmentItem*> NamedEnchantMap;
	static NamedEnchantMap discoveredEnchantments;
};


class PersistentWeaponEnchantments
{
private:
	EnchantmentInfoMap			playerWeaponEnchants;
	EnchantmentInfoReferenceVec	newCraftedEnchantments; //Vector of iterators pointing to all enchantments newly detected during last Update()
  	static bool bInitialized;

public:
	template <typename SerializeInterface_T>
	void Serialize(SerializeInterface_T* const intfc)
	{
		for (EnchantmentInfoMap::iterator it = playerWeaponEnchants.begin(); it != playerWeaponEnchants.end(); ++it)
			it->second.Serialize(intfc);
	}
	void Add(EnchantmentItem* enchantment, EnchantmentInfoEntry &info)
	{
		playerWeaponEnchants[enchantment] = info; //Add to main tracker
	}
	UInt32 GetSize() { return playerWeaponEnchants.size(); }
	static KnownBaseEnchantments* GetKnown(const bool &invalidate = false); //Get all base enchantments currently known by the player
	void PostCraftUpdate(); //Fixes any new custom enchantments crafted at the enchantment table & vectorizes an iterator to each new enchantment
	void PostCraftUpdate(EnchantmentInfoReferenceVec* &newEnchantments); //Same as above, but returns the new enchantment vector as well
	void Reset(); //Reset info for new game or new save load
	EnchantmentInfoReferenceVec* GetNewCraftedEnchantments() { return &newCraftedEnchantments; }

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