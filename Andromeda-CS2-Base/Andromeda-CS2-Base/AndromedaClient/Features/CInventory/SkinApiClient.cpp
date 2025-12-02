#include "SkinApiClient.hpp"

#include <AndromedaClient/Features/CInventory/CSkinDatabase.hpp>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>

#include <fstream>
#include <sstream>
#include <windows.h>
#include <winhttp.h>
#include <ImGui/imgui.h>
#include <unordered_map>

#pragma comment(lib, "winhttp.lib")

static constexpr const char* kSkinCacheFile = "SkinCache.json";

auto SkinApiClient::LoadCache(std::string& outJson) -> bool {
    std::ifstream in(kSkinCacheFile, std::ios::in | std::ios::binary);
    if (!in.is_open()) return false;
    std::ostringstream ss; ss << in.rdbuf();
    outJson = ss.str();
    return !outJson.empty();
}

auto SkinApiClient::SaveCache(const std::string& json) -> bool {
    std::ofstream out(kSkinCacheFile, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;
    out.write(json.data(), (std::streamsize)json.size());
    return true;
}

auto SkinApiClient::FetchJson(std::string& outJson) -> bool {
    if (m_Endpoint.empty()) return false;

    URL_COMPONENTS comp{0};
    comp.dwStructSize = sizeof(comp);
    comp.dwSchemeLength = (DWORD)-1;
    comp.dwHostNameLength = (DWORD)-1;
    comp.dwUrlPathLength = (DWORD)-1;
    comp.dwExtraInfoLength = (DWORD)-1;

    wchar_t wurl[1024];
    MultiByteToWideChar(CP_UTF8, 0, m_Endpoint.c_str(), -1, wurl, 1024);
    if (!WinHttpCrackUrl(wurl, 0, 0, &comp)) return false;

    std::wstring host(comp.lpszHostName, comp.dwHostNameLength);
    std::wstring path(comp.lpszUrlPath, comp.dwUrlPathLength);
    HINTERNET hSession = WinHttpOpen(L"Andromeda/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;
    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), comp.nPort ? comp.nPort : INTERNET_DEFAULT_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }
    DWORD flags = 0;
    if (comp.nScheme == INTERNET_SCHEME_HTTPS) flags |= WINHTTP_FLAG_SECURE;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    bool ok = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
        && WinHttpReceiveResponse(hRequest, nullptr);
    std::string result;
    if (ok) {
        DWORD dwSize = 0;
        do {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
            if (dwSize == 0) break;
            std::string buf; buf.resize(dwSize);
            DWORD dwRead = 0;
            if (!WinHttpReadData(hRequest, buf.data(), dwSize, &dwRead)) break;
            result.append(buf.data(), dwRead);
        } while (dwSize > 0);
    }
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (result.empty()) return false;
    outJson.swap(result);
    return true;
}

