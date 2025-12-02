#include "CTriggerbot.hpp"

#include <Fnv1a/Hash_Fnv1a_Constexpr.hpp>

#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Interface/IEngineToClient.hpp>
#include <CS2/SDK/Interface/CInputSystem.hpp>
#include <CS2/SDK/Update/CUserCmd.hpp>
#include <CS2/SDK/Update/CCSGOInput.hpp>
#include <CS2/SDK/Types/CEntityData.hpp>
#include <CS2/SDK/FunctionListSDK.hpp>

#include <GameClient/CL_Players.hpp>
#include <GameClient/CL_Weapons.hpp>
#include <GameClient/CL_Trace.hpp>
#include <GameClient/CL_VisibleCheck.hpp>
#include <GameClient/CL_Bypass.hpp>

#include <AndromedaClient/Settings/Settings.hpp>

#include <ImGui/imgui.h>

static CTriggerbot g_Triggerbot{};

// Hitbox name hashes for filtering
static constexpr uint64_t HASH_HEAD = hash_64_fnv1a_const( "head_0" );
static constexpr uint64_t HASH_NECK = hash_64_fnv1a_const( "neck_0" );
static constexpr uint64_t HASH_SPINE_1 = hash_64_fnv1a_const( "spine_1" );
static constexpr uint64_t HASH_SPINE_2 = hash_64_fnv1a_const( "spine_2" );
static constexpr uint64_t HASH_PELVIS = hash_64_fnv1a_const( "pelvis" );
static constexpr uint64_t HASH_ARM_UPPER_L = hash_64_fnv1a_const( "arm_upper_l" );
static constexpr uint64_t HASH_ARM_UPPER_R = hash_64_fnv1a_const( "arm_upper_r" );
static constexpr uint64_t HASH_ARM_LOWER_L = hash_64_fnv1a_const( "arm_lower_l" );
static constexpr uint64_t HASH_ARM_LOWER_R = hash_64_fnv1a_const( "arm_lower_r" );
static constexpr uint64_t HASH_LEG_UPPER_L = hash_64_fnv1a_const( "leg_upper_l" );
static constexpr uint64_t HASH_LEG_UPPER_R = hash_64_fnv1a_const( "leg_upper_r" );
static constexpr uint64_t HASH_ANKLE_L = hash_64_fnv1a_const( "ankle_l" );
static constexpr uint64_t HASH_ANKLE_R = hash_64_fnv1a_const( "ankle_r" );

auto CTriggerbot::OnCreateMove( CCSGOInput* pInput , CUserCmd* pUserCmd ) -> void
{
	// Check if triggerbot is enabled
	if ( !Settings::Legit::TriggerbotEnabled )
		return;

	// Check if we're in game
	if ( !SDK::Interfaces::EngineToClient()->IsInGame() )
		return;

	// Check hotkey - use ImGui key state
	if ( !ImGui::IsKeyDown( Settings::Legit::TriggerbotKey ) )
	{
		// Reset state when key is released
		m_bWaitingForDelay = false;
		m_bBurstActive = false;
		m_nBurstShotsRemaining = 0;
		return;
	}

	// Check if local player is alive
	if ( !GetCL_Players()->IsLocalPlayerAlive() )
		return;

	auto pLocalPlayerPawn = GetCL_Players()->GetLocalPlayerPawn();
	if ( !pLocalPlayerPawn )
		return;

	// Get current weapon
	auto pWeapon = GetCL_Weapons()->GetLocalActiveWeapon();
	if ( !pWeapon )
		return;

	// Get weapon data
	auto pWeaponVData = GetCL_Weapons()->GetLocalWeaponVData();
	if ( !pWeaponVData )
		return;

	// Check weapon type - skip grenades, knives (except when melee attacking), C4, etc.
	auto weaponType = pWeaponVData->m_WeaponType().m_Type;
	
	if ( weaponType == CSWeaponType_t::WEAPONTYPE_GRENADE ||
		 weaponType == CSWeaponType_t::WEAPONTYPE_C4 ||
		 weaponType == CSWeaponType_t::WEAPONTYPE_EQUIPMENT ||
		 weaponType == CSWeaponType_t::WEAPONTYPE_TASER ||
		 weaponType == CSWeaponType_t::WEAPONTYPE_STACKABLEITEM ||
		 weaponType == CSWeaponType_t::WEAPONTYPE_BREACHCHARGE ||
		 weaponType == CSWeaponType_t::WEAPONTYPE_BUMPMINE ||
		 weaponType == CSWeaponType_t::WEAPONTYPE_TABLET ||
		 weaponType == CSWeaponType_t::WEAPONTYPE_ZONE_REPULSOR ||
		 weaponType == CSWeaponType_t::WEAPONTYPE_SHIELD )
	{
		return;
	}

	// Handle auto-scope for snipers
	if ( Settings::Legit::TriggerbotAutoScope )
	{
		if ( HandleAutoScope( pUserCmd ) )
			return; // Still scoping in, wait
	}

	// Check if weapon has ammo
	if ( pWeapon->m_iClip1() <= 0 )
		return;

	// Check if weapon is reloading
	if ( pWeapon->m_bInReload() )
		return;

	// Trace to check if crosshair is on enemy
	auto [boneHash, pHitEntity] = GetCL_Trace()->TraceToBoneEntity( pInput , nullptr , nullptr );

	if ( !pHitEntity || boneHash == 0 )
	{
		// No target, reset delay
		m_bWaitingForDelay = false;
		m_bBurstActive = false;
		m_nBurstShotsRemaining = 0;
		return;
	}

	// Check if valid target
	if ( !IsValidTarget( pHitEntity ) )
	{
		m_bWaitingForDelay = false;
		m_bBurstActive = false;
		m_nBurstShotsRemaining = 0;
		return;
	}

	// Check hitbox filter
	// 0 = All, 1 = Head Only, 2 = Body Only
	if ( Settings::Legit::TriggerbotHitboxFilter == 1 )
	{
		// Head only
		if ( boneHash != HASH_HEAD && boneHash != HASH_NECK )
			return;
	}
	else if ( Settings::Legit::TriggerbotHitboxFilter == 2 )
	{
		// Body only - exclude head and limbs
		if ( boneHash == HASH_HEAD || boneHash == HASH_NECK ||
			 boneHash == HASH_ARM_LOWER_L || boneHash == HASH_ARM_LOWER_R ||
			 boneHash == HASH_ANKLE_L || boneHash == HASH_ANKLE_R )
		{
			return;
		}
	}

	// Check visibility if required
	if ( Settings::Legit::TriggerbotVisibleOnly )
	{
		auto pTargetPawn = reinterpret_cast<C_CSPlayerPawn*>( pHitEntity );
		if ( !GetCL_VisibleCheck()->IsPlayerPawnVisible( pTargetPawn ) )
			return;
	}

	// Handle delay
	uint64_t currentTime = GetTickCount64();
	
	if ( !m_bWaitingForDelay && !m_bBurstActive )
	{
		// Start delay timer
		m_bWaitingForDelay = true;
		m_DelayStartTime = currentTime;
	}

	if ( m_bWaitingForDelay )
	{
		// Check if delay has passed
		if ( currentTime - m_DelayStartTime < static_cast<uint64_t>( Settings::Legit::TriggerbotDelayMs ) )
			return;
		
		m_bWaitingForDelay = false;
		
		// Initialize burst mode if enabled
		if ( Settings::Legit::TriggerbotBurstMode )
		{
			m_bBurstActive = true;
			m_nBurstShotsRemaining = Settings::Legit::TriggerbotBurstCount;
		}
	}

	// Fire!
	if ( ShouldFire() )
	{
		GetCL_Bypass()->SetAttack( pUserCmd , true );
		
		// Handle burst mode
		if ( m_bBurstActive )
		{
			m_nBurstShotsRemaining--;
			if ( m_nBurstShotsRemaining <= 0 )
			{
				m_bBurstActive = false;
				m_bWaitingForDelay = false;
			}
		}
	}
}

