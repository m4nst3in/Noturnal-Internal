#pragma once

#include <Common/Common.hpp>
#include <CS2/SDK/Math/Vector3.hpp>

class C_CSPlayerPawn;
class CCSWeaponBaseVData;

// Maximum number of penetrations allowed
constexpr int MAX_PENETRATIONS = 4;

// Hit groups
enum HitGroup_t
{
	HITGROUP_GENERIC = 0,
	HITGROUP_HEAD = 1,
	HITGROUP_CHEST = 2,
	HITGROUP_STOMACH = 3,
	HITGROUP_LEFTARM = 4,
	HITGROUP_RIGHTARM = 5,
	HITGROUP_LEFTLEG = 6,
	HITGROUP_RIGHTLEG = 7,
	HITGROUP_NECK = 8,
	HITGROUP_GEAR = 10,
};

// Damage calculation result
struct AutowallDamageInfo_t
{
	bool m_bCanDamage = false;
	float m_flDamage = 0.0f;
	int m_nPenetrations = 0;
	Vector3 m_vecHitPos;
	int m_nHitGroup = 0;
};

class CAutowall final
{
public:
	// Main damage calculation function
	auto CanDamage( const Vector3& vecStart , const Vector3& vecEnd , C_CSPlayerPawn* pTarget ) -> AutowallDamageInfo_t;

	// Get damage at a specific position
	auto GetDamageToPoint( const Vector3& vecEnd , C_CSPlayerPawn* pTarget ) -> float;

	// Check if we can penetrate to target with minimum damage
	auto CanPenetrate( const Vector3& vecEnd , C_CSPlayerPawn* pTarget , float flMinDamage ) -> bool;

private:
	// Scale damage based on distance and range modifier
	auto ScaleDamage( float flDamage , float flDistance , float flRangeModifier , float flRange ) -> float;

	// Scale damage for armor
	auto ScaleDamageForArmor( float flDamage , float flArmorRatio , int nArmorValue ) -> float;

	// Apply hitgroup multipliers
	auto ApplyHitgroupDamage( float flDamage , int nHitGroup , float flHeadshotMultiplier ) -> float;

	// Handle bullet penetration simulation
	auto SimulatePenetration( Vector3& vecStart , Vector3& vecDir , float& flDamage , 
							   float flPenetration , int& nPenetrations ) -> bool;
};

auto GetAutowall() -> CAutowall*;
