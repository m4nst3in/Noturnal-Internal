#pragma once

#include <Common/Common.hpp>
#include <map>

enum class WeaponGroup
{
	PISTOL = 0,
	HEAVY_PISTOL = 1,
	SMG = 2,
	RIFLE = 3,
	SNIPER = 4,
	AUTO_SNIPER = 5,
	SHOTGUN = 6,
	MACHINEGUN = 7
};

enum class TargetPriority
{
	FOV = 0,
	DISTANCE = 1,
	HP = 2,
	THREAT = 3
};

enum class AutoStopMode
{
	FULL = 0,        // Stop completely
	MIN_SPEED = 1,   // Stop to minimum speed for accuracy
	PREDICTIVE = 2,  // Predict when target will stop
	QUICK = 3        // Quick counter-strafing
};

struct WeaponConfig
{
	int min_damage = 30;
	int min_damage_visible = 30;
	int hitchance = 50;
	int hitchance_visible = 60;
	bool auto_scope = true;
	bool auto_stop = true;
	AutoStopMode auto_stop_type = AutoStopMode::FULL;
};

struct RageBotConfig
{
	// General
	bool enabled = false;
	bool silent_aim = true;
	bool auto_fire = true;
	bool auto_scope = true;
	bool no_spread = true;
	bool penetration = true;
	
	float max_fov = 180.0f;
	float max_distance = 8192.0f;
	int delay_shot = 0; // ms
	
	// Target priority
	TargetPriority target_priority = TargetPriority::FOV;
	
	// Multipoint
	bool multipoint_enabled = true;
	float multipoint_scale = 75.0f; // 0-100%
	bool safe_points = true;
	bool safe_point_on_limbs = false;
	bool body_aim_fallback = true;
	
	// Hitbox selection
	bool target_head = true;
	bool target_chest = true;
	bool target_pelvis = false;
	
	// Per-weapon configs
	std::map<WeaponGroup, WeaponConfig> weapon_configs;
	
	// Overrides (keybinds would be handled elsewhere)
	int damage_override_value = 100;
	bool damage_override_active = false;
	bool safe_point_override_active = false;
	
	RageBotConfig()
	{
		// Initialize default weapon configs
		weapon_configs[WeaponGroup::PISTOL] = { 30, 40, 50, 60, false, true, AutoStopMode::FULL };
		weapon_configs[WeaponGroup::HEAVY_PISTOL] = { 40, 50, 55, 65, false, true, AutoStopMode::FULL };
		weapon_configs[WeaponGroup::SMG] = { 25, 35, 45, 55, false, true, AutoStopMode::FULL };
		weapon_configs[WeaponGroup::RIFLE] = { 30, 40, 55, 65, false, true, AutoStopMode::FULL };
		weapon_configs[WeaponGroup::SNIPER] = { 80, 90, 70, 80, true, true, AutoStopMode::FULL };
		weapon_configs[WeaponGroup::AUTO_SNIPER] = { 60, 70, 60, 70, true, true, AutoStopMode::FULL };
		weapon_configs[WeaponGroup::SHOTGUN] = { 40, 50, 40, 50, false, true, AutoStopMode::FULL };
		weapon_configs[WeaponGroup::MACHINEGUN] = { 30, 40, 50, 60, false, true, AutoStopMode::FULL };
	}
};
