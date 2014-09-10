#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include "skse/GameRTTI.h"
#include "EnchantmentInfo.h"
#include "MenuHandler.h"
#include "EnchantmentDataPluginInterface.h"
#include "[PluginLibrary]/SerializeForm.h"
#include "CraftHooks.h"
#include <shlobj.h>

IDebugLog						gLog;
const char*						kLogPath = "\\My Games\\Skyrim\\Logs\\EnchantReloadFix.log";

PluginHandle					g_pluginHandle = kPluginHandle_Invalid;
SKSESerializationInterface*		g_serialization = NULL;
SKSEMessagingInterface*			g_messageInterface = NULL;
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

	if(intfc->OpenRecord('DATA', kSerializationDataVersion))
		enchantTracker.Serialize(intfc);
}


UInt32 ProcessLoadEntry(SKSESerializationInterface* intfc, UInt32* const length)
{
	EnchantmentInfoEntry thisEntry;
	UInt32 sizeRead;
	UInt32 sizeExpected;
	thisEntry.Deserialize(intfc, &sizeRead, &sizeExpected);
	(*length) -= sizeRead;

	if (sizeRead == sizeExpected)
	{
		EnchantmentItem* thisEnchant = DYNAMIC_CAST(LookupFormByID(thisEntry.formID), TESForm, EnchantmentItem);
		if (!thisEnchant)
			return 0;

		thisEnchant->data.calculations.flags |= thisEntry.attributes.flags; //Set to manual calc
		thisEnchant->data.calculations.cost = thisEntry.attributes.enchantmentCost; //Correct enchantment cost
		
		//This ended up causing a crash on subsequent loads, most likely because the game rebuilds the condition table.
		//I could probably work around it by detaching all conditions during Revert, and then letting the Load process
		//re-attach them. But I'm just going to disable them for now to be safe, it's relatively unimportant.

		// if (thisEntry.cData.hasConditions) //Update enchantment conditions
		// {
		// 	EnchantmentItem* parentEnchant = DYNAMIC_CAST(LookupFormByID(thisEntry.cData.parentFormID), TESForm, EnchantmentItem);
		// 	if (parentEnchant)
		// 	{
		// 		for (UInt32 i = 0; (i < parentEnchant->effectItemList.count) && (i < thisEnchant->effectItemList.count); ++i)
		// 		{
		// 			MagicItem::EffectItem* parentEffect = NULL;
		// 			parentEnchant->effectItemList.GetNthItem(i, parentEffect);
		// 			if (parentEffect && parentEffect->condition)
		// 			{
		// 				MagicItem::EffectItem* childEffect = NULL;
		// 				thisEnchant->effectItemList.GetNthItem(i, childEffect);
		// 				childEffect->condition = parentEffect->condition;
		// 			}
		// 		}
		// 	}
		// }

		enchantTracker.Add(thisEnchant, thisEntry); //Add to main tracker
		return 1;
	}
	else
	{
		_MESSAGE("Error Reading From Cosave: INVALID CHUNK SIZE (%u Expected %u)"
		" -- Data Entry Will Be Skipped [form 0x%08X]", sizeRead, sizeExpected, thisEntry.formID);
		return 0;
	}
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
					while ((SInt32)length > 0) //Safety cast
						recordsRead += ProcessLoadEntry(intfc, &length);
				else
				{
					_MESSAGE("Error Reading From Cosave: UNKNOWN DATA VERSION %u, Aborting...\n", version);
					error = true;
				}
				break;
			}
			
			default:
				_MESSAGE("Error Reading From Cosave: UNHANDLED TYPE %08X, Aborting...\n", type);
				error = true;
				break;
		}
	}

	if (recordsRead && !error)
		_MESSAGE("  %u Enchantment Records Successfully Patched.\n", recordsRead);
}





void InitialLoadSetup()
{
	_MESSAGE("Building Event Sinks...");

	//Add event sinks
	g_trackedStatsEventDispatcher->AddEventSink(&g_trackedStatsEventHandler);

}

void SKSEMessageReceptor(SKSEMessagingInterface::Message* msg)
{
	if (msg->type == SKSEMessagingInterface::kMessage_PostLoad)
	{
		//other plugins should register to receive interface/message here
	}

	else if (msg->type == SKSEMessagingInterface::kMessage_PostPostLoad)
	{
		//broadcast enchantment data interface
		g_messageInterface->Dispatch(g_pluginHandle, 'Itfc', &g_enchantmentDataInterface, sizeof(void*), NULL);
	}

	else if (msg->type == SKSEMessagingInterface::kMessage_InputLoaded)
	{
		//kMessage_InputLoaded only sent once, on initial Main Menu load
		InitialLoadSetup();
	}
}


extern "C"
{

bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
{

	gLog.OpenRelative(CSIDL_MYDOCUMENTS, kLogPath);

	_MESSAGE("EnchantReloadFix SKSE Plugin\nby egocarib\n\n"
		"{ Fixes player-enchanted items having inflated gold value }\n"
		"{ and draining higher amounts of charge after game reload }\n");

	//Populate plugin info structure
	info->infoVersion =	PluginInfo::kInfoVersion;
	info->name =		"EnchantReloadFix Plugin";
	info->version =		1;

	//Store plugin handle so we can identify ourselves later
	g_pluginHandle = skse->GetPluginHandle();

	if(skse->isEditor)
		{  _MESSAGE("Loaded In Editor, Marking As Incompatible");   return false;  }
	else if(skse->runtimeVersion != RUNTIME_VERSION_1_9_32_0)
		{  _MESSAGE("Unsupported Runtime Version %08X", skse->runtimeVersion);   return false;  }

	//Get the serialization interface and query its version
	g_serialization = (SKSESerializationInterface *)skse->QueryInterface(kInterface_Serialization);
	if(!g_serialization)
		{  _MESSAGE("Couldn't Get Serialization Interface");   return false;  }
	if(g_serialization->version < SKSESerializationInterface::kVersion)
		{  _MESSAGE("Serialization Interface Too Old (%d Expected %d)", g_serialization->version, SKSESerializationInterface::kVersion);   return false;  }

	//Get the messaging interface and query its version
	g_messageInterface = (SKSEMessagingInterface *)skse->QueryInterface(kInterface_Messaging);
	if(!g_messageInterface)
		{ _MESSAGE("Couldn't Get Messaging Interface"); return false; }
	if(g_messageInterface->interfaceVersion < SKSEMessagingInterface::kInterfaceVersion)
		{ _MESSAGE("Messaging Interface Too Old (%d Expected %d)", g_messageInterface->interfaceVersion, SKSEMessagingInterface::kInterfaceVersion); return false; }

	return true;
}


bool SKSEPlugin_Load(const SKSEInterface * skse)
{
	//WeaponEnchantCraftHook::CraftHook_Commit();
	CraftHook_Commit();
	GetCostliestEffectItemHook::CostliestEffect_Hook_Commit();

	//Register callbacks and unique ID for serialization
	g_serialization->SetUniqueID(g_pluginHandle, 'EGOC');
	g_serialization->SetRevertCallback(g_pluginHandle, Serialization_Revert);
	g_serialization->SetSaveCallback(g_pluginHandle, Serialization_Save);
	g_serialization->SetLoadCallback(g_pluginHandle, Serialization_Load);

	//Register callback for SKSE messaging interface
	g_messageInterface->RegisterListener(g_pluginHandle, "SKSE", SKSEMessageReceptor);

	return true;
}

};
