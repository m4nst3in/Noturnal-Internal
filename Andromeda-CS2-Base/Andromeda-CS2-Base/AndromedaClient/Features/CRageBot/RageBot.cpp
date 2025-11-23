#include "RageBot.hpp"

#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Update/CCSGOInput.hpp>
#include <CS2/SDK/Update/CUserCmd.hpp>
#include <CS2/SDK/Types/CEntityData.hpp>
#include <CS2/SDK/Interface/IEngineToClient.hpp>
#include <CS2/SDK/Math/Math.hpp>

#include <AndromedaClient/Settings/Settings.hpp>

#include <GameClient/CL_Players.hpp>
#include <GameClient/CL_Weapons.hpp>
#include <GameClient/CL_Bones.hpp>
#include <GameClient/CL_VisibleCheck.hpp>
#include <GameClient/CEntityCache/CEntityCache.hpp>

#include <chrono>
#include <algorithm>

static CRageBot g_RageBot{};

auto CRageBot::OnCreateMove( CCSGOInput* pInput , CUserCmd* pUserCmd ) -> void
{
	if ( !Settings::RageBot::Enabled )
		return;

	auto pLocalPawn = GetCL_Players()->GetLocalPlayerPawn();
	if ( !pLocalPawn || !pLocalPawn->IsAlive() )
		return;

	auto pWeapon = GetCL_Weapons()->GetLocalActiveWeapon();
	if ( !pWeapon || !IsWeaponReady( pWeapon , pLocalPawn ) )
		return;

	// Check delay between shots
	auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()
	).count();
	
	if ( Settings::RageBot::DelayShot > 0 && ( currentTime - m_LastShotTime ) < static_cast<uint64_t>( Settings::RageBot::DelayShot ) )
		return;

	// Select best target
	auto target = SelectBestTarget( pLocalPawn );
	if ( !target.pawn )
		return;

	// Auto-stop if configured
	AutoStop( pUserCmd , pLocalPawn );

	// Auto-scope if needed
	if ( Settings::RageBot::AutoScope )
	{
		AutoScope( pUserCmd , pWeapon );
	}

	// Calculate aim angles
	auto localEyePos = GetCL_Players()->GetLocalEyeOrigin();
	auto aimAngles = Math::CalcAngle( localEyePos , target.aim_point );
	
	// Compensate for spread/recoil
	if ( Settings::RageBot::NoSpread )
	{
		CompensateSpread( pUserCmd , aimAngles );
	}
	
	// Set view angles (silent aim)
	if ( Settings::RageBot::SilentAim )
	{
		pUserCmd->cmd.mutable_base()->set_viewangles_x( aimAngles.m_x );
		pUserCmd->cmd.mutable_base()->set_viewangles_y( aimAngles.m_y );
		pUserCmd->cmd.mutable_base()->set_viewangles_z( aimAngles.m_z );
	}

	// Auto-fire if configured
	if ( Settings::RageBot::AutoFire )
	{
		AutoFire( pUserCmd , target );
		m_LastShotTime = currentTime;
	}
}

