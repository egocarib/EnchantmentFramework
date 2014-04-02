#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include "skse/GameRTTI.h"
#include "EnchantmentInfo.h"
#include "MenuHandler.h"
#include <shlobj.h>

using namespace EnchantmentInfoLib;


IDebugLog					gLog;

const char*					kLogPath = "\\My Games\\Skyrim\\Logs\\EnchantReloadFix.log";

PluginHandle				g_pluginHandle = kPluginHandle_Invalid;

SKSESerializationInterface*	g_serialization = NULL;




//___________________________________________________________________________________________________________
//============================================= SERIALIZATION ===============================================


const UInt32 kDataVersion = 1;


void Serialization_Revert(SKSESerializationInterface * intfc)
{
	_MESSAGE("Initializing...");
	_playerEnchantments.clear();
	_knownBaseEnchantments.clear();
}

void Serialization_Save(SKSESerializationInterface * intfc)
{
	_MESSAGE("Saving...");

	//Update map with new enchantment data
	BuildPersistentFormsEnchantmentMap();

	if (_playerEnchantments.size() == 0)
		return;

	if(intfc->OpenRecord('DATA', kDataVersion))
	{
		for (EnchantmentInfoMap::iterator it = _playerEnchantments.begin(); it != _playerEnchantments.end(); ++it)
		{
			intfc->WriteRecordData(&it->second, sizeof(it->second));
			_MESSAGE("Wrote data to save: [Enchantment: 0x%08X] [Flags: %s] [Cost: %d]"
				,it->second.formID
				,it->second.flags ? "MANUAL" : "AUTO"
				,it->second.enchantmentCost);
		}
	}
}

void Serialization_Load(SKSESerializationInterface * intfc)
{
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
				if(version == kDataVersion)
				{
					for (;length > 0; length -= sizeof(EnchantmentInfoEntry))
					{
						_MESSAGE("  read remaining length = %d", length);
						EnchantmentInfoEntry thisEntry;
						UInt32 sizeRead = intfc->ReadRecordData(&thisEntry, sizeof(EnchantmentInfoEntry));
						if (sizeRead == sizeof(EnchantmentInfoEntry))
						{
							_MESSAGE("  about to read thisEntry.formID..");
							EnchantmentItem* pEnch = DYNAMIC_CAST(LookupFormByID(thisEntry.formID), TESForm, EnchantmentItem);
							_MESSAGE("  checking if cast succeeded..");
							if (pEnch)
							{
								_MESSAGE("  about set enchant flag data..");
								pEnch->data.unk00.unk04 |= thisEntry.flags;
								_MESSAGE("  about set enchant cost data..");
								pEnch->data.unk00.unk00 = thisEntry.enchantmentCost;
								_MESSAGE("Read & Set data from cosave: [Enchantment: 0x%08X] [Flags: %s] [Cost: %d]"
									,thisEntry.formID
									,thisEntry.flags ? "MANUAL" : "AUTO"
									,thisEntry.enchantmentCost);
								++recordsRead;
							}
						}
						else
							_MESSAGE("Error reading from cosave: INVALID CHUNK SIZE (%u expected %u)"
								" -- Data entry will be skipped.", sizeRead, sizeof(EnchantmentInfoEntry));
					}
				}
				else {  _MESSAGE("Error reading from cosave: UNKNOWN DATA VERSION %u, aborting...", version);   error = true;  }
				break;
			}
			
			default:
				_MESSAGE("Error reading from cosave: UNHANDLED TYPE %08X, aborting...", type);   error = true;
				break;
		}
	}

	if (recordsRead)
		_MESSAGE("%u enchantment records successfully processed.", recordsRead);

	// else if (BuildKnownBaseEnchantmentVec() && BuildPersistentFormsEnchantmentMap())
	// 	RunFirstLoadEnchantmentFix();

	MenuHandler::InitializeMenuMonitor(); //this [or part of this other than the menu registration] should probably go in Plugin_Load instead to avoid re-constructing variables
 				//also I should test multiple loads to make sure it doesn't result in multiple event registrations and firings building up
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
