#include "Hook_CreateMove.hpp"

#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Interface/IEngineToClient.hpp>
#include <CS2/SDK/Interface/IEngineCvar.hpp>
#include <AndromedaClient/Settings/Settings.hpp>
#include <CS2/SDK/Update/CCSGOInput.hpp>
#include <Common/MemoryEngine.hpp>

#include <AndromedaClient/CAndromedaClient.hpp>
#include <windows.h>

#include <GameClient/CL_Players.hpp>
#include <GameClient/CL_Bypass.hpp>

auto Hook_CreateMove( CCSGOInput* pCCSGOInput , uint32_t split_screen_index , char a3 ) -> bool
{
    static ULONGLONG s_lastLog = 0; ULONGLONG now = GetTickCount64();
    if (now - s_lastLog > 500) { DEV_LOG("[hook] CreateMove called\n"); s_lastLog = now; }
    auto Result = false;

	if ( auto* pLocalPlayerController = GetCL_Players()->GetLocalPlayerController(); pLocalPlayerController )
	{
		Result = CreateMove_o( pCCSGOInput , split_screen_index , a3 );
		const auto pUserCmd = pCCSGOInput->GetUserCmd( pLocalPlayerController );

		if ( !pUserCmd )
			return Result;

        if ( SDK::Interfaces::EngineToClient()->IsInGame() && GetCL_Players()->GetLocalPlayerPawn() )
        {
            static bool s_unlocked = false;
            if ( !s_unlocked )
            {
                if ( auto* cvar = SDK::Interfaces::EngineCvar() )
                {
                    cvar->UnlockCheatCVars();
                }
                if ( auto addr = reinterpret_cast<uintptr_t>( FindPattern( CLIENT_DLL , XorStr( "75 ? 44 88 66 01 44 89 A6 ? ? ? ? E9" ) ) ) )
                {
                    DWORD oldProt;
                    VirtualProtect( reinterpret_cast<void*>( addr ) , 1 , PAGE_EXECUTE_READWRITE , &oldProt );
                    *reinterpret_cast<uint8_t*>( addr ) = 0xEB;
                    VirtualProtect( reinterpret_cast<void*>( addr ) , 1 , oldProt , &oldProt );
                }
                s_unlocked = true;
            }
			GetCL_Bypass()->PreClientCreateMove( pUserCmd );
			GetAndromedaClient()->OnCreateMove( pCCSGOInput , pUserCmd );
			GetCL_Bypass()->PostClientCreateMove( pCCSGOInput , pUserCmd );
		}
	}

	return Result;
}

auto Hook_MessageLite_SerializePartialToArray( google::protobuf::Message* pMsg , void* out_buffer , int size ) -> bool
{
#if DISABLE_PROTOBUF == 0
	const google::protobuf::Descriptor* descriptor = pMsg->GetDescriptor();
	const std::string message_name = descriptor->name();

	if ( message_name == XorStr( "CBaseUserCmdPB" ) )
	{
		GetCL_Bypass()->OnCBaseUserCmdPB( reinterpret_cast<CBaseUserCmdPB*>( pMsg ) );
	}
#endif

	return MessageLite_SerializePartialToArray_o( pMsg , out_buffer , size );
}
