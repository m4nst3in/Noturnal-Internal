#include "InventoryChanger.hpp"

#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Interface/IEngineToClient.hpp>
#include <CS2/SDK/Update/CGlobalVarsBase.hpp>
#include <GameClient/CL_Weapons.hpp>
#include <CS2/SDK/Types/CEntityData.hpp>
#include <CS2/SDK/Types/CBaseTypes.hpp>
#include <CS2/SDK/Cstrike15/CCSPlayerInventory.hpp>
#include <CS2/SDK/Cstrike15/CCSInventoryManager.hpp>
#include <CS2/SDK/Econ/CEconItem.hpp>
#include <CS2/SDK/Econ/CEconItemDefinition.hpp>
#include <CS2/SDK/FunctionListSDK.hpp>
#include <CS2/SDK/GCSDK/GCSDKTypes/EconItemConstants.hpp>
#include <AndromedaClient/Features/CInventory/CSkinDatabase.hpp>
#include <GameClient/CL_Players.hpp>
#include <CS2/SDK/Interface/CGameEntitySystem.hpp>
#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/istreamwrapper.h>
#include <fstream>
#include <windows.h>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <unordered_map>

static InventoryChanger g_InvChanger{};

auto GetInventoryChanger() -> InventoryChanger* { return &g_InvChanger; }

static auto ic_now_string() -> std::string {
    using namespace std::chrono;
    auto tp = system_clock::now();
    auto t = system_clock::to_time_t(tp);
    tm bt{};
    localtime_s(&bt, &t);
    char buf[32];
    strftime(buf, sizeof(buf), "%H:%M:%S", &bt);
    return std::string(buf);
}

static auto ic_write_debug(const std::string& line) -> void {
    DEV_LOG("%s\n", ("[IC] " + line).c_str());
    static bool opened = false;
    static std::ofstream fout;
    if (!opened) {
        fout.open("inventory_changer.log", std::ios::out | std::ios::app);
        opened = true;
    }
    if (fout.is_open()) fout << "[IC] " << line << '\n';
}

auto InventoryChanger::SetSelection(int def_index, const SkinSelection& sel) -> void {
    if (m_IsCT) m_LoadoutCT[def_index] = sel; else m_LoadoutT[def_index] = sel;
    ApplyToLoadout(def_index, m_IsCT, sel);
    ApplyNowForActive();
}

auto InventoryChanger::GetSelection(int def_index) const -> SkinSelection {
    const auto& map = m_IsCT ? m_LoadoutCT : m_LoadoutT;
    auto it = map.find(def_index);
    if (it != map.end()) return it->second;
    return SkinSelection{};
}

auto InventoryChanger::OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd) -> void {
    ApplyLoop();
}

