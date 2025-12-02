#include "CAutowall.hpp"

#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Update/GameTrace.hpp>
#include <CS2/SDK/Types/CEntityData.hpp>
#include <CS2/SDK/FunctionListSDK.hpp>

#include <GameClient/CL_Players.hpp>
#include <GameClient/CL_Weapons.hpp>

#include <AndromedaClient/Settings/Settings.hpp>

static CAutowall g_Autowall{};

auto CAutowall::CanDamage( const Vector3& vecStart , const Vector3& vecEnd , C_CSPlayerPawn* pTarget ) -> AutowallDamageInfo_t
{
	AutowallDamageInfo_t result{};

	if ( !pTarget )
		return result;

	auto pLocalPlayerPawn = GetCL_Players()->GetLocalPlayerPawn();
	if ( !pLocalPlayerPawn )
		return result;

	// Get weapon data
	auto pWeaponVData = GetCL_Weapons()->GetLocalWeaponVData();
	if ( !pWeaponVData )
		return result;

	// Get weapon stats
	float flDamage = static_cast<float>( pWeaponVData->m_nDamage() );
	float flPenetration = pWeaponVData->m_flPenetration();
	float flRange = pWeaponVData->m_flRange();
	float flRangeModifier = pWeaponVData->m_flRangeModifier();
	float flArmorRatio = pWeaponVData->m_flArmorRatio();
	float flHeadshotMultiplier = pWeaponVData->m_flHeadshotMultiplier();

	// Calculate direction
	Vector3 vecDir = vecEnd - vecStart;
	float flDistance = vecDir.Length();
	vecDir.Normalize();

	// Check if within range
	if ( flDistance > flRange )
	{
		result.m_bCanDamage = false;
		return result;
	}

	// Trace to target
	Ray_t Ray;
	CGameTrace GameTrace;
	CTraceFilter Filter( 0x1C1003 , pLocalPlayerPawn , 3 , 15 );

	Vector3 currentStart = vecStart;
	int nPenetrations = 0;
	float currentDamage = flDamage;

	while ( nPenetrations <= MAX_PENETRATIONS )
	{
		if ( !IGamePhysicsQuery_TraceShape( SDK::Pointers::CVPhys2World() , Ray , currentStart , vecEnd , &Filter , &GameTrace ) )
			break;

		// Check if we hit the target
		if ( GameTrace.pHitEntity == pTarget )
		{
			// Apply distance damage falloff
			float actualDistance = ( GameTrace.vecPosition - vecStart ).Length();
			currentDamage = ScaleDamage( currentDamage , actualDistance , flRangeModifier , flRange );

			// Get hitgroup
			int hitGroup = GameTrace.GetHitGroup();
			result.m_nHitGroup = hitGroup;

			// Apply hitgroup damage multiplier
			currentDamage = ApplyHitgroupDamage( currentDamage , hitGroup , flHeadshotMultiplier );

			// Apply armor damage reduction
			if ( pTarget->HasArmor( hitGroup ) )
			{
				int nArmorValue = pTarget->m_ArmorValue();
				currentDamage = ScaleDamageForArmor( currentDamage , flArmorRatio , nArmorValue );
			}

			result.m_bCanDamage = true;
			result.m_flDamage = currentDamage;
			result.m_nPenetrations = nPenetrations;
			result.m_vecHitPos = GameTrace.vecPosition;
			return result;
		}

		// Check if we hit nothing (clear path)
		if ( !GameTrace.DidHit() )
			break;

		// Try to penetrate
		if ( !SimulatePenetration( currentStart , vecDir , currentDamage , flPenetration , nPenetrations ) )
			break;

		// Move start position past the obstacle
		currentStart = GameTrace.vecPosition + vecDir * 2.0f;
	}

	return result;
}

auto CAutowall::GetDamageToPoint( const Vector3& vecEnd , C_CSPlayerPawn* pTarget ) -> float
{
	Vector3 vecStart = GetCL_Players()->GetLocalEyeOrigin();
	auto result = CanDamage( vecStart , vecEnd , pTarget );
	return result.m_flDamage;
}

auto CAutowall::CanPenetrate( const Vector3& vecEnd , C_CSPlayerPawn* pTarget , float flMinDamage ) -> bool
{
	Vector3 vecStart = GetCL_Players()->GetLocalEyeOrigin();
	auto result = CanDamage( vecStart , vecEnd , pTarget );
	return result.m_bCanDamage && result.m_flDamage >= flMinDamage;
}

auto CAutowall::ScaleDamage( float flDamage , float flDistance , float flRangeModifier , float flRange ) -> float
{
	// Apply range modifier based on distance
	// CS2 damage falloff: damage *= rangeModifier ^ (distance / 500)
	if ( flDistance > 0.0f && flRangeModifier < 1.0f )
	{
		float flDistanceFactor = flDistance / 500.0f;
		flDamage *= powf( flRangeModifier , flDistanceFactor );
	}

	return flDamage;
}

auto CAutowall::ScaleDamageForArmor( float flDamage , float flArmorRatio , int nArmorValue ) -> float
{
	if ( nArmorValue <= 0 )
		return flDamage;

	// Calculate armor damage absorption
	float flArmorBonus = 0.5f;
	float flArmorRatioCalc = flArmorRatio * 0.5f;
	
	float flDamageToHealth = flDamage * flArmorRatioCalc;
	float flDamageToArmor = ( flDamage - flDamageToHealth ) * flArmorBonus;

	// Can't absorb more than armor value
	if ( flDamageToArmor > static_cast<float>( nArmorValue ) )
		flDamageToArmor = static_cast<float>( nArmorValue ) * ( 1.0f / flArmorBonus );

	flDamage -= flDamageToArmor;

	return flDamage < 0.0f ? 0.0f : flDamage;
}

auto CAutowall::ApplyHitgroupDamage( float flDamage , int nHitGroup , float flHeadshotMultiplier ) -> float
{
	switch ( nHitGroup )
	{
	case HITGROUP_HEAD:
		flDamage *= flHeadshotMultiplier;
		break;
	case HITGROUP_CHEST:
	case HITGROUP_NECK:
		flDamage *= 1.0f;
		break;
	case HITGROUP_STOMACH:
		flDamage *= 1.25f;
		break;
	case HITGROUP_LEFTARM:
	case HITGROUP_RIGHTARM:
		flDamage *= 1.0f;
		break;
	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG:
		flDamage *= 0.75f;
		break;
	default:
		break;
	}

	return flDamage;
}

auto CAutowall::SimulatePenetration( Vector3& vecStart , Vector3& vecDir , float& flDamage , 
									  float flPenetration , int& nPenetrations ) -> bool
{
	// Simple penetration simulation
	// In a full implementation, this would check surface properties and calculate
	// penetration depth based on material type
	
	if ( flPenetration <= 0.0f )
		return false;

	if ( nPenetrations >= MAX_PENETRATIONS )
		return false;

	// Base penetration damage loss (simplified)
	// Real implementation would use surface data from trace
	float flPenDamageMod = 0.16f; // Default penetration modifier
	
	// Apply penetration damage loss
	flDamage -= flDamage * flPenDamageMod;

	// Minimum damage threshold
	if ( flDamage < 1.0f )
		return false;

	nPenetrations++;
	return true;
}

auto GetAutowall() -> CAutowall*
{
	return &g_Autowall;
}