auto SkinApiClient::ParseIntoDatabase(const std::string& json, CSkinDatabase* db) -> bool {
    rapidjson::Document doc;
    doc.Parse(json.c_str());
    if (doc.HasParseError()) return false;

    std::vector<SkinData_t> skins;
    std::vector<WeaponSkinData_t> weapons;
    std::unordered_map<int, bool> weaponAdded;

    auto parseHexColor = [](const std::string& hex) -> ImU32 {
        if (hex.size() < 7 || hex[0] != '#') return 0;
        auto h = hex.c_str() + 1;
        auto hex2int = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
            if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
            return 0;
        };
        int r = hex2int(h[0]) * 16 + hex2int(h[1]);
        int g = hex2int(h[2]) * 16 + hex2int(h[3]);
        int b = hex2int(h[4]) * 16 + hex2int(h[5]);
        return IM_COL32(r, g, b, 255);
    };

    auto mapRarity = [](const std::string& r) {
        if (r == "Consumer Grade") return ESkinRarity::Common;
        if (r == "Industrial Grade") return ESkinRarity::Uncommon;
        if (r == "Mil-Spec") return ESkinRarity::Rare;
        if (r == "Restricted") return ESkinRarity::Mythical;
        if (r == "Classified") return ESkinRarity::Legendary;
        if (r == "Covert") return ESkinRarity::Ancient;
        if (r == "Contraband") return ESkinRarity::Contraband;
        return ESkinRarity::Rare;
    };
    auto mapCategory = [](const std::string& c) {
        if (c == "Pistols") return EWeaponCategory::Pistols;
        if (c == "SMGs") return EWeaponCategory::SMGs;
        if (c == "Rifles") return EWeaponCategory::Rifles;
        if (c == "Shotguns") return EWeaponCategory::Shotguns;
        if (c == "Snipers") return EWeaponCategory::Snipers;
        if (c == "Machine Guns") return EWeaponCategory::MachineGuns;
        if (c == "Knives") return EWeaponCategory::Knives;
        if (c == "Gloves") return EWeaponCategory::Gloves;
        return EWeaponCategory::Rifles;
    };

    if (doc.IsArray()) {
        for (auto& v : doc.GetArray()) {
            if (!v.IsObject()) continue;
            int pk = 0;
            if (v.HasMember("paint_index")) {
                if (v["paint_index"].IsInt()) pk = v["paint_index"].GetInt();
                else if (v["paint_index"].IsString()) pk = atoi(v["paint_index"].GetString());
            }
            std::string name = v.HasMember("name") && v["name"].IsString() ? v["name"].GetString() : "";
            std::string desc = v.HasMember("description") && v["description"].IsString() ? v["description"].GetString() : "";
            std::string rarityName;
            if (v.HasMember("rarity") && v["rarity"].IsObject()) {
                auto& rr = v["rarity"];
                if (rr.HasMember("name") && rr["name"].IsString()) rarityName = rr["name"].GetString();
            }
            ESkinRarity rarity = mapRarity(rarityName);
            SkinData_t s(pk, name, desc, rarity);
            if (v.HasMember("rarity") && v["rarity"].IsObject()) {
                auto& rr = v["rarity"];
                if (rr.HasMember("color") && rr["color"].IsString()) {
                    s.m_uRarityColor = parseHexColor(rr["color"].GetString());
                }
            }
            if (v.HasMember("min_float") && v["min_float"].IsNumber()) s.m_fMinFloat = v["min_float"].GetFloat();
            if (v.HasMember("max_float") && v["max_float"].IsNumber()) s.m_fMaxFloat = v["max_float"].GetFloat();
            if (v.HasMember("image") && v["image"].IsString()) s.m_sImageUrl = v["image"].GetString();

            int defIndex = 0;
            if (v.HasMember("weapon") && v["weapon"].IsObject()) {
                auto& w = v["weapon"];
                if (w.HasMember("weapon_id") && w["weapon_id"].IsInt()) defIndex = w["weapon_id"].GetInt();
                std::string wname = w.HasMember("name") && w["name"].IsString() ? w["name"].GetString() : "";
                std::string catName;
                if (v.HasMember("category") && v["category"].IsObject()) {
                    auto& c = v["category"];
                    if (c.HasMember("name") && c["name"].IsString()) catName = c["name"].GetString();
                }
                if (defIndex > 0) {
                    s.m_vCompatibleWeapons.push_back(defIndex);
                    if (!weaponAdded[defIndex]) {
                        weaponAdded[defIndex] = true;
                        weapons.emplace_back(defIndex, wname, std::string{}, mapCategory(catName), true);
                    }
                }
            }
            skins.push_back(std::move(s));
        }
    } else {
        return false;
    }
    db->UpdateFromApi(skins, weapons);
    return true;
}

auto SkinApiClient::FetchAndUpdate(CSkinDatabase* db) -> bool {
    std::string json;
    bool fetched = FetchJson(json);
    if (!fetched) {
        if (!LoadCache(json)) return false;
    } else {
        SaveCache(json);
    }
    return ParseIntoDatabase(json, db);
}