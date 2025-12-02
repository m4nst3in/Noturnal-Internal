#include <Common/Common.hpp>
#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Interface/IEngineToClient.hpp>
#include "OffsetPatternTester.hpp"

static DWORD WINAPI DevThread(LPVOID)
{
    for (;;)
    {
        if ( SDK::Interfaces::EngineToClient()->IsInGame() )
        {
            if ( GetAsyncKeyState( VK_F5 ) & 1 )
                RunDevOffsetPatternTests();
        }
        Sleep( 100 );
    }
    return 0;
}

struct AutoDevRunner
{
    AutoDevRunner()
    {
        CreateThread( nullptr , 0 , DevThread , nullptr , 0 , nullptr );
    }
};

static AutoDevRunner g_AutoDevRunner{};