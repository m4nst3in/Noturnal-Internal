#include "CAndromedaClient.hpp"

#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Interface/IEngineToClient.hpp>
#include <CS2/SDK/Interface/IGameEvent.hpp>
#include <CS2/SDK/Update/CCSGOInput.hpp>

#include <AndromedaClient/GUI/CAndromedaMenu.hpp>
#include <AndromedaClient/CAndromedaGUI.hpp>
#include <AndromedaClient/Fonts/CFontManager.hpp>
#include <AndromedaClient/Render/CRenderStackSystem.hpp>
#include <AndromedaClient/Features/CVisual/CVisual.hpp>
#include <AndromedaClient/Features/CInventory/InventoryChanger.hpp>
#include <GameClient/CL_Weapons.hpp>
#include <CS2/SDK/FunctionListSDK.hpp>


#include <GameClient/CEntityCache/CEntityCache.hpp>
#include <AndromedaClient/Features/CInventory/SkinApiClient.hpp>
#include <AndromedaClient/Features/CInventory/CSkinDatabase.hpp>

static CAndromedaClient g_CAndromedaClient{};

auto CAndromedaClient::OnInit() -> void
{
    SkinApiClient api;
    api.SetEndpoint("https://raw.githubusercontent.com/ByMykel/CSGO-API/main/public/api/en/skins.json");
    api.FetchAndUpdate(GetSkinDatabase());
    GetInventoryChanger()->SetDebug(true);
}

auto CAndromedaClient::OnFrameStageNotify( int FrameStage ) -> void
{
    if ( SDK::Interfaces::EngineToClient()->IsInGame() ) {
        GetInventoryChanger()->OnFrameStageNotify(FrameStage);
        GetInventoryChanger()->ApplyLoop();
    }
}

auto CAndromedaClient::OnFireEventClientSide( IGameEvent* pGameEvent ) -> void
{
    if ( SDK::Interfaces::EngineToClient()->IsInGame() ) {
        GetInventoryChanger()->OnFireEventClientSide( pGameEvent );
        GetInventoryChanger()->ApplyLoop();
    }
}

auto CAndromedaClient::OnAddEntity( CEntityInstance* pInst , CHandle handle ) -> void
{
    GetEntityCache()->OnAddEntity( pInst , handle );
    DEV_LOG("[client] OnAddEntity\n");
    GetInventoryChanger()->ApplyOnAdd( pInst , handle );
}

auto CAndromedaClient::OnRemoveEntity( CEntityInstance* pInst , CHandle handle ) -> void
{
	GetEntityCache()->OnRemoveEntity( pInst , handle );
}

auto CAndromedaClient::OnRender() -> void
{
	if ( GetAndromedaGUI()->IsVisible() )
		GetAndromedaMenu()->OnRenderMenu();

	GetFontManager()->FirstInitFonts();
	GetFontManager()->m_VerdanaFont.DrawString( 1 , 1 , ImColor( 255 , 255 , 0 ) , FW1_LEFT , XorStr( CHEAT_NAME ) );

    if ( SDK::Interfaces::EngineToClient()->IsInGame() )
    {
        GetRenderStackSystem()->OnRenderStack();
    }
}

auto CAndromedaClient::OnClientOutput() -> void
{
	if ( SDK::Interfaces::EngineToClient()->IsInGame() )
	{
		GetVisual()->OnClientOutput();
	}
}

auto CAndromedaClient::OnCreateMove( CCSGOInput* pInput , CUserCmd* pUserCmd ) -> void
{
    GetVisual()->OnCreateMove(pInput, pUserCmd);
    GetInventoryChanger()->OnCreateMove(pInput, pUserCmd);
}

auto GetAndromedaClient() -> CAndromedaClient*
{
	return &g_CAndromedaClient;
}
