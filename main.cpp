#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include "skse/GameRTTI.h"
#include "EnchantmentInfo.h"
#include "MenuHandler.h"
#include <shlobj.h>

IDebugLog						gLog;
const char*						kLogPath = "\\My Games\\Skyrim\\Logs\\EnchantReloadFix.log";

PluginHandle					g_pluginHandle = kPluginHandle_Invalid;
SKSESerializationInterface*		g_serialization = NULL;
const UInt32 					kSerializationDataVersion = 1;

MenuCore						menu;
PersistentWeaponEnchantments	enchantTracker;


void Serialization_Revert(SKSESerializationInterface * intfc)
{
	_MESSAGE("Initializing...");
	menu.InitializeMenuMonitor();
	enchantTracker.Reset();
}

void Serialization_Save(SKSESerializationInterface * intfc)
{
	_MESSAGE("Saving...");
	menu.InitializeMenuMonitor(); //Init Menu here too, for now, because New Game doesn't trigger Revert or Load.

	if (enchantTracker.size() == 0)
		return;

	if(intfc->OpenRecord('DATA', kSerializationDataVersion))
	{
		for (PersistentWeaponEnchantments::iterator it = enchantTracker.begin(); it != enchantTracker.end(); ++it)
		{
			intfc->WriteRecordData(&it->second, sizeof(it->second));
			_MESSAGE("Wrote data to save: [Enchantment: 0x%08X] [Flags: %s] [Cost: %d]"
				,it->second.formID
				,it->second.flags ? "MANUAL" : "AUTO"
				,it->second.enchantmentCost);
		}
	}
}

UInt32 ProcessLoadEntry(SKSESerializationInterface* intfc)
{
	EnchantmentInfoEntry thisEntry;
	UInt32 sizeRead = intfc->ReadRecordData(&thisEntry, sizeof(EnchantmentInfoEntry));
	if (sizeRead == sizeof(EnchantmentInfoEntry))
	{
		EnchantmentItem* thisEnchant = DYNAMIC_CAST(LookupFormByID(thisEntry.formID), TESForm, EnchantmentItem);
		if (!thisEnchant)
			return 0;

		thisEnchant->data.unk00.unk04 |= thisEntry.flags; //Set to manualCalc and correct cost
		thisEnchant->data.unk00.unk00 = thisEntry.enchantmentCost;
		
		if (thisEntry.cData.hasConditions) //Update enchantment conditions
		{
			EnchantmentItem* parentEnchant = DYNAMIC_CAST(LookupFormByID(thisEntry.cData.parentFormID), TESForm, EnchantmentItem);
			if (parentEnchant)
			{
				for (UInt32 i = 0; (i < parentEnchant->effectItemList.count) && (i < thisEnchant->effectItemList.count); ++i)
				{
					MagicItem::EffectItem* parentEffect = NULL;
					parentEnchant->effectItemList.GetNthItem(i, parentEffect);
					if (parentEffect && parentEffect->condition)
					{
						MagicItem::EffectItem* childEffect = NULL;
						thisEnchant->effectItemList.GetNthItem(i, childEffect);
						childEffect->condition = parentEffect->condition;
					}
				}
			}
		}

		enchantTracker[thisEnchant] = thisEntry; //Add to main tracker

		_MESSAGE("Read & Set data from cosave: [Enchantment: 0x%08X] [Flags: %s] [Cost: %d] [Conditions: %s]"
			,thisEntry.formID
			,thisEntry.flags ? "MANUAL" : "AUTO"
			,thisEntry.enchantmentCost
		    ,thisEntry.cData.hasConditions ? "YES" : "NO");
		return 1;
	}
	else
		_MESSAGE("Error reading from cosave: INVALID CHUNK SIZE (%u expected %u)"
		" -- Data entry will be skipped.", sizeRead, sizeof(EnchantmentInfoEntry));
	return 0;
}

void Serialization_Load(SKSESerializationInterface* intfc)
{
	menu.InitializeMenuMonitor();
	enchantTracker.Reset();
	_MESSAGE("Loading...");

	UInt32	type;
	UInt32	version;
	UInt32	length;
	bool	error = false;
	UInt32  recordsRead = 0;

	while(!error && intfc->GetNextRecordInfo(&type, &version, &length))
	{
		switch(type)
		{
			case 'DATA':
			{
				if(version == kSerializationDataVersion)
				{
					for (;length > 0; length -= sizeof(EnchantmentInfoEntry))
					{
						_MESSAGE("  read remaining length = %d", length);
						recordsRead += ProcessLoadEntry(intfc);
					}
				}
				else
				{
					_MESSAGE("Error reading from cosave: UNKNOWN DATA VERSION %u, aborting...", version);
					error = true;
				}
				break;
			}
			
			default:
				_MESSAGE("Error reading from cosave: UNHANDLED TYPE %08X, aborting...", type);
				error = true;
				break;
		}
	}

	if (recordsRead)
		_MESSAGE("%u enchantment records successfully patched.", recordsRead);
}



extern "C"
{

bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
{

	gLog.OpenRelative(CSIDL_MYDOCUMENTS, kLogPath);

	_MESSAGE("EnchantReloadFix SKSE Plugin\nby egocarib\n\n"
		"{ Fixes player-enchanted items having inflated gold value }\n"
		"{ and draining higher amounts of charge after game reload }\n");

	// populate plugin info structure
	info->infoVersion =	PluginInfo::kInfoVersion;
	info->name =		"EnchantReloadFix Plugin";
	info->version =		1;

	// store plugin handle so we can identify ourselves later
	g_pluginHandle = skse->GetPluginHandle();

	if(skse->isEditor)
		{  _MESSAGE("loaded in editor, marking as incompatible");   return false;  }
	else if(skse->runtimeVersion != RUNTIME_VERSION_1_9_32_0)
		{  _MESSAGE("unsupported runtime version %08X", skse->runtimeVersion);   return false;  }

	// get the serialization interface and query its version
	g_serialization = (SKSESerializationInterface *)skse->QueryInterface(kInterface_Serialization);
	if(!g_serialization)
		{  _MESSAGE("couldn't get serialization interface");   return false;  }
	if(g_serialization->version < SKSESerializationInterface::kVersion)
		{  _MESSAGE("serialization interface too old (%d expected %d)", g_serialization->version, SKSESerializationInterface::kVersion);   return false;  }

	return true;
}


bool SKSEPlugin_Load(const SKSEInterface * skse)
{
	// register callbacks and unique ID for serialization
	g_serialization->SetUniqueID(g_pluginHandle, 'EGOC');
	g_serialization->SetRevertCallback(g_pluginHandle, Serialization_Revert);
	g_serialization->SetSaveCallback(g_pluginHandle, Serialization_Save);
	g_serialization->SetLoadCallback(g_pluginHandle, Serialization_Load);

	return true;
}

};
