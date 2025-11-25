#include "CCameraView.hpp"

#include <CS2/SDK/Update/CCSGOInput.hpp>
#include <GameClient/CL_Players.hpp>

auto CCameraView::SetThirdPerson(CCSGOInput* input, bool enable) -> void
{
    auto* ctrl = GetCL_Players()->GetLocalPlayerController();
    if ( !ctrl ) return;
    if ( !input ) return;
    input->m_bInThirdPerson() = enable;
}