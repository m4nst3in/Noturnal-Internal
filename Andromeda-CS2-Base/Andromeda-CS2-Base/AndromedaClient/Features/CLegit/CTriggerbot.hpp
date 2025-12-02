#pragma once

#include <Common/Common.hpp>

class CCSGOInput;
class CUserCmd;
class C_CSPlayerPawn;
class C_BaseEntity;

class CTriggerbot final
{
public:
	auto OnCreateMove( CCSGOInput* pInput , CUserCmd* pUserCmd ) -> void;

private:
	auto IsValidTarget( C_BaseEntity* pEntity ) -> bool;
	auto ShouldFire() -> bool;
	auto HandleAutoScope( CUserCmd* pUserCmd ) -> bool;

private:
	// Timing variables
	uint64_t m_LastTargetTime = 0;
	uint64_t m_DelayStartTime = 0;
	bool m_bWaitingForDelay = false;
	
	// Burst mode variables
	int m_nBurstShotsRemaining = 0;
	bool m_bBurstActive = false;
};

auto GetTriggerbot() -> CTriggerbot*;
