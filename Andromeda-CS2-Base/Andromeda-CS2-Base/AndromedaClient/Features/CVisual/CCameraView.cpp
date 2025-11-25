#include "CCameraView.hpp"

#include <CS2/SDK/Update/CCSGOInput.hpp>
#include <GameClient/CL_Players.hpp>
#include <Common/DevLog.hpp>
#include <CS2/SDK/Update/Offsets.hpp>

auto CCameraView::SetThirdPerson(CCSGOInput* input, bool enable) -> void
{
    if ( !input ) return;
    const auto base = reinterpret_cast<uintptr_t>( input );
    auto* addr = reinterpret_cast<bool*>( base + g_CCSGOInput_m_bInThirdPerson );
    const bool before = *addr;
    if ( before != enable )
        DEV_LOG( "[ThirdPerson] base=%p off=0x%X addr=%p before=%d enable=%d\n" , input , (unsigned)g_CCSGOInput_m_bInThirdPerson , addr , before ? 1 : 0 , enable ? 1 : 0 );

    input->m_bInThirdPerson() = enable;

    const bool after = *addr;
    DEV_LOG( "[ThirdPerson] after=%d (same=%d)\n" , after ? 1 : 0 , ( before == after ) ? 1 : 0 );
}