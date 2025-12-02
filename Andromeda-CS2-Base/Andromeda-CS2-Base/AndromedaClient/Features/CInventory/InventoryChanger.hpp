#pragma once

#include <Common/Common.hpp>
#include <unordered_map>
#include <unordered_set>
#include <string>

class CCSGOInput;
class CUserCmd;
class C_CSWeaponBaseGun;
class C_BasePlayerWeapon;
class CEntityInstance;
class CHandle;
class IGameEvent;

struct SkinSelection {
    int paintkit = 0;
    int seed = 0;
    float wear = 0.0f;
    int stattrak = -1;
    std::string custom_name;
};

class InventoryChanger {
public:
    auto OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd) -> void;
    auto OnFrameStageNotify(int frameStage) -> void;
    auto OnFireEventClientSide(IGameEvent* pGameEvent) -> void;

    auto SetSelectedTeamCT(bool ct) -> void { m_IsCT = ct; }
    auto SetSelection(int def_index, const SkinSelection& sel) -> void;
    auto GetSelection(int def_index) const -> SkinSelection;
    auto SaveProfiles(const std::string& path) const -> bool;
    auto LoadProfiles(const std::string& path) -> bool;
    auto ApplyToLoadout(int def_index, bool ct, const SkinSelection& sel) -> bool;
    auto ApplyNowForActive() -> void;
    auto ApplyForAllOwnedWeapons() -> void;
    auto ApplyOnAdd(CEntityInstance* pInst, CHandle handle) -> void;
    auto ResetActiveSync() -> void { m_LastActiveEntIdx = -1; }
    auto ApplyToWeaponForced(C_CSWeaponBaseGun* weapon, const SkinSelection& sel) -> void;
    auto SetDebug(bool v) -> void { m_Debug = v; }
    auto IsDebug() const -> bool { return m_Debug; }
    auto ApplyLoop() -> void;
    auto ApplyGloves() -> void;
    auto IsOurItem(uint64_t id) const -> bool { return m_AddedItemIDs.find(id) != m_AddedItemIDs.end(); }
    std::unordered_set<int> m_EquippedDefaults;

private:
    auto ApplyToWeapon(C_CSWeaponBaseGun* weapon, const SkinSelection& sel) -> void;

private:
    bool m_IsCT = true;
    std::unordered_map<int, SkinSelection> m_LoadoutCT;
    std::unordered_map<int, SkinSelection> m_LoadoutT;
    int m_LastActiveEntIdx = -1;
    bool m_Debug = true;
    std::unordered_set<int> m_PendingWeapons;
    std::unordered_set<int> m_SkinnedEnts;
    std::unordered_set<uint64_t> m_AddedItemIDs;
    std::unordered_map<int,int> m_ApplyRetryCount;
};

auto GetInventoryChanger() -> InventoryChanger*;