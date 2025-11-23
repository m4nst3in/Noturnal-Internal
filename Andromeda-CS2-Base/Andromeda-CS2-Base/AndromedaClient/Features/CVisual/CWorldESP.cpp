#include "CWorldESP.hpp"

#include <AndromedaClient/Render/CRenderStackSystem.hpp>
#include <AndromedaClient/Fonts/CFontManager.hpp>
#include <AndromedaClient/Settings/Settings.hpp>
#include <GameClient/CL_ItemDefinition.hpp>

#include <ImGui/imgui.h>

auto RenderWorldWeapon( C_BasePlayerWeapon* pBaseWeapon , const Rect_t& bbox , bool bDraw ) -> void
{
    if ( !Settings::Visual::ShowWorldWeapons ) return;
    if ( !pBaseWeapon ) return;
    if ( pBaseWeapon->m_hOwnerEntity().Get() ) return;
    if ( !bDraw ) return;

    const ImVec2 minW = { bbox.x , bbox.y };
    const ImVec2 maxW = { bbox.w , bbox.h };
    const float textY = minW.y - 12.f;
    const ImVec2 textPos = ImVec2( ( minW.x + maxW.x ) * 0.5f , textY );

    const bool isBombDropped = reinterpret_cast<C_BaseEntity*>( pBaseWeapon )->IsC4();
    const char* weaponText = "";
    if ( isBombDropped ) weaponText = "C4";
    else if ( auto* pCSWeapon = reinterpret_cast<C_CSWeaponBase*>( pBaseWeapon ) )
    {
        if ( auto* pAttr = pCSWeapon->m_AttributeManager() )
        {
            if ( auto* pItem = pAttr->m_Item() )
            {
                const int defIndex = pItem->m_iItemDefinitionIndex();
                if ( Settings::Visual::WeaponLabelMode == 0 ) weaponText = GetWeaponNameFromDefinitionIndex( defIndex );
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

    ImColor weaponColor = ImColor( 255 , 255 , 255 );
    if ( isBombDropped ) weaponColor = ImColor( Settings::Visual::BombColor[0] , Settings::Visual::BombColor[1] , Settings::Visual::BombColor[2] , Settings::Visual::BombColor[3] );
    else if ( Settings::Visual::WorldWeaponColorMode == 1 ) weaponColor = ImColor( Settings::Visual::WorldWeaponCustomColor[0] , Settings::Visual::WorldWeaponCustomColor[1] , Settings::Visual::WorldWeaponCustomColor[2] , Settings::Visual::WorldWeaponCustomColor[3] );
    else if ( Settings::Visual::WorldWeaponColorMode == 3 )
    {
        const float t = static_cast<float>( ImGui::GetTime() ) * Settings::Visual::WorldWeaponRGBSpeed;
        const float h = fmodf( t , 1.0f ); float r , g , b; ImGui::ColorConvertHSVtoRGB( h , 1.0f , 1.0f , r , g , b ); weaponColor = ImColor( r , g , b , 1.0f );
    }
    if ( !isBombDropped && Settings::Visual::WorldWeaponColorMode == 2 )
    {
        const ImU32 top = ImColor( Settings::Visual::WorldWeaponGradientTop[0] , Settings::Visual::WorldWeaponGradientTop[1] , Settings::Visual::WorldWeaponGradientTop[2] , Settings::Visual::WorldWeaponGradientTop[3] );
        const ImU32 bot = ImColor( Settings::Visual::WorldWeaponGradientBottom[0] , Settings::Visual::WorldWeaponGradientBottom[1] , Settings::Visual::WorldWeaponGradientBottom[2] , Settings::Visual::WorldWeaponGradientBottom[3] );
        const ImVec2 bgMin = ImVec2( minW.x , textY - 10.f ); const ImVec2 bgMax = ImVec2( maxW.x , textY + 10.f );
        GetRenderStackSystem()->DrawRectFilledMultiColor( bgMin , bgMax , top , top , bot , bot );
    }
    if ( weaponText && weaponText[0] ) GetRenderStackSystem()->DrawString( &GetFontManager()->m_VerdanaFont , textPos , FW1_CENTER , weaponColor , "%s" , weaponText );
}