#pragma once

#include <Common/Common.hpp>
#include <ImGui/imgui.h>
#include <unordered_map>
#include <vector>
#include <GameClient/CL_ItemDefinition.hpp>

namespace Settings
{
    namespace Visual
    {
        inline auto Active = true;
        inline auto Team = true;
        inline auto Enemy = true;
        inline auto PlayerBox = true;
        inline auto ThirdPerson = false;
        inline float ThirdPersonDistance = 110.f;

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

        inline auto ShowWorldWeapons = true;
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
        inline auto ChamsEnabled = false;
        inline auto ChamsGlow = true;
        inline auto ChamsIgnoreZ = true;
        inline float ChamsVisibleColor[4] = { 0.f , 1.f , 0.f , 1.f };
        inline float ChamsInvisibleColor[4] = { 1.f , 0.f , 0.f , 1.f };
        inline auto ChamsOverlay = true;
        inline auto ViewmodelChamsEnabled = false;
        inline float ViewmodelChamsColor[4] = { 0.0f , 1.0f , 1.0f , 0.6f };
    }
    namespace Misc
    {
        inline auto MenuAlpha = 200;
        inline auto MenuStyle = 0;
    }
    
    namespace Rage
    {
        inline auto AimbotEnabled = false;
        inline auto Silent = false;
        inline float MaximumFov = 0.0f;
        inline auto VisualizeAimbot = false;
        inline auto AntiExploit = false;
        inline auto TargetDormant = false;
        inline auto Autofire = false;
        inline auto HideShot = false;
        inline auto DoubleTap = false;
        inline int DefensiveLag = 0;   // None/Low/Medium/High
        inline int DefensiveFake = 0;  // None/Low/Medium/High
        inline auto DelayShot = false;
        inline auto ForceExtraSafety = false;
        inline auto ForceBodyaim = false;
        inline auto HeadshotOnly = false;
        inline auto KnifeBot = false;

        inline float HitChance = 0.0f;       // 0–100
        inline float PointScale = 0.0f;      // 0–100
        inline float MinDamage = 0.0f;       // hp
        inline auto OverrideActive = false;  // gear button visual
        inline int   Hitboxes = 0;           // None/Head/Chest/Stomach/Arms/Legs
        inline int   Multipoint = 0;         // None/Low/Medium/High

        inline auto Autostop = false;
        inline int   SafepointMode = 0;      // None/Smart/Always
        inline int   Safety = 0;             // None/Low/Medium/High
        inline int   ExtraSafety = 0;        // None/Low/Medium/High
        inline int   Bodyaim = 0;            // None/When lethal/Always
        inline auto IgnoreLimbsOnMoving = false;
    }
    
    namespace Legit
    {
        // Triggerbot
        inline auto TriggerbotEnabled = false;
        inline auto TriggerbotKey = ImGuiKey_MouseLeft; // ALT por padrão
        inline auto TriggerbotVisibleOnly = true;
        inline auto TriggerbotAutoScope = true;
        inline int TriggerbotDelayMs = 50; // Delay em milissegundos
        
        // Modo de disparo
        inline auto TriggerbotBurstMode = false; // Para armas semi-auto
        inline int TriggerbotBurstCount = 3;
        
        // Smooth (para versões futuras)
        inline auto TriggerbotSmoothEnabled = false;
        inline float TriggerbotSmoothAmount = 5.0f;
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
