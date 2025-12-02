#include "SkinImageCache.hpp"
#include <windows.h>
#include <winhttp.h>
#include <wincodec.h>
#include <vector>
#include <sstream>

#include <AndromedaClient/CAndromedaGUI.hpp>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "windowscodecs.lib")

static SkinImageCache g_SkinImageCache{};
auto GetSkinImageCache() -> SkinImageCache* { return &g_SkinImageCache; }

auto SkinImageCache::Clear() -> void {
    for (auto& kv : m_Cache) {
        if (kv.second.srv) kv.second.srv->Release();
    }
    m_Cache.clear();
}

auto SkinImageCache::GetOrFetch(int paintkit, const std::string& url, SkinPreview& out) -> bool {
    auto it = m_Cache.find(paintkit);
    if (it != m_Cache.end()) { out = it->second; return out.srv != nullptr; }
    std::vector<unsigned char> data;
    if (!Download(url, data) || data.empty()) return false;
    SkinPreview prev{};
    if (!CreateSRV(data, prev)) return false;
    m_Cache[paintkit] = prev;
    out = prev;
    return true;
}

auto SkinImageCache::Download(const std::string& url, std::vector<unsigned char>& out) -> bool {
    if (url.empty()) return false;
    URL_COMPONENTS comp{0}; comp.dwStructSize = sizeof(comp);
    comp.dwSchemeLength = comp.dwHostNameLength = comp.dwUrlPathLength = comp.dwExtraInfoLength = (DWORD)-1;
    wchar_t wurl[1024]; MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, wurl, 1024);
    if (!WinHttpCrackUrl(wurl, 0, 0, &comp)) return false;
    std::wstring host(comp.lpszHostName, comp.dwHostNameLength);
    std::wstring path(comp.lpszUrlPath, comp.dwUrlPathLength);
    HINTERNET hSession = WinHttpOpen(L"Andromeda/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;
    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), comp.nPort ? comp.nPort : INTERNET_DEFAULT_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }
    DWORD flags = 0; if (comp.nScheme == INTERNET_SCHEME_HTTPS) flags |= WINHTTP_FLAG_SECURE;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }
    bool ok = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(hRequest, nullptr);
    if (ok) {
        DWORD avail = 0;
        do {
            avail = 0; if (!WinHttpQueryDataAvailable(hRequest, &avail)) break; if (avail == 0) break;
            std::vector<unsigned char> buf; buf.resize(avail);
            DWORD read = 0; if (!WinHttpReadData(hRequest, buf.data(), avail, &read)) break;
            out.insert(out.end(), buf.begin(), buf.begin() + read);
        } while (avail > 0);
    }
    WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
    return !out.empty();
}

auto SkinImageCache::CreateSRV(const std::vector<unsigned char>& data, SkinPreview& out) -> bool {
    if (data.empty()) return false;
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    IWICImagingFactory* factory = nullptr;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))) return false;
    IWICStream* stream = nullptr;
    if (FAILED(factory->CreateStream(&stream))) { factory->Release(); return false; }
    if (FAILED(stream->InitializeFromMemory(reinterpret_cast<BYTE*>(const_cast<unsigned char*>(data.data())), (DWORD)data.size()))) {
        stream->Release(); factory->Release(); return false;
    }
    IWICBitmapDecoder* decoder = nullptr;
    if (FAILED(factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnLoad, &decoder))) {
        stream->Release(); factory->Release(); return false;
    }
    IWICBitmapFrameDecode* frame = nullptr; UINT w=0,h=0;
    if (FAILED(decoder->GetFrame(0, &frame))) { decoder->Release(); stream->Release(); factory->Release(); return false; }
    frame->GetSize(&w, &h);
    IWICFormatConverter* converter = nullptr;
    if (FAILED(factory->CreateFormatConverter(&converter))) { frame->Release(); decoder->Release(); stream->Release(); factory->Release(); return false; }
    if (FAILED(converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom))) {
        converter->Release(); frame->Release(); decoder->Release(); stream->Release(); factory->Release(); return false;
    }
    std::vector<unsigned char> rgba; rgba.resize(w * h * 4);
    const UINT stride = w * 4; const UINT size = stride * h;
    WICRect rc{0,0,(INT)w,(INT)h};
    if (FAILED(converter->CopyPixels(&rc, stride, size, rgba.data()))) {
        converter->Release(); frame->Release(); decoder->Release(); stream->Release(); factory->Release(); return false;
    }
    ID3D11Device* dev = GetAndromedaGUI()->GetDevice(); ID3D11DeviceContext* ctx = GetAndromedaGUI()->GetDeviceContext();
    if (!dev || !ctx) { converter->Release(); frame->Release(); decoder->Release(); stream->Release(); factory->Release(); return false; }
    D3D11_TEXTURE2D_DESC td{}; td.Width = w; td.Height = h; td.MipLevels = 1; td.ArraySize = 1; td.Format = DXGI_FORMAT_R8G8B8A8_UNORM; td.SampleDesc.Count = 1; td.BindFlags = D3D11_BIND_SHADER_RESOURCE; td.Usage = D3D11_USAGE_IMMUTABLE;
    D3D11_SUBRESOURCE_DATA srd{}; srd.pSysMem = rgba.data(); srd.SysMemPitch = stride;
    ID3D11Texture2D* tex = nullptr; if (FAILED(dev->CreateTexture2D(&td, &srd, &tex))) { converter->Release(); frame->Release(); decoder->Release(); stream->Release(); factory->Release(); return false; }
    D3D11_SHADER_RESOURCE_VIEW_DESC svd{}; svd.Format = td.Format; svd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; svd.Texture2D.MipLevels = 1;
    ID3D11ShaderResourceView* srv = nullptr; if (FAILED(dev->CreateShaderResourceView(tex, &svd, &srv))) { tex->Release(); converter->Release(); frame->Release(); decoder->Release(); stream->Release(); factory->Release(); return false; }
    tex->Release(); converter->Release(); frame->Release(); decoder->Release(); stream->Release(); factory->Release();
    out.srv = srv; out.width = (int)w; out.height = (int)h;
    return true;
}