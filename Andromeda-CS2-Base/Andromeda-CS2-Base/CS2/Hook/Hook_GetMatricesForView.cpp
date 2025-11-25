#include "Hook_GetMatricesForView.hpp"

#include <CS2/SDK/Math/Math.hpp>
#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Interface/IEngineToClient.hpp>
#include <AndromedaClient/Settings/Settings.hpp>
#include <GameClient/CL_Players.hpp>
#include <CS2/SDK/Update/Offsets.hpp>

#include <AndromedaClient/Features/CVisual/CVisual.hpp>

auto Hook_GetMatricesForView( void* rcx , void* view ,
                              VMatrix* pWorldToView ,
                              VMatrix* pViewToProjection ,
                              VMatrix* pWorldToProjection ,
                              VMatrix* pWorldToPixels ) -> void
{
    if ( SDK::Interfaces::EngineToClient()->IsInGame() && Settings::Visual::ThirdPerson )
    {
        auto* pawn = GetCL_Players()->GetLocalPlayerPawn();
        if ( pawn )
        {
            auto* ang = reinterpret_cast<QAngle*>( reinterpret_cast<uintptr_t>( view ) + g_CViewSetup_angView );
            auto eye = GetCL_Players()->GetLocalEyeOrigin();
            auto cam = Math::CalculateCameraPosition( eye , -Settings::Visual::ThirdPersonDistance , *ang );
            auto* org = reinterpret_cast<Vector3*>( reinterpret_cast<uintptr_t>( view ) + g_CViewSetup_vecOrigin );
            *org = cam;
        }
    }
    GetMatricesForView_o( rcx , view , pWorldToView , pViewToProjection , pWorldToProjection , pWorldToPixels );

    GetVisual()->CalculateBoundingBoxes();
}
