#include "CraftHooks.h"
#include "skse/GameTypes.h"
#include "skse/GameRTTI.h"
#include "skse/SafeWrite.h"
#include "skse/Utilities.h"

//#include "skse/GameObjects.h"

EnchantmentCraftMonitor		g_craftData;





EnchantmentItem* __stdcall CraftWeaponEnchantment(UInt32 type, tArray<MagicItem::EffectItem>* effectArray, EnchantmentItem* createdEnchantment/*, UInt32 type*/)
{
	enum
	{
		kType_ArmorEnchantment,
		kType_WeaponEnchantment
	};

	if (type == kType_WeaponEnchantment) _MESSAGE("-----------------------weapon enchantment-----------------------");
	else if (type == kType_ArmorEnchantment) _MESSAGE("-----------------------armor enchantment-----------------------");

	g_craftData.Commit(createdEnchantment);

	// _MESSAGE("\nCreateEnchantment event for enchantment 0x%08X", createdEnchantment->formID);
	// for (UInt32 n = 0; n < effectArray->count; n++)
	// {
	// 	MagicItem::EffectItem* effectItem = NULL;
	// 	effectItem = &effectArray->arr.entries[n];
	// 	_MESSAGE("    effect %u mgef  =  %s [%08X]", n, ((DYNAMIC_CAST(effectItem->mgef, EffectSetting, TESFullName))->name.data), effectItem->mgef->formID);
	// }

	return createdEnchantment;
}

static const UInt32 kCreateWeaponEnchantment_Hook_Base		= 0x00689D30;
static const UInt32 kCreateWeaponEnchantment_Hook_Enter		= kCreateWeaponEnchantment_Hook_Base + 0x20;
static const UInt32 kCreateWeaponEnchantment_Hook_Return	= kCreateWeaponEnchantment_Hook_Base + 0x28;
static const UInt32 kCreateArmorEnchantment_Hook_Base		= 0x00689D80;
static const UInt32 kCreateArmorEnchantment_Hook_Enter		= kCreateArmorEnchantment_Hook_Base + 0x20;
static const UInt32 kCreateArmorEnchantment_Hook_Return		= kCreateArmorEnchantment_Hook_Base + 0x28;
static const UInt32 kCreateEnchantment_Core					= 0x006899C0; //this is what actually does the work

__declspec(naked) void CreateEnchantment_Entry(void)
{
		__asm
		{
				sub			esp, 4						//shift 4 args on the stack down to reserve placeholder		(esp + 0x04)
				mov			[esp], edi
				mov			edi, [esp + 8]
				mov			[esp + 4], edi
				mov			edi, [esp + 0xC]
				mov			[esp + 8], edi
				mov			edi, [esp + 0x10]			//final shift leaves copy of arg in placeholder @ esp+0x10 [0 = armorEnch, 1 = weaponEnch]
				mov			[esp + 0xC], edi
				mov			edi, [esp]					//restore edi now that shift is complete

				call		[kCreateEnchantment_Core]	// (returns EnchantmentItem*)
				add			esp, 0x10					// cdecl call, do manual cleanup (esp now points to our saved arg)

				pushad									// (save registers to stack)								(esp + 0x20)
				push		eax							// push result from the previous call as third argument		(esp + 0x04)
				mov			eax, [esp + 0x30 + 0x04]	// get original parameter back from stack...  [orig. 0x04 offset + 2 local vars + 0x28]
				push		eax							// ...and push as second argument

				mov			eax, [esp + 0x28]			// get the arg we saved above when shifting the stack down...
				push		eax							// ...and push as first argument

				call 		CraftWeaponEnchantment		// stdcall so we don't need to clean up
				mov  		[esp + 0x20], eax 			// save result into placeholder, we no longer need the value that was there
				popad									// (restore registers)

				pop			eax							// pop result from placeholder

				jmp  		[kCreateWeaponEnchantment_Hook_Return]
		}
}

void CraftHook_Commit(void)
{
	WriteRelJump(kCreateWeaponEnchantment_Hook_Enter, (UInt32)CreateEnchantment_Entry);
	WriteRelJump(kCreateArmorEnchantment_Hook_Enter, (UInt32)CreateEnchantment_Entry);
}



//(this works fine, just decided to use the approach above instead)
	// class WeaponEnchantCraftHook
	// {
	// public:
	// 	EnchantmentItem* CraftEvent_Hook(tArray<MagicItem::EffectItem>* effectArray)
	// 	{
	// 		EnchantmentItem* result = CALL_MEMBER_FN(this, CreateOffensiveEnchantment)(effectArray);

	// 		//do stuff

	// 		return result;
	// 	}

	// 	MEMBER_FN_PREFIX(WeaponEnchantCraftHook);
	// 	DEFINE_MEMBER_FN(CreateOffensiveEnchantment, EnchantmentItem *, 0x00689D30, tArray<MagicItem::EffectItem> * effectArray);

	// 	static void CraftHook_Commit(void)
	// 	{
	// 		WriteRelCall(0x00532FBC, GetFnAddr(&WeaponEnchantCraftHook::CraftEvent_Hook));
	// 		WriteRelCall(0x00851BF0 + 0x23C, GetFnAddr(&WeaponEnchantCraftHook::CraftEvent_Hook));
	// 		WriteRelCall(0x00852DF6, GetFnAddr(&WeaponEnchantCraftHook::CraftEvent_Hook));
	// 	}

	// };


MagicItem::EffectItem* GetCostliestEffectItemHook::CostliestEffect_Hook(UInt32 arg1, bool arg2)
{
	EffectItem* result = CALL_MEMBER_FN(this, GetCostliestEffectItem)(arg1, arg2); //Member Fn of MagicItem

	// _MESSAGE("\nCostliestEffect Event");

	MagicItem* magicItem = ((MagicItem*)this);
	EnchantmentItem* enchantment = DYNAMIC_CAST(magicItem, MagicItem, EnchantmentItem);
	// if (enchantment)
	// 	_MESSAGE("    thisEnchantment = %08X [%s]", enchantment->formID, (DYNAMIC_CAST(enchantment, EnchantmentItem, TESFullName))->name.data);

	g_craftData.Push(enchantment); //Queue parent enchantments that are being combined to craft this new enchantment

	return result;
}

void GetCostliestEffectItemHook::CostliestEffect_Hook_Commit(void)
{
	WriteRelCall(0x00851DEB, GetFnAddr(&GetCostliestEffectItemHook::CostliestEffect_Hook));
}