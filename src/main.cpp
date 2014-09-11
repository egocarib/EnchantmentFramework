#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include "skse/GameRTTI.h"
#include "EnchantmentInfo.h"
#include "MenuHandler.h"
#include "CraftHooks.h"
#include "EnchantmentDataPluginInterface.h"
#include "[PluginLibrary]/SerializeForm.h"
#include <shlobj.h>

IDebugLog						g_Log;
const char*						kLogPath = "\\My Games\\Skyrim\\Logs\\EnchantReloadFix.log";

PluginHandle					g_pluginHandle = kPluginHandle_Invalid;
SKSESerializationInterface*		g_serialization = NULL;
SKSEMessagingInterface*			g_messageInterface = NULL;
const UInt32 					kSerializationDataVersion = 2;



void PreLoadSetup()
{
	_MESSAGE("Building event sinks...");
	MenuManager::GetSingleton()->MenuOpenCloseEventDispatcher()->AddEventSink(&g_menuEventHandler);
}

void PostLoadSetup()
{
	static bool firstLoad = true;
	if (firstLoad)
	{
		firstLoad = false;
		_MESSAGE("First load, initializating...");
		EnchantmentDataHandler::Visit(&g_weaponEnchantmentConditions);
	}
}


void Serialization_Revert(SKSESerializationInterface * intfc)
{
	g_enchantTracker.Reset();
}


void Serialization_Save(SKSESerializationInterface * intfc)
{
	_MESSAGE("Saving...");
	PostLoadSetup(); //necessary if the player starts a new game after initial load, no other message/serialization events will occur until the first autosave
	if(intfc->OpenRecord('DATA', kSerializationDataVersion))
		g_enchantTracker.Serialize(intfc);
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

		g_enchantTracker.Push(thisEnchant, thisEntry); //Add to main tracker
		return 1;
	}
	else
	{
		_MESSAGE("Error Reading Cosave: Invalid Chunk Size (%u Expected %u)"
		" -- Data Entry Will Be Skipped [form 0x%08X]", sizeRead, sizeExpected, thisEntry.formID);
		return 0;
	}
}


void Serialization_Load(SKSESerializationInterface* intfc)
{
	g_enchantTracker.Reset();
	_MESSAGE("Loading...");

	UInt32	type;
	UInt32	version;
	UInt32	length;
	bool	error = false;
	UInt32  recordsRead = 0;

	while(!error && intfc->GetNextRecordInfo(&type, &version, &length))
	{
		if (type == 'DATA')
		{
			if(version == kSerializationDataVersion)
				while ((SInt32)length > 0) //Safety cast
					recordsRead += ProcessLoadEntry(intfc, &length);
			else
				{ _MESSAGE("Error Reading Cosave: Unknown Data Version %u, Aborting...\n", version); error = true; }
		}
		else
			{ _MESSAGE("Error Reading Cosave: Unhandled Type %08X, Aborting...\n", type); error = true; }
	}

	if (recordsRead && !error)
		_MESSAGE("  %u Enchantment Records Successfully Patched.\n", recordsRead);
}


void SKSEMessageReceptor(SKSEMessagingInterface::Message* msg)
{
	if (msg->type == SKSEMessagingInterface::kMessage_PostLoadGame) //after a save is loaded (d/n work when new game chosen)
	{
		PostLoadSetup();
	}

	if (msg->type == SKSEMessagingInterface::kMessage_PostLoad) //post plugin load (no game data)
	{
		//other plugins should register to receive interface/message here
	}

	else if (msg->type == SKSEMessagingInterface::kMessage_PostPostLoad) //right after postload (no game data)
	{
		//broadcast enchantment data interface
		_MESSAGE("Broadcasting interface...");
		g_messageInterface->Dispatch(g_pluginHandle, 'Itfc', &g_enchantmentDataInterface, sizeof(void*), NULL);
	}

	else if (msg->type == SKSEMessagingInterface::kMessage_InputLoaded) //initial main menu load (limited game data)
	{
		PreLoadSetup();
	}
}


extern "C"
{

bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
{
	g_Log.OpenRelative(CSIDL_MYDOCUMENTS, kLogPath);

	_MESSAGE("EnchantReloadFix SKSE Plugin\nby egocarib\n\n"
		"{ Fixes player-enchanted items having inflated gold value }\n"
		"{ and draining higher amounts of charge after game reload }\n");

	//Populate plugin info structure
	info->infoVersion =	PluginInfo::kInfoVersion;
	info->name =		"EnchantReloadFix Plugin";
	info->version =		2;

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
	_MESSAGE("Planting hooks...");
	CreateEnchantmentHook_Commit();
	GetCostliestEffectItemHook::Hook_Commit();

	_MESSAGE("Interfacing with skse...");

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