auto CTriggerbot::IsValidTarget( C_BaseEntity* pEntity ) -> bool
{
	if ( !pEntity )
		return false;

	// Check if it's a player pawn
	if ( !pEntity->IsPlayerPawn() )
		return false;

	auto pTargetPawn = reinterpret_cast<C_CSPlayerPawn*>( pEntity );
	
	// Check if alive
	if ( !pTargetPawn->IsAlive() )
		return false;

	// Get local player
	auto pLocalPlayerPawn = GetCL_Players()->GetLocalPlayerPawn();
	if ( !pLocalPlayerPawn )
		return false;

	// Don't shoot ourselves
	if ( pTargetPawn == pLocalPlayerPawn )
		return false;

	// Team check - don't shoot teammates
	if ( pTargetPawn->m_iTeamNum() == pLocalPlayerPawn->m_iTeamNum() )
		return false;

	// Check if target has immunity (gun game immunity)
	if ( pTargetPawn->m_bGunGameImmunity() )
		return false;

	// Check if dormant
	auto pSceneNode = pTargetPawn->m_pGameSceneNode();
	if ( pSceneNode && pSceneNode->m_bDormant() )
		return false;

	return true;
}

auto CTriggerbot::ShouldFire() -> bool
{
	// Additional checks before firing
	auto pWeapon = GetCL_Weapons()->GetLocalActiveWeapon();
	if ( !pWeapon )
		return false;

	// Check next attack tick (weapon ready to fire)
	auto pWeaponVData = GetCL_Weapons()->GetLocalWeaponVData();
	if ( !pWeaponVData )
		return false;

	return true;
}

auto CTriggerbot::HandleAutoScope( CUserCmd* pUserCmd ) -> bool
{
	auto pWeaponVData = GetCL_Weapons()->GetLocalWeaponVData();
	if ( !pWeaponVData )
		return false;

	auto weaponType = pWeaponVData->m_WeaponType().m_Type;
	
	// Only apply to sniper rifles
	if ( weaponType != CSWeaponType_t::WEAPONTYPE_SNIPER_RIFLE )
		return false;

	auto pLocalPlayerPawn = GetCL_Players()->GetLocalPlayerPawn();
	if ( !pLocalPlayerPawn )
		return false;

	// Check if already scoped
	if ( pLocalPlayerPawn->m_bIsScoped() )
		return false;

	// Scope in using attack2
	pUserCmd->button_states.buttonstate1 |= IN_ATTACK2;
	pUserCmd->cmd.mutable_base()->mutable_buttons_pb()->set_buttonstate1( pUserCmd->button_states.buttonstate1 );

	// Return true to indicate we're still scoping in
	return true;
}

auto GetTriggerbot() -> CTriggerbot*
{
	return &g_Triggerbot;
}
