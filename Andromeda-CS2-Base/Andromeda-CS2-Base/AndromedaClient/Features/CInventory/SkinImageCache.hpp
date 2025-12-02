#pragma once

#include <Common/Common.hpp>
#include <d3d11.h>
#include <string>
#include <unordered_map>

struct SkinPreview {
    ID3D11ShaderResourceView* srv = nullptr;
    int width = 0;
    int height = 0;
};

class SkinImageCache {
public:
    auto GetOrFetch(int paintkit, const std::string& url, SkinPreview& out) -> bool;
    auto Clear() -> void;

private:
    auto Download(const std::string& url, std::vector<unsigned char>& out) -> bool;
    auto CreateSRV(const std::vector<unsigned char>& data, SkinPreview& out) -> bool;

private:
    std::unordered_map<int, SkinPreview> m_Cache;
};

auto GetSkinImageCache() -> SkinImageCache*;