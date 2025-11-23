#include "CGrenadeESP.hpp"

#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Update/CGlobalVarsBase.hpp>
#include <CS2/SDK/Update/GameTrace.hpp>
#include <CS2/SDK/FunctionListSDK.hpp>

#include <AndromedaClient/Render/CRender.hpp>
#include <AndromedaClient/Render/CRenderStackSystem.hpp>
#include <AndromedaClient/Fonts/CFontManager.hpp>
#include <AndromedaClient/Settings/Settings.hpp>
#include <GameClient/CL_ItemDefinition.hpp>
#include <GameClient/CL_Players.hpp>
#include <GameClient/CL_Weapons.hpp>


#include <ImGui/imgui.h>
#include <algorithm>
#include <unordered_map>
#include <deque>
#include "CTrajectoryPhysics.hpp"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

struct GrenadeTrackInfo
{
    std::deque<ImVec2> trail;
    std::deque<Vector3> trailWorld;
    float effectStart = 0.f;
    bool smokeActive = false;
    bool mollyActive = false;
    bool smokeStarted = false;
    bool stopped = false;
    float stoppedTime = 0.f;
};

static std::unordered_map<uint32_t,GrenadeTrackInfo> g_GrenadeTracks;

static auto GetGrenadeColor( bool isHE , bool isFlash , bool isSmoke , bool isMolotov , bool isDecoy , C_BaseEntity* pProj ) -> ImColor
{
    bool useTeamTint = Settings::Visual::GrenadeTeamTint;
    int localTeam = TEAM_UNKNOWN;
    if ( auto* lp = GetCL_Players()->GetLocalPlayerController() ) localTeam = lp->m_iTeamNum();
    int ownerTeam = TEAM_UNKNOWN;
    if ( auto* owner = pProj->m_hOwnerEntity().Get<C_BaseEntity>() ) ownerTeam = owner->m_iTeamNum();
    bool isAlly = useTeamTint && localTeam != TEAM_UNKNOWN && ownerTeam == localTeam;

    if ( isHE ) return ImColor( ( isAlly ? Settings::Colors::Visual::GrenadeHE_Ally : Settings::Colors::Visual::GrenadeHE_Enemy )[0] , ( isAlly ? Settings::Colors::Visual::GrenadeHE_Ally : Settings::Colors::Visual::GrenadeHE_Enemy )[1] , ( isAlly ? Settings::Colors::Visual::GrenadeHE_Ally : Settings::Colors::Visual::GrenadeHE_Enemy )[2] , ( isAlly ? Settings::Colors::Visual::GrenadeHE_Ally : Settings::Colors::Visual::GrenadeHE_Enemy )[3] );
    if ( isFlash ) return ImColor( ( isAlly ? Settings::Colors::Visual::GrenadeFlash_Ally : Settings::Colors::Visual::GrenadeFlash_Enemy )[0] , ( isAlly ? Settings::Colors::Visual::GrenadeFlash_Ally : Settings::Colors::Visual::GrenadeFlash_Enemy )[1] , ( isAlly ? Settings::Colors::Visual::GrenadeFlash_Ally : Settings::Colors::Visual::GrenadeFlash_Enemy )[2] , ( isAlly ? Settings::Colors::Visual::GrenadeFlash_Ally : Settings::Colors::Visual::GrenadeFlash_Enemy )[3] );
    if ( isSmoke ) return ImColor( ( isAlly ? Settings::Colors::Visual::GrenadeSmoke_Ally : Settings::Colors::Visual::GrenadeSmoke_Enemy )[0] , ( isAlly ? Settings::Colors::Visual::GrenadeSmoke_Ally : Settings::Colors::Visual::GrenadeSmoke_Enemy )[1] , ( isAlly ? Settings::Colors::Visual::GrenadeSmoke_Ally : Settings::Colors::Visual::GrenadeSmoke_Enemy )[2] , ( isAlly ? Settings::Colors::Visual::GrenadeSmoke_Ally : Settings::Colors::Visual::GrenadeSmoke_Enemy )[3] );
    if ( isMolotov ) return ImColor( ( isAlly ? Settings::Colors::Visual::GrenadeMolotov_Ally : Settings::Colors::Visual::GrenadeMolotov_Enemy )[0] , ( isAlly ? Settings::Colors::Visual::GrenadeMolotov_Ally : Settings::Colors::Visual::GrenadeMolotov_Enemy )[1] , ( isAlly ? Settings::Colors::Visual::GrenadeMolotov_Ally : Settings::Colors::Visual::GrenadeMolotov_Enemy )[2] , ( isAlly ? Settings::Colors::Visual::GrenadeMolotov_Ally : Settings::Colors::Visual::GrenadeMolotov_Enemy )[3] );
    if ( isDecoy ) return ImColor( ( isAlly ? Settings::Colors::Visual::GrenadeDecoy_Ally : Settings::Colors::Visual::GrenadeDecoy_Enemy )[0] , ( isAlly ? Settings::Colors::Visual::GrenadeDecoy_Ally : Settings::Colors::Visual::GrenadeDecoy_Enemy )[1] , ( isAlly ? Settings::Colors::Visual::GrenadeDecoy_Ally : Settings::Colors::Visual::GrenadeDecoy_Enemy )[2] , ( isAlly ? Settings::Colors::Visual::GrenadeDecoy_Ally : Settings::Colors::Visual::GrenadeDecoy_Enemy )[3] );
    return ImColor( 255 , 255 , 255 );
}

