#pragma once

#include <d3d11.h>
#include <Common/Common.hpp>

using DrawIndexed_t = void(__stdcall*)(ID3D11DeviceContext* ctx, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);

extern DrawIndexed_t DrawIndexed_o;

auto InitDrawIndexedHook(ID3D11DeviceContext* ctx) -> bool;
auto ShutdownDrawIndexedHook() -> void;