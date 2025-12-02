#include "CVisual.hpp"

#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Update/CGlobalVarsBase.hpp>
#include <CS2/SDK/Interface/IEngineToClient.hpp>

#include <AndromedaClient/Render/CRender.hpp>
#include <AndromedaClient/Render/CRenderStackSystem.hpp>
#include <AndromedaClient/Fonts/CFontManager.hpp>
#include <AndromedaClient/Settings/Settings.hpp>
#include <GameClient/CL_ItemDefinition.hpp>
#include <GameClient/CL_Weapons.hpp>
#include <ImGui/imgui.h>
#include <algorithm>
#include <unordered_map>
#include <deque>
#include <CS2/SDK/Update/GameTrace.hpp>
#include <CS2/SDK/FunctionListSDK.hpp>
#include <AndromedaClient/Features/CVisual/CCameraView.hpp>

// Headers das Features
#include <AndromedaClient/Features/CVisual/CTimerC4.hpp>
#include <AndromedaClient/Features/CVisual/CGrenadeESP.hpp> // <--- IMPORTANTE: Contém RenderLocalGrenadePrediction
#include <AndromedaClient/Features/CVisual/CWorldESP.hpp>
#include <AndromedaClient/Features/CPlayerESP.hpp>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#include <GameClient/CEntityCache/CEntityCache.hpp>
#include <GameClient/CL_Players.hpp>
#include <GameClient/CL_VisibleCheck.hpp>

static CVisual g_CVisual{};

// (Structs antigos removidos pois não são mais usados aqui)

auto CVisual::OnRender() -> void
{
    if ( !Settings::Visual::Active )
        return;

    const auto& CachedVec = GetEntityCache()->GetCachedEntity();

    std::scoped_lock Lock( GetEntityCache()->GetLock() );

    // ====================================================================
    // CORREÇÃO AQUI:
    // Em vez de calcular a física aqui (código antigo), chamamos o novo sistema.
    // Isso vai invocar o CTrajectoryPhysics que tem o Debug e a correção de vetores.
    // ====================================================================
    if ( Settings::Visual::ShowGrenadePrediction )
    {
        RenderLocalGrenadePrediction(); 
    }
    // ====================================================================

    // Loop de Entidades (Jogadores, Armas, C4, Granadas voando)
    for ( const auto& CachedEntity : *CachedVec )
    {
        auto pEntity = CachedEntity.m_Handle.Get();

        if ( !pEntity )
            continue;

        auto hEntity = pEntity->pEntityIdentity()->Handle();

        if ( hEntity != CachedEntity.m_Handle )
            continue;

        switch ( CachedEntity.m_Type )
        {
            case CachedEntity_t::PLAYER_CONTROLLER:
            {
                auto* pCCSPlayerController = reinterpret_cast<CCSPlayerController*>( pEntity );

                if ( CachedEntity.m_bDraw )
                    OnRenderPlayerEsp( pCCSPlayerController , CachedEntity.m_Bbox , CachedEntity.m_bVisible );
            }
            break;
            case CachedEntity_t::BASE_WEAPON:
            {
                RenderWorldWeapon( reinterpret_cast<C_BasePlayerWeapon*>( pEntity ) , CachedEntity.m_Bbox , CachedEntity.m_bDraw );
            }
            break;
            case CachedEntity_t::PLANTED_C4:
            {
                RenderTimerC4( reinterpret_cast<C_PlantedC4*>( pEntity ) , CachedEntity.m_Bbox , CachedEntity.m_bDraw );
            }
            break;
            case CachedEntity_t::GRENADE_PROJECTILE:
            {
                // Isso desenha granadas que JÁ foram jogadas por outros (ESP)
                RenderGrenadeESP( reinterpret_cast<C_BaseEntity*>( pEntity ) );
            }
            break;
        }
    }
}

auto CVisual::OnRenderPlayerEsp( CCSPlayerController* pCCSPlayerController , const Rect_t& bBox , const bool bVisible ) -> void
{
    RenderPlayerESP( pCCSPlayerController , bBox , bVisible );
}

auto CVisual::OnClientOutput() -> void
{
    OnRender();
}

auto CVisual::OnCreateMove(CCSGOInput* pInput, CUserCmd* pUserCmd) -> void
{
    if ( SDK::Interfaces::EngineToClient()->IsInGame() )
        CCameraView::SetThirdPerson( pInput, Settings::Visual::ThirdPerson );

    if ( !Settings::Visual::Active )
        return;

    const auto CachedVec = GetEntityCache()->GetCachedEntity();

    for ( auto& CachedEntity : *CachedVec )
    {
        auto pEntity = CachedEntity.m_Handle.Get();

        if ( !pEntity )
            continue;

        auto hEntity = pEntity->pEntityIdentity()->Handle();

        if ( hEntity != CachedEntity.m_Handle )
            continue;

        switch ( CachedEntity.m_Type )
        {
            case CachedEntity_t::PLAYER_CONTROLLER:
            {
                auto* pCCSPlayerController = reinterpret_cast<CCSPlayerController*>( pEntity );

                CachedEntity.m_bVisible = GetCL_VisibleCheck()->IsPlayerControllerVisible( pCCSPlayerController );

                if ( Settings::Visual::ChamsEnabled && Settings::Visual::ChamsGlow )
                {
                    auto* pPawn = pCCSPlayerController->m_hPawn().Get<C_CSPlayerPawn>();
                    if ( pPawn && pCCSPlayerController->m_bPawnIsAlive() )
                    {
                        auto visCol = Settings::Visual::ChamsVisibleColor;
                        auto invisCol = Settings::Visual::ChamsInvisibleColor;
                        Color cVis( (int)(visCol[0]*255.f) , (int)(visCol[1]*255.f) , (int)(visCol[2]*255.f) , (int)(visCol[3]*255.f) );
                        Color cInvis( (int)(invisCol[0]*255.f) , (int)(invisCol[1]*255.f) , (int)(invisCol[2]*255.f) , (int)(invisCol[3]*255.f) );

                        pPawn->m_Glow().m_bGlowing() = true;
                        pPawn->m_Glow().m_glowColorOverride() = CachedEntity.m_bVisible ? cVis : cInvis;
                    }
                }
                else
                {
                    auto* pPawn = pCCSPlayerController->m_hPawn().Get<C_CSPlayerPawn>();
                    if ( pPawn )
                        pPawn->m_Glow().m_bGlowing() = false;
                }

                if ( Settings::Visual::ChamsEnabled && Settings::Visual::ChamsOverlay && CachedEntity.m_bDraw )
                {
                    bool isEnemy = false;
                    if ( auto* pLocal = GetCL_Players()->GetLocalPlayerController(); pLocal )
                        isEnemy = pCCSPlayerController->m_iTeamNum() != pLocal->m_iTeamNum();
                    bool draw = ( Settings::Visual::Team && !isEnemy ) || ( Settings::Visual::Enemy && isEnemy );
                    if ( draw )
                    {
                        auto visCol = Settings::Visual::ChamsVisibleColor;
                        auto invisCol = Settings::Visual::ChamsInvisibleColor;
                        ImColor overlay = CachedEntity.m_bVisible ? ImColor( visCol[0] , visCol[1] , visCol[2] , visCol[3] )
                                                              : ImColor( invisCol[0] , invisCol[1] , invisCol[2] , invisCol[3] );

                        auto* pPawn = pCCSPlayerController->m_hPawn().Get<C_CSPlayerPawn>();
                        if ( pPawn )
                        {
                            std::vector<ImVec2> pts;
                            const char* names[] = { "head", "neck_0", "spine_0", "spine_1", "spine_2", "pelvis", "arm_upper_l", "arm_lower_l", "hand_l", "arm_upper_r", "arm_lower_r", "hand_r", "leg_upper_l", "leg_lower_l", "foot_l", "leg_upper_r", "leg_lower_r", "foot_r" };
                            for ( auto& n : names )
                            {
                                int id = pPawn->GetBoneIdByName( n );
                                if ( id != -1 )
                                {
                                    Vector3 p; if ( pPawn->m_pGameSceneNode()->GetBonePosition( id , p ) )
                                    {
                                        ImVec2 s; if ( Math::WorldToScreen( p , s ) ) pts.push_back( s );
                                    }
                                }
                            }
                            if ( pts.size() >= 3 )
                            {
                                std::sort( pts.begin() , pts.end() , []( const ImVec2& a , const ImVec2& b ){ if ( a.x == b.x ) return a.y < b.y; return a.x < b.x; } );
                                pts.erase( std::unique( pts.begin() , pts.end() , []( const ImVec2& a , const ImVec2& b ){ return a.x == b.x && a.y == b.y; } ) , pts.end() );
                                if ( pts.size() >= 3 )
                                {
                                    std::vector<ImVec2> H; H.reserve( pts.size() * 2 );
                                    auto cross = []( const ImVec2& O , const ImVec2& A , const ImVec2& B )
                                    { return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x); };
                                    for ( const auto& p : pts )
                                    {
                                        while ( H.size() >= 2 && cross( H[ H.size()-2 ] , H[ H.size()-1 ] , p ) <= 0 ) H.pop_back();
                                        H.push_back( p );
                                    }
                                    size_t t = H.size() + 1;
                                    for ( auto it = pts.rbegin(); it != pts.rend(); ++it )
                                    {
                                        while ( H.size() >= t && cross( H[ H.size()-2 ] , H[ H.size()-1 ] , *it ) <= 0 ) H.pop_back();
                                        H.push_back( *it );
                                    }
                                    if ( !H.empty() ) H.pop_back();
                                    if ( H.size() >= 3 )
                                    {
                                        ImVec2 c( 0 , 0 ); for ( auto& p : H ) { c.x += p.x; c.y += p.y; } c.x /= H.size(); c.y /= H.size();
                                        for ( size_t i = 0; i < H.size(); ++i )
                                        {
                                            const ImVec2& a = H[i];
                                            const ImVec2& b = H[ ( i + 1 ) % H.size() ];
                                            GetRenderStackSystem()->DrawTriangleFilled( c , a , b , overlay );
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                if ( Settings::Visual::ViewmodelChamsEnabled )
                {
                    auto* pPawn = pCCSPlayerController->m_hPawn().Get<C_CSPlayerPawn>();
                    if ( pPawn )
                    {
                        auto vcol = Settings::Visual::ViewmodelChamsColor;
                        Color cVM( (int)(vcol[0]*255.f) , (int)(vcol[1]*255.f) , (int)(vcol[2]*255.f) , (int)(vcol[3]*255.f) );
                        for ( auto* vm : pPawn->GetViewModels() )
                        {
                            if ( vm )
                            {
                                vm->m_Glow().m_bGlowing() = true;
                                vm->m_Glow().m_glowColorOverride() = cVM;
                            }
                        }
                    }
                }
            }
            break;
            default:
                break;
        }
    }
}

auto CVisual::CalculateBoundingBoxes() -> void
{
    if ( !SDK::Interfaces::EngineToClient()->IsInGame() )
        return;

    const auto& CachedVec = GetEntityCache()->GetCachedEntity();

    std::scoped_lock Lock( GetEntityCache()->GetLock() );

    for ( auto& it : *CachedVec )
    {
        auto pEntity = it.m_Handle.Get();

        if ( !pEntity )
            continue;

        auto hEntity = pEntity->pEntityIdentity()->Handle();

        if ( hEntity != it.m_Handle )
            continue;

        switch ( it.m_Type )
        {
            case CachedEntity_t::PLAYER_CONTROLLER:
            {
                auto pPlayerController = reinterpret_cast<CCSPlayerController*>( pEntity );
                auto pPlayerPawn = pPlayerController->m_hPawn().Get<C_CSPlayerPawn>();

                if ( pPlayerPawn && pPlayerPawn->IsPlayerPawn() && pPlayerController->m_bPawnIsAlive() )
                    it.m_bDraw = pPlayerPawn->GetBoundingBox( it.m_Bbox );
            }
            break;
            case CachedEntity_t::BASE_WEAPON:
            {
                auto pBaseWeapon = reinterpret_cast<C_BasePlayerWeapon*>( pEntity );
                if ( pBaseWeapon && !pBaseWeapon->m_hOwnerEntity().Get() )
                    it.m_bDraw = pBaseWeapon->GetBoundingBox( it.m_Bbox );
            }
            break;
            case CachedEntity_t::PLANTED_C4:
            {
                auto pPlanted = reinterpret_cast<C_PlantedC4*>( pEntity );
                if ( pPlanted )
                    it.m_bDraw = pPlanted->GetBoundingBox( it.m_Bbox );
            }
            break;
            case CachedEntity_t::GRENADE_PROJECTILE:
            {
                auto pGren = reinterpret_cast<C_BaseEntity*>( pEntity );
                if ( pGren )
                    it.m_bDraw = pGren->GetBoundingBox( it.m_Bbox );
            }
            break;
        }
    }
}

auto GetVisual() -> CVisual*
{
    return &g_CVisual;
}