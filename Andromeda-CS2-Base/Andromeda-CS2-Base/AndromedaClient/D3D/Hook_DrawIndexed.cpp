#include "Hook_DrawIndexed.hpp"
#include <MinHook/MinHook.h>
#include <d3dcompiler.h>
#include <AndromedaClient/Settings/Settings.hpp>

static ID3D11PixelShader* g_psSolid = nullptr;
static ID3D11Buffer* g_cbColor = nullptr;
static ID3D11DepthStencilState* g_dsDisable = nullptr;
static ID3D11DepthStencilState* g_dsEnable = nullptr;

DrawIndexed_t DrawIndexed_o = nullptr;

static const char* g_psSrc = R"(
cbuffer CBColor : register(b1) { float4 Color; }
float4 main() : SV_TARGET { return Color; }
)";

static auto EnsurePipeline(ID3D11Device* dev) -> bool
{
    if (!dev) return false;
    if (!g_psSolid)
    {
        ID3DBlob* psb=nullptr,*err=nullptr;
        if (FAILED(D3DCompile(g_psSrc, strlen(g_psSrc), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psb, &err))) return false;
        if (FAILED(dev->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, &g_psSolid))) { if(psb) psb->Release(); return false; }
        if(psb) psb->Release();
        D3D11_BUFFER_DESC cbd{}; cbd.Usage=D3D11_USAGE_DYNAMIC; cbd.ByteWidth=sizeof(float)*4; cbd.BindFlags=D3D11_BIND_CONSTANT_BUFFER; cbd.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
        if (FAILED(dev->CreateBuffer(&cbd,nullptr,&g_cbColor))) return false;
        D3D11_DEPTH_STENCIL_DESC ds{}; ds.DepthEnable=FALSE; ds.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO; ds.DepthFunc=D3D11_COMPARISON_ALWAYS;
        if (FAILED(dev->CreateDepthStencilState(&ds,&g_dsDisable))) return false;
        ds.DepthEnable=TRUE; ds.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL; ds.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;
        if (FAILED(dev->CreateDepthStencilState(&ds,&g_dsEnable))) return false;
    }
    return true;
}

static void UpdateColor(ID3D11DeviceContext* ctx, const float col[4])
{
    D3D11_MAPPED_SUBRESOURCE m{}; if (SUCCEEDED(ctx->Map(g_cbColor,0,D3D11_MAP_WRITE_DISCARD,0,&m))) { memcpy(m.pData,col,sizeof(float)*4); ctx->Unmap(g_cbColor,0); }
    ctx->PSSetConstantBuffers(1,1,&g_cbColor);
}

static void __stdcall Hook_DrawIndexed(ID3D11DeviceContext* ctx, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
    if (!ctx || !Settings::Visual::ChamsEnabled)
        return DrawIndexed_o(ctx, IndexCount, StartIndexLocation, BaseVertexLocation);

    ID3D11Buffer* vb=nullptr; UINT stride=0, offset=0; ctx->IAGetVertexBuffers(0,1,&vb,&stride,&offset);
    ID3D11Buffer* ib=nullptr; DXGI_FORMAT fmt=DXGI_FORMAT_UNKNOWN; UINT ibOffset=0; ctx->IAGetIndexBuffer(&ib,&fmt,&ibOffset);

    bool candidate = (vb!=nullptr) && (ib!=nullptr) && (fmt==DXGI_FORMAT_R16_UINT || fmt==DXGI_FORMAT_R32_UINT) && (stride>=32 && stride<=52) && (IndexCount>=9000 && IndexCount<=24000);

    if (!candidate)
        return DrawIndexed_o(ctx, IndexCount, StartIndexLocation, BaseVertexLocation);

    ID3D11Device* dev=nullptr; ctx->GetDevice(&dev);
    if (!EnsurePipeline(dev)) { if(dev) dev->Release(); return DrawIndexed_o(ctx, IndexCount, StartIndexLocation, BaseVertexLocation); }
    if (dev) dev->Release();

    ID3D11PixelShader* oldPS=nullptr; ctx->PSGetShader(&oldPS,nullptr,nullptr);
    ID3D11DepthStencilState* oldDS=nullptr; UINT oldStencilRef=0; ctx->OMGetDepthStencilState(&oldDS,&oldStencilRef);

    if (Settings::Visual::ChamsIgnoreZ)
    {
        float invis[4] = { Settings::Visual::ChamsInvisibleColor[0], Settings::Visual::ChamsInvisibleColor[1], Settings::Visual::ChamsInvisibleColor[2], Settings::Visual::ChamsInvisibleColor[3] };
        UpdateColor(ctx, invis);
        ctx->PSSetShader(g_psSolid,nullptr,0);
        ctx->OMSetDepthStencilState(g_dsDisable, 0);
        DrawIndexed_o(ctx, IndexCount, StartIndexLocation, BaseVertexLocation);
    }

    float vis[4] = { Settings::Visual::ChamsVisibleColor[0], Settings::Visual::ChamsVisibleColor[1], Settings::Visual::ChamsVisibleColor[2], Settings::Visual::ChamsVisibleColor[3] };
    UpdateColor(ctx, vis);
    ctx->PSSetShader(g_psSolid,nullptr,0);
    ctx->OMSetDepthStencilState(g_dsEnable, 0);
    DrawIndexed_o(ctx, IndexCount, StartIndexLocation, BaseVertexLocation);

    ctx->PSSetShader(oldPS,nullptr,0);
    if (oldPS) oldPS->Release();
    ctx->OMSetDepthStencilState(oldDS, oldStencilRef);
    if (oldDS) oldDS->Release();

    return;
}

auto InitDrawIndexedHook(ID3D11DeviceContext* ctx) -> bool
{
    if (!ctx) return false;
    void** vtbl = *reinterpret_cast<void***>(ctx);
    void* target = vtbl[12]; // DrawIndexed
    if (MH_CreateHook(target, &Hook_DrawIndexed, reinterpret_cast<LPVOID*>(&DrawIndexed_o)) != MH_OK) return false;
    if (MH_EnableHook(target) != MH_OK) return false;
    return true;
}

auto ShutdownDrawIndexedHook() -> void
{
    if (DrawIndexed_o)
    {
        MH_DisableHook(reinterpret_cast<void*>(DrawIndexed_o));
        DrawIndexed_o = nullptr;
    }
    if (g_psSolid) { g_psSolid->Release(); g_psSolid=nullptr; }
    if (g_cbColor) { g_cbColor->Release(); g_cbColor=nullptr; }
    if (g_dsDisable) { g_dsDisable->Release(); g_dsDisable=nullptr; }
    if (g_dsEnable) { g_dsEnable->Release(); g_dsEnable=nullptr; }
}