auto CRageBot::SelectBestTarget( C_CSPlayerPawn* pLocalPawn ) -> TargetInfo
{
	TargetInfo bestTarget;
	float bestPriority = std::numeric_limits<float>::max();
	
	auto localEyePos = GetCL_Players()->GetLocalEyeOrigin();
	auto pLocalController = GetCL_Players()->GetLocalPlayerController();
	
	if ( !pLocalController )
		return bestTarget;
	
	auto localTeam = pLocalController->m_iTeamNum();

	// Iterate through all cached entities
	std::scoped_lock lock( GetEntityCache()->GetLock() );
	auto cachedEntities = GetEntityCache()->GetCachedEntity();
	
	for ( const auto& cached : *cachedEntities )
	{
		if ( cached.m_Type != CachedEntity_t::PLAYER_CONTROLLER )
			continue;
			
		auto pController = reinterpret_cast<CCSPlayerController*>( cached.m_Handle.Get() );
		if ( !pController )
			continue;
		
		// Skip local player and teammates
		if ( pController == pLocalController || pController->m_iTeamNum() == localTeam )
			continue;
		
		auto pPawn = reinterpret_cast<C_CSPlayerPawn*>( pController->m_hPawn().Get() );
		if ( !IsValidTarget( pLocalPawn , pPawn ) )
			continue;
		
		// Calculate priority for this target
		float priority = 0.0f;
		if ( !CalculateTargetPriority( pLocalPawn , pPawn , priority ) )
			continue;
		
		// Generate multipoints and find best shot
		std::vector<int> hitboxes;
		if ( Settings::RageBot::TargetHead ) hitboxes.push_back( 6 ); // Head hitbox
		if ( Settings::RageBot::TargetChest ) hitboxes.push_back( 5 ); // Chest hitbox
		if ( Settings::RageBot::TargetPelvis ) hitboxes.push_back( 2 ); // Pelvis hitbox
		
		for ( int hitbox : hitboxes )
		{
			std::vector<Vector3> points;
			
			if ( Settings::RageBot::MultipointEnabled )
			{
				GenerateMultipoints( pPawn , hitbox , points );
			}
			else
			{
				// Just use center point based on hitbox
				const char* boneName = "head_0";
				if ( hitbox == 5 ) // Chest
					boneName = "spine_2";
				else if ( hitbox == 2 ) // Pelvis
					boneName = "pelvis";
				
				auto bonePos = GetCL_Bones()->GetBonePositionByName( pPawn , boneName );
				if ( bonePos.Length() > 0 )
					points.push_back( bonePos );
			}
			
			// Evaluate each point
			for ( const auto& point : points )
			{
				bool visible = GetCL_VisibleCheck()->IsBoneVisible( pPawn , point );
				
				// Calculate FOV
				auto fov = CalculateFOV( pLocalPawn->m_angEyeAngles() , localEyePos , point );
				if ( fov > Settings::RageBot::MaxFOV )
					continue;
				
				// Calculate hitchance
				auto hitchance = CalculateHitchance( pLocalPawn , point , GetCL_Weapons()->GetLocalActiveWeapon() );
				hitchance = AdaptiveHitchance( pPawn , hitchance );
				
				float requiredHitchance = GetRequiredHitchance( GetCL_Weapons()->GetLocalActiveWeapon() , visible );
				if ( hitchance < requiredHitchance )
					continue;
				
				// Check min damage
				int minDamage = GetMinDamage( GetCL_Weapons()->GetLocalActiveWeapon() , visible );
				if ( !CanHitMinDamage( pLocalPawn , pPawn , point , minDamage ) )
					continue;
				
				// This point is valid, check if it's better than current best
				if ( priority < bestPriority )
				{
					bestPriority = priority;
					bestTarget.pawn = pPawn;
					bestTarget.aim_point = point;
					bestTarget.hitbox = hitbox;
					bestTarget.fov = fov;
					bestTarget.visible = visible;
					bestTarget.hitchance = hitchance;
					bestTarget.damage = pPawn->m_iHealth(); // Store actual target health
				}
			}
		}
	}
	
	return bestTarget;
}

auto CRageBot::IsValidTarget( C_CSPlayerPawn* pLocalPawn , C_CSPlayerPawn* pTarget ) -> bool
{
	if ( !pTarget )
		return false;
	
	if ( !pTarget->IsAlive() )
		return false;
	
	if ( pTarget->m_iHealth() <= 0 )
		return false;
	
	// Check distance
	auto localPos = GetCL_Players()->GetLocalOrigin();
	auto targetPos = pTarget->GetOrigin();
	auto distance = CalculateDistance( localPos , targetPos );
	
	if ( distance > Settings::RageBot::MaxDistance )
		return false;
	
	return true;
}

auto CRageBot::CalculateTargetPriority( C_CSPlayerPawn* pLocalPawn , C_CSPlayerPawn* pTarget , float& outPriority ) -> bool
{
	auto localEyePos = GetCL_Players()->GetLocalEyeOrigin();
	auto targetPos = pTarget->GetOrigin();
	
	switch ( static_cast<TargetPriority>( Settings::RageBot::TargetPriority ) )
	{
		case TargetPriority::FOV:
		{
			auto fov = CalculateFOV( pLocalPawn->m_angEyeAngles() , localEyePos , targetPos );
			outPriority = fov;
			break;
		}
		case TargetPriority::DISTANCE:
		{
			auto distance = CalculateDistance( localEyePos , targetPos );
			outPriority = distance;
			break;
		}
		case TargetPriority::HP:
		{
			outPriority = static_cast<float>( pTarget->m_iHealth() );
			break;
		}
		case TargetPriority::THREAT:
		{
			// Simplified threat calculation - could check if aiming at us
			outPriority = 100.0f;
			break;
		}
	}
	
	return true;
}

