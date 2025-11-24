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

auto CVisual::OnCreateMove() -> void
{
    if ( SDK::Interfaces::EngineToClient()->IsInGame() )
        CCameraView::SetThirdPerson( Settings::Visual::ThirdPerson );

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