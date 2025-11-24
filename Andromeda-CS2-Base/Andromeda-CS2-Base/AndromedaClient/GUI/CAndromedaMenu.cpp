#include "CAndromedaMenu.hpp"

#include <ImGui/imgui.h>

#include <AndromedaClient/Settings/Settings.hpp>
#include <AndromedaClient/CAndromedaGUI.hpp>

static CAndromedaMenu g_CAndromedaMenu{};

auto CAndromedaMenu::OnRenderMenu() -> void
{
    const float MenuAlpha = static_cast<float>( Settings::Misc::MenuAlpha ) / 255.f;

    ImGui::PushStyleVar( ImGuiStyleVar_Alpha , MenuAlpha );
    ImGui::SetNextWindowSize( ImVec2( 500 , 500 ) , ImGuiCond_FirstUseEver );

    if ( ImGui::Begin( XorStr( CHEAT_NAME ) , 0 ) )
    {
        if ( ImGui::BeginTabBar( XorStr( "MainTabs" ) , ImGuiTabBarFlags_None ) )
        {
            

            

            if ( ImGui::BeginTabItem( XorStr( "Visuals" ) ) )
            {
                ImGui::Columns( 3 , 0 , false );

                ImGui::BeginChild( XorStr( "Group_Visuals_ESP" ) , ImVec2( 0 , 180 ) , true );
                ImGui::Text( XorStr( "ESP" ) );
                ImGui::Separator();
                ImGui::Checkbox( XorStr( "Enable" ) , &Settings::Visual::Active );
                ImGui::Checkbox( XorStr( "Show Allies" ) , &Settings::Visual::Team );
                ImGui::Checkbox( XorStr( "Show Enemies" ) , &Settings::Visual::Enemy );
                ImGui::Checkbox( XorStr( "Show Box" ) , &Settings::Visual::PlayerBox );
                const char* boxTypes[] = { "Box" , "Outline Box" , "Coal Box" , "Outline Coal Box" };
                ImGui::Combo( XorStr( "Box type" ) , &Settings::Visual::PlayerBoxType , boxTypes , IM_ARRAYSIZE( boxTypes ) );
                const char* modes[] = { "Team default" , "Custom" , "Gradient" , "RGB" };
                ImGui::Combo( XorStr( "Box color mode" ) , &Settings::Visual::BoxColorMode , modes , IM_ARRAYSIZE( modes ) );
                if ( Settings::Visual::BoxColorMode == 1 )
                {
                    ImGui::ColorEdit4( XorStr( "Custom color" ) , Settings::Visual::BoxCustomColor , ImGuiColorEditFlags_NoInputs );
                }
                else if ( Settings::Visual::BoxColorMode == 2 )
                {
                    ImGui::ColorEdit4( XorStr( "Top" ) , Settings::Visual::BoxGradientTop , ImGuiColorEditFlags_NoInputs );
                    ImGui::ColorEdit4( XorStr( "Bottom" ) , Settings::Visual::BoxGradientBottom , ImGuiColorEditFlags_NoInputs );
                    ImGui::Text( XorStr( "Gradient fill + outline" ) );
                }
                else if ( Settings::Visual::BoxColorMode == 3 )
                {
                    ImGui::SliderFloat( XorStr( "RGB speed" ) , &Settings::Visual::BoxRGBSpeed , 0.1f , 5.0f , XorStr( "%.2fx" ) );
                }

                ImGui::Separator();
                ImGui::Checkbox( XorStr( "Show Name" ) , &Settings::Visual::ShowName );
                const char* posOpts[] = { "Above" , "Below" };
                ImGui::Combo( XorStr( "Name position" ) , &Settings::Visual::NamePosition , posOpts , IM_ARRAYSIZE( posOpts ) );
                ImGui::Combo( XorStr( "Name color mode" ) , &Settings::Visual::NameColorMode , modes , IM_ARRAYSIZE( modes ) );
                if ( Settings::Visual::NameColorMode == 1 )
                    ImGui::ColorEdit4( XorStr( "Name color" ) , Settings::Visual::NameCustomColor , ImGuiColorEditFlags_NoInputs );
                else if ( Settings::Visual::NameColorMode == 2 )
                {
                    ImGui::ColorEdit4( XorStr( "Name top" ) , Settings::Visual::NameGradientTop , ImGuiColorEditFlags_NoInputs );
                    ImGui::ColorEdit4( XorStr( "Name bottom" ) , Settings::Visual::NameGradientBottom , ImGuiColorEditFlags_NoInputs );
                }
                else if ( Settings::Visual::NameColorMode == 3 )
                    ImGui::SliderFloat( XorStr( "Name RGB speed" ) , &Settings::Visual::NameRGBSpeed , 0.1f , 5.0f , XorStr( "%.2fx" ) );

                ImGui::Separator();
                ImGui::Checkbox( XorStr( "Show Weapon" ) , &Settings::Visual::ShowWeapon );
                ImGui::Checkbox( XorStr( "Show World Weapons" ) , &Settings::Visual::ShowWorldWeapons );
                ImGui::Combo( XorStr( "World weapons color mode" ) , &Settings::Visual::WorldWeaponColorMode , modes , IM_ARRAYSIZE( modes ) );
                if ( Settings::Visual::WorldWeaponColorMode == 1 )
                    ImGui::ColorEdit4( XorStr( "World weapons color" ) , Settings::Visual::WorldWeaponCustomColor , ImGuiColorEditFlags_NoInputs );
                else if ( Settings::Visual::WorldWeaponColorMode == 2 )
                {
                    ImGui::ColorEdit4( XorStr( "World weapons top" ) , Settings::Visual::WorldWeaponGradientTop , ImGuiColorEditFlags_NoInputs );
                    ImGui::ColorEdit4( XorStr( "World weapons bottom" ) , Settings::Visual::WorldWeaponGradientBottom , ImGuiColorEditFlags_NoInputs );
                }
                else if ( Settings::Visual::WorldWeaponColorMode == 3 )
                    ImGui::SliderFloat( XorStr( "World weapons RGB speed" ) , &Settings::Visual::WorldWeaponRGBSpeed , 0.1f , 5.0f , XorStr( "%.2fx" ) );
                const char* weaponLabelOpts[] = { "Text" , "Icon" };
                ImGui::Combo( XorStr( "Weapon display" ) , &Settings::Visual::WeaponLabelMode , weaponLabelOpts , IM_ARRAYSIZE( weaponLabelOpts ) );
                ImGui::Combo( XorStr( "Weapon position" ) , &Settings::Visual::WeaponPosition , posOpts , IM_ARRAYSIZE( posOpts ) );
                ImGui::Combo( XorStr( "Weapon color mode" ) , &Settings::Visual::WeaponColorMode , modes , IM_ARRAYSIZE( modes ) );
                if ( Settings::Visual::WeaponColorMode == 1 )
                    ImGui::ColorEdit4( XorStr( "Weapon color" ) , Settings::Visual::WeaponCustomColor , ImGuiColorEditFlags_NoInputs );
                else if ( Settings::Visual::WeaponColorMode == 2 )
                {
                    ImGui::ColorEdit4( XorStr( "Weapon top" ) , Settings::Visual::WeaponGradientTop , ImGuiColorEditFlags_NoInputs );
                    ImGui::ColorEdit4( XorStr( "Weapon bottom" ) , Settings::Visual::WeaponGradientBottom , ImGuiColorEditFlags_NoInputs );
                }
                else if ( Settings::Visual::WeaponColorMode == 3 )
                    ImGui::SliderFloat( XorStr( "Weapon RGB speed" ) , &Settings::Visual::WeaponRGBSpeed , 0.1f , 5.0f , XorStr( "%.2fx" ) );
                ImGui::EndChild();

                ImGui::NextColumn();
                ImGui::BeginChild( XorStr( "Group_Visuals_2" ) , ImVec2( 0 , 180 ) , true );
                ImGui::Text( XorStr( "HUD" ) );
                ImGui::Separator();
                ImGui::Checkbox( XorStr( "Show Health Bar" ) , &Settings::Visual::ShowHealthBar );
                ImGui::ColorEdit4( XorStr( "Health top" ) , Settings::Visual::HealthGradientTop , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "Health bottom" ) , Settings::Visual::HealthGradientBottom , ImGuiColorEditFlags_NoInputs );
                ImGui::SliderFloat( XorStr( "Health width" ) , &Settings::Visual::HealthBarWidth , 1.0f , 6.0f , XorStr( "%.1f px" ) );
                ImGui::Separator();
                ImGui::Checkbox( XorStr( "Show Reload" ) , &Settings::Visual::ShowReload );
                ImGui::ColorEdit4( XorStr( "Reload color" ) , Settings::Visual::ReloadColor , ImGuiColorEditFlags_NoInputs );
                ImGui::Separator();
                ImGui::Checkbox( XorStr( "Show Bomb (C4)" ) , &Settings::Visual::ShowBomb );
                ImGui::ColorEdit4( XorStr( "Bomb color" ) , Settings::Visual::BombColor , ImGuiColorEditFlags_NoInputs );
                ImGui::Separator();
                ImGui::Checkbox( XorStr( "Show C4 Timer" ) , &Settings::Visual::ShowC4Timer );
                ImGui::Checkbox( XorStr( "Show C4 Damage" ) , &Settings::Visual::ShowC4Damage );
                ImGui::Checkbox( XorStr( "Show C4 Progress Bar" ) , &Settings::Visual::ShowC4ProgressBar );
                ImGui::Checkbox( XorStr( "Show C4 Radius" ) , &Settings::Visual::ShowC4Radius );
                ImGui::Checkbox( XorStr( "Show C4 Site Label" ) , &Settings::Visual::ShowC4SiteLabel );
                ImGui::Checkbox( XorStr( "Show Defuser Icon" ) , &Settings::Visual::ShowDefuserIcon );
                ImGui::Checkbox( XorStr( "Show Defuser Progress Ring" ) , &Settings::Visual::ShowDefuserProgressRing );
                ImGui::Separator();
                ImGui::Checkbox( XorStr( "C4 Timer Fixed on Screen" ) , &Settings::Visual::C4TimerFixed );
                const char* c4Anchors[] = { "Top Center" , "Bottom Right" };
                ImGui::Combo( XorStr( "C4 Timer Anchor" ) , &Settings::Visual::C4TimerAnchor , c4Anchors , IM_ARRAYSIZE( c4Anchors ) );
                ImGui::SliderFloat( XorStr( "C4 Timer X Offset" ) , &Settings::Visual::C4TimerOffsetX , 0.f , 400.f , XorStr( "%.0f px" ) );
                ImGui::SliderFloat( XorStr( "C4 Timer Y Offset" ) , &Settings::Visual::C4TimerOffsetY , 0.f , 200.f , XorStr( "%.0f px" ) );
                ImGui::SliderFloat( XorStr( "C4 Timer Bar Width" ) , &Settings::Visual::C4TimerBarWidth , 100.f , 400.f , XorStr( "%.0f px" ) );
                ImGui::Separator();
                ImGui::SliderFloat( XorStr( "C4 Lethal Radius" ) , &Settings::Visual::C4LethalRadius , 300.f , 700.f , XorStr( "%.0f" ) );
                ImGui::SliderFloat( XorStr( "C4 Damage Radius" ) , &Settings::Visual::C4DamageRadius , 600.f , 1200.f , XorStr( "%.0f" ) );
                ImGui::Separator();
                ImGui::Text( XorStr( "Grenade ESP" ) );
                ImGui::Checkbox( XorStr( "Show Grenades" ) , &Settings::Visual::ShowGrenades );
                ImGui::Checkbox( XorStr( "Trajectory" ) , &Settings::Visual::ShowGrenadeTrajectory );
                ImGui::Checkbox( XorStr( "Timers" ) , &Settings::Visual::ShowGrenadeTimers );
                ImGui::Checkbox( XorStr( "Radius" ) , &Settings::Visual::ShowGrenadeRadius );
                ImGui::Checkbox( XorStr( "Landing marker" ) , &Settings::Visual::ShowGrenadeLandingMarker );
                ImGui::Checkbox( XorStr( "Team tinted colors" ) , &Settings::Visual::GrenadeTeamTint );
                ImGui::SliderInt( XorStr( "Max trajectory points" ) , &Settings::Visual::GrenadeTrajectoryMaxPoints , 8 , 64 );
                ImGui::SliderFloat( XorStr( "Trajectory thickness" ) , &Settings::Visual::GrenadeTrajectoryThickness , 1.f , 4.f , XorStr( "%.1f" ) );
                ImGui::SliderFloat( XorStr( "HE radius" ) , &Settings::Visual::GrenadeHERadius , 300.f , 600.f , XorStr( "%.0f" ) );
                ImGui::SliderFloat( XorStr( "Flash radius" ) , &Settings::Visual::GrenadeFlashRadius , 500.f , 900.f , XorStr( "%.0f" ) );
                ImGui::SliderFloat( XorStr( "Smoke radius" ) , &Settings::Visual::GrenadeSmokeRadius , 100.f , 220.f , XorStr( "%.0f" ) );
                ImGui::SliderFloat( XorStr( "Molotov radius" ) , &Settings::Visual::GrenadeMolotovRadius , 120.f , 260.f , XorStr( "%.0f" ) );
                ImGui::SliderFloat( XorStr( "Smoke duration" ) , &Settings::Visual::GrenadeSmokeDuration , 10.f , 25.f , XorStr( "%.0f s" ) );
                ImGui::SliderFloat( XorStr( "Molotov duration" ) , &Settings::Visual::GrenadeMolotovDuration , 5.f , 12.f , XorStr( "%.0f s" ) );
                ImGui::Separator();
                ImGui::Checkbox( XorStr( "Show local prediction" ) , &Settings::Visual::ShowGrenadePrediction );
                ImGui::SliderFloat( XorStr( "Sim initial speed" ) , &Settings::Visual::GrenadeSimInitialSpeed , 400.f , 1200.f , XorStr( "%.0f" ) );
                ImGui::SliderFloat( XorStr( "Sim gravity" ) , &Settings::Visual::GrenadeSimGravity , 400.f , 1200.f , XorStr( "%.0f" ) );
                ImGui::SliderFloat( XorStr( "Sim restitution" ) , &Settings::Visual::GrenadeSimRestitution , 0.1f , 0.9f , XorStr( "%.2f" ) );
                ImGui::SliderFloat( XorStr( "Sim max time" ) , &Settings::Visual::GrenadeSimMaxTime , 1.f , 5.f , XorStr( "%.1f s" ) );
                ImGui::SliderFloat( XorStr( "Trail fade after stop" ) , &Settings::Visual::GrenadeTrailFadeTime , 0.5f , 3.0f , XorStr( "%.1f s" ) );
                ImGui::Separator();
                ImGui::ColorEdit4( XorStr( "HE color" ) , Settings::Colors::Visual::GrenadeHE , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "Flash color" ) , Settings::Colors::Visual::GrenadeFlash , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "Smoke color" ) , Settings::Colors::Visual::GrenadeSmoke , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "Molotov color" ) , Settings::Colors::Visual::GrenadeMolotov , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "Decoy color" ) , Settings::Colors::Visual::GrenadeDecoy , ImGuiColorEditFlags_NoInputs );
                ImGui::Separator();
                ImGui::Text( XorStr( "Team Tinted" ) );
                ImGui::ColorEdit4( XorStr( "HE ally" ) , Settings::Colors::Visual::GrenadeHE_Ally , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "HE enemy" ) , Settings::Colors::Visual::GrenadeHE_Enemy , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "Flash ally" ) , Settings::Colors::Visual::GrenadeFlash_Ally , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "Flash enemy" ) , Settings::Colors::Visual::GrenadeFlash_Enemy , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "Smoke ally" ) , Settings::Colors::Visual::GrenadeSmoke_Ally , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "Smoke enemy" ) , Settings::Colors::Visual::GrenadeSmoke_Enemy , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "Molotov ally" ) , Settings::Colors::Visual::GrenadeMolotov_Ally , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "Molotov enemy" ) , Settings::Colors::Visual::GrenadeMolotov_Enemy , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "Decoy ally" ) , Settings::Colors::Visual::GrenadeDecoy_Ally , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "Decoy enemy" ) , Settings::Colors::Visual::GrenadeDecoy_Enemy , ImGuiColorEditFlags_NoInputs );
                ImGui::Combo( XorStr( "C4 timer color mode" ) , &Settings::Visual::C4TimerColorMode , modes , IM_ARRAYSIZE( modes ) );
                if ( Settings::Visual::C4TimerColorMode == 1 )
                    ImGui::ColorEdit4( XorStr( "C4 timer color" ) , Settings::Visual::C4TimerCustomColor , ImGuiColorEditFlags_NoInputs );
                else if ( Settings::Visual::C4TimerColorMode == 2 )
                {
                    ImGui::ColorEdit4( XorStr( "C4 timer top" ) , Settings::Visual::C4TimerGradientTop , ImGuiColorEditFlags_NoInputs );
                    ImGui::ColorEdit4( XorStr( "C4 timer bottom" ) , Settings::Visual::C4TimerGradientBottom , ImGuiColorEditFlags_NoInputs );
                }
                else if ( Settings::Visual::C4TimerColorMode == 3 )
                    ImGui::SliderFloat( XorStr( "C4 timer RGB speed" ) , &Settings::Visual::C4TimerRGBSpeed , 0.1f , 5.0f , XorStr( "%.2fx" ) );
                ImGui::ColorEdit4( XorStr( "C4 lethal color" ) , Settings::Visual::C4DamageLethalColor , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "C4 warn color" ) , Settings::Visual::C4DamageWarnColor , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "C4 safe color" ) , Settings::Visual::C4DamageSafeColor , ImGuiColorEditFlags_NoInputs );
                ImGui::EndChild();

                ImGui::NextColumn();
                ImGui::BeginChild( XorStr( "Group_Visuals_3" ) , ImVec2( 0 , 180 ) , true );
                ImGui::Text( XorStr( "Team Colors" ) );
                ImGui::Separator();
                ImGui::ColorEdit4( XorStr( "TT box" ) , Settings::Colors::Visual::PlayerBoxTT , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "TT visible" ) , Settings::Colors::Visual::PlayerBoxTT_Visible , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "CT box" ) , Settings::Colors::Visual::PlayerBoxCT , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "CT visible" ) , Settings::Colors::Visual::PlayerBoxCT_Visible , ImGuiColorEditFlags_NoInputs );
                ImGui::EndChild();

                ImGui::Columns( 1 );
                ImGui::EndTabItem();
            }

            if ( ImGui::BeginTabItem( XorStr( "Misc" ) ) )
            {
                ImGui::Columns( 1 , 0 , false );
                ImGui::BeginChild( XorStr( "Group_Misc_2" ) , ImVec2( 0 , 160 ) , true );
                ImGui::EndChild();
                ImGui::Columns( 1 );
                ImGui::EndTabItem();
            }

            if ( ImGui::BeginTabItem( XorStr( "Inventory" ) ) )
            {
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }

	ImGui::End();

	ImGui::PopStyleVar();
}

auto GetAndromedaMenu() -> CAndromedaMenu*
{
	return &g_CAndromedaMenu;
}