auto CRageBot::GenerateMultipoints( C_CSPlayerPawn* pTarget , int hitbox , std::vector<Vector3>& points ) -> void
{
	// Get center bone position based on hitbox
	const char* boneName = "head_0";
	
	if ( hitbox == 5 ) // Chest
		boneName = "spine_2";
	else if ( hitbox == 2 ) // Pelvis
		boneName = "pelvis";
	
	auto centerPos = GetCL_Bones()->GetBonePositionByName( pTarget , boneName );
	if ( centerPos.Length() == 0 )
		return;
	
	// Add center point
	points.push_back( centerPos );
	
	// Generate multipoints based on scale
	float scale = Settings::RageBot::MultipointScale / 100.0f;
	float offset = 2.0f * scale; // Base offset in units
	
	if ( Settings::RageBot::SafePoints && ShouldUseSafePoint( pTarget ) )
	{
		GetSafePoints( pTarget , hitbox , points );
	}
	else
	{
		// Generate regular multipoints
		if ( hitbox == 6 ) // Head
		{
			points.push_back( centerPos + Vector3( offset , 0 , 0 ) ); // Left
			points.push_back( centerPos + Vector3( -offset , 0 , 0 ) ); // Right
			points.push_back( centerPos + Vector3( 0 , offset , 0 ) ); // Forward
			points.push_back( centerPos + Vector3( 0 , 0 , offset ) ); // Top
		}
		else if ( hitbox == 5 ) // Chest
		{
			points.push_back( centerPos + Vector3( offset , 0 , 0 ) ); // Left
			points.push_back( centerPos + Vector3( -offset , 0 , 0 ) ); // Right
			points.push_back( centerPos + Vector3( 0 , 0 , offset ) ); // Upper
			points.push_back( centerPos + Vector3( 0 , 0 , -offset ) ); // Lower
		}
		else if ( hitbox == 2 ) // Pelvis
		{
			points.push_back( centerPos + Vector3( offset , 0 , 0 ) ); // Left
			points.push_back( centerPos + Vector3( -offset , 0 , 0 ) ); // Right
		}
	}
}

auto CRageBot::GetSafePoints( C_CSPlayerPawn* pTarget , int hitbox , std::vector<Vector3>& points ) -> void
{
	// Safe points are more conservative - use center and safer positions
	// This is a simplified implementation
	const char* boneName = "head_0";
	
	if ( hitbox == 5 )
		boneName = "spine_2";
	else if ( hitbox == 2 )
		boneName = "pelvis";
	
	auto centerPos = GetCL_Bones()->GetBonePositionByName( pTarget , boneName );
	if ( centerPos.Length() > 0 )
	{
		points.push_back( centerPos );
	}
}

auto CRageBot::ShouldUseSafePoint( C_CSPlayerPawn* pTarget ) -> bool
{
	if ( !Settings::RageBot::SafePoints )
		return false;
	
	// Use safe points when target is moving slowly or crouching
	auto velocity = pTarget->m_vecVelocity();
	float speed = velocity.Length();
	
	return speed < 50.0f; // Standing still or slow movement
}

auto CRageBot::CalculateHitchance( C_CSPlayerPawn* pLocalPawn , const Vector3& point , C_CSWeaponBaseGun* pWeapon ) -> float
{
	// Simplified hitchance calculation
	// Real implementation would use weapon spread, inaccuracy, etc.
	
	auto localEyePos = GetCL_Players()->GetLocalEyeOrigin();
	auto distance = CalculateDistance( localEyePos , point );
	
	// Start with base hitchance of 100%
	float baseHitchance = 100.0f;
	
	// Reduce hitchance based on distance
	float distanceFactor = 1.0f - ( distance / Settings::RageBot::MaxDistance );
	
	return baseHitchance * std::max( 0.0f , distanceFactor );
}

auto CRageBot::AdaptiveHitchance( C_CSPlayerPawn* pTarget , float baseHitchance ) -> float
{
	// Adjust hitchance based on target velocity
	auto velocity = pTarget->m_vecVelocity();
	float speed = velocity.Length();
	
	if ( speed > 200.0f )
		return baseHitchance * 0.8f; // Reduce for fast moving targets
	else if ( speed < 50.0f )
		return baseHitchance * 1.2f; // Increase for stationary targets
	
	return baseHitchance;
}

