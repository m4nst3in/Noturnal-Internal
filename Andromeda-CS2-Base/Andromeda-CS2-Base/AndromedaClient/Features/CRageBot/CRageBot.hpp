#pragma once

// Main RageBot header - include this to use the rage bot system

#include "RageBot.hpp"
#include "Config.hpp"

// Quick access macro for rage bot instance
#define RAGEBOT GetRageBot()

// Usage example:
// 
// In your CreateMove hook:
//   RAGEBOT->OnCreateMove( pInput , pUserCmd );
//
// To configure:
//   Settings::RageBot::Enabled = true;
//   Settings::RageBot::AutoFire = true;
//   Settings::RageBot::MaxFOV = 180.0f;
//
// Per-weapon configuration:
//   Settings::RageBot::RifleMinDamage = 30;
//   Settings::RageBot::SniperHitchance = 70;