auto RenderGrenadeESP( C_BaseEntity* pProj ) -> void
{
    if ( !Settings::Visual::ShowGrenades ) return;

    Vector3 origin = pProj->GetOrigin();
    ImVec2 screen;
    const bool onScreen = Math::WorldToScreen( origin , screen );

    const uint32_t id = pProj->pEntityIdentity() ? pProj->pEntityIdentity()->Handle().GetEntryIndex() : 0u;
    auto& track = g_GrenadeTracks[id];

    auto* bind = pProj->GetSchemaClassBinding();
    const char* name = bind ? bind->m_bindingName() : nullptr;
    bool isSmoke = name && strcmp( name , XorStr( "C_SmokeGrenadeProjectile" ) ) == 0;
    bool isHE = name && strcmp( name , XorStr( "C_HEGrenadeProjectile" ) ) == 0;
    bool isFlash = name && strcmp( name , XorStr( "C_FlashbangProjectile" ) ) == 0;
    bool isDecoy = name && strcmp( name , XorStr( "C_DecoyProjectile" ) ) == 0;
    bool isMolotov = name && strcmp( name , XorStr( "C_MolotovProjectile" ) ) == 0;

    ImColor col = GetGrenadeColor( isHE , isFlash , isSmoke , isMolotov , isDecoy , pProj );

    if ( onScreen && Settings::Visual::ShowGrenadeTrajectory )
    {
        track.trail.push_back( screen );
        track.trailWorld.push_back( origin );
        if ( static_cast<int>( track.trail.size() ) > Settings::Visual::GrenadeTrajectoryMaxPoints )
            track.trail.pop_front();
        if ( static_cast<int>( track.trailWorld.size() ) > Settings::Visual::GrenadeTrajectoryMaxPoints )
            track.trailWorld.pop_front();

        for ( size_t i = 1; i < track.trail.size(); ++i )
            GetRenderStackSystem()->DrawLine( track.trail[i-1] , track.trail[i] , col , Settings::Visual::GrenadeTrajectoryThickness );
    }

    if ( Settings::Visual::ShowGrenadeRadius )
    {
        float rad = 0.f;
        if ( isHE ) rad = Settings::Visual::GrenadeHERadius;
        else if ( isFlash ) rad = Settings::Visual::GrenadeFlashRadius;
        else if ( isSmoke ) rad = Settings::Visual::GrenadeSmokeRadius;
        else if ( isMolotov ) rad = Settings::Visual::GrenadeMolotovRadius;
        else if ( isDecoy ) rad = Settings::Visual::GrenadeSmokeRadius * 0.5f;
        if ( rad > 0.f ) GetRender()->DrawCircle3D( origin , rad , col );
    }

    if ( Settings::Visual::ShowGrenadeLandingMarker && track.trailWorld.size() >= static_cast<size_t>( Settings::Visual::GrenadeLandingStableFrames ) )
    {
        float accum = 0.f;
        for ( size_t i = track.trailWorld.size() - Settings::Visual::GrenadeLandingStableFrames + 1; i < track.trailWorld.size(); ++i )
        {
            const Vector3 a = track.trailWorld[i-1];
            const Vector3 b = track.trailWorld[i];
            const float dx = b.m_x - a.m_x;
            const float dy = b.m_y - a.m_y;
            accum += sqrtf( dx*dx + dy*dy );
        }
        if ( accum <= Settings::Visual::GrenadeLandingStableDist )
        {
            ImVec2 c;
            if ( Math::WorldToScreen( track.trailWorld.back() , c ) )
            {
                const float s = Settings::Visual::GrenadeMarkerSize;
                GetRenderStackSystem()->DrawLine( ImVec2( c.x - s , c.y - s ) , ImVec2( c.x + s , c.y + s ) , col , 2.f );
                GetRenderStackSystem()->DrawLine( ImVec2( c.x - s , c.y + s ) , ImVec2( c.x + s , c.y - s ) , col , 2.f );
            }
            if ( auto* gvars = SDK::Pointers::GlobalVarsBase() )
            {
                track.stopped = true;
                track.stoppedTime = gvars->m_flCurrentTime();
            }
        }
    }

    if ( track.stopped )
    {
        if ( auto* gvars = SDK::Pointers::GlobalVarsBase() )
        {
            float now = gvars->m_flCurrentTime();
            if ( ( now - track.stoppedTime ) > Settings::Visual::GrenadeTrailFadeTime )
                g_GrenadeTracks.erase( id );
        }
    }

    if ( Settings::Visual::ShowGrenadeTimers )
    {
        if ( auto* gvars = SDK::Pointers::GlobalVarsBase() )
        {
            float now = gvars->m_flCurrentTime();
            if ( isSmoke )
            {
                if ( pProj->IsSmokeGrenadeProjectile() )
                {
                    auto* sp = reinterpret_cast<C_SmokeGrenadeProjectile*>( pProj );
                    bool did = sp->m_bDidSmokeEffect();
                    if ( did && !track.smokeStarted )
                    {
                        track.smokeStarted = true;
                        track.smokeActive = true;
                        track.effectStart = now;
                    }
                }
                float remain = std::max( 0.f , Settings::Visual::GrenadeSmokeDuration - ( now - track.effectStart ) );
                if ( onScreen )
                    GetRenderStackSystem()->DrawString( &GetFontManager()->m_VerdanaFont , ImVec2( screen.x , screen.y - 20.f ) , FW1_CENTER , col , "%s %.1fs" , "SMOKE" , remain );
            }
            else if ( isMolotov )
            {
                if ( !track.mollyActive )
                {
                    if ( track.trailWorld.size() >= static_cast<size_t>( Settings::Visual::GrenadeLandingStableFrames ) )
                    {
                        float accum = 0.f;
                        for ( size_t i = track.trailWorld.size() - Settings::Visual::GrenadeLandingStableFrames + 1; i < track.trailWorld.size(); ++i )
                        {
                            const Vector3 a = track.trailWorld[i-1];
                            const Vector3 b = track.trailWorld[i];
                            const float dx = b.m_x - a.m_x;
                            const float dy = b.m_y - a.m_y;
                            accum += sqrtf( dx*dx + dy*dy );
                        }
                        if ( accum <= Settings::Visual::GrenadeLandingStableDist )
                        {
                            track.mollyActive = true;
                            track.effectStart = now;
                        }
                    }
                }
                float remain = std::max( 0.f , Settings::Visual::GrenadeMolotovDuration - ( now - track.effectStart ) );
                if ( track.mollyActive && onScreen )
                    GetRenderStackSystem()->DrawString( &GetFontManager()->m_VerdanaFont , ImVec2( screen.x , screen.y - 20.f ) , FW1_CENTER , col , "%s %.1fs" , "MOLLY" , remain );
            }
        }
    }
}

