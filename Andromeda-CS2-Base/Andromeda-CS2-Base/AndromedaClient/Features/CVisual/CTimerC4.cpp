#include "CTimerC4.hpp"

#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Update/CGlobalVarsBase.hpp>

#include <AndromedaClient/Render/CRender.hpp>
#include <AndromedaClient/Render/CRenderStackSystem.hpp>
#include <AndromedaClient/Fonts/CFontManager.hpp>
#include <AndromedaClient/Settings/Settings.hpp>
#include <GameClient/CL_Players.hpp>
#include <GameClient/CEntityCache/CEntityCache.hpp>

#include <ImGui/imgui.h>
#include <algorithm>
#include <unordered_map>
#include <cmath>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

struct DefuseInfo { float start; uint32_t defuserId; };
static std::unordered_map<uint32_t,DefuseInfo> s_DefuseInfo;
static std::unordered_map<uint32_t,float> s_BombStart;

static auto TimerColor() -> ImColor
{
    ImColor timerColor = ImColor( 255 , 255 , 255 );
    if ( Settings::Visual::C4TimerColorMode == 1 )
        timerColor = ImColor( Settings::Visual::C4TimerCustomColor[0] , Settings::Visual::C4TimerCustomColor[1] , Settings::Visual::C4TimerCustomColor[2] , Settings::Visual::C4TimerCustomColor[3] );
    else if ( Settings::Visual::C4TimerColorMode == 3 )
    {
        const float t = static_cast<float>( ImGui::GetTime() ) * Settings::Visual::C4TimerRGBSpeed;
        const float h = fmodf( t , 1.0f );
        float r , g , b; ImGui::ColorConvertHSVtoRGB( h , 1.0f , 1.0f , r , g , b );
        timerColor = ImColor( r , g , b , 1.0f );
    }
    return timerColor;
}

auto RenderTimerC4( C_PlantedC4* pPlanted , const Rect_t& bbox , bool bHasBBox ) -> void
{
    if ( !Settings::Visual::ShowC4Timer ) return;
    if ( !pPlanted || !pPlanted->pEntityIdentity() || pPlanted->m_bBombDefused() || pPlanted->m_bHasExploded() ) return;
    auto* gvars = SDK::Pointers::GlobalVarsBase(); if ( !gvars ) return;

    const float now = gvars->m_flCurrentTime();
    float blow = pPlanted->m_flC4Blow().m_Value; if ( !std::isfinite( blow ) ) return;
    float timeToBlow = std::max( 0.f , blow - now );

    bool isDefusing = false; bool hasDefuser = false; CCSPlayerController* pDefuserCtrl = nullptr;
    {
        const auto& CachedVec2 = GetEntityCache()->GetCachedEntity();
        for ( const auto& ce : *CachedVec2 )
        {
            if ( ce.m_Type != CachedEntity_t::PLAYER_CONTROLLER ) continue;
            auto* pCtrl = reinterpret_cast<CCSPlayerController*>( ce.m_Handle.Get() ); if ( !pCtrl ) continue;
            auto* pPawn = pCtrl->m_hPawn().Get<C_CSPlayerPawn>(); if ( !pPawn ) continue;
            if ( pPawn->m_bIsDefusing() )
            {
                isDefusing = true; if ( auto* pItem = pPawn->m_pItemServices() ) hasDefuser = pItem->m_bHasDefuser(); pDefuserCtrl = pCtrl; break;
            }
        }
    }

    const uint32_t bombId = pPlanted->pEntityIdentity()->Handle().GetEntryIndex();
    if ( isDefusing )
    {
        const uint32_t defId = ( pDefuserCtrl && pDefuserCtrl->pEntityIdentity() ) ? pDefuserCtrl->pEntityIdentity()->Handle().GetEntryIndex() : 0u;
        auto it = s_DefuseInfo.find( bombId );
        if ( it == s_DefuseInfo.end() ) s_DefuseInfo[bombId] = { now , defId };
        else if ( it->second.defuserId != defId ) { it->second.start = now; it->second.defuserId = defId; }
    }
    else s_DefuseInfo.erase( bombId );

    float defuseLen = hasDefuser ? 5.0f : 10.0f;
    float defuseRemain = defuseLen;
    if ( isDefusing )
    {
        auto itInfo = s_DefuseInfo.find( bombId );
        if ( itInfo != s_DefuseInfo.end() ) defuseRemain = std::max( 0.f , defuseLen - ( now - itInfo->second.start ) );
    }

    ImVec2 center1 , center2;
    if ( Settings::Visual::C4TimerFixed )
    {
        const ImVec2 screen = ImGui::GetIO().DisplaySize;
        if ( Settings::Visual::C4TimerAnchor == 0 )
        {
            const float cx = screen.x * 0.5f; const float y1 = Settings::Visual::C4TimerOffsetY; const float y2 = y1 + 16.f; center1 = ImVec2( cx , y1 ); center2 = ImVec2( cx , y2 );
        }
        else
        {
            const float w = Settings::Visual::C4TimerBarWidth; const float cx = screen.x - Settings::Visual::C4TimerOffsetX - w * 0.5f; const float y1 = screen.y - Settings::Visual::C4TimerOffsetY; const float y2 = y1 - 16.f; center1 = ImVec2( cx , y1 ); center2 = ImVec2( cx , y2 );
        }
    }
    else
    {
        ImVec2 minB = { bbox.x , bbox.y }; ImVec2 maxB = { bbox.w , bbox.h };
        ImVec2 anchor = ImVec2( 0.f , 0.f ); bool haveAnchor = false; ImVec2 s;
        if ( Math::WorldToScreen( pPlanted->GetOrigin() , s ) ) { anchor = s; haveAnchor = true; }
        const float line1Y = ( haveAnchor ? anchor.y : minB.y ) - 14.f; const float line2Y = line1Y - 14.f; const float centerX = haveAnchor ? anchor.x : ( ( minB.x + maxB.x ) * 0.5f ); center1 = ImVec2( centerX , line1Y ); center2 = ImVec2( centerX , line2Y );
    }

    ImColor timerColor = TimerColor();
    const int sec = static_cast<int>( timeToBlow ); const int msec = static_cast<int>( ( timeToBlow - sec ) * 1000.f );
    GetRenderStackSystem()->DrawString( &GetFontManager()->m_VerdanaFont , center1 , FW1_CENTER , timerColor , "C4: %02d.%03d" , sec , msec );

    if ( !isDefusing )
    {
        const ImColor defCol = ImColor( 255 , 255 , 255 , 200 ); const char* siteTxt = nullptr; if ( Settings::Visual::ShowC4SiteLabel ) siteTxt = pPlanted->m_nBombSite() == 0 ? "SITE A" : "SITE B"; GetRenderStackSystem()->DrawString( &GetFontManager()->m_VerdanaFont , center2 , FW1_CENTER , defCol , siteTxt ? siteTxt : "NOT DEFUSING" );
    }

    if ( Settings::Visual::ShowC4ProgressBar )
    {
        const uint32_t id = pPlanted->pEntityIdentity()->Handle().GetEntryIndex(); if ( s_BombStart.find( id ) == s_BombStart.end() ) s_BombStart[id] = now;
        const float totalLen = std::max( 0.1f , std::max( 0.f , pPlanted->m_flC4Blow().m_Value - s_BombStart[id] ) ); const float frac = std::clamp( timeToBlow / totalLen , 0.f , 1.f );
        const float barH = 4.f; ImVec2 barMin , barMax; const float cx = center1.x; const float w = Settings::Visual::C4TimerBarWidth; barMin = ImVec2( cx - w * 0.5f , center1.y + 6.f ); barMax = ImVec2( cx + w * 0.5f , center1.y + 6.f + barH );
        GetRenderStackSystem()->DrawFillBox( barMin , barMax , ImColor( 0 , 0 , 0 , 150 ) ); const ImVec2 fillMax = ImVec2( barMin.x + ( barMax.x - barMin.x ) * frac , barMax.y ); GetRenderStackSystem()->DrawFillBox( barMin , fillMax , timerColor );
        if ( isDefusing )
        {
            const float dfrac = std::clamp( 1.f - ( defuseRemain / defuseLen ) , 0.f , 1.f ); const ImColor defCol = ImColor( 0 , 150 , 255 , 255 ); const ImVec2 dbarMin = ImVec2( cx - w * 0.5f , center2.y - 6.f ); const ImVec2 dbarMax = ImVec2( cx + w * 0.5f , center2.y - 2.f ); GetRenderStackSystem()->DrawFillBox( dbarMin , dbarMax , ImColor( 0 , 0 , 0 , 150 ) ); const ImVec2 dfillMax = ImVec2( dbarMin.x + ( dbarMax.x - dbarMin.x ) * dfrac , dbarMax.y ); GetRenderStackSystem()->DrawFillBox( dbarMin , dfillMax , defCol );
        }
    }

    if ( Settings::Visual::ShowC4Damage )
    {
        auto* lpCtrl = GetCL_Players()->GetLocalPlayerController(); if ( lpCtrl )
        {
            auto* lpPawn = lpCtrl->m_hPawn().Get<C_CSPlayerPawn>(); if ( lpPawn )
            {
                const Vector3 bombOrigin = pPlanted->GetOrigin(); const Vector3 localOrigin = lpPawn->GetOrigin(); const float dx = localOrigin.m_x - bombOrigin.m_x; const float dy = localOrigin.m_y - bombOrigin.m_y; const float dist = sqrtf( dx*dx + dy*dy ); float lethalR = Settings::Visual::C4LethalRadius; float maxR = Settings::Visual::C4DamageRadius;
                ImColor dCol = ImColor( Settings::Visual::C4DamageSafeColor[0] , Settings::Visual::C4DamageSafeColor[1] , Settings::Visual::C4DamageSafeColor[2] , Settings::Visual::C4DamageSafeColor[3] ); const ImVec2 center3 = ImVec2( center2.x , center2.y - 14.f );
                if ( dist <= lethalR ) { dCol = ImColor( Settings::Visual::C4DamageLethalColor[0] , Settings::Visual::C4DamageLethalColor[1] , Settings::Visual::C4DamageLethalColor[2] , Settings::Visual::C4DamageLethalColor[3] ); GetRenderStackSystem()->DrawString( &GetFontManager()->m_VerdanaFont , center3 , FW1_CENTER , dCol , "LETHAL" ); }
                else if ( dist <= maxR ) { dCol = ImColor( Settings::Visual::C4DamageWarnColor[0] , Settings::Visual::C4DamageWarnColor[1] , Settings::Visual::C4DamageWarnColor[2] , Settings::Visual::C4DamageWarnColor[3] ); const float k = ( dist - lethalR ) / ( maxR - lethalR ); const float dmg = 80.f * ( 1.f - k ) + 10.f; GetRenderStackSystem()->DrawString( &GetFontManager()->m_VerdanaFont , center3 , FW1_CENTER , dCol , "DMG: %.0f" , dmg ); }
                else { GetRenderStackSystem()->DrawString( &GetFontManager()->m_VerdanaFont , center3 , FW1_CENTER , dCol , "SAFE" ); }
                if ( Settings::Visual::ShowC4Radius ) { GetRender()->DrawCircle3D( bombOrigin , lethalR , ImColor( Settings::Visual::C4DamageLethalColor[0] , Settings::Visual::C4DamageLethalColor[1] , Settings::Visual::C4DamageLethalColor[2] , Settings::Visual::C4DamageLethalColor[3] ) ); GetRender()->DrawCircle3D( bombOrigin , maxR , ImColor( Settings::Visual::C4DamageWarnColor[0] , Settings::Visual::C4DamageWarnColor[1] , Settings::Visual::C4DamageWarnColor[2] , Settings::Visual::C4DamageWarnColor[3] ) ); }
            }
        }
    }

    if ( isDefusing && Settings::Visual::ShowDefuserIcon )
    {
        auto* pPawn = pDefuserCtrl ? pDefuserCtrl->m_hPawn().Get<C_CSPlayerPawn>() : nullptr; ImVec2 iconPos; bool havePos = false;
        if ( pPawn )
        {
            Rect_t rb{}; if ( pPawn->GetBoundingBox( rb ) ) { const ImVec2 pmin = { rb.x , rb.y }; const ImVec2 pmax = { rb.w , rb.h }; iconPos = ImVec2( ( pmin.x + pmax.x ) * 0.5f , pmin.y - 10.f ); havePos = true; }
            else { ImVec2 ps; if ( Math::WorldToScreen( pPawn->GetOrigin() , ps ) ) { iconPos = ImVec2( ps.x , ps.y - 20.f ); havePos = true; } }
        }
        if ( havePos )
        {
            const ImColor defCol = ImColor( 0 , 150 , 255 , 255 ); GetRender()->DrawCircleFilled( iconPos , 6.f , defCol ); GetRender()->DrawCircle( iconPos , 7.f , ImColor( 255 , 255 , 255 , 200 ) ); if ( Settings::Visual::ShowDefuserProgressRing ) { const float dfrac = std::clamp( 1.f - ( defuseRemain / defuseLen ) , 0.f , 1.f ); const float radius = 9.f; const float start = -M_PI_F * 0.5f; const float end = start + dfrac * ( M_PI_F * 2.f ); const float step = M_PI_F * 2.f / 48.f; ImVec2 prev = iconPos; for ( float a = start; a <= end; a += step ) { const ImVec2 pt = ImVec2( iconPos.x + cosf( a ) * radius , iconPos.y + sinf( a ) * radius ); GetRender()->DrawLine( prev , pt , defCol , 2.f ); prev = pt; } }
        }
    }
}
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif