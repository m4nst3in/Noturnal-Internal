#include "Hook_GetMatricesForView.hpp"

#include <CS2/SDK/Math/Math.hpp>
#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Interface/IEngineToClient.hpp>
#include <AndromedaClient/Settings/Settings.hpp>
#include <GameClient/CL_Players.hpp>
#include <CS2/SDK/Update/Offsets.hpp>
#include <CS2/SDK/Types/CEntityData.hpp>
#include <CS2/SDK/Update/GameTrace.hpp>
#include <CS2/SDK/FunctionListSDK.hpp>

#include <AndromedaClient/Features/CVisual/CVisual.hpp>

static VMatrix g_WorldToView{};
static VMatrix g_WorldToProjection{};

auto Hook_OverrideView( void* rcx , void* view ,
                        VMatrix* pWorldToView ,
                        VMatrix* pViewToProjection ,
                        VMatrix* pWorldToProjection ,
                        VMatrix* pWorldToPixels ) -> void
{
    OverrideView_o( rcx , view , pWorldToView , pViewToProjection , pWorldToProjection , pWorldToPixels );

    if ( SDK::Interfaces::EngineToClient()->IsInGame() )
    {
        auto* pawn = GetCL_Players()->GetLocalPlayerPawn();
        if ( pawn && pawn->IsAlive() )
        {
            if ( Settings::Visual::ThirdPerson )
            {
                auto* ang = reinterpret_cast<QAngle*>( reinterpret_cast<uintptr_t>( view ) + g_CViewSetup_angView );
                auto eye = GetCL_Players()->GetLocalEyeOrigin();
                QAngle adjusted = *ang;
                adjusted.m_x = -adjusted.m_x;
                auto desired = Math::CalculateCameraPosition( eye , -Settings::Visual::ThirdPersonDistance , adjusted );

                Ray_t ray{};
                CGameTrace tr{};
                CTraceFilter filter( 0x1C3003 , pawn , 3 , 15 );

                Vector3 vStart = eye;
                Vector3 vEnd = desired;
                if ( SDK::Pointers::CVPhys2World() && *SDK::Pointers::CVPhys2World() &&
                     IGamePhysicsQuery_TraceShape( SDK::Pointers::CVPhys2World() , ray , vStart , vEnd , &filter , &tr ) )
                {
                    if ( tr.DidHit() )
                    {
                        vEnd = tr.vecPosition;
                    }
                }

                auto* org = reinterpret_cast<Vector3*>( reinterpret_cast<uintptr_t>( view ) + g_CViewSetup_vecOrigin );
                *org = vEnd;

                QAngle look = Math::CalcAngle( vEnd , eye );
                look.Normalize();
                ang->m_x = look.m_x;
                ang->m_y = look.m_y;
                ang->m_z = 0.f;
            }
        }
    }

    if ( pWorldToView ) g_WorldToView = *pWorldToView;
    if ( pWorldToProjection ) g_WorldToProjection = *pWorldToProjection;
    GetVisual()->CalculateBoundingBoxes();
}

auto GetWorldToViewMatrix() -> const VMatrix*
{
    return &g_WorldToView;
}

auto GetWorldToProjectionMatrix() -> const VMatrix*
{
    return &g_WorldToProjection;
}
