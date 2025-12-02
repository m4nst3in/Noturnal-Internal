#include "Hook_FireEventClientSide.hpp"

#include <AndromedaClient/CAndromedaClient.hpp>
#include <Common/Common.hpp>
#include <CS2/SDK/FunctionListSDK.hpp>

auto Hook_FireEventClientSide( IGameEventManager2* pGameEventManager2 , IGameEvent* pGameEvent ) -> bool
{
    const char* name = IGameEvent_GetName( pGameEvent );
    DEV_LOG( "[hook] FireEventClientSide: %s\n" , name ? name : "<null>" );
    GetAndromedaClient()->OnFireEventClientSide( pGameEvent );

    return FireEventClientSide_o( pGameEventManager2 , pGameEvent );
}