auto InventoryChanger::ApplyToWeapon(C_CSWeaponBaseGun* weapon, const SkinSelection& sel) -> void {
    if (!weapon) return;
    auto econ = reinterpret_cast<C_EconEntity*>(weapon);
    auto* attr = econ->m_AttributeManager();
    if (!attr) return;
    auto* view = attr->m_Item();
    if (!view) return;
    
    auto* base = reinterpret_cast<C_CSWeaponBase*>(weapon);
    if (!base->pEntityIdentity()) return;
    int entIdx = base->pEntityIdentity()->Handle().GetEntryIndex();

    int seed = std::clamp(sel.seed, 0, 999);
    float wear = std::clamp(sel.wear, 0.0f, 1.0f);
    DEV_LOG("[IC] %s ApplyToWeapon: Ent=%d DefIndex=%d Sel{pk=%d,seed=%d,wear=%.3f,st=%d} Curr{pk=%d,seed=%d,wear=%.3f}\n",
            ic_now_string().c_str(), entIdx, (int)view->m_iItemDefinitionIndex(), sel.paintkit, seed, wear, sel.stattrak,
            econ->m_nFallbackPaintKit(), econ->m_nFallbackSeed(), econ->m_flFallbackWear());
    
    if (view->m_iItemIDHigh() != -1) {
        m_ApplyRetryCount[entIdx] = 0;
    }

    bool values_correct = (econ->m_nFallbackPaintKit() == sel.paintkit &&
                           econ->m_nFallbackSeed() == seed &&
                           std::abs(econ->m_flFallbackWear() - wear) < 0.001f);

    if (values_correct && view->m_iItemIDHigh() == -1 && m_ApplyRetryCount[entIdx] > 20) {
        return; 
    }

    if (!values_correct && m_ApplyRetryCount[entIdx] > 20) {
        m_ApplyRetryCount[entIdx] = 0; 
    }

    if (m_Debug && m_ApplyRetryCount[entIdx] <= 5) {
        DEV_LOG("[IC] %s APPLYING (Try %d): Ent=%d PK=%d\n", ic_now_string().c_str(), m_ApplyRetryCount[entIdx], entIdx, sel.paintkit);
    }
    
    econ->m_nFallbackPaintKit() = sel.paintkit;
    econ->m_nFallbackSeed() = seed;
    econ->m_flFallbackWear() = wear;
    if (sel.stattrak >= 0) econ->m_nFallbackStatTrak() = sel.stattrak;

    auto* inv = CCSPlayerInventory::Get();
    if (inv) {
        uint32_t ownerLow = (uint32_t)(inv->GetOwner().ID() & 0xFFFFFFFF);
        uint32_t ownerHigh = (uint32_t)((inv->GetOwner().ID() >> 32) & 0xFFFFFFFF);
        DEV_LOG("[IC] %s ApplyToWeapon: Fallback IDs before {high=%d,low=%d,acc=%u}\n", ic_now_string().c_str(), (int)view->m_iItemIDHigh(), (int)view->m_iItemIDLow(), view->m_iAccountID());
        view->m_bDisallowSOC() = false;
        view->m_bInitialized() = false;
        view->m_bRestoreCustomMaterialAfterPrecache() = true;
        view->m_iItemIDHigh() = -1;
        view->m_iItemIDLow() = 0;
        view->m_iAccountID() = ownerLow;
        DEV_LOG("[IC] %s ApplyToWeapon: Fallback IDs after {high=%d,low=%d,acc=%u}\n", ic_now_string().c_str(), (int)view->m_iItemIDHigh(), (int)view->m_iItemIDLow(), view->m_iAccountID());
        econ->m_OriginalOwnerXuidLow() = ownerLow;
        econ->m_OriginalOwnerXuidHigh() = ownerHigh;
        DEV_LOG("[IC] %s ApplyToWeapon: Set OriginalOwnerXuid to local (low=%u,high=%u)\n", ic_now_string().c_str(), ownerLow, ownerHigh);
    }

    bool is_applied = (view->m_iItemIDHigh() == -1 &&
                       econ->m_nFallbackPaintKit() == sel.paintkit &&
                       econ->m_nFallbackSeed() == seed);
    if (is_applied) return;
    if (m_SkinnedEnts.find(entIdx) != m_SkinnedEnts.end()) return;

    auto* staticDef = C_EconItemView_GetStaticData(view);
    bool isKnifeDef = (staticDef && staticDef->IsKnife(false));
    int originalDefIdx = (int)view->m_iItemDefinitionIndex();
    if (!isKnifeDef && originalDefIdx > 0) {
        view->m_iItemDefinitionIndex() = 0;
        reinterpret_cast<CEntityInstance*>(base)->PostDataUpdate();
        reinterpret_cast<CEntityInstance*>(base)->OnDataChanged(1);
        view->m_iItemDefinitionIndex() = originalDefIdx;
        DEV_LOG("[IC] %s ApplyToWeapon: DefIndex nudge applied (0 -> %d)\n", ic_now_string().c_str(), originalDefIdx);
    }
    reinterpret_cast<CEntityInstance*>(base)->PostDataUpdate();
    reinterpret_cast<CEntityInstance*>(base)->OnDataChanged(1);
    C_CSWeaponBase_UpdateSkin(base, 1);
    ic_write_debug(ic_now_string() + std::string(" ApplyToWeapon: PostDataUpdate+OnDataChanged+UpdateSkin done"));
    if (auto* pawnVM = GetCL_Players()->GetLocalPlayerPawn()) {
        auto vms = pawnVM->GetViewModels();
        DEV_LOG("[IC] %s ApplyToWeapon: ViewModels count=%zu\n", ic_now_string().c_str(), vms.size());
        for (auto* vm : vms) {
            if (vm) {
                reinterpret_cast<CEntityInstance*>(vm)->PostDataUpdate();
                reinterpret_cast<CEntityInstance*>(vm)->OnDataChanged(1);
                ic_write_debug(ic_now_string() + std::string(" ApplyToWeapon: HUD vm PostDataUpdate+OnDataChanged"));
            }
        }
        auto* arms = pawnVM->m_hHudModelArms().Get<C_CS2HudModelArms>();
        if (arms) {
            reinterpret_cast<CEntityInstance*>(arms)->PostDataUpdate();
            reinterpret_cast<CEntityInstance*>(arms)->OnDataChanged(1);
            ic_write_debug(ic_now_string() + std::string(" ApplyToWeapon: HUD arms PostDataUpdate+OnDataChanged"));
        }
    }

    m_SkinnedEnts.insert(entIdx);
    DEV_LOG("[IC] APPLIED Skin on Ent=%d. Parando updates futuros.\n", entIdx);

    m_ApplyRetryCount[entIdx]++;
}

