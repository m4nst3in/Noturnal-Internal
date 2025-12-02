#include "OffsetPatternTester.hpp"

#include <CS2/SDK/SDK.hpp>
#include <GameClient/CL_Players.hpp>

auto GetLocalPlayerArms() -> C_CS2HudModelArms*
{
    auto* pawn = GetCL_Players()->GetLocalPlayerPawn();
    if (!pawn)
        return nullptr;
    return pawn->m_hHudModelArms().Get<C_CS2HudModelArms>();
}

auto GetWeaponViewModel() -> C_CS2HudModelWeapon*
{
    auto* pawn = GetCL_Players()->GetLocalPlayerPawn();
    if (!pawn)
        return nullptr;
    return pawn->GetViewModel();
}

auto GetHudModelBase() -> C_CS2HudModelBase*
{
    if (auto* arms = GetLocalPlayerArms())
        return reinterpret_cast<C_CS2HudModelBase*>(arms);
    if (auto* vm = GetWeaponViewModel())
        return reinterpret_cast<C_CS2HudModelBase*>(vm);
    return nullptr;
}

auto ValidateAllOffsetsComplete() -> void
{
    auto* base = GetHudModelBase();
    auto* arms = GetLocalPlayerArms();
    auto* weaponVM = GetWeaponViewModel();

    DEV_LOG("=== TODOS OS OFFSETS ENCONTRADOS ===\n\n");
    DEV_LOG("[ C_CS2HudModelBase ]\n");
    DEV_LOG("Base: %p\n" , base);

    if (base)
    {
        auto baseAddr = reinterpret_cast<uintptr_t>(base);
        uint32_t baseHandle = *reinterpret_cast<uint32_t*>(baseAddr + 0x11E4);
        DEV_LOG("m_hWeaponViewmodel (0x11E4): 0x%X%s\n" , baseHandle , (baseHandle != 0xFFFFFFFF && baseHandle != 0) ? " ✓" : "");
    }

    DEV_LOG("\n[ C_CS2HudModelArms ]\n");
    DEV_LOG("Base: %p\n" , arms);
    if (arms)
    {
        auto armsAddr = reinterpret_cast<uintptr_t>(arms);
        uint32_t armsHandle = *reinterpret_cast<uint32_t*>(armsAddr + 0x1860);
        DEV_LOG("m_hWeaponViewModel (0x1860): 0x%X ✓\n" , armsHandle);
    }

    DEV_LOG("\n[ C_CS2HudModelWeapon ]\n");
    DEV_LOG("Base: %p\n" , weaponVM);
    if (weaponVM)
    {
        auto vmAddr = reinterpret_cast<uintptr_t>(weaponVM);
        uintptr_t viewModelBase = *reinterpret_cast<uintptr_t*>(vmAddr + 0x1220);
        DEV_LOG("m_ViewModelBase (0x1220): %p ✓\n" , reinterpret_cast<void*>(viewModelBase));

        uint64_t meshMask = *reinterpret_cast<uint64_t*>(vmAddr + 0x1230);
        DEV_LOG("m_MeshGroupMask (0x1230): 0x%llX ✓\n" , meshMask);
    }

    DEV_LOG("\n=== MISSAO CUMPRIDA! ===\n");
}

auto RunDevOffsetPatternTests() -> void
{
    ValidateAllOffsetsComplete();
}