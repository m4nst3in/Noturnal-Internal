#pragma once

#include <Common/Common.hpp>

class CCSInventoryManager;

// Definição do Hook para EquipItemInLoadout
using EquipItemInLoadout_t = bool(__fastcall*)(CCSInventoryManager*, int, int, uint64_t);
inline EquipItemInLoadout_t EquipItemInLoadout_o = nullptr;

auto Hook_EquipItemInLoadout(CCSInventoryManager* pInventoryManager, int iTeam, int iSlot, uint64_t iItemID) -> bool;