auto CRageBot::GetMinDamage( C_CSWeaponBaseGun* pWeapon , bool visible ) -> int
{
	if ( Settings::RageBot::DamageOverrideActive )
		return Settings::RageBot::DamageOverrideValue;
	
	auto weaponGroup = GetWeaponGroup( pWeapon );
	
	switch ( weaponGroup )
	{
		case WeaponGroup::PISTOL:
			return visible ? Settings::RageBot::PistolMinDamageVisible : Settings::RageBot::PistolMinDamage;
		case WeaponGroup::HEAVY_PISTOL:
			return visible ? Settings::RageBot::HeavyPistolMinDamageVisible : Settings::RageBot::HeavyPistolMinDamage;
		case WeaponGroup::SMG:
			return visible ? Settings::RageBot::SMGMinDamageVisible : Settings::RageBot::SMGMinDamage;
		case WeaponGroup::RIFLE:
			return visible ? Settings::RageBot::RifleMinDamageVisible : Settings::RageBot::RifleMinDamage;
		case WeaponGroup::SNIPER:
			return visible ? Settings::RageBot::SniperMinDamageVisible : Settings::RageBot::SniperMinDamage;
		case WeaponGroup::AUTO_SNIPER:
			return visible ? Settings::RageBot::AutoSniperMinDamageVisible : Settings::RageBot::AutoSniperMinDamage;
		default:
			return 30;
	}
}

auto CRageBot::CanHitMinDamage( C_CSPlayerPawn* pLocalPawn , C_CSPlayerPawn* pTarget , const Vector3& point , int minDamage ) -> bool
{
	// Simplified damage check
	// Real implementation would trace and calculate actual damage with penetration
	
	// For now, assume we can always hit at least minDamage if target is valid
	// This would need proper damage calculation based on weapon, distance, and penetration
	return true;
}

auto CRageBot::GetWeaponGroup( C_CSWeaponBaseGun* pWeapon ) -> WeaponGroup
{
	if ( !pWeapon )
		return WeaponGroup::PISTOL;
	
	auto weaponType = GetCL_Weapons()->GetLocalWeaponType();
	
	switch ( weaponType )
	{
		case CSWeaponType_t::WEAPONTYPE_PISTOL:
		{
			auto defIndex = GetCL_Weapons()->GetLocalWeaponDefinitionIndex();
			// Desert Eagle, R8 Revolver are heavy pistols
			if ( defIndex == 1 || defIndex == 64 )
				return WeaponGroup::HEAVY_PISTOL;
			return WeaponGroup::PISTOL;
		}
		case CSWeaponType_t::WEAPONTYPE_SUBMACHINEGUN:
			return WeaponGroup::SMG;
		case CSWeaponType_t::WEAPONTYPE_RIFLE:
			return WeaponGroup::RIFLE;
		case CSWeaponType_t::WEAPONTYPE_SNIPER_RIFLE:
		{
			auto defIndex = GetCL_Weapons()->GetLocalWeaponDefinitionIndex();
			// G3SG1, SCAR-20 are auto-snipers
			if ( defIndex == 11 || defIndex == 38 )
				return WeaponGroup::AUTO_SNIPER;
			return WeaponGroup::SNIPER;
		}
		case CSWeaponType_t::WEAPONTYPE_SHOTGUN:
			return WeaponGroup::SHOTGUN;
		case CSWeaponType_t::WEAPONTYPE_MACHINEGUN:
			return WeaponGroup::MACHINEGUN;
		default:
			return WeaponGroup::PISTOL;
	}
}

auto CRageBot::CompensateSpread( CUserCmd* pUserCmd , const QAngle& aimAngles ) -> void
{
	// No spread compensation - sets exact aim angles
	// Real implementation would get weapon spread pattern and compensate
	
	pUserCmd->cmd.mutable_base()->set_viewangles_x( aimAngles.m_x );
	pUserCmd->cmd.mutable_base()->set_viewangles_y( aimAngles.m_y );
	pUserCmd->cmd.mutable_base()->set_viewangles_z( aimAngles.m_z );
}

