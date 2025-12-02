#include "CChams3D.hpp"
#include <d3dcompiler.h>
#include <CS2/Hook/Hook_GetMatricesForView.hpp>
#include <AndromedaClient/Settings/Settings.hpp>
#include <GameClient/CL_Players.hpp>
#include <GameClient/CEntityCache/CEntityCache.hpp>
#include <CS2/SDK/Types/CEntityData.hpp>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#include <algorithm>

static CChams3D g_Chams3D{};

struct CBWVP { float m[16]; float color[4]; };

static const char* g_vsSrc = R"(
cbuffer CBWVP : register(b0) { float4x4 WVP; float4 Color; }
struct VSIN { float3 pos : POSITION; };
struct VSOUT { float4 pos : SV_POSITION; float4 color : COLOR; };
VSOUT main(VSIN i){ VSOUT o; o.pos = mul(WVP, float4(i.pos,1)); o.color = Color; return o; }
)";

static const char* g_psSrc = R"(
struct PSIN { float4 pos : SV_POSITION; float4 color : COLOR; };
float4 main(PSIN i) : SV_TARGET { return i.color; }
)";

auto CChams3D::Init(ID3D11Device* dev) -> bool
{
    if (!dev) return false;
    ID3DBlob* vsb=nullptr,*psb=nullptr,*err=nullptr;
    if (FAILED(D3DCompile(g_vsSrc, strlen(g_vsSrc), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsb, &err))) return false;
    if (FAILED(D3DCompile(g_psSrc, strlen(g_psSrc), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psb, &err))) { if(vsb) vsb->Release(); return false; }
    if (FAILED(dev->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, &vs))) { vsb->Release(); psb->Release(); return false; }
    if (FAILED(dev->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, &ps))) { vsb->Release(); psb->Release(); return false; }
    D3D11_INPUT_ELEMENT_DESC ied{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0 };
    if (FAILED(dev->CreateInputLayout(&ied,1,vsb->GetBufferPointer(),vsb->GetBufferSize(),&il))) { vsb->Release(); psb->Release(); return false; }
    vsb->Release(); psb->Release();
    D3D11_BUFFER_DESC cbd{}; cbd.Usage=D3D11_USAGE_DYNAMIC; cbd.ByteWidth=sizeof(CBWVP); cbd.BindFlags=D3D11_BIND_CONSTANT_BUFFER; cbd.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
    if (FAILED(dev->CreateBuffer(&cbd,nullptr,&cbWVP))) return false;
    D3D11_DEPTH_STENCIL_DESC ds{}; ds.DepthEnable=FALSE; ds.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO; ds.DepthFunc=D3D11_COMPARISON_ALWAYS;
    if (FAILED(dev->CreateDepthStencilState(&ds,&dsDisable))) return false;
    ds.DepthEnable=TRUE; ds.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL; ds.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;
    if (FAILED(dev->CreateDepthStencilState(&ds,&dsEnable))) return false;
    D3D11_BUFFER_DESC vbd{}; vbd.Usage=D3D11_USAGE_DYNAMIC; vbd.ByteWidth=sizeof(Chams3DVertex)*4096; vbd.BindFlags=D3D11_BIND_VERTEX_BUFFER; vbd.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
    if (FAILED(dev->CreateBuffer(&vbd,nullptr,&vb))) return false;
    return true;
}

static inline void MulVP(const float* a,const float* b,float* out){ for(int r=0;r<4;r++) for(int c=0;c<4;c++){ out[r*4+c]=0; for(int k=0;k<4;k++) out[r*4+c]+=a[r*4+k]*b[k*4+c]; } }

static auto LoadWVP(CBWVP& cb, const VMatrix* w2p, const float color[4]) -> void
{
    for(int r=0;r<4;r++) for(int c=0;c<4;c++) cb.m[r*4+c]=reinterpret_cast<const float*>(w2p)[r*4+c];
    cb.color[0]=color[0]; cb.color[1]=color[1]; cb.color[2]=color[2]; cb.color[3]=color[3];
}

static auto BuildSegmentQuad(const Vector3& a, const Vector3& b, std::vector<Chams3DVertex>& out) -> void
{
    Vector3 mid = (a + b) * 0.5f;
    Vector3 dir = b - a; float len = dir.Length(); if (len < 0.001f) len = 0.001f; dir /= len;
    Vector3 camRight = Vector3(1,0,0); Vector3 camUp = Vector3(0,1,0);
    float width = 2.0f; float halfW = width*0.5f; float halfH = std::max<float>(1.5f, len*0.5f);
    Vector3 r = camRight * halfW; Vector3 u = camUp * halfH;
    Vector3 p0 = mid - r - u;
    Vector3 p1 = mid + r - u;
    Vector3 p2 = mid + r + u;
    Vector3 p3 = mid - r + u;
    out.push_back({p0.m_x,p0.m_y,p0.m_z}); out.push_back({p1.m_x,p1.m_y,p1.m_z}); out.push_back({p2.m_x,p2.m_y,p2.m_z});
    out.push_back({p0.m_x,p0.m_y,p0.m_z}); out.push_back({p2.m_x,p2.m_y,p2.m_z}); out.push_back({p3.m_x,p3.m_y,p3.m_z});
}

auto CChams3D::Render(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* dsv) -> void
{
    if (!ctx || !vs || !ps || !il || !cbWVP || !vb) return;
    if (!Settings::Visual::ChamsEnabled) return;
    const VMatrix* w2p = GetWorldToProjectionMatrix(); if (!w2p) return;
    float visCol[4] = { Settings::Visual::ChamsVisibleColor[0], Settings::Visual::ChamsVisibleColor[1], Settings::Visual::ChamsVisibleColor[2], Settings::Visual::ChamsVisibleColor[3] };
    float invisCol[4] = { Settings::Visual::ChamsInvisibleColor[0], Settings::Visual::ChamsInvisibleColor[1], Settings::Visual::ChamsInvisibleColor[2], Settings::Visual::ChamsInvisibleColor[3] };
    auto* cached = GetEntityCache()->GetCachedEntity(); if (!cached) return;
    std::scoped_lock lk(GetEntityCache()->GetLock());
    UINT stride = sizeof(Chams3DVertex), offset = 0; ctx->IASetInputLayout(il); ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); ctx->IASetVertexBuffers(0,1,&vb,&stride,&offset);
    D3D11_MAPPED_SUBRESOURCE map{};
    for (const auto& it : *cached)
    {
        if (it.m_Type != CachedEntity_t::PLAYER_CONTROLLER || !it.m_bDraw) continue;
        auto* pc = reinterpret_cast<CCSPlayerController*>(it.m_Handle.Get()); if (!pc) continue;
        auto* pawn = pc->m_hPawn().Get<C_CSPlayerPawn>(); if (!pawn) continue;
        std::vector<Chams3DVertex> verts; verts.reserve(1024);
        const char* names[] = { "spine_2","spine_1","spine_0","pelvis","arm_upper_l","arm_lower_l","arm_upper_r","arm_lower_r","leg_upper_l","leg_lower_l","leg_upper_r","leg_lower_r" };
        for (int i=0;i<11;i++)
        {
            int idA = pawn->GetBoneIdByName(names[i]); int idB = pawn->GetBoneIdByName(names[i+1]); if (idA==-1||idB==-1) continue;
            Vector3 a,b; if (!pawn->m_pGameSceneNode()->GetBonePosition(idA,a)) continue; if (!pawn->m_pGameSceneNode()->GetBonePosition(idB,b)) continue;
            std::vector<Chams3DVertex> tmp; BuildSegmentQuad(a,b,tmp); verts.insert(verts.end(), tmp.begin(), tmp.end());
        }
        if (verts.empty()) continue;
        if (FAILED(ctx->Map(vb,0,D3D11_MAP_WRITE_DISCARD,0,&map))) continue;
        memcpy(map.pData, verts.data(), verts.size()*sizeof(Chams3DVertex));
        ctx->Unmap(vb,0);
        CBWVP cb{}; LoadWVP(cb, w2p, Settings::Visual::ChamsIgnoreZ ? invisCol : visCol);
        ctx->OMSetDepthStencilState(Settings::Visual::ChamsIgnoreZ ? dsDisable : dsEnable, 0);
        ctx->VSSetShader(vs,nullptr,0); ctx->PSSetShader(ps,nullptr,0);
        ctx->VSSetConstantBuffers(0,1,&cbWVP);
        D3D11_MAPPED_SUBRESOURCE mcb{}; if (SUCCEEDED(ctx->Map(cbWVP,0,D3D11_MAP_WRITE_DISCARD,0,&mcb))) { memcpy(mcb.pData,&cb,sizeof(cb)); ctx->Unmap(cbWVP,0); }
        ctx->Draw(static_cast<UINT>(verts.size()),0);
        if (Settings::Visual::ChamsIgnoreZ)
        {
            CBWVP cb2{}; LoadWVP(cb2, w2p, visCol);
            ctx->OMSetDepthStencilState(dsEnable, 0);
            D3D11_MAPPED_SUBRESOURCE mcb2{}; if (SUCCEEDED(ctx->Map(cbWVP,0,D3D11_MAP_WRITE_DISCARD,0,&mcb2))) { memcpy(mcb2.pData,&cb2,sizeof(cb2)); ctx->Unmap(cbWVP,0); }
            ctx->Draw(static_cast<UINT>(verts.size()),0);
        }
    }
    ctx->OMSetDepthStencilState(nullptr,0);
}

auto CChams3D::Destroy() -> void
{
    if (vs) vs->Release(); vs=nullptr; if (ps) ps->Release(); ps=nullptr; if (il) il->Release(); il=nullptr; if (cbWVP) cbWVP->Release(); cbWVP=nullptr; if (vb) vb->Release(); vb=nullptr; if (dsDisable) dsDisable->Release(); dsDisable=nullptr; if (dsEnable) dsEnable->Release(); dsEnable=nullptr;
}

auto GetChams3D() -> CChams3D*
{
    return &g_Chams3D;
}
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#include <algorithm>