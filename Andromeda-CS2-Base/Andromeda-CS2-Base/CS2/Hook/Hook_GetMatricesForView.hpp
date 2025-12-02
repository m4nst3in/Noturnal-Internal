#pragma once

#include <Common/Common.hpp>

class VMatrix;

auto Hook_OverrideView( void* rcx , void* view ,
                        VMatrix* pWorldToView ,
                        VMatrix* pViewToProjection ,
                        VMatrix* pWorldToProjection ,
                        VMatrix* pWorldToPixels ) -> void;

auto GetWorldToViewMatrix() -> const VMatrix*;
auto GetWorldToProjectionMatrix() -> const VMatrix*;

using OverrideView_t = decltype( &Hook_OverrideView );
inline OverrideView_t OverrideView_o = nullptr;