auto CRageBot::AutoStop( CUserCmd* pUserCmd , C_CSPlayerPawn* pLocalPawn ) -> void
{
	auto stopMode = AutoStopMode::FULL; // Default to full stop
	
	// Get current velocity
	auto velocity = pLocalPawn->m_vecVelocity();
	float speed = velocity.Length();
	
	if ( speed < 1.0f )
		return; // Already stopped
	
	switch ( stopMode )
	{
		case AutoStopMode::FULL:
		{
			// Remove all movement inputs
			pUserCmd->button_states.buttonstate1 &= ~IN_FORWARD;
			pUserCmd->button_states.buttonstate1 &= ~IN_BACK;
			pUserCmd->button_states.buttonstate1 &= ~IN_MOVELEFT;
			pUserCmd->button_states.buttonstate1 &= ~IN_MOVERIGHT;
			break;
		}
		case AutoStopMode::QUICK:
		{
			// Counter-strafe
			if ( velocity.m_x > 0 )
				pUserCmd->button_states.buttonstate1 |= IN_MOVELEFT;
			else if ( velocity.m_x < 0 )
				pUserCmd->button_states.buttonstate1 |= IN_MOVERIGHT;
			
			if ( velocity.m_y > 0 )
				pUserCmd->button_states.buttonstate1 |= IN_BACK;
			else if ( velocity.m_y < 0 )
				pUserCmd->button_states.buttonstate1 |= IN_FORWARD;
			break;
		}
		default:
			break;
	}
}

auto CRageBot::AutoScope( CUserCmd* pUserCmd , C_CSWeaponBaseGun* pWeapon ) -> void
{
	if ( !pWeapon )
		return;
	
	auto weaponType = GetCL_Weapons()->GetLocalWeaponType();
	
	// Check if weapon is a sniper
	if ( weaponType != CSWeaponType_t::WEAPONTYPE_SNIPER_RIFLE )
		return;
	
	auto pLocalPawn = GetCL_Players()->GetLocalPlayerPawn();
	if ( !pLocalPawn )
		return;
	
	// Check if already scoped
	if ( pLocalPawn->m_bIsScoped() )
		return;
	
	// Trigger scope
	pUserCmd->button_states.buttonstate1 |= IN_ATTACK2;
}

auto CRageBot::AutoFire( CUserCmd* pUserCmd , const TargetInfo& target ) -> void
{
	if ( !target.pawn )
		return;
	
	// Set attack button
	pUserCmd->button_states.buttonstate1 |= IN_ATTACK;
}

auto CRageBot::IsWeaponReady( C_CSWeaponBaseGun* pWeapon , C_CSPlayerPawn* pLocalPawn ) -> bool
{
	if ( !pWeapon || !pLocalPawn )
		return false;
	
	// Check if weapon has ammo
	if ( pWeapon->m_iClip1() <= 0 )
		return false;
	
	// Check if weapon is reloading
	if ( pWeapon->m_bInReload() )
		return false;
	
	// Check if player can attack
	if ( pLocalPawn->m_bWaitForNoAttack() )
		return false;
	
	return true;
}

auto CRageBot::CalculateFOV( const QAngle& viewAngles , const Vector3& start , const Vector3& end ) -> float
{
	auto aimAngles = Math::CalcAngle( start , end );
	
	auto delta = aimAngles - viewAngles;
	Math::NormalizeAngles( delta );
	
	return std::sqrtf( delta.m_x * delta.m_x + delta.m_y * delta.m_y );
}

auto CRageBot::CalculateDistance( const Vector3& start , const Vector3& end ) -> float
{
	return ( end - start ).Length();
}

auto CRageBot::GetRequiredHitchance( C_CSWeaponBaseGun* pWeapon , bool visible ) -> float
{
	auto weaponGroup = GetWeaponGroup( pWeapon );
	
	switch ( weaponGroup )
	{
		case WeaponGroup::PISTOL:
			return static_cast<float>( visible ? Settings::RageBot::PistolHitchanceVisible : Settings::RageBot::PistolHitchance );
		case WeaponGroup::HEAVY_PISTOL:
			return static_cast<float>( visible ? Settings::RageBot::HeavyPistolHitchanceVisible : Settings::RageBot::HeavyPistolHitchance );
		case WeaponGroup::SMG:
			return static_cast<float>( visible ? Settings::RageBot::SMGHitchanceVisible : Settings::RageBot::SMGHitchance );
		case WeaponGroup::RIFLE:
			return static_cast<float>( visible ? Settings::RageBot::RifleHitchanceVisible : Settings::RageBot::RifleHitchance );
		case WeaponGroup::SNIPER:
			return static_cast<float>( visible ? Settings::RageBot::SniperHitchanceVisible : Settings::RageBot::SniperHitchance );
		case WeaponGroup::AUTO_SNIPER:
			return static_cast<float>( visible ? Settings::RageBot::AutoSniperHitchanceVisible : Settings::RageBot::AutoSniperHitchance );
		default:
			return 50.0f;
	}
}

auto GetRageBot() -> CRageBot*
{
	return &g_RageBot;
}
