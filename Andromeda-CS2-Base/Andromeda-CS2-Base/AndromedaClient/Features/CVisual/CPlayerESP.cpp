#include <AndromedaClient/Features/CPlayerESP.hpp>
#include <AndromedaClient/Features/CVisual/CVisual.hpp>
#include <algorithm>

#include <AndromedaClient/Render/CRenderStackSystem.hpp>
#include <AndromedaClient/Fonts/CFontManager.hpp>
#include <AndromedaClient/Settings/Settings.hpp>
#include <GameClient/CL_ItemDefinition.hpp>
#include <GameClient/CL_Players.hpp>

#include <ImGui/imgui.h>

auto RenderPlayerESP( CCSPlayerController* pCCSPlayerController , const Rect_t& bBox , const bool bVisible ) -> void
{
    if ( !pCCSPlayerController->m_bPawnIsAlive() ) return;
    auto* pC_CSPlayerPawn = pCCSPlayerController->m_hPawn().Get<C_CSPlayerPawn>();
    if ( !pC_CSPlayerPawn || !pC_CSPlayerPawn->IsPlayerPawn() ) return;
    auto IsEnemy = false;
    if ( auto* pLocalPlayerController = GetCL_Players()->GetLocalPlayerController(); pLocalPlayerController )
        IsEnemy = pCCSPlayerController->m_iTeamNum() != pLocalPlayerController->m_iTeamNum();
    const ImVec2 min = { bBox.x, bBox.y };
    const ImVec2 max = { bBox.w, bBox.h };
    auto Draw = false;
    if ( Settings::Visual::Team && !IsEnemy ) Draw = true;
    if ( Settings::Visual::Enemy && IsEnemy ) Draw = true;
    if ( !Draw ) return;

    if ( Settings::Visual::PlayerBox )
    {
        auto PlayerColor = ImColor( 255 , 255 , 255 );
        if ( pCCSPlayerController->m_iTeamNum() == TEAM_TT )
        {
            PlayerColor = ImColor( Settings::Colors::Visual::PlayerBoxTT[0] , Settings::Colors::Visual::PlayerBoxTT[1] , Settings::Colors::Visual::PlayerBoxTT[2] , Settings::Colors::Visual::PlayerBoxTT[3] );
            if ( bVisible )
                PlayerColor = ImColor( Settings::Colors::Visual::PlayerBoxTT_Visible[0] , Settings::Colors::Visual::PlayerBoxTT_Visible[1] , Settings::Colors::Visual::PlayerBoxTT_Visible[2] , Settings::Colors::Visual::PlayerBoxTT_Visible[3] );
        }
        else if ( pCCSPlayerController->m_iTeamNum() == TEAM_CT )
        {
            PlayerColor = ImColor( Settings::Colors::Visual::PlayerBoxCT[0] , Settings::Colors::Visual::PlayerBoxCT[1] , Settings::Colors::Visual::PlayerBoxCT[2] , Settings::Colors::Visual::PlayerBoxCT[3] );
            if ( bVisible )
                PlayerColor = ImColor( Settings::Colors::Visual::PlayerBoxCT_Visible[0] , Settings::Colors::Visual::PlayerBoxCT_Visible[1] , Settings::Colors::Visual::PlayerBoxCT_Visible[2] , Settings::Colors::Visual::PlayerBoxCT_Visible[3] );
        }
        if ( Settings::Visual::BoxColorMode == 1 )
            PlayerColor = ImColor( Settings::Visual::BoxCustomColor[0] , Settings::Visual::BoxCustomColor[1] , Settings::Visual::BoxCustomColor[2] , Settings::Visual::BoxCustomColor[3] );
        else if ( Settings::Visual::BoxColorMode == 2 )
        {
            const ImU32 top = ImColor( Settings::Visual::BoxGradientTop[0] , Settings::Visual::BoxGradientTop[1] , Settings::Visual::BoxGradientTop[2] , Settings::Visual::BoxGradientTop[3] );
            const ImU32 bot = ImColor( Settings::Visual::BoxGradientBottom[0] , Settings::Visual::BoxGradientBottom[1] , Settings::Visual::BoxGradientBottom[2] , Settings::Visual::BoxGradientBottom[3] );
            const float inset = ( Settings::Visual::PlayerBoxType == CVisual::OUTLINE_COAL_BOX || Settings::Visual::PlayerBoxType == CVisual::COAL_BOX ) ? 2.f : 1.f;
            const ImVec2 fillMin = ImVec2( min.x + inset , min.y + inset );
            const ImVec2 fillMax = ImVec2( max.x - inset , max.y - inset );
            GetRenderStackSystem()->DrawRectFilledMultiColor( fillMin , fillMax , top , top , bot , bot );
        }
        else if ( Settings::Visual::BoxColorMode == 3 )
        {
            const float t = static_cast<float>( ImGui::GetTime() ) * Settings::Visual::BoxRGBSpeed;
            const float h = fmodf( t , 1.0f );
            float r , g , b; ImGui::ColorConvertHSVtoRGB( h , 1.0f , 1.0f , r , g , b );
            PlayerColor = ImColor( r , g , b , 1.0f );
        }
        if ( Settings::Visual::PlayerBoxType == CVisual::BOX )
            GetRenderStackSystem()->DrawBox( min , max , PlayerColor );
        else if ( Settings::Visual::PlayerBoxType == CVisual::OUTLINE_BOX )
            GetRenderStackSystem()->DrawOutlineBox( min , max , PlayerColor );
        else if ( Settings::Visual::PlayerBoxType == CVisual::COAL_BOX )
            GetRenderStackSystem()->DrawCoalBox( min , max , PlayerColor );
        else if ( Settings::Visual::PlayerBoxType == CVisual::OUTLINE_COAL_BOX )
            GetRenderStackSystem()->DrawOutlineCoalBox( min , max , PlayerColor );
    }

    if ( Settings::Visual::ShowName )
    {
        const char* name = pCCSPlayerController->m_sSanitizedPlayerName();
        ImColor nameColor = ImColor( 255 , 255 , 255 );
        if ( Settings::Visual::NameColorMode == 1 )
            nameColor = ImColor( Settings::Visual::NameCustomColor[0] , Settings::Visual::NameCustomColor[1] , Settings::Visual::NameCustomColor[2] , Settings::Visual::NameCustomColor[3] );
        else if ( Settings::Visual::NameColorMode == 3 )
        {
            const float t = static_cast<float>( ImGui::GetTime() ) * Settings::Visual::NameRGBSpeed;
            const float h = fmodf( t , 1.0f );
            float r , g , b; ImGui::ColorConvertHSVtoRGB( h , 1.0f , 1.0f , r , g , b );
            nameColor = ImColor( r , g , b , 1.0f );
        }
        const float textY = Settings::Visual::NamePosition == 0 ? ( min.y - 12.f ) : ( max.y + 2.f );
        const ImVec2 textPos = ImVec2( ( min.x + max.x ) * 0.5f , textY );
        if ( Settings::Visual::NameColorMode == 2 )
        {
            const ImU32 top = ImColor( Settings::Visual::NameGradientTop[0] , Settings::Visual::NameGradientTop[1] , Settings::Visual::NameGradientTop[2] , Settings::Visual::NameGradientTop[3] );
            const ImU32 bot = ImColor( Settings::Visual::NameGradientBottom[0] , Settings::Visual::NameGradientBottom[1] , Settings::Visual::NameGradientBottom[2] , Settings::Visual::NameGradientBottom[3] );
            const ImVec2 bgMin = ImVec2( min.x , textY - 10.f );
            const ImVec2 bgMax = ImVec2( max.x , textY + 10.f );
            GetRenderStackSystem()->DrawRectFilledMultiColor( bgMin , bgMax , top , top , bot , bot );
        }
        GetRenderStackSystem()->DrawString( &GetFontManager()->m_VerdanaFont , textPos , FW1_CENTER , nameColor , "%s" , name );
    }

    if ( Settings::Visual::ShowWeapon )
    {
        const char* weaponText = "";
        ImColor weaponColor = ImColor( 255 , 255 , 255 );
        if ( auto* pPawn = pCCSPlayerController->m_hPawn().Get<C_CSPlayerPawn>() )
        {
            if ( auto* pWeapons = pPawn->m_pWeaponServices() )
            {
                if ( auto pWeapon = pWeapons->m_hActiveWeapon().Get<C_CSWeaponBaseGun>() )
                {
                    if ( auto pAttr = pWeapon->m_AttributeManager() )
                    {
                        if ( auto pItem = pAttr->m_Item() )
                        {
                            const int defIndex = pItem->m_iItemDefinitionIndex();
                            if ( Settings::Visual::WeaponLabelMode == 0 )
                                weaponText = GetWeaponNameFromDefinitionIndex( defIndex );
                            else
                            {
                                if ( defIndex >= 500 ) weaponText = GetKnifeIconNameFromDefinitionIndex( defIndex );
                                else
                                {
                                    const char* desc = GetWeaponDescFromDefinitionIndex( defIndex );
                                    if ( desc && strncmp( desc , "weapon_" , 7 ) == 0 ) weaponText = desc + 7; else weaponText = desc;
                                }
                            }
                        }
                    }
                }
            }
        }
        if ( Settings::Visual::WeaponColorMode == 1 )
            weaponColor = ImColor( Settings::Visual::WeaponCustomColor[0] , Settings::Visual::WeaponCustomColor[1] , Settings::Visual::WeaponCustomColor[2] , Settings::Visual::WeaponCustomColor[3] );
        else if ( Settings::Visual::WeaponColorMode == 3 )
        {
            const float t = static_cast<float>( ImGui::GetTime() ) * Settings::Visual::WeaponRGBSpeed;
            const float h = fmodf( t , 1.0f );
            float r , g , b; ImGui::ColorConvertHSVtoRGB( h , 1.0f , 1.0f , r , g , b );
            weaponColor = ImColor( r , g , b , 1.0f );
        }
        const float weapY = Settings::Visual::WeaponPosition == 0 ? ( min.y - 26.f ) : ( max.y + 16.f );
        const ImVec2 weapPos = ImVec2( ( min.x + max.x ) * 0.5f , weapY );
        if ( Settings::Visual::WeaponColorMode == 2 )
        {
            const ImU32 top = ImColor( Settings::Visual::WeaponGradientTop[0] , Settings::Visual::WeaponGradientTop[1] , Settings::Visual::WeaponGradientTop[2] , Settings::Visual::WeaponGradientTop[3] );
            const ImU32 bot = ImColor( Settings::Visual::WeaponGradientBottom[0] , Settings::Visual::WeaponGradientBottom[1] , Settings::Visual::WeaponGradientBottom[2] , Settings::Visual::WeaponGradientBottom[3] );
            const ImVec2 bgMin = ImVec2( min.x , weapY - 10.f );
            const ImVec2 bgMax = ImVec2( max.x , weapY + 10.f );
            GetRenderStackSystem()->DrawRectFilledMultiColor( bgMin , bgMax , top , top , bot , bot );
        }
        if ( weaponText && weaponText[0] )
            GetRenderStackSystem()->DrawString( &GetFontManager()->m_VerdanaFont , weapPos , FW1_CENTER , weaponColor , "%s" , weaponText );
    }

    if ( Settings::Visual::ShowHealthBar )
    {
        const int health = pC_CSPlayerPawn->m_iHealth();
        const int maxHealthRaw = pC_CSPlayerPawn->m_iMaxHealth();
        const int maxHealth = maxHealthRaw > 1 ? maxHealthRaw : 1;
        const float frac = std::clamp( (float)health / (float)maxHealth , 0.0f , 1.0f );
        const float barGap = 2.f;
        const ImVec2 barMin = ImVec2( min.x - ( Settings::Visual::HealthBarWidth + barGap ) , min.y );
        const ImVec2 barMax = ImVec2( min.x - barGap , max.y );
        GetRenderStackSystem()->DrawFillBox( barMin , barMax , ImColor( 0 , 0 , 0 , 150 ) );
        const float fullH = ( max.y - min.y );
        const float fillH = fullH * frac;
        const ImVec2 fillMin = ImVec2( barMin.x , max.y - fillH );
        const ImVec2 fillMax = ImVec2( barMax.x , max.y );
        const ImU32 top = ImColor( Settings::Visual::HealthGradientTop[0] , Settings::Visual::HealthGradientTop[1] , Settings::Visual::HealthGradientTop[2] , Settings::Visual::HealthGradientTop[3] );
        const ImU32 bot = ImColor( Settings::Visual::HealthGradientBottom[0] , Settings::Visual::HealthGradientBottom[1] , Settings::Visual::HealthGradientBottom[2] , Settings::Visual::HealthGradientBottom[3] );
        GetRenderStackSystem()->DrawRectFilledMultiColor( fillMin , fillMax , top , top , bot , bot );
    }

    if ( Settings::Visual::ShowReload )
    {
        bool inReload = false;
        if ( auto* pPawn = pCCSPlayerController->m_hPawn().Get<C_CSPlayerPawn>() )
        {
            if ( auto* pWeapons = pPawn->m_pWeaponServices() )
            {
                if ( auto pActive = pWeapons->m_hActiveWeapon().Get<C_CSWeaponBase>() ) inReload = pActive->m_bInReload();
            }
        }
        if ( inReload )
        {
            const float baseY = Settings::Visual::ShowWeapon ? ( Settings::Visual::WeaponPosition == 0 ? ( min.y - 40.f ) : ( max.y + 30.f ) ) : ( max.y + 2.f );
            const ImVec2 pos = ImVec2( ( min.x + max.x ) * 0.5f , baseY );
            const ImColor col = ImColor( Settings::Visual::ReloadColor[0] , Settings::Visual::ReloadColor[1] , Settings::Visual::ReloadColor[2] , Settings::Visual::ReloadColor[3] );
            GetRenderStackSystem()->DrawString( &GetFontManager()->m_VerdanaFont , pos , FW1_CENTER , col , "%s" , "RELOAD" );
        }
    }

    if ( Settings::Visual::ShowBomb )
    {
        bool hasBomb = false;
        if ( pCCSPlayerController->m_iTeamNum() == TEAM_TT )
        {
            if ( auto* pPawn = pCCSPlayerController->m_hPawn().Get<C_CSPlayerPawn>() )
            {
                if ( auto* pWeapons = pPawn->m_pWeaponServices() )
                {
                    const auto& vec = pWeapons->m_hMyWeapons();
                    const int count = vec.Count();
                    for ( int i = 0; i < count; ++i )
                    {
                        if ( auto pWeap = vec[i].Get<C_BasePlayerWeapon>() )
                        {
                            if ( pWeap->IsC4() ) { hasBomb = true; break; }
                        }
                    }
                }
            }
        }
        if ( hasBomb )
        {
            const float baseY = Settings::Visual::ShowWeapon ? ( Settings::Visual::WeaponPosition == 0 ? ( min.y - 70.f ) : ( max.y + 28.f ) ) : ( max.y + 8.f );
            const ImVec2 pos = ImVec2( ( min.x + max.x ) * 0.5f , baseY );
            const ImColor col = ImColor( Settings::Visual::BombColor[0] , Settings::Visual::BombColor[1] , Settings::Visual::BombColor[2] , Settings::Visual::BombColor[3] );
            GetRenderStackSystem()->DrawString( &GetFontManager()->m_VerdanaFont , pos , FW1_CENTER , col , "%s" , "C4" );
        }
    }
}