#pragma once

#include <Common/Common.hpp>
#include <CS2/SDK/Math/Vector3.hpp>
#include <CS2/SDK/Math/QAngle.hpp>
#include "Config.hpp"

class CCSGOInput;
class CUserCmd;
class C_CSPlayerPawn;
class C_CSWeaponBaseGun;

struct TargetInfo
{
	C_CSPlayerPawn* pawn = nullptr;
	Vector3 aim_point;
	int hitbox = 0;
	float fov = 0.0f;
	float distance = 0.0f;
	bool visible = false;
	int damage = 0;
	float hitchance = 0.0f;
};

class CRageBot final
{
public:
	auto OnCreateMove( CCSGOInput* pInput , CUserCmd* pUserCmd ) -> void;
	
	auto GetConfig() -> RageBotConfig&
	{
		return m_Config;
	}

private:
	// Target selection
	auto SelectBestTarget( C_CSPlayerPawn* pLocalPawn ) -> TargetInfo;
	auto IsValidTarget( C_CSPlayerPawn* pLocalPawn , C_CSPlayerPawn* pTarget ) -> bool;
	auto CalculateTargetPriority( C_CSPlayerPawn* pLocalPawn , C_CSPlayerPawn* pTarget , float& outPriority ) -> bool;
	
	// Multipoint
	auto GenerateMultipoints( C_CSPlayerPawn* pTarget , int hitbox , std::vector<Vector3>& points ) -> void;
	auto GetSafePoints( C_CSPlayerPawn* pTarget , int hitbox , std::vector<Vector3>& points ) -> void;
	auto ShouldUseSafePoint( C_CSPlayerPawn* pTarget ) -> bool;
	
	// Hitchance
	auto CalculateHitchance( C_CSPlayerPawn* pLocalPawn , const Vector3& point , C_CSWeaponBaseGun* pWeapon ) -> float;
	auto AdaptiveHitchance( C_CSPlayerPawn* pTarget , float baseHitchance ) -> float;
	
	// Damage
	auto GetMinDamage( C_CSWeaponBaseGun* pWeapon , bool visible ) -> int;
	auto CanHitMinDamage( C_CSPlayerPawn* pLocalPawn , C_CSPlayerPawn* pTarget , const Vector3& point , int minDamage ) -> bool;
	auto GetWeaponGroup( C_CSWeaponBaseGun* pWeapon ) -> WeaponGroup;
	
	// No Spread
	auto CompensateSpread( CUserCmd* pUserCmd , const QAngle& aimAngles ) -> void;
	
	// Auto operations
	auto AutoStop( CUserCmd* pUserCmd , C_CSPlayerPawn* pLocalPawn ) -> void;
	auto AutoScope( CUserCmd* pUserCmd , C_CSWeaponBaseGun* pWeapon ) -> void;
	auto AutoFire( CUserCmd* pUserCmd , const TargetInfo& target ) -> void;
	
	// Utilities
	auto IsWeaponReady( C_CSWeaponBaseGun* pWeapon , C_CSPlayerPawn* pLocalPawn ) -> bool;
	auto CalculateFOV( const QAngle& viewAngles , const Vector3& start , const Vector3& end ) -> float;
	auto CalculateDistance( const Vector3& start , const Vector3& end ) -> float;
	auto GetRequiredHitchance( C_CSWeaponBaseGun* pWeapon , bool visible ) -> float;

private:
	RageBotConfig m_Config;
	uint64_t m_LastShotTime = 0;
};

auto GetRageBot() -> CRageBot*;