auto RenderLocalGrenadePrediction() -> void
{
    if (!Settings::Visual::ShowGrenadePrediction) return;

    auto* lpPawn = GetCL_Players()->GetLocalPlayerPawn();
    if (!lpPawn) return;

    auto* pWeapon = GetCL_Weapons()->GetLocalWeapon(); // Sua função que retorna C_CSWeaponBase*
    if (!pWeapon) return;

    // Detectar Input (Isso depende de como você acessa o input no seu cheat)
    // Se você estiver no Paint/Render Loop, você pode usar GetAsyncKeystate ou ler de uma classe de Input
    // Exemplo genérico:
    bool bLeft = (GetAsyncKeyState(VK_LBUTTON) & 0x8000);
    bool bRight = (GetAsyncKeyState(VK_RBUTTON) & 0x8000);

    // Obter trajetória calculada
    auto points = CTrajectoryPhysics::Simulate(lpPawn, pWeapon, bLeft, bRight);

    if (points.size() < 2) return;

    // Desenhar
    ImColor colLine = ImColor(255, 255, 255, 255);
    ImColor colEnd = ImColor(255, 50, 50, 255);

    for (size_t i = 1; i < points.size(); ++i)
    {
        GetRenderStackSystem()->DrawLine(
            points[i-1].m_ScreenPos, 
            points[i].m_ScreenPos, 
            colLine, 
            Settings::Visual::GrenadeTrajectoryThickness
        );
    }

    // Desenhar marcador onde vai cair/parar
    if (!points.empty())
    {
        GetRenderStackSystem()->DrawCircle(points.back().m_ScreenPos, 5.0f, colEnd);
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