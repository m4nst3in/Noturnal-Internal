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
#include <AndromedaClient/Features/CVisual/CTimerC4.hpp>
#include <AndromedaClient/Features/CVisual/CGrenadeESP.hpp>
#include <AndromedaClient/Features/CVisual/CWorldESP.hpp>
#include <AndromedaClient/Features/CPlayerESP.hpp>
#include <AndromedaClient/Features/CVisual/CTimerC4.hpp>

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

auto CVisual::OnRender() -> void
{
    if ( !Settings::Visual::Active )
        return;

    const auto& CachedVec = GetEntityCache()->GetCachedEntity();

    std::scoped_lock Lock( GetEntityCache()->GetLock() );

    if ( Settings::Visual::ShowGrenadePrediction )
    {
        if ( GetCL_Weapons()->GetLocalWeaponType() == CSWeaponType_t::WEAPONTYPE_GRENADE )
        {
            auto* lpPawn = GetCL_Players()->GetLocalPlayerPawn();
            if ( lpPawn )
            {
                Vector3 pos = GetCL_Players()->GetLocalEyeOrigin();
                QAngle ang = lpPawn->m_angEyeAngles();
                Vector3 fwd , right , up; Math::AngleVectors( ang , fwd , right , up );
                const float releaseForward = 16.f;
                const float releaseRight = 8.f;
                const float releaseUp = -6.f;
                pos = pos + fwd * releaseForward + right * releaseRight + up * releaseUp;

                int def = GetCL_Weapons()->GetLocalWeaponDefinitionIndex();
                const char* desc = GetWeaponDescFromDefinitionIndex( def );
                float speed = Settings::Visual::GrenadeSimInitialSpeed;
                if ( desc )
                {
                    if ( strstr( desc , "smoke" ) ) speed = 750.f;
                    else if ( strstr( desc , "molotov" ) || strstr( desc , "incendiary" ) ) speed = 750.f;
                    else if ( strstr( desc , "flashbang" ) ) speed = 1000.f;
                    else if ( strstr( desc , "hegrenade" ) || strstr( desc , "he" ) ) speed = 1000.f;
                }
                Vector3 vel = fwd * speed;

                std::vector<ImVec2> pts;
                const int steps = static_cast<int>( Settings::Visual::GrenadeSimMaxTime / Settings::Visual::GrenadeSimStep );
                CTraceFilter filter( 0x1C1003 , lpPawn , 3 , 15 );
                for ( int i = 0; i < steps; ++i )
                {
                    vel.m_z -= Settings::Visual::GrenadeSimGravity * Settings::Visual::GrenadeSimStep;
                    Vector3 next = pos + vel * Settings::Visual::GrenadeSimStep;
                    Ray_t ray{}; CGameTrace tr{};
                    if ( IGamePhysicsQuery_TraceShape( SDK::Pointers::CVPhys2World() , ray , pos , next , &filter , &tr ) )
                    {
                        if ( tr.DidHit() )
                        {
                            pos = tr.vecPosition;
                            const Vector3 n = tr.vecNormal;
                            const float dotv = vel.m_x * n.m_x + vel.m_y * n.m_y + vel.m_z * n.m_z;
                            vel = vel - n * ( ( 1.f + Settings::Visual::GrenadeSimRestitution ) * dotv );
                            vel = vel * Settings::Visual::GrenadeSimRestitution;
                        }
                        else
                        {
                            pos = next;
                        }
                    }
                    else
                    {
                        pos = next;
                    }
                    ImVec2 s;
                    if ( Math::WorldToScreen( pos , s ) )
                        pts.push_back( s );
                    const float speedMag = sqrtf( vel.m_x*vel.m_x + vel.m_y*vel.m_y + vel.m_z*vel.m_z );
                    if ( speedMag < 10.f )
                        break;
                }
                ImColor col = ImColor( Settings::Colors::Visual::GrenadeHE[0] , Settings::Colors::Visual::GrenadeHE[1] , Settings::Colors::Visual::GrenadeHE[2] , Settings::Colors::Visual::GrenadeHE[3] );
                if ( desc )
                {
                    if ( strstr( desc , "smoke" ) ) col = ImColor( Settings::Colors::Visual::GrenadeSmoke[0] , Settings::Colors::Visual::GrenadeSmoke[1] , Settings::Colors::Visual::GrenadeSmoke[2] , Settings::Colors::Visual::GrenadeSmoke[3] );
                    else if ( strstr( desc , "molotov" ) || strstr( desc , "incendiary" ) ) col = ImColor( Settings::Colors::Visual::GrenadeMolotov[0] , Settings::Colors::Visual::GrenadeMolotov[1] , Settings::Colors::Visual::GrenadeMolotov[2] , Settings::Colors::Visual::GrenadeMolotov[3] );
                    else if ( strstr( desc , "flashbang" ) ) col = ImColor( Settings::Colors::Visual::GrenadeFlash[0] , Settings::Colors::Visual::GrenadeFlash[1] , Settings::Colors::Visual::GrenadeFlash[2] , Settings::Colors::Visual::GrenadeFlash[3] );
                    else if ( strstr( desc , "decoy" ) ) col = ImColor( Settings::Colors::Visual::GrenadeDecoy[0] , Settings::Colors::Visual::GrenadeDecoy[1] , Settings::Colors::Visual::GrenadeDecoy[2] , Settings::Colors::Visual::GrenadeDecoy[3] );
                }
                for ( size_t i = 1; i < pts.size(); ++i )
                {
                    GetRenderStackSystem()->DrawLine( pts[i-1] , pts[i] , col , Settings::Visual::GrenadeTrajectoryThickness );
                }
            }
        }
    }

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
