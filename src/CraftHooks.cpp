#include "CraftHooks.h"
#include "skse/GameTypes.h"
#include "skse/GameRTTI.h"
#include "skse/SafeWrite.h"
#include "skse/Utilities.h"

EnchantCraftMonitor		g_craftData;


EnchantmentItem* __stdcall CraftWeaponEnchantment(tArray<MagicItem::EffectItem>* effectArray, EnchantmentItem* createdEnchantment)
{
	g_craftData.Commit(createdEnchantment, EnchantCraftMonitor::kEnchantmentType_Weapon);
	return createdEnchantment;
}

EnchantmentItem* __stdcall CraftArmorEnchantment(tArray<MagicItem::EffectItem>* effectArray, EnchantmentItem* createdEnchantment)
{
	g_craftData.Commit(createdEnchantment, EnchantCraftMonitor::kEnchantmentType_Armor);
	return createdEnchantment;
}

static const UInt32 kCreateWeaponEnchantment_Hook_Base		= 0x00689D30;
static const UInt32 kCreateWeaponEnchantment_Hook_Enter		= kCreateWeaponEnchantment_Hook_Base + 0x20;
static const UInt32 kCreateWeaponEnchantment_Hook_Return	= kCreateWeaponEnchantment_Hook_Base + 0x28;
static const UInt32 kCreateArmorEnchantment_Hook_Base		= 0x00689D80;
static const UInt32 kCreateArmorEnchantment_Hook_Enter		= kCreateArmorEnchantment_Hook_Base + 0x20;
static const UInt32 kCreateArmorEnchantment_Hook_Return		= kCreateArmorEnchantment_Hook_Base + 0x28;
static const UInt32 kCreateEnchantment_Core					= 0x006899C0; //this is what actually does the work

__declspec(naked) void CreateWeaponEnchantment_Entry(void)
{
		__asm
		{
				call		[kCreateEnchantment_Core]	// (returns EnchantmentItem*)
				add			esp, 0x10					// cdecl call, do manual cleanup

				sub			esp, 4						// shift stack pointer down to reserve a placeholder		sub 0x04

				pushad									// (save registers to stack)								sub 0x20
				push		eax							// push result from the previous call as second argument	sub 0x04
				mov			eax, [esp + 0x30 + 0x04]	// get original param back from stack...   [orig offset 0x4] + [0x28 + 0x8 local vars]
				push		eax							// ...and push as first argument
				call 		CraftWeaponEnchantment		// stdcall so we don't need to do work
				mov  		[esp + 0x20], eax 			// save result into placeholder
				popad									// (restore registers [shifts esp back up 0x20])

				pop			eax							// pop result from placeholder

				jmp  		[kCreateWeaponEnchantment_Hook_Return]
		}
}

__declspec(naked) void CreateArmorEnchantment_Entry(void)
{
		__asm
		{
				call		[kCreateEnchantment_Core]	// (returns EnchantmentItem*)
				add			esp, 0x10					// cdecl call, do manual cleanup

				sub			esp, 4						// shift stack pointer down to reserve a placeholder		sub 0x04

				pushad									// (save registers to stack)								sub 0x20
				push		eax							// push result from the previous call as second argument	sub 0x04
				mov			eax, [esp + 0x30 + 0x04]	// get original param back from stack...   [orig offset 0x4] + [0x28 + 0x8 local vars]
				push		eax							// ...and push as first argument
				call 		CraftArmorEnchantment		// stdcall so we don't need to do work
				mov  		[esp + 0x20], eax 			// save result into placeholder
				popad									// (restore registers [shifts esp back up 0x20])

				pop			eax							// pop result from placeholder

				jmp  		[kCreateArmorEnchantment_Hook_Return]
		}
}

void CreateEnchantmentHook_Commit(void)
{
	WriteRelJump(kCreateWeaponEnchantment_Hook_Enter, (UInt32)CreateWeaponEnchantment_Entry);
	WriteRelJump(kCreateArmorEnchantment_Hook_Enter, (UInt32)CreateArmorEnchantment_Entry);
}


//(older version of the above)
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

	MagicItem* magicItem = ((MagicItem*)this);
	EnchantmentItem* enchantment = DYNAMIC_CAST(magicItem, MagicItem, EnchantmentItem);
	g_craftData.Push(enchantment); //Queue parent enchantments that are being combined to craft this new enchantment

	return result;
}

void GetCostliestEffectItemHook::Hook_Commit(void)
{
	WriteRelCall(0x00851DEB, GetFnAddr(&GetCostliestEffectItemHook::CostliestEffect_Hook));
}