#pragma once

#include <Common/Common.hpp>
#include <d3d11.h>
#include <vector>
#include <CS2/SDK/Math/Vector3.hpp>

struct Chams3DVertex { float x,y,z; };

class CChams3D
{
public:
    auto Init(ID3D11Device* dev) -> bool;
    auto Render(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* dsv) -> void;
    auto Destroy() -> void;
private:
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11InputLayout* il = nullptr;
    ID3D11Buffer* cbWVP = nullptr;
    ID3D11Buffer* vb = nullptr;
    ID3D11DepthStencilState* dsDisable = nullptr;
    ID3D11DepthStencilState* dsEnable = nullptr;
};

auto GetChams3D() -> CChams3D*;