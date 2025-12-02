#include "CSkinDatabase.hpp"
#include <ImGui/imgui.h>
#include <algorithm>

auto CSkinDatabase::Get() -> CSkinDatabase* { static CSkinDatabase inst; return &inst; }
auto GetSkinDatabase() -> CSkinDatabase* { return CSkinDatabase::Get(); }

auto CSkinDatabase::Initialize() -> void {
    if (m_bInitialized) return;
    LoadStaticSkinDatabase();
    BuildCompatibilityMap();
    m_bInitialized = true;
}

auto CSkinDatabase::GetSkinsForWeapon(int defIndex) const -> std::vector<const SkinData_t*> {
    std::vector<const SkinData_t*> out;
    auto it = m_mWeaponToSkins.find(defIndex);
    if (it == m_mWeaponToSkins.end()) return out;
    for (int pk : it->second) {
        auto it2 = m_mPaintKitToSkin.find(pk);
        if (it2 != m_mPaintKitToSkin.end()) out.push_back(it2->second);
    }
    return out;
}

auto CSkinDatabase::GetSkinByPaintKit(int paintKitID) const -> const SkinData_t* {
    auto it = m_mPaintKitToSkin.find(paintKitID);
    return it == m_mPaintKitToSkin.end() ? nullptr : it->second;
}

auto CSkinDatabase::GetWeaponsByCategory(EWeaponCategory category) const -> std::vector<const WeaponSkinData_t*> {
    std::vector<const WeaponSkinData_t*> out;
    for (auto& w : m_vWeapons) if (w.m_eCategory == category) out.push_back(&w);
    return out;
}

auto CSkinDatabase::GetWeaponByDefIndex(int defIndex) const -> const WeaponSkinData_t* {
    auto it = m_mDefIndexToWeapon.find(defIndex);
    return it == m_mDefIndexToWeapon.end() ? nullptr : it->second;
}

auto CSkinDatabase::FilterSkins(const std::vector<const SkinData_t*>& skins, const std::string& query) const -> std::vector<const SkinData_t*> {
    if (query.empty()) return skins;
    std::vector<const SkinData_t*> out;
    std::string q = query; std::transform(q.begin(), q.end(), q.begin(), ::tolower);
    for (auto* s : skins) {
        std::string n = s->m_sName; std::transform(n.begin(), n.end(), n.begin(), ::tolower);
        if (n.find(q) != std::string::npos) out.push_back(s);
    }
    return out;
}

auto CSkinDatabase::GetRarityName(ESkinRarity rarity) -> const char* {
    switch (rarity) {
        case ESkinRarity::Common: return "Consumer Grade";
        case ESkinRarity::Uncommon: return "Industrial Grade";
        case ESkinRarity::Rare: return "Mil-Spec";
        case ESkinRarity::Mythical: return "Restricted";
        case ESkinRarity::Legendary: return "Classified";
        case ESkinRarity::Ancient: return "Covert";
        case ESkinRarity::Contraband: return "Contraband";
        default: return "Unknown";
    }
}

auto CSkinDatabase::GetRarityColor(ESkinRarity rarity) -> ImU32 {
    switch (rarity) {
        case ESkinRarity::Common: return IM_COL32(255,255,255,255);
        case ESkinRarity::Uncommon: return IM_COL32(173,216,230,255);
        case ESkinRarity::Rare: return IM_COL32(0,102,204,255);
        case ESkinRarity::Mythical: return IM_COL32(147,112,219,255);
        case ESkinRarity::Legendary: return IM_COL32(255,105,180,255);
        case ESkinRarity::Ancient: return IM_COL32(220,20,60,255);
        case ESkinRarity::Contraband: return IM_COL32(255,165,0,255);
        default: return IM_COL32(200,200,200,255);
    }
}

auto CSkinDatabase::GetWearName(ESkinWear wear) -> const char* {
    switch (wear) {
        case ESkinWear::FactoryNew: return "Factory New";
        case ESkinWear::MinimalWear: return "Minimal Wear";
        case ESkinWear::FieldTested: return "Field-Tested";
        case ESkinWear::WellWorn: return "Well-Worn";
        case ESkinWear::BattleScarred: return "Battle-Scarred";
        default: return "Unknown";
    }
}

auto CSkinDatabase::GetWearRange(ESkinWear wear) -> std::pair<float, float> {
    switch (wear) {
        case ESkinWear::FactoryNew: return {0.00f, 0.07f};
        case ESkinWear::MinimalWear: return {0.07f, 0.15f};
        case ESkinWear::FieldTested: return {0.15f, 0.38f};
        case ESkinWear::WellWorn: return {0.38f, 0.45f};
        case ESkinWear::BattleScarred: return {0.45f, 1.00f};
        default: return {0.00f, 1.00f};
    }
}

auto CSkinDatabase::UpdateFromApi(const std::vector<SkinData_t>& skins, const std::vector<WeaponSkinData_t>& weapons) -> void {
    m_vSkins = skins;
    m_vWeapons = weapons;
    m_mPaintKitToSkin.clear();
    m_mDefIndexToWeapon.clear();
    m_mWeaponToSkins.clear();
    for (auto& s : m_vSkins) m_mPaintKitToSkin[s.m_nPaintKitID] = &s;
    for (auto& w : m_vWeapons) m_mDefIndexToWeapon[w.m_nDefIndex] = &w;
    BuildCompatibilityMap();
    m_bInitialized = true;
}

auto CSkinDatabase::LoadStaticSkinDatabase() -> void {
    m_vWeapons.clear();
    m_vSkins.clear();
}

auto CSkinDatabase::LoadSkinsFromGame() -> void {}
auto CSkinDatabase::LoadWeaponsFromGame() -> void {}

auto CSkinDatabase::BuildCompatibilityMap() -> void {
    m_mWeaponToSkins.clear();
    for (auto& s : m_vSkins) {
        for (int def : s.m_vCompatibleWeapons) {
            m_mWeaponToSkins[def].push_back(s.m_nPaintKitID);
        }
    }
}