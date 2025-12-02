#include "Hook_OnAddEntity.hpp"

#include <AndromedaClient/CAndromedaClient.hpp>
#include <Common/Common.hpp>

auto Hook_OnAddEntity( CGameEntitySystem* pCGameEntitySystem , CEntityInstance* pInst , CHandle handle ) -> void
{
    DEV_LOG("[hook] OnAddEntity: inst=%p handle=%u\n", pInst, handle.GetEntryIndex());
    OnAddEntity_o( pCGameEntitySystem , pInst , handle );
    GetAndromedaClient()->OnAddEntity( pInst , handle );
}