auto InventoryChanger::ApplyToLoadout(int def_index, bool ct, const SkinSelection& sel) -> bool {
    auto* inv = CCSPlayerInventory::Get();
    if (!inv) return false;
    auto* item = CEconItem::Create();
    if (!item) return false;
    auto ids = inv->GetHighestIDs();
    item->m_ulID = ids.first + 1;
    item->m_ulOriginalID = item->m_ulID;
    auto owner = inv->GetOwner();
    item->m_unAccountID = (uint32_t)(owner.ID() & 0xFFFFFFFF);
    item->m_unInventory = ids.second + 1;
    item->m_unDefIndex = (uint16_t)def_index;
    
    item->m_nQuality = IQ_UNIQUE; 
    item->m_unLevel = 1;
    
    int econRarity = IR_RARE;
    if (auto* sd = GetSkinDatabase()->GetSkinByPaintKit(sel.paintkit)) {
        switch (sd->m_eRarity) {
            case ESkinRarity::Common:      econRarity = IR_COMMON; break;
            case ESkinRarity::Uncommon:    econRarity = IR_UNCOMMON; break;
            case ESkinRarity::Rare:        econRarity = IR_RARE; break;
            case ESkinRarity::Mythical:    econRarity = IR_MYTHICAL; break;
            case ESkinRarity::Legendary:   econRarity = IR_LEGENDARY; break;
            case ESkinRarity::Ancient:     econRarity = IR_ANCIENT; break;
            case ESkinRarity::Contraband:  econRarity = IR_IMMORTAL; break;
            default:                       econRarity = IR_RARE; break;
        }
    }
    item->m_nRarity = econRarity;
    item->m_unFlags = 0;

    item->SetPaintKit((float)sel.paintkit);
    item->SetPaintSeed((float)sel.seed);
    item->SetPaintWear(sel.wear);
    if (sel.stattrak >= 0) item->SetStatTrak(sel.stattrak);

    if (!inv->AddEconItem(item)) {
        item->Destruct();
        return false;
    }
    m_AddedItemIDs.insert(item->m_ulID);
    auto* view = inv->GetItemViewForItem(item->m_ulID);
    if (!view) return false;
    auto* def = C_EconItemView_GetStaticData(view);
    if (!def) return false;
    int slot = def->LoadoutSlot();
    auto* mgr = CCSInventoryManager::Get();
    if (!mgr) return false;
    bool okCT = mgr->EquipItemInLoadout(3, slot, item->m_ulID);
    bool okT = mgr->EquipItemInLoadout(2, slot, item->m_ulID);
    return okCT || okT;
}

auto InventoryChanger::ApplyNowForActive() -> void {
    auto* weapon = GetCL_Weapons()->GetLocalActiveWeapon();
    if (!weapon) return;
    int def_index = GetCL_Weapons()->GetLocalWeaponDefinitionIndex();
    if (def_index <= 0) return;
    auto sel = GetSelection(def_index);
    if (sel.paintkit <= 0) return;
    ApplyToWeapon(weapon, sel);
}

auto InventoryChanger::ApplyForAllOwnedWeapons() -> void {}

