#include "Hook_EquipItemInLoadout.hpp"
#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Cstrike15/CCSPlayerInventory.hpp>
#include <CS2/SDK/Econ/CEconItem.hpp>
#include <CS2/SDK/Econ/CEconItemDefinition.hpp>
#include <AndromedaClient/Features/CInventory/InventoryChanger.hpp>
#include <CS2/SDK/FunctionListSDK.hpp>
#include <Common/Common.hpp>

auto Hook_EquipItemInLoadout(CCSInventoryManager* pInventoryManager, int iTeam, int iSlot, uint64_t iItemID) -> bool {
    auto* pInvChanger = GetInventoryChanger();
    auto* pInventory  = CCSPlayerInventory::Get();
    if (pInvChanger && pInventory) {
        auto* pItemView = pInventory->GetItemViewForItem(iItemID);
        if (pItemView) {
            auto* def = C_EconItemView_GetStaticData(pItemView);
            if (def) {
                const bool isKnife = def->IsKnife(false);
                const bool isGlove = def->IsGlove(false);
                if (pInvChanger->IsOurItem(iItemID) && !isKnife && !isGlove) {
                    const int defIndex = pItemView->m_iItemDefinitionIndex();
                    const uint64_t defaultItemID = (std::uint64_t(0xF) << 60) | (uint64_t)defIndex;
                    DEV_LOG("[hook] EquipItemInLoadout override -> team=%d slot=%d def=%d defaultID=%llu\n", iTeam, iSlot, defIndex, defaultItemID);
                    return EquipItemInLoadout_o(pInventoryManager, iTeam, iSlot, defaultItemID);
                }
            }
        }
    }
    return EquipItemInLoadout_o(pInventoryManager, iTeam, iSlot, iItemID);
}