#include "Hook_FrameStageNotify.hpp"

#include <AndromedaClient/CAndromedaClient.hpp>
#include <Common/Common.hpp>
#include <windows.h>

auto Hook_FrameStageNotify( CSource2Client* pCSource2Client , int FrameStage ) -> void
{
    static ULONGLONG last = 0;
    if (FrameStage == 6) {
        ULONGLONG now = GetTickCount64();
        if (now - last > 500) {
            DEV_LOG("[hook] FrameStageNotify: %d\n", FrameStage);
            last = now;
        }
    }
    FrameStageNotify_o( pCSource2Client , FrameStage );
    GetAndromedaClient()->OnFrameStageNotify( FrameStage );
}