auto InventoryChanger::ApplyOnAdd(CEntityInstance* pInst, CHandle /*handle*/) -> void {
    if (!pInst || !SDK::Interfaces::EngineToClient()->IsInGame()) return;
    int idx = -1;
    auto* base = reinterpret_cast<C_BaseEntity*>(pInst);
    if (base && base->pEntityIdentity()) idx = base->pEntityIdentity()->Handle().GetEntryIndex();
    if (idx > 0 && idx < 2048) m_PendingWeapons.insert(idx);
    DEV_LOG("[IC] %s OnAdd: idx=%d\n", ic_now_string().c_str(), idx);

    if (!base || !base->IsBasePlayerWeapon()) return;

    auto* inv = CCSPlayerInventory::Get();
    if (!inv) return;
    auto ownerId = inv->GetOwner().ID();

    auto* wep = reinterpret_cast<C_CSWeaponBaseGun*>(pInst);
    if (!wep) return;

    auto* econ = reinterpret_cast<C_EconEntity*>(wep);
    if (!econ) return;

    if (econ->GetOriginalOwnerXuid() != ownerId) return;

    auto* attr = econ->m_AttributeManager();
    if (!attr) return;
    auto* view = attr->m_Item();
    if (!view) return;

    int def = view->m_iItemDefinitionIndex();
    if (def <= 0 || def > 65535) return;
    DEV_LOG("[IC] %s OnAdd: def=%d\n", ic_now_string().c_str(), def);

    auto sel = GetSelection(def);
    if (sel.paintkit <= 0) return;

    ApplyToWeapon(wep, sel);

    auto* defData = C_EconItemView_GetStaticData(view);
    if (defData) {
        int team = wep->m_iOriginalTeamNumber();
        C_EconItemView* target = nullptr;
        if (defData->IsWeapon()) {
            for (int i = 0; i <= 56; ++i) {
                C_EconItemView* pItemView = inv->GetItemInLoadout(team, i);
                if (!pItemView) continue;
                if (pItemView->m_iItemDefinitionIndex() == static_cast<int>(defData->m_nDefIndex())) {
                    target = pItemView;
                    break;
                }
            }
        } else {
            int slot = defData->LoadoutSlot();
            target = inv->GetItemInLoadout(team, slot);
        }
        if (target && target->m_iItemDefinitionIndex() != view->m_iItemDefinitionIndex()) {
            view->m_iItemDefinitionIndex() = target->m_iItemDefinitionIndex();
            C_CSWeaponBase_UpdateSubclass(reinterpret_cast<C_CSWeaponBase*>(wep));
            C_CSWeaponBase_UpdateSkin(reinterpret_cast<C_CSWeaponBase*>(wep), 1);
            ic_write_debug(ic_now_string() + std::string(" OnAdd: subclass/skin updated from loadout"));
        }
    }
}

auto InventoryChanger::SaveProfiles(const std::string& path) const -> bool {
    rapidjson::Document doc; doc.SetObject();
    auto& alloc = doc.GetAllocator();
    rapidjson::Value ct(rapidjson::kArrayType);
    for (const auto& kv : m_LoadoutCT) {
        rapidjson::Value it(rapidjson::kObjectType);
        it.AddMember("def_index", kv.first, alloc);
        it.AddMember("paintkit", kv.second.paintkit, alloc);
        it.AddMember("seed", kv.second.seed, alloc);
        it.AddMember("wear", kv.second.wear, alloc);
        it.AddMember("stattrak", kv.second.stattrak, alloc);
        it.AddMember("name", rapidjson::Value().SetString(kv.second.custom_name.c_str(), (rapidjson::SizeType)kv.second.custom_name.size(), alloc), alloc);
        ct.PushBack(it, alloc);
    }
    rapidjson::Value tt(rapidjson::kArrayType);
    for (const auto& kv : m_LoadoutT) {
        rapidjson::Value it(rapidjson::kObjectType);
        it.AddMember("def_index", kv.first, alloc);
        it.AddMember("paintkit", kv.second.paintkit, alloc);
        it.AddMember("seed", kv.second.seed, alloc);
        it.AddMember("wear", kv.second.wear, alloc);
        it.AddMember("stattrak", kv.second.stattrak, alloc);
        it.AddMember("name", rapidjson::Value().SetString(kv.second.custom_name.c_str(), (rapidjson::SizeType)kv.second.custom_name.size(), alloc), alloc);
        tt.PushBack(it, alloc);
    }
    doc.AddMember("ct", ct, alloc);
    doc.AddMember("t", tt, alloc);
    std::ofstream out(path);
    if (!out.is_open()) return false;
    rapidjson::OStreamWrapper osw(out);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> wr(osw);
    wr.SetIndent('\t', 1);
    doc.Accept(wr);
    return true;
}

auto InventoryChanger::LoadProfiles(const std::string& path) -> bool {
    std::ifstream in(path);
    if (!in.is_open()) return false;
    rapidjson::IStreamWrapper isw(in);
    rapidjson::Document doc; doc.ParseStream(isw);
    if (doc.HasParseError() || !doc.IsObject()) return false;
    m_LoadoutCT.clear(); m_LoadoutT.clear();
    if (doc.HasMember("ct") && doc["ct"].IsArray()) {
        for (auto& it : doc["ct"].GetArray()) {
            if (!it.IsObject()) continue;
            SkinSelection s{};
            int def = it.HasMember("def_index") && it["def_index"].IsInt() ? it["def_index"].GetInt() : 0;
            s.paintkit = it.HasMember("paintkit") && it["paintkit"].IsInt() ? it["paintkit"].GetInt() : 0;
            s.seed = it.HasMember("seed") && it["seed"].IsInt() ? it["seed"].GetInt() : 0;
            s.wear = it.HasMember("wear") && it["wear"].IsNumber() ? it["wear"].GetFloat() : 0.0f;
            s.stattrak = it.HasMember("stattrak") && it["stattrak"].IsInt() ? it["stattrak"].GetInt() : -1;
            if (it.HasMember("name") && it["name"].IsString()) s.custom_name = it["name"].GetString();
            m_LoadoutCT[def] = s;
        }
    }
    if (doc.HasMember("t") && doc["t"].IsArray()) {
        for (auto& it : doc["t"].GetArray()) {
            if (!it.IsObject()) continue;
            SkinSelection s{};
            int def = it.HasMember("def_index") && it["def_index"].IsInt() ? it["def_index"].GetInt() : 0;
            s.paintkit = it.HasMember("paintkit") && it["paintkit"].IsInt() ? it["paintkit"].GetInt() : 0;
            s.seed = it.HasMember("seed") && it["seed"].IsInt() ? it["seed"].GetInt() : 0;
            s.wear = it.HasMember("wear") && it["wear"].IsNumber() ? it["wear"].GetFloat() : 0.0f;
            s.stattrak = it.HasMember("stattrak") && it["stattrak"].IsInt() ? it["stattrak"].GetInt() : -1;
            if (it.HasMember("name") && it["name"].IsString()) s.custom_name = it["name"].GetString();
            m_LoadoutT[def] = s;
        }
    }
    return true;
}
auto InventoryChanger::ApplyLoop() -> void {
    if (!SDK::Interfaces::EngineToClient()->IsInGame()) return;
    static bool s_checkedFns = false;
    if (!s_checkedFns) {
        auto* fl = GetFunctionList();
        void* pUpdSkin = fl ? fl->C_CSWeaponBase_UpdateSkin.GetFunction() : nullptr;
        void* pUpdSubc = fl ? fl->C_CSWeaponBase_UpdateSubclass.GetFunction() : nullptr;
        void* pEquip   = fl ? fl->CCSInventoryManager_EquipItemInLoadout.GetFunction() : nullptr;
        DEV_LOG("[IC] Ptrs: UpdateSkin=%p UpdateSubclass=%p EquipItemInLoadout=%p\n", pUpdSkin, pUpdSubc, pEquip);
        s_checkedFns = true;
    }
    if (auto* gv = SDK::Pointers::GlobalVarsBase()) {
        static int lastTick = 0;
        if (gv->m_nTickCount() - lastTick < 10) return;
        lastTick = gv->m_nTickCount();
    }
    auto* pawn = GetCL_Players()->GetLocalPlayerPawn();
    if (!pawn) return;
    m_IsCT = (pawn->m_iTeamNum() == 3);
    auto* ws = pawn->m_pWeaponServices();
    if (!ws) return;
    auto& vec = ws->m_hMyWeapons();
    for (int i = 0; i < (int)vec.Count(); ++i) {
        auto h = vec[i];
        auto* wep = h.Get<C_CSWeaponBaseGun>();
        if (!wep) continue;
        auto* attr = wep->m_AttributeManager();
        if (!attr) continue;
        auto* view = attr->m_Item();
        if (!view) continue;
        
        int def = view->m_iItemDefinitionIndex();
        if (def <= 0 || def > 65535) continue;
        
        auto sel = GetSelection(def);
        if (sel.paintkit <= 0) continue;
        
        ApplyToWeapon(wep, sel);
        m_PendingWeapons.erase(h.GetEntryIndex());
    }
    if (m_PendingWeapons.size() > 256) m_PendingWeapons.clear();

    this->ApplyGloves();
}

auto InventoryChanger::OnFrameStageNotify(int frameStage) -> void {
    if (!SDK::Interfaces::EngineToClient()->IsInGame()) return;
    if (frameStage != 6) return;

    auto* inv = CCSPlayerInventory::Get();
    if (!inv) return;
    auto ownerID = inv->GetOwner().ID();
    uint32_t ownerLow = (uint32_t)(ownerID & 0xFFFFFFFF);

    auto* pawn = GetCL_Players()->GetLocalPlayerPawn();
    if (!pawn || !pawn->IsAlive()) return;
    int team = pawn->m_iTeamNum();

    CGameEntitySystem* es = SDK::Interfaces::GameEntitySystem();
    if (!es) return;
    int highest = es->GetHighestEntityIndex();
    DEV_LOG("[IC] %s Stage6: enter team=%d highest=%d\n", ic_now_string().c_str(), team, highest);

    for (int i = 65; i <= highest; ++i) {
        C_BaseEntity* ent = es->GetBaseEntity(i);
        if (!ent || !ent->IsBasePlayerWeapon()) continue;

        C_CSWeaponBase* wep = reinterpret_cast<C_CSWeaponBase*>(ent);
        auto* econ = reinterpret_cast<C_EconEntity*>(wep);
        if (!econ) continue;
        if (econ->GetOriginalOwnerXuid() != ownerID) continue;
        DEV_LOG("[IC] %s Stage6: candidate entIndex=%d owner=local\n", ic_now_string().c_str(), i);

        auto* attr = econ->m_AttributeManager();
        if (!attr) continue;
        auto* view = attr->m_Item();
        if (!view) continue;

        auto* def = C_EconItemView_GetStaticData(view);
        if (!def) continue;
        C_EconItemView* loadoutView = nullptr;
        if (def->IsWeapon()) {
            for (int i = 0; i <= 56; ++i) {
                C_EconItemView* pItemView = inv->GetItemInLoadout(team, i);
                if (!pItemView) continue;
                if (pItemView->m_iItemDefinitionIndex() == static_cast<int>(def->m_nDefIndex())) {
                    loadoutView = pItemView;
                    break;
                }
            }
        } else {
            int slot = def->LoadoutSlot();
            loadoutView = inv->GetItemInLoadout(team, slot);
        }
        if (!loadoutView) continue;
        if (m_AddedItemIDs.find(loadoutView->m_iItemID()) == m_AddedItemIDs.end()) continue;

        CEconItemDefinition* loadoutDef = loadoutView->GetStaticData();
        if (!loadoutDef) continue;
        bool isKnife = loadoutDef->IsKnife(false);
        bool defMatches = (static_cast<int>(loadoutDef->m_nDefIndex()) == view->m_iItemDefinitionIndex());
        if (!defMatches && !isKnife) {
            continue;
        }

        view->m_bDisallowSOC() = false;
        loadoutView->m_bDisallowSOC() = false;

        view->m_iItemID() = loadoutView->m_iItemID();
        view->m_iItemIDHigh() = loadoutView->m_iItemIDHigh();
        view->m_iItemIDLow() = loadoutView->m_iItemIDLow();
        view->m_iAccountID() = loadoutView->m_iAccountID();
        if (isKnife) {
            view->m_iItemDefinitionIndex() = static_cast<int>(loadoutDef->m_nDefIndex());
            C_CSWeaponBase_UpdateSubclass(wep);
            const char* knifeModel = loadoutDef->m_pszModelName();
            if (knifeModel && *knifeModel) {
                wep->SetModel(knifeModel);
            }
            DEV_LOG("[IC] %s Stage6: Knife subclass+model applied DefIndex=%d ModelName=%s\n", ic_now_string().c_str(), (int)view->m_iItemDefinitionIndex(), knifeModel ? knifeModel : "");
        }
        if (view->m_iItemIDHigh() != -1 && view->m_iAccountID() == ownerLow) {
            view->m_bDisallowSOC() = false;
            view->m_bInitialized() = false;
            view->m_bRestoreCustomMaterialAfterPrecache() = true;
            view->m_iItemIDHigh() = -1;
            view->m_iItemIDLow() = 0;
            view->m_iAccountID() = ownerLow;
            auto* staticDef2 = C_EconItemView_GetStaticData(view);
            bool isKnife2 = (staticDef2 && staticDef2->IsKnife(false));
            int originalDefIdx2 = (int)view->m_iItemDefinitionIndex();
            if (!isKnife2 && originalDefIdx2 > 0) {
                view->m_iItemDefinitionIndex() = 0;
                reinterpret_cast<CEntityInstance*>(wep)->PostDataUpdate();
                reinterpret_cast<CEntityInstance*>(wep)->OnDataChanged(1);
                view->m_iItemDefinitionIndex() = originalDefIdx2;
                DEV_LOG("[IC] %s Stage6: DefIndex nudge applied (0 -> %d)\n", ic_now_string().c_str(), originalDefIdx2);
            }
            reinterpret_cast<CEntityInstance*>(wep)->PostDataUpdate();
            reinterpret_cast<CEntityInstance*>(wep)->OnDataChanged(1);
            C_CSWeaponBase_UpdateSkin(wep, 1);
            ic_write_debug(ic_now_string() + std::string(" Stage6: Anti-revert applied and UpdateSkin done"));
            auto* pawnVM = GetCL_Players()->GetLocalPlayerPawn();
            if (pawnVM) {
                auto vms = pawnVM->GetViewModels();
                DEV_LOG("[IC] %s Stage6: HUD vm count=%zu\n", ic_now_string().c_str(), vms.size());
                for (auto* vm : vms) {
                    if (vm) {
                        reinterpret_cast<CEntityInstance*>(vm)->PostDataUpdate();
                        reinterpret_cast<CEntityInstance*>(vm)->OnDataChanged(1);
                        ic_write_debug(ic_now_string() + std::string(" Stage6: HUD vm PostDataUpdate+OnDataChanged"));
                    }
                }
                auto* arms = pawnVM->m_hHudModelArms().Get<C_CS2HudModelArms>();
                if (arms) {
                    reinterpret_cast<CEntityInstance*>(arms)->PostDataUpdate();
                    reinterpret_cast<CEntityInstance*>(arms)->OnDataChanged(1);
                    ic_write_debug(ic_now_string() + std::string(" Stage6: HUD arms PostDataUpdate+OnDataChanged"));
                }
            }
        }
        auto* staticDef3 = C_EconItemView_GetStaticData(view);
        bool isKnife3 = (staticDef3 && staticDef3->IsKnife(false));
        int originalDefIdx3 = (int)view->m_iItemDefinitionIndex();
        if (!isKnife3 && originalDefIdx3 > 0) {
            view->m_iItemDefinitionIndex() = 0;
            reinterpret_cast<CEntityInstance*>(wep)->PostDataUpdate();
            reinterpret_cast<CEntityInstance*>(wep)->OnDataChanged(1);
            view->m_iItemDefinitionIndex() = originalDefIdx3;
            DEV_LOG("[IC] %s Stage6: Final DefIndex nudge applied (0 -> %d)\n", ic_now_string().c_str(), originalDefIdx3);
        }
        int wepEntIdx = wep->pEntityIdentity() ? wep->pEntityIdentity()->Handle().GetEntryIndex() : -1;
        if (wepEntIdx >= 0 && m_SkinnedEnts.find(wepEntIdx) == m_SkinnedEnts.end()) {
            auto* staticDef3 = C_EconItemView_GetStaticData(view);
            bool isKnife3 = (staticDef3 && staticDef3->IsKnife(false));
            int originalDefIdx3 = (int)view->m_iItemDefinitionIndex();
            if (!isKnife3 && originalDefIdx3 > 0) {
                view->m_iItemDefinitionIndex() = 0;
                reinterpret_cast<CEntityInstance*>(wep)->PostDataUpdate();
                reinterpret_cast<CEntityInstance*>(wep)->OnDataChanged(1);
                view->m_iItemDefinitionIndex() = originalDefIdx3;
                DEV_LOG("[IC] %s Stage6: Final DefIndex nudge applied (0 -> %d)\n", ic_now_string().c_str(), originalDefIdx3);
            }
            reinterpret_cast<CEntityInstance*>(wep)->PostDataUpdate();
            reinterpret_cast<CEntityInstance*>(wep)->OnDataChanged(1);
            C_CSWeaponBase_UpdateSkin(wep, 1);
            m_SkinnedEnts.insert(wepEntIdx);
        }
        ic_write_debug(ic_now_string() + std::string(" Stage6: Final UpdateSkin done"));
        auto* pawnVM2 = GetCL_Players()->GetLocalPlayerPawn();
        if (pawnVM2) {
            auto vms2 = pawnVM2->GetViewModels();
            DEV_LOG("[IC] %s Stage6: HUD vm2 count=%zu\n", ic_now_string().c_str(), vms2.size());
            for (auto* vm : vms2) {
                if (vm) {
                    reinterpret_cast<CEntityInstance*>(vm)->PostDataUpdate();
                    reinterpret_cast<CEntityInstance*>(vm)->OnDataChanged(1);
                    ic_write_debug(ic_now_string() + std::string(" Stage6: HUD vm2 PostDataUpdate+OnDataChanged"));
                }
            }
            auto* arms2 = pawnVM2->m_hHudModelArms().Get<C_CS2HudModelArms>();
            if (arms2) {
                reinterpret_cast<CEntityInstance*>(arms2)->PostDataUpdate();
                reinterpret_cast<CEntityInstance*>(arms2)->OnDataChanged(1);
                ic_write_debug(ic_now_string() + std::string(" Stage6: HUD arms2 PostDataUpdate+OnDataChanged"));
            }
        }
    }
}

auto InventoryChanger::OnFireEventClientSide( IGameEvent* pGameEvent ) -> void {
    if (!pGameEvent) return;
    const char* name = IGameEvent_GetName( pGameEvent );
    if (!name) return;
    if (strcmp(name, "item_purchase") == 0 || strcmp(name, "round_start") == 0) {
        m_ApplyRetryCount.clear();
        m_PendingWeapons.clear();
        ic_write_debug( std::string("Event: ") + name + " -> reset retry counters" );
    }
}

auto InventoryChanger::ApplyGloves() -> void {
    auto* inv = CCSPlayerInventory::Get();
    if (!inv) return;
    auto* pawn = GetCL_Players()->GetLocalPlayerPawn();
    if (!pawn || !pawn->IsAlive()) return;

    auto* glovesView = &pawn->m_EconGloves();
    if (!glovesView) return;
    auto* glovesDef = glovesView->GetStaticData();
    if (!glovesDef) return;

    int team = pawn->m_iTeamNum();
    int slot = glovesDef->LoadoutSlot();
    auto* equipped = inv->GetItemInLoadout(team, slot);
    if (!equipped) return;

    if (glovesView->m_iItemID() != equipped->m_iItemID()) {
        glovesView->m_bDisallowSOC() = false;
        glovesView->m_bInitialized() = true;
        glovesView->m_iItemID() = equipped->m_iItemID();
        glovesView->m_iItemIDHigh() = equipped->m_iItemIDHigh();
        glovesView->m_iItemIDLow() = equipped->m_iItemIDLow();
        glovesView->m_iAccountID() = equipped->m_iAccountID();
        glovesView->m_iItemDefinitionIndex() = equipped->m_iItemDefinitionIndex();
        pawn->m_bNeedToReApplyGloves() = true;
    }
}