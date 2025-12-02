#pragma once

#include <Common/Common.hpp>
#include <CS2/SDK/Types/CEntityData.hpp>

auto GetLocalPlayerArms() -> C_CS2HudModelArms*;
auto GetWeaponViewModel() -> C_CS2HudModelWeapon*;
auto GetHudModelBase() -> C_CS2HudModelBase*;

auto ValidateAllOffsetsComplete() -> void;
auto RunDevOffsetPatternTests() -> void;