#pragma once

#include <Common/Common.hpp>
#include <ImGui/imgui.h>

namespace Settings
{
    namespace Visual
    {
        inline auto Active = true;
        inline auto Team = true;
        inline auto Enemy = true;
        inline auto PlayerBox = true;

        inline auto PlayerBoxType = 3;

        inline auto BoxColorMode = 0;
        inline float BoxCustomColor[4] = { 1.f , 1.f , 1.f , 1.f };
        inline float BoxGradientTop[4] = { 1.f , 1.f , 1.f , 1.f };
        inline float BoxGradientBottom[4] = { 1.f , 0.f , 0.f , 1.f };
        inline float BoxRGBSpeed = 1.0f;

        inline auto ShowName = true;
        inline auto NamePosition = 0; // 0=Above,1=Below
        inline auto NameColorMode = 0; // 0=Team default,1=Custom,2=Gradient,3=RGB
        inline float NameCustomColor[4] = { 1.f , 1.f , 1.f , 1.f };
        inline float NameGradientTop[4] = { 1.f , 1.f , 1.f , 1.f };
        inline float NameGradientBottom[4] = { 1.f , 0.f , 0.f , 1.f };
        inline float NameRGBSpeed = 1.0f;

        inline auto ShowWeapon = true;
        inline auto WeaponLabelMode = 0; // 0=Text,1=Icon
        inline auto WeaponPosition = 1; // 0=Above,1=Below
        inline auto WeaponColorMode = 0; // 0=Team default,1=Custom,2=Gradient,3=RGB
        inline float WeaponCustomColor[4] = { 1.f , 1.f , 1.f , 1.f };
        inline float WeaponGradientTop[4] = { 1.f , 1.f , 1.f , 1.f };
        inline float WeaponGradientBottom[4] = { 0.f , 0.f , 1.f , 1.f };
        inline float WeaponRGBSpeed = 1.0f;

        inline auto ShowWorldWeapons = false;
        inline auto WorldWeaponColorMode = 0;
        inline float WorldWeaponCustomColor[4] = { 1.f , 1.f , 1.f , 1.f };
        inline float WorldWeaponGradientTop[4] = { 1.f , 1.f , 1.f , 1.f };
        inline float WorldWeaponGradientBottom[4] = { 0.f , 0.f , 1.f , 1.f };
        inline float WorldWeaponRGBSpeed = 1.0f;

        inline auto ShowHealthBar = true;
        inline float HealthGradientTop[4] = { 0.f , 1.f , 0.f , 1.f };
        inline float HealthGradientBottom[4] = { 0.f , 1.f , 0.f , 1.f };
        inline float HealthBarWidth = 2.0f;

        inline auto ShowReload = true;
        inline float ReloadColor[4] = { 1.f , 0.85f , 0.f , 1.f };

        inline auto ShowBomb = true;
        inline float BombColor[4] = { 1.f , 0.5f , 0.f , 1.f };

        inline auto ShowC4Timer = true;
        inline auto ShowC4Damage = true;
        inline auto C4TimerColorMode = 0;
        inline float C4TimerCustomColor[4] = { 1.f , 1.f , 1.f , 1.f };
        inline float C4TimerGradientTop[4] = { 1.f , 1.f , 1.f , 1.f };
        inline float C4TimerGradientBottom[4] = { 1.f , 0.f , 0.f , 1.f };
        inline float C4TimerRGBSpeed = 1.0f;
        inline float C4DamageLethalColor[4] = { 1.f , 0.f , 0.f , 1.f };
        inline float C4DamageWarnColor[4] = { 1.f , 0.85f , 0.f , 1.f };
        inline float C4DamageSafeColor[4] = { 0.f , 1.f , 0.f , 1.f };
        inline auto ShowC4ProgressBar = true;
        inline auto ShowC4Radius = true;
        inline auto ShowC4SiteLabel = true;
        inline auto ShowDefuserIcon = true;
        inline auto ShowDefuserProgressRing = true;
        inline auto DebugC4 = true;
        inline auto DebugWorldWeapons = false;
        inline auto C4TimerFixed = true;
        inline auto C4TimerAnchor = 1;
        inline float C4TimerOffsetX = 20.f;
        inline float C4TimerOffsetY = 20.f;
        inline float C4TimerBarWidth = 180.f;
        inline float C4LethalRadius = 500.f;
        inline float C4DamageRadius = 800.f;

        inline auto ShowGrenades = true;
        inline auto ShowGrenadeTrajectory = true;
        inline auto ShowGrenadeTimers = true;
        inline auto ShowGrenadeRadius = true;
        inline auto ShowGrenadeLandingMarker = true;
        inline int GrenadeTrajectoryMaxPoints = 32;
        inline float GrenadeLandingStableDist = 4.f;
        inline int GrenadeLandingStableFrames = 6;
        inline float GrenadeHERadius = 400.f;
        inline float GrenadeFlashRadius = 650.f;
        inline float GrenadeSmokeRadius = 150.f;
        inline float GrenadeMolotovRadius = 180.f;
        inline float GrenadeSmokeDuration = 18.f;
        inline float GrenadeMolotovDuration = 7.f;
        inline float GrenadeTrajectoryThickness = 2.f;
        inline float GrenadeMarkerSize = 6.f;
        inline auto GrenadeTeamTint = true;
        inline auto ShowGrenadePrediction = true;
        inline float GrenadeSimInitialSpeed = 800.f;
        inline float GrenadeSimGravity = 800.f;
        inline float GrenadeSimRestitution = 0.45f;
        inline float GrenadeSimMaxTime = 3.0f;
        inline float GrenadeSimStep = 0.02f;
        inline float GrenadeTrailFadeTime = 1.5f;
    }
    namespace Misc
    {
        inline auto MenuAlpha = 200;
        inline auto MenuStyle = 0;
    }
    
    namespace RageBot
    {
        // General settings
        inline auto Enabled = false;
        inline auto SilentAim = true;
        inline auto AutoFire = true;
        inline auto AutoScope = true;
        inline auto NoSpread = true;
        inline auto Penetration = true;
        
        inline float MaxFOV = 180.0f;
        inline float MaxDistance = 8192.0f;
        inline int DelayShot = 0;
        
        // Target priority (0=FOV, 1=Distance, 2=HP, 3=Threat)
        inline int TargetPriority = 0;
        
        // Multipoint settings
        inline auto MultipointEnabled = true;
        inline float MultipointScale = 75.0f;
        inline auto SafePoints = true;
        inline auto SafePointOnLimbs = false;
        inline auto BodyAimFallback = true;
        
        // Hitbox selection
        inline auto TargetHead = true;
        inline auto TargetChest = true;
        inline auto TargetPelvis = false;
        
        // Per-weapon configs (Pistol)
        inline int PistolMinDamage = 30;
        inline int PistolMinDamageVisible = 40;
        inline int PistolHitchance = 50;
        inline int PistolHitchanceVisible = 60;
        inline auto PistolAutoScope = false;
        inline auto PistolAutoStop = true;
        
        // Per-weapon configs (Heavy Pistol)
        inline int HeavyPistolMinDamage = 40;
        inline int HeavyPistolMinDamageVisible = 50;
        inline int HeavyPistolHitchance = 55;
        inline int HeavyPistolHitchanceVisible = 65;
        inline auto HeavyPistolAutoScope = false;
        inline auto HeavyPistolAutoStop = true;
        
        // Per-weapon configs (SMG)
        inline int SMGMinDamage = 25;
        inline int SMGMinDamageVisible = 35;
        inline int SMGHitchance = 45;
        inline int SMGHitchanceVisible = 55;
        inline auto SMGAutoScope = false;
        inline auto SMGAutoStop = true;
        
        // Per-weapon configs (Rifle)
        inline int RifleMinDamage = 30;
        inline int RifleMinDamageVisible = 40;
        inline int RifleHitchance = 55;
        inline int RifleHitchanceVisible = 65;
        inline auto RifleAutoScope = false;
        inline auto RifleAutoStop = true;
        
        // Per-weapon configs (Sniper)
        inline int SniperMinDamage = 80;
        inline int SniperMinDamageVisible = 90;
        inline int SniperHitchance = 70;
        inline int SniperHitchanceVisible = 80;
        inline auto SniperAutoScope = true;
        inline auto SniperAutoStop = true;
        
        // Per-weapon configs (Auto Sniper)
        inline int AutoSniperMinDamage = 60;
        inline int AutoSniperMinDamageVisible = 70;
        inline int AutoSniperHitchance = 60;
        inline int AutoSniperHitchanceVisible = 70;
        inline auto AutoSniperAutoScope = true;
        inline auto AutoSniperAutoStop = true;
        
        // Overrides
        inline int DamageOverrideValue = 100;
        inline auto DamageOverrideActive = false;
        inline auto SafePointOverrideActive = false;
    }
    
	namespace Colors
	{
		namespace Visual
		{
			inline float PlayerBoxTT[4] = { 255.f / 255.f , 75.f / 255.f , 75.f / 255.f  , 1.f };
			inline float PlayerBoxTT_Visible[4] = { 0 , 1.f , 0.f , 1.f };
			inline float PlayerBoxCT[4] = { 65.f / 255.f , 200 / 255.f , 255.f / 255.f , 1.f };
			inline float PlayerBoxCT_Visible[4] = { 0 , 1.f , 0.f , 1.f };

			inline float GrenadeHE[4] = { 1.f , 0.3f , 0.3f , 1.f };
			inline float GrenadeFlash[4] = { 1.f , 1.f , 0.f , 1.f };
			inline float GrenadeSmoke[4] = { 0.6f , 0.6f , 0.6f , 1.f };
			inline float GrenadeMolotov[4] = { 1.f , 0.5f , 0.f , 1.f };
			inline float GrenadeDecoy[4] = { 0.4f , 0.8f , 1.f , 1.f };

			inline float GrenadeHE_Ally[4] = { 0.6f , 0.9f , 0.6f , 1.f };
			inline float GrenadeHE_Enemy[4] = { 1.f , 0.3f , 0.3f , 1.f };
			inline float GrenadeFlash_Ally[4] = { 0.8f , 0.8f , 0.f , 1.f };
			inline float GrenadeFlash_Enemy[4] = { 1.f , 1.f , 0.f , 1.f };
			inline float GrenadeSmoke_Ally[4] = { 0.5f , 0.7f , 0.9f , 1.f };
			inline float GrenadeSmoke_Enemy[4] = { 0.6f , 0.6f , 0.6f , 1.f };
			inline float GrenadeMolotov_Ally[4] = { 1.f , 0.7f , 0.2f , 1.f };
			inline float GrenadeMolotov_Enemy[4] = { 1.f , 0.5f , 0.f , 1.f };
			inline float GrenadeDecoy_Ally[4] = { 0.4f , 0.9f , 1.f , 1.f };
			inline float GrenadeDecoy_Enemy[4] = { 0.4f , 0.8f , 1.f , 1.f };
		}
	}
}
