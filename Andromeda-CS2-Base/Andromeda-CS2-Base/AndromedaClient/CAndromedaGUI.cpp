#include "CAndromedaGUI.hpp"
#include <ImGui/imgui_impl_dx11.h>
#include <AndromedaClient/Fonts/MontserratBytes.hpp>
#include <AndromedaClient/Fonts/FontAwesomeBytes.hpp>

#include <ShlObj_core.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <vector>

#include <ImGui/imgui_impl_win32.h>
#include <ImGui/imgui_impl_dx11.h>

#include <DllLauncher.hpp>
#include <Common/CrashLog.hpp>
#include <Common/DevLog.hpp>
#include <CS2/CHook_Loader.hpp>
#include <Common/Helpers/StringHelper.hpp>

#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Interface/IEngineToClient.hpp>
#include <CS2/SDK/SDL3/SDL3_Functions.hpp>

#include <CS2/Hook/Hook_IsRelativeMouseMode.hpp>

#include <AndromedaClient/CAndromedaClient.hpp>
#include <AndromedaClient/Settings/Settings.hpp>
#include <AndromedaClient/D3D/Hook_DrawIndexed.hpp>
#include <AndromedaClient/Features/CVisual/CChams3D.hpp>

static CAndromedaGUI g_AndromedaGUI{};
#if __has_include("../../fatality.win-CS2-master/cs2_internal/src/resources/josefin_sans.h")
#include "../../fatality.win-CS2-master/cs2_internal/src/resources/josefin_sans.h"
#define HAS_JOSEFIN 1
#else
#define HAS_JOSEFIN 0
#endif
#if __has_include("../../fatality.win-CS2-master/cs2_internal/src/resources/josefin_sans_bold.h")
#include "../../fatality.win-CS2-master/cs2_internal/src/resources/josefin_sans_bold.h"
#define HAS_JOSEFIN_BOLD 1
#else
#define HAS_JOSEFIN_BOLD 0
#endif

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hwnd , UINT msg , WPARAM wParam , LPARAM lParam );

auto CAndromedaGUI::OnInit( IDXGISwapChain* pSwapChain ) -> void
{
	DXGI_SWAP_CHAIN_DESC SwapChainDesc;

	if ( FAILED( pSwapChain->GetDevice( IID_PPV_ARGS( &m_pDevice ) ) ) )
	{
		DEV_LOG( "[error] CAndromedaGUI::OnInit: #1\n" );
		return;
	}

	m_pDevice->GetImmediateContext( &m_pDeviceContext );

	if ( FAILED( pSwapChain->GetDesc( &SwapChainDesc ) ) )
	{
		DEV_LOG( "[error] CAndromedaGUI::OnInit: #2\n" );
		return;
	}

	m_hCS2Window = SwapChainDesc.OutputWindow;

	m_pImGuiContext = ImGui::CreateContext();

	m_GuiFile = GetDllDir() + XorStr( GUI_FILE );

	if ( !m_pFreeType_Font )
		m_pFreeType_Font = new FreeTypeBuild();

	ImGui::SetCurrentContext( m_pImGuiContext );

	ImGui::GetIO().IniFilename = m_GuiFile.c_str();
	ImGui::GetIO().LogFilename = "";

	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	ImGui::GetIO().BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

    DEV_LOG( "[gui] OnInit: hwnd=0x%p device=0x%p ctx=0x%p" , m_hCS2Window , m_pDevice , m_pDeviceContext );
    ImGui_ImplWin32_Init( m_hCS2Window );
    ImGui_ImplDX11_Init( m_pDevice , m_pDeviceContext );
    InitDrawIndexedHook( m_pDeviceContext );

    InitFont();
    LoadFontsFromBytes();
    UpdateStyle();

    m_WndProc_o = (WNDPROC)SetWindowLongPtrA( m_hCS2Window , GWLP_WNDPROC , (LONG_PTR)GUI_WndProc );
    DEV_LOG( "[gui] SetWindowLongPtrA: old_wndproc=0x%p new_wndproc=0x%p" , m_WndProc_o , GUI_WndProc );

    m_bInit = true;
    DEV_LOG( "[gui] OnInit: completed" );
}

auto CAndromedaGUI::OnDestroy() -> void
{
	SetWindowLongPtrA( m_hCS2Window , GWLP_WNDPROC , (LONG_PTR)GetAndromedaGUI()->m_WndProc_o );

	m_bVisible = false;

	if ( m_pFreeType_Font )
	{
		delete m_pFreeType_Font;
		m_pFreeType_Font = nullptr;
	}

	ClearRenderTargetView();

    ShutdownDrawIndexedHook();
    ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();

    ImGui::DestroyContext();

	m_bInit = false;
}

auto CAndromedaGUI::InitFont() -> void
{
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    ImFont* baseFont = nullptr;

    // --- 1. Carregar Fonte Principal (Base) ---
    if ( m_pMontserratData && m_uMontserratSize > 0 )
    {
        ImFontConfig cfg;
        cfg.FontDataOwnedByAtlas = false;
        cfg.PixelSnapH = true;
        cfg.OversampleH = 2;
        cfg.OversampleV = 2;
        baseFont = io.Fonts->AddFontFromMemoryTTF( (void*)m_pMontserratData , (int)m_uMontserratSize , m_fFontSizePx , &cfg );
    }
    else
    {
        static const ImWchar Range[] = { 0x0020 , 0xFFFC , 0 };
        wchar_t* szWindowsFontPath = nullptr;
        if ( SHGetKnownFolderPath( FOLDERID_Fonts , 0 , 0 , &szWindowsFontPath ) == S_OK )
        {
            std::wstring TahomaFont = std::wstring( szWindowsFontPath ) + L"\\tahoma.ttf";
            baseFont = io.Fonts->AddFontFromFileTTF( unicode_to_utf8( TahomaFont ).c_str() , 16.f , nullptr , Range );
        }
        CoTaskMemFree( szWindowsFontPath );
    }

    // --- 2. Carregar Ícones (MergeMode = true) ---
    // IMPORTANTE: Isso deve ser feito LOGO APÓS a fonte Base e ANTES da fonte de Título
    if ( m_pFontAwesomeData && m_uFontAwesomeSize > 0 )
    {
        ImFontConfig iconCfg;
        iconCfg.MergeMode = true; // Fundir na última fonte adicionada (baseFont)
        iconCfg.PixelSnapH = true;
        iconCfg.FontDataOwnedByAtlas = false;
        iconCfg.OversampleH = 2;
        iconCfg.OversampleV = 2;
        iconCfg.GlyphMinAdvanceX = 0.0f;
        iconCfg.GlyphOffset = ImVec2( 0.0f , 1.0f ); // Ajuste vertical se necessário
        
        // Intervalo padrão do FontAwesome Solid (Verifique se seus ícones estão aqui)
        static const ImWchar icon_ranges[] = { 0xE000 , 0xF8FF , 0 }; 
        
        io.Fonts->AddFontFromMemoryTTF( (void*)m_pFontAwesomeData , (int)m_uFontAwesomeSize , m_fFontSizePx , &iconCfg , icon_ranges );
    }

    // --- 3. Carregar Fonte de Título (Separada, sem Merge) ---
    if ( m_pMontserratData && m_uMontserratSize > 0 )
    {
        ImFontConfig tcfg;
        tcfg.FontDataOwnedByAtlas = false;
        tcfg.PixelSnapH = true;
        tcfg.OversampleH = 2;
        tcfg.OversampleV = 2;
        m_pTitleFont = io.Fonts->AddFontFromMemoryTTF( (void*)m_pMontserratData , (int)m_uMontserratSize , m_fFontSizePx * 1.6f , &tcfg );
        
    }
    else
    {
        // Fallback para fonte do Windows se Montserrat não existir
        static const ImWchar Range[] = { 0x0020 , 0xFFFC , 0 };
        wchar_t* szWindowsFontPath = nullptr;
        if ( SHGetKnownFolderPath( FOLDERID_Fonts , 0 , 0 , &szWindowsFontPath ) == S_OK )
        {
            std::wstring TahomaFont = std::wstring( szWindowsFontPath ) + L"\\tahoma.ttf";
            ImFontConfig tcfg{}; tcfg.PixelSnapH = true; tcfg.OversampleH = 2; tcfg.OversampleV = 2;
            m_pTitleFont = io.Fonts->AddFontFromFileTTF( unicode_to_utf8( TahomaFont ).c_str() , 25.f , &tcfg , Range );
        }
        CoTaskMemFree( szWindowsFontPath );
    }

    if ( baseFont )
        io.FontDefault = baseFont;

    ImGui_ImplDX11_InvalidateDeviceObjects();
    ImGui_ImplDX11_CreateDeviceObjects();

    if ( m_pFreeType_Font )
        m_pFreeType_Font->ResetBuildFont();
}

void CAndromedaGUI::OnPresent( IDXGISwapChain* pSwapChain )
{
    if ( !m_bInit )
        OnInit( pSwapChain );
    else
        OnRender( pSwapChain );
}

void CAndromedaGUI::OnResizeBuffers( IDXGISwapChain* pSwapChain )
{
	DEV_LOG( "[gui] OnResizeBuffers called" );
	// Only clear the render target view, don't destroy the entire GUI
	ClearRenderTargetView();
}
 
void CAndromedaGUI::OnRender( IDXGISwapChain* pSwapChain )
{
	if ( m_pFreeType_Font && m_pFreeType_Font->PreNewFrame() )
	{
		ImGui_ImplDX11_InvalidateDeviceObjects();
		ImGui_ImplDX11_CreateDeviceObjects();
	}

    // Always recreate render target view if it's null or invalid
    if ( !m_pRenderTargetView )
    {
        DEV_LOG( "[gui] Recreating RenderTargetView" );
        ID3D11Texture2D* pBackBuffer = nullptr;

        if ( SUCCEEDED( pSwapChain->GetBuffer( 0 , IID_PPV_ARGS( &pBackBuffer ) ) ) && pBackBuffer )
        {
            D3D11_TEXTURE2D_DESC backBufferDesc;
            pBackBuffer->GetDesc( &backBufferDesc );

            D3D11_RENDER_TARGET_VIEW_DESC RenderTargetDesc = {};
            RenderTargetDesc.Format = backBufferDesc.Format;
            RenderTargetDesc.ViewDimension = (backBufferDesc.SampleDesc.Count > 1) ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;

            m_pDevice->CreateRenderTargetView( pBackBuffer , &RenderTargetDesc , &m_pRenderTargetView );
            pBackBuffer->Release();
        }
    }

    if ( !m_pRenderTargetView )
        return;

    ImGui::SetCurrentContext( m_pImGuiContext );

    // Hotkey check using GetAsyncKeyState
    {
        static bool s_wasPressed = false;
        
        // Check if INSERT is currently pressed (high bit set)
        bool isPressed = (GetAsyncKeyState( VK_INSERT ) & 0x8000) != 0;
        
        // Toggle only on rising edge (key just pressed, wasn't pressed before)
        if ( isPressed && !s_wasPressed )
        {
            DEV_LOG( "[gui] VK_INSERT pressed -> toggle (visible=%d)" , m_bVisible ? 0 : 1 );
            OnReopenGUI();
        }
        s_wasPressed = isPressed;
    }

    m_pDeviceContext->OMGetRenderTargets( 1 , &m_pMainRenderTarget , &m_pDepthStencilView );
    m_pDeviceContext->OMSetRenderTargets( 1 , &m_pRenderTargetView , nullptr );

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();

    ImGui::NewFrame();

    GetAndromedaClient()->OnRender();

    ImGui::EndFrame();
    ImGui::Render();

    ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData() );
    if ( GetChams3D()->Init( m_pDevice ) )
        GetChams3D()->Render( m_pDeviceContext , m_pDepthStencilView );
    
    // Restore original render targets
    m_pDeviceContext->OMSetRenderTargets( 1 , &m_pMainRenderTarget , m_pDepthStencilView );
    
    // Release references to avoid leaks
    if ( m_pMainRenderTarget )
    {
        m_pMainRenderTarget->Release();
        m_pMainRenderTarget = nullptr;
    }
    if ( m_pDepthStencilView )
    {
        m_pDepthStencilView->Release();
        m_pDepthStencilView = nullptr;
    }
}

auto CAndromedaGUI::OnReopenGUI() -> void
{
    m_bVisible = !m_bVisible;

	ImGui::GetIO().MouseDrawCursor = m_bVisible;
	ShowCursor( !m_bVisible );

	IsRelativeMouseMode_o( SDK::Interfaces::InputSystem() , m_bVisible ? false : m_bMainActive );

	if ( m_bVisible )
	{
		if ( m_vecMousePosSave.x == 0.f && m_vecMousePosSave.y == 0.f )
			m_vecMousePosSave = ImGui::GetIO().DisplaySize / 2.f;

		ImGui::GetIO().MousePos = m_vecMousePosSave;

        if ( SDK::Interfaces::EngineToClient()->IsInGame() )
            GetSDL3Functions()->SDL_WarpMouseInWindow_o( nullptr , ImGui::GetIO().MousePos.x , ImGui::GetIO().MousePos.y );
	}
	else
	{
		m_vecMousePosSave = ImGui::GetIO().MousePos;
	}
}

LRESULT WINAPI CAndromedaGUI::GUI_WndProc( HWND hwnd , UINT uMsg , WPARAM wParam , LPARAM lParam )
{
	if ( uMsg == WM_QUIT || uMsg == WM_CLOSE || uMsg == WM_DESTROY )
	{
		GetDllLauncher()->OnDestroy();
		return true;
	}

    if ( GetAndromedaGUI()->m_bInit )
    {
        // Block INSERT key from propagating when menu is visible
        if ( wParam == VK_INSERT && (uMsg == WM_KEYUP || uMsg == WM_KEYDOWN) )
            return true;

		if ( GetAndromedaGUI()->IsVisible() && ImGui_ImplWin32_WndProcHandler( hwnd , uMsg , wParam , lParam ) == 0 )
			return true;
	}

    return CallWindowProcA( GetAndromedaGUI()->m_WndProc_o , hwnd , uMsg , wParam , lParam );
}

auto CAndromedaGUI::SetIndigoStyle() -> void
{
	auto& style = ImGui::GetStyle();
	auto& colors = style.Colors;

	float roundness = 2.0f;

	style.WindowBorderSize = 1.0f;
	style.FrameBorderSize = 1.0f;
	style.WindowMinSize = ImVec2( 75.f , 50.f );
	style.FramePadding = ImVec2( 5 , 5 );
	style.ItemSpacing = ImVec2( 6 , 5 );
	style.ItemInnerSpacing = ImVec2( 2 , 4 );
	style.Alpha = 1.0f;
	style.WindowRounding = 0.f;
	style.FrameRounding = roundness;
	style.PopupRounding = 0;
	style.PopupBorderSize = 1.f;
	style.IndentSpacing = 6.0f;
	style.ColumnsMinSpacing = 50.0f;
	style.GrabMinSize = 14.0f;
	style.GrabRounding = roundness;
	style.ScrollbarSize = 12.0f;
	style.ScrollbarRounding = roundness;

	style.AntiAliasedFill = true;
	style.AntiAliasedLines = true;
	style.AntiAliasedLinesUseTex = true;

	colors[ImGuiCol_Text] = ImVec4( 1.00f , 1.00f , 1.00f , 1.00f );
	colors[ImGuiCol_TextDisabled] = ImVec4( 0.50f , 0.50f , 0.50f , 1.00f );

	colors[ImGuiCol_WindowBg] = ImVec4( 0.20f , 0.23f , 0.31f , 1.00f );
	colors[ImGuiCol_ChildBg] = ImVec4( 0.20f , 0.23f , 0.31f , 1.00f );
	colors[ImGuiCol_PopupBg] = ImVec4( 0.20f , 0.23f , 0.31f , 1.00f );

	colors[ImGuiCol_Border] = ImVec4( 0.f , 0.f , 0.f , 1.00f );
	colors[ImGuiCol_BorderShadow] = ImVec4( 0.00f , 0.00f , 0.00f , 0.00f );

	colors[ImGuiCol_FrameBg] = ImVec4( 0.25f , 0.28f , 0.38f , 1.00f );
	colors[ImGuiCol_FrameBgHovered] = ImVec4( 0.25f , 0.28f , 0.38f , 1.00f );
	colors[ImGuiCol_FrameBgActive] = ImVec4( 0.25f , 0.28f , 0.38f , 1.00f );

	colors[ImGuiCol_TitleBg] = ImVec4( 0.00f , 0.43f , 1.00f , 1.00f );
	colors[ImGuiCol_TitleBgActive] = ImVec4( 0.00f , 0.55f , 1.00f , 1.00f );
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4( 0.10f , 0.69f , 1.00f , 1.00f );

	colors[ImGuiCol_MenuBarBg] = ImVec4( 0.25f , 0.28f , 0.38f , 1.00f );

	colors[ImGuiCol_ScrollbarBg] = ImVec4( 0.00f , 0.00f , 0.00f , 0.00f );
	colors[ImGuiCol_ScrollbarGrab] = ImVec4( 0.39f , 0.44f , 0.56f , 1.00f );
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4( 0.12f , 0.43f , 1.00f , 1.00f );
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4( 0.00f , 0.55f , 1.00f , 1.00f );

	colors[ImGuiCol_CheckMark] = ImVec4( 0.00f , 0.55f , 1.00f , 1.00f );

	colors[ImGuiCol_SliderGrab] = ImVec4( 0.00f , 0.55f , 1.00f , 1.00f );
	colors[ImGuiCol_SliderGrabActive] = ImVec4( 0.10f , 0.69f , 1.00f , 1.00f );

	colors[ImGuiCol_Button] = ImVec4( 0.25f , 0.28f , 0.38f , 1.00f );
	colors[ImGuiCol_ButtonHovered] = ImVec4( 0.12f , 0.43f , 1.00f , 1.00f );
	colors[ImGuiCol_ButtonActive] = ImVec4( 0.00f , 0.55f , 1.00f , 1.00f );

	colors[ImGuiCol_Header] = ImVec4( 0.00f , 0.43f , 1.00f , 1.00f );
	colors[ImGuiCol_HeaderHovered] = ImVec4( 0.00f , 0.55f , 1.00f , 1.00f );
	colors[ImGuiCol_HeaderActive] = ImVec4( 0.00f , 0.43f , 1.00f , 1.00f );

	colors[ImGuiCol_Separator] = ImVec4( 0.43f , 0.43f , 0.50f , 0.50f );
	colors[ImGuiCol_SeparatorHovered] = ImVec4( 0.10f , 0.40f , 0.75f , 0.78f );
	colors[ImGuiCol_SeparatorActive] = ImVec4( 0.10f , 0.40f , 0.75f , 1.00f );

	colors[ImGuiCol_ResizeGrip] = ImVec4( 0.26f , 0.59f , 0.98f , 0.25f );
	colors[ImGuiCol_ResizeGripHovered] = ImVec4( 0.26f , 0.59f , 0.98f , 0.67f );
	colors[ImGuiCol_ResizeGripActive] = ImVec4( 0.26f , 0.59f , 0.98f , 0.95f );

	colors[ImGuiCol_Tab] = ImVec4( 0.00f , 0.50f , 1.00f , 1.00f );
	colors[ImGuiCol_TabHovered] = ImVec4( 0.12f , 0.69f , 1.00f , 1.00f );

	colors[ImGuiCol_TabActive] = ImVec4( 0.12f , 0.69f , 1.00f , 1.00f );
	colors[ImGuiCol_TabUnfocused] = ImVec4( 0.07f , 0.10f , 0.15f , 0.97f );
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4( 0.14f , 0.26f , 0.42f , 1.00f );

	colors[ImGuiCol_PlotLines] = ImVec4( 0.61f , 0.61f , 0.61f , 1.00f );
	colors[ImGuiCol_PlotLinesHovered] = ImVec4( 1.00f , 0.43f , 0.35f , 1.00f );
	colors[ImGuiCol_PlotHistogram] = ImVec4( 0.10f , 0.69f , 1.00f , 1.00f );
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4( 1.00f , 0.60f , 0.00f , 1.00f );

	colors[ImGuiCol_TextSelectedBg] = ImVec4( 0.26f , 0.59f , 0.98f , 0.35f );
	colors[ImGuiCol_DragDropTarget] = ImVec4( 1.00f , 1.00f , 0.00f , 0.90f );

	colors[ImGuiCol_NavHighlight] = ImVec4( 0.26f , 0.59f , 0.98f , 1.00f );
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4( 1.00f , 1.00f , 1.00f , 0.70f );
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4( 0.80f , 0.80f , 0.80f , 0.20f );

	colors[ImGuiCol_ModalWindowDimBg] = ImVec4( 0.80f , 0.80f , 0.80f , 0.35f );
	colors[ImGuiCol_WindowShadow] = ImVec4( 0.f , 0.f , 1.f , 1.f );
}

auto CAndromedaGUI::SetVermillionStyle() -> void
{
	auto& style = ImGui::GetStyle();
	auto& colors = style.Colors;

	float roundness = 2.0f;

	style.WindowBorderSize = 1.0f;
	style.FrameBorderSize = 1.0f;
	style.WindowMinSize = ImVec2( 75.f , 50.f );
	style.FramePadding = ImVec2( 5 , 5 );
	style.ItemSpacing = ImVec2( 6 , 5 );
	style.ItemInnerSpacing = ImVec2( 2 , 4 );
	style.Alpha = 1.0f;
	style.WindowRounding = 0.f;
	style.FrameRounding = roundness;
	style.PopupRounding = 0;
	style.PopupBorderSize = 1.f;
	style.IndentSpacing = 6.0f;
	style.ColumnsMinSpacing = 50.0f;
	style.GrabMinSize = 14.0f;
	style.GrabRounding = roundness;
	style.ScrollbarSize = 12.0f;
	style.ScrollbarRounding = roundness;

	style.AntiAliasedFill = true;
	style.AntiAliasedLines = true;
	style.AntiAliasedLinesUseTex = true;

	colors[ImGuiCol_Text] = ImVec4( 1.00f , 1.00f , 1.00f , 0.75f );
	colors[ImGuiCol_TextDisabled] = ImVec4( 0.55f , 0.35f , 0.75f , 0.78f );

	colors[ImGuiCol_WindowBg] = ImVec4( 0.17f , 0.20f , 0.25f , 1.00f );
	colors[ImGuiCol_ChildBg] = ImVec4( 0.20f , 0.22f , 0.27f , 0.57f );
	colors[ImGuiCol_PopupBg] = ImVec4( 0.17f , 0.20f , 0.25f , 1.00f );

	colors[ImGuiCol_Border] = ImVec4( 0.00f , 0.00f , 0.00f , 1.00f );
	colors[ImGuiCol_BorderShadow] = ImVec4( 0.00f , 0.00f , 0.00f , 0.00f );

	colors[ImGuiCol_FrameBg] = ImVec4( 0.37f , 0.36f , 0.46f , 0.24f );
	colors[ImGuiCol_FrameBgHovered] = ImVec4( 0.55f , 0.35f , 0.75f , 0.78f );
	colors[ImGuiCol_FrameBgActive] = ImVec4( 0.55f , 0.35f , 0.75f , 1.00f );

	colors[ImGuiCol_TitleBg] = ImVec4( 0.20f , 0.22f , 0.27f , 1.00f );
	colors[ImGuiCol_TitleBgActive] = ImVec4( 0.55f , 0.35f , 0.75f , 1.00f );
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4( 0.20f , 0.22f , 0.27f , 0.75f );

	colors[ImGuiCol_MenuBarBg] = ImVec4( 0.20f , 0.22f , 0.27f , 0.47f );

	colors[ImGuiCol_ScrollbarBg] = ImVec4( 0.20f , 0.22f , 0.27f , 1.00f );
	colors[ImGuiCol_ScrollbarGrab] = ImVec4( 0.09f , 0.15f , 0.16f , 1.00f );
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4( 0.55f , 0.35f , 0.75f , 0.78f );
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4( 0.55f , 0.35f , 0.75f , 1.00f );

	colors[ImGuiCol_CheckMark] = ImVec4( 0.55f , 0.35f , 0.75f , 1.00f );

	colors[ImGuiCol_SliderGrab] = ImVec4( 0.55f , 0.35f , 0.75f , 0.37f );
	colors[ImGuiCol_SliderGrabActive] = ImVec4( 0.65f , 0.40f , 0.85f , 1.00f );

	colors[ImGuiCol_Button] = ImVec4( 0.45f , 0.30f , 0.65f , 1.00f );
	colors[ImGuiCol_ButtonHovered] = ImVec4( 0.55f , 0.35f , 0.75f , 0.86f );
	colors[ImGuiCol_ButtonActive] = ImVec4( 0.55f , 0.35f , 0.75f , 1.00f );

	colors[ImGuiCol_Header] = ImVec4( 0.55f , 0.35f , 0.75f , 0.76f );
	colors[ImGuiCol_HeaderHovered] = ImVec4( 0.55f , 0.35f , 0.75f , 0.86f );
	colors[ImGuiCol_HeaderActive] = ImVec4( 0.55f , 0.35f , 0.75f , 1.00f );

	colors[ImGuiCol_Separator] = ImVec4( 0.15f , 0.10f , 0.20f , 0.35f );
	colors[ImGuiCol_SeparatorHovered] = ImVec4( 0.55f , 0.35f , 0.75f , 0.59f );
	colors[ImGuiCol_SeparatorActive] = ImVec4( 0.55f , 0.35f , 0.75f , 1.00f );

	colors[ImGuiCol_ResizeGrip] = ImVec4( 0.55f , 0.35f , 0.75f , 0.63f );
	colors[ImGuiCol_ResizeGripHovered] = ImVec4( 0.55f , 0.35f , 0.75f , 0.78f );
	colors[ImGuiCol_ResizeGripActive] = ImVec4( 0.55f , 0.35f , 0.75f , 1.00f );

	colors[ImGuiCol_Tab] = ImVec4( 0.55f , 0.35f , 0.75f , 0.76f );
	colors[ImGuiCol_TabHovered] = ImVec4( 0.55f , 0.35f , 0.75f , 0.86f );
	colors[ImGuiCol_TabActive] = ImVec4( 0.55f , 0.35f , 0.75f , 1.00f );
	colors[ImGuiCol_TabUnfocused] = ImVec4( 0.07f , 0.10f , 0.15f , 0.97f );
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4( 0.14f , 0.26f , 0.42f , 1.00f );

	colors[ImGuiCol_PlotLines] = ImVec4( 0.78f , 0.93f , 0.89f , 0.63f );
	colors[ImGuiCol_PlotLinesHovered] = ImVec4( 0.65f , 0.40f , 0.85f , 1.00f );
	colors[ImGuiCol_PlotHistogram] = ImVec4( 0.86f , 0.93f , 0.89f , 0.63f );
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4( 0.65f , 0.40f , 0.85f , 1.00f );

	colors[ImGuiCol_TextSelectedBg] = ImVec4( 0.55f , 0.35f , 0.75f , 0.43f );
	colors[ImGuiCol_DragDropTarget] = ImVec4( 1.00f , 1.00f , 0.00f , 0.90f );

	colors[ImGuiCol_NavHighlight] = ImVec4( 0.45f , 0.45f , 0.90f , 0.80f );
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4( 1.00f , 1.00f , 1.00f , 0.70f );
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4( 0.80f , 0.80f , 0.80f , 0.20f );

	colors[ImGuiCol_ModalWindowDimBg] = ImVec4( 0.80f , 0.80f , 0.80f , 0.35f );
	colors[ImGuiCol_WindowShadow] = ImVec4( 0.5f , 0.3f , 0.7f , 1.f );
}

auto CAndromedaGUI::SetClassicSteamStyle() -> void
{
	ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.6000000238418579f;
	style.WindowPadding = ImVec2( 8.0f , 8.0f );
	style.WindowRounding = 0.0f;
	style.WindowBorderSize = 1.0f;
	style.WindowMinSize = ImVec2( 32.0f , 32.0f );
	style.WindowTitleAlign = ImVec2( 0.0f , 0.5f );
	style.WindowMenuButtonPosition = ImGuiDir_Left;
	style.ChildRounding = 0.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 0.0f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2( 4.0f , 3.0f );
	style.FrameRounding = 0.0f;
	style.FrameBorderSize = 1.0f;
	style.ItemSpacing = ImVec2( 8.0f , 4.0f );
	style.ItemInnerSpacing = ImVec2( 4.0f , 4.0f );
	style.CellPadding = ImVec2( 4.0f , 2.0f );
	style.IndentSpacing = 21.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 14.0f;
	style.ScrollbarRounding = 0.0f;
	style.GrabMinSize = 10.0f;
	style.GrabRounding = 0.0f;
	style.TabRounding = 0.0f;
	style.TabBorderSize = 0.0f;
	style.TabCloseButtonMinWidthUnselected = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2( 0.5f , 0.5f );
	style.SelectableTextAlign = ImVec2( 0.0f , 0.0f );

	style.Colors[ImGuiCol_Text] = ImVec4( 1.0f , 1.0f , 1.0f , 1.0f );
	style.Colors[ImGuiCol_TextDisabled] = ImVec4( 0.4980392158031464f , 0.4980392158031464f , 0.4980392158031464f , 1.0f );
	style.Colors[ImGuiCol_WindowBg] = ImVec4( 0.2862745225429535f , 0.3372549116611481f , 0.2588235437870026f , 1.0f );
	style.Colors[ImGuiCol_ChildBg] = ImVec4( 0.2862745225429535f , 0.3372549116611481f , 0.2588235437870026f , 1.0f );
	style.Colors[ImGuiCol_PopupBg] = ImVec4( 0.239215686917305f , 0.2666666805744171f , 0.2000000029802322f , 1.0f );
	style.Colors[ImGuiCol_Border] = ImVec4( 0.5372549295425415f , 0.5686274766921997f , 0.5098039507865906f , 0.5f );
	style.Colors[ImGuiCol_BorderShadow] = ImVec4( 0.1372549086809158f , 0.1568627506494522f , 0.1098039224743843f , 0.5199999809265137f );
	style.Colors[ImGuiCol_FrameBg] = ImVec4( 0.239215686917305f , 0.2666666805744171f , 0.2000000029802322f , 1.0f );
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4( 0.2666666805744171f , 0.2980392277240753f , 0.2274509817361832f , 1.0f );
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4( 0.2980392277240753f , 0.3372549116611481f , 0.2588235437870026f , 1.0f );
	style.Colors[ImGuiCol_TitleBg] = ImVec4( 0.239215686917305f , 0.2666666805744171f , 0.2000000029802322f , 1.0f );
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4( 0.2862745225429535f , 0.3372549116611481f , 0.2588235437870026f , 1.0f );
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4( 0.0f , 0.0f , 0.0f , 0.5099999904632568f );
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4( 0.239215686917305f , 0.2666666805744171f , 0.2000000029802322f , 1.0f );
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4( 0.3490196168422699f , 0.4196078479290009f , 0.3098039329051971f , 1.0f );
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4( 0.2784313857555389f , 0.3176470696926117f , 0.239215686917305f , 1.0f );
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4( 0.2470588237047195f , 0.2980392277240753f , 0.2196078449487686f , 1.0f );
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4( 0.2274509817361832f , 0.2666666805744171f , 0.2078431397676468f , 1.0f );
	style.Colors[ImGuiCol_CheckMark] = ImVec4( 0.5882353186607361f , 0.5372549295425415f , 0.1764705926179886f , 1.0f );
	style.Colors[ImGuiCol_SliderGrab] = ImVec4( 0.3490196168422699f , 0.4196078479290009f , 0.3098039329051971f , 1.0f );
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4( 0.5372549295425415f , 0.5686274766921997f , 0.5098039507865906f , 0.5f );
	style.Colors[ImGuiCol_Button] = ImVec4( 0.2862745225429535f , 0.3372549116611481f , 0.2588235437870026f , 0.4000000059604645f );
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4( 0.3490196168422699f , 0.4196078479290009f , 0.3098039329051971f , 1.0f );
	style.Colors[ImGuiCol_ButtonActive] = ImVec4( 0.5372549295425415f , 0.5686274766921997f , 0.5098039507865906f , 0.5f );
	style.Colors[ImGuiCol_Header] = ImVec4( 0.3490196168422699f , 0.4196078479290009f , 0.3098039329051971f , 1.0f );
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4( 0.3490196168422699f , 0.4196078479290009f , 0.3098039329051971f , 0.6000000238418579f );
	style.Colors[ImGuiCol_HeaderActive] = ImVec4( 0.5372549295425415f , 0.5686274766921997f , 0.5098039507865906f , 0.5f );
	style.Colors[ImGuiCol_Separator] = ImVec4( 0.1372549086809158f , 0.1568627506494522f , 0.1098039224743843f , 1.0f );
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4( 0.5372549295425415f , 0.5686274766921997f , 0.5098039507865906f , 1.0f );
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4( 0.5882353186607361f , 0.5372549295425415f , 0.1764705926179886f , 1.0f );
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4( 0.1882352977991104f , 0.2274509817361832f , 0.1764705926179886f , 0.0f );
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4( 0.5372549295425415f , 0.5686274766921997f , 0.5098039507865906f , 1.0f );
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4( 0.5882353186607361f , 0.5372549295425415f , 0.1764705926179886f , 1.0f );
	style.Colors[ImGuiCol_Tab] = ImVec4( 0.3490196168422699f , 0.4196078479290009f , 0.3098039329051971f , 1.0f );
	style.Colors[ImGuiCol_TabHovered] = ImVec4( 0.5372549295425415f , 0.5686274766921997f , 0.5098039507865906f , 0.7799999713897705f );
	style.Colors[ImGuiCol_TabActive] = ImVec4( 0.5882353186607361f , 0.5372549295425415f , 0.1764705926179886f , 1.0f );
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4( 0.239215686917305f , 0.2666666805744171f , 0.2000000029802322f , 1.0f );
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4( 0.3490196168422699f , 0.4196078479290009f , 0.3098039329051971f , 1.0f );
	style.Colors[ImGuiCol_PlotLines] = ImVec4( 0.6078431606292725f , 0.6078431606292725f , 0.6078431606292725f , 1.0f );
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4( 0.5882353186607361f , 0.5372549295425415f , 0.1764705926179886f , 1.0f );
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4( 1.0f , 0.7764706015586853f , 0.2784313857555389f , 1.0f );
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4( 1.0f , 0.6000000238418579f , 0.0f , 1.0f );
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4( 0.1882352977991104f , 0.1882352977991104f , 0.2000000029802322f , 1.0f );
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4( 0.3098039329051971f , 0.3098039329051971f , 0.3490196168422699f , 1.0f );
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4( 0.2274509817361832f , 0.2274509817361832f , 0.2470588237047195f , 1.0f );
	style.Colors[ImGuiCol_TableRowBg] = ImVec4( 0.0f , 0.0f , 0.0f , 0.0f );
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4( 1.0f , 1.0f , 1.0f , 0.05999999865889549f );
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4( 0.5882353186607361f , 0.5372549295425415f , 0.1764705926179886f , 1.0f );
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4( 0.729411780834198f , 0.6666666865348816f , 0.239215686917305f , 1.0f );
	style.Colors[ImGuiCol_NavHighlight] = ImVec4( 0.5882353186607361f , 0.5372549295425415f , 0.1764705926179886f , 1.0f );
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4( 1.0f , 1.0f , 1.0f , 0.699999988079071f );
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4( 0.800000011920929f , 0.800000011920929f , 0.800000011920929f , 0.2000000029802322f );
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4( 0.800000011920929f , 0.800000011920929f , 0.800000011920929f , 0.3499999940395355f );
	style.Colors[ImGuiCol_WindowShadow] = ImVec4( 1.f , 1.f , 0.f , 1.f );
}

auto CAndromedaGUI::SetNoturnalStyle() -> void
{
    auto& style = ImGui::GetStyle();
    auto& colors = style.Colors;

    // Fatality-inspired spacing and sizing
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.ChildBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.WindowMinSize = ImVec2( 100.f , 100.f );
    style.WindowPadding = ImVec2( 12.f , 12.f );
    style.FramePadding = ImVec2( 8.f , 4.f );
    style.ItemSpacing = ImVec2( 8.f , 6.f );
    style.ItemInnerSpacing = ImVec2( 6.f , 4.f );
    style.CellPadding = ImVec2( 4.f , 4.f );
    style.Alpha = 1.0f;
    style.DisabledAlpha = 0.5f;
    style.WindowRounding = 6.f;
    style.ChildRounding = 4.f;
    style.FrameRounding = 3.f;
    style.PopupRounding = 4.f;
    style.TabRounding = 4.f;
    style.TabBorderSize = 0.f;
    style.IndentSpacing = 12.0f;
    style.ColumnsMinSpacing = 8.0f;
    style.GrabMinSize = 8.0f;
    style.GrabRounding = 2.f;
    style.ScrollbarSize = 10.0f;
    style.ScrollbarRounding = 3.f;
    style.WindowTitleAlign = ImVec2( 0.5f , 0.5f );

    style.AntiAliasedFill = true;
    style.AntiAliasedLines = true;
    style.AntiAliasedLinesUseTex = true;

    // Purple color palette
    // bg_bottom(17,15,35), bg_block(25,21,52), outline(60,53,93)
    const float accent_r = 140.f/255.f;
    const float accent_g = 90.f/255.f;
    const float accent_b = 190.f/255.f;

    colors[ImGuiCol_Text] = ImVec4( 0.96f , 0.96f , 0.96f , 1.00f );
    colors[ImGuiCol_TextDisabled] = ImVec4( 0.43f , 0.39f , 0.59f , 1.00f );

    // Window backgrounds - deep dark purple tones
    colors[ImGuiCol_WindowBg] = ImVec4( 17.f/255.f , 15.f/255.f , 35.f/255.f , 0.98f );
    colors[ImGuiCol_ChildBg] = ImVec4( 25.f/255.f , 21.f/255.f , 52.f/255.f , 1.00f );
    colors[ImGuiCol_PopupBg] = ImVec4( 25.f/255.f , 21.f/255.f , 52.f/255.f , 0.98f );

    // Borders - subtle purple outline
    colors[ImGuiCol_Border] = ImVec4( 60.f/255.f , 53.f/255.f , 93.f/255.f , 0.60f );
    colors[ImGuiCol_BorderShadow] = ImVec4( 0.00f , 0.00f , 0.00f , 0.00f );

    // Frame backgrounds - block overlay colors
    colors[ImGuiCol_FrameBg] = ImVec4( 33.f/255.f , 27.f/255.f , 69.f/255.f , 1.00f );
    colors[ImGuiCol_FrameBgHovered] = ImVec4( 45.f/255.f , 38.f/255.f , 85.f/255.f , 1.00f );
    colors[ImGuiCol_FrameBgActive] = ImVec4( accent_r , accent_g , accent_b , 0.45f );

    // Title bars
    colors[ImGuiCol_TitleBg] = ImVec4( 17.f/255.f , 15.f/255.f , 35.f/255.f , 1.00f );
    colors[ImGuiCol_TitleBgActive] = ImVec4( 25.f/255.f , 21.f/255.f , 52.f/255.f , 1.00f );
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4( 17.f/255.f , 15.f/255.f , 35.f/255.f , 0.75f );

    colors[ImGuiCol_MenuBarBg] = ImVec4( 17.f/255.f , 15.f/255.f , 35.f/255.f , 1.00f );

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = ImVec4( 17.f/255.f , 15.f/255.f , 35.f/255.f , 0.50f );
    colors[ImGuiCol_ScrollbarGrab] = ImVec4( 60.f/255.f , 53.f/255.f , 93.f/255.f , 1.00f );
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4( accent_r , accent_g , accent_b , 0.70f );
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4( accent_r , accent_g , accent_b , 1.00f );

    // Checkmark - TRANSPARENT (we draw our own custom checkmark)
    colors[ImGuiCol_CheckMark] = ImVec4( 0.0f , 0.0f , 0.0f , 0.0f );

    // Sliders
    colors[ImGuiCol_SliderGrab] = ImVec4( accent_r , accent_g , accent_b , 0.80f );
    colors[ImGuiCol_SliderGrabActive] = ImVec4( accent_r , accent_g , accent_b , 1.00f );

    // Buttons
    colors[ImGuiCol_Button] = ImVec4( 33.f/255.f , 27.f/255.f , 69.f/255.f , 1.00f );
    colors[ImGuiCol_ButtonHovered] = ImVec4( accent_r , accent_g , accent_b , 0.65f );
    colors[ImGuiCol_ButtonActive] = ImVec4( accent_r , accent_g , accent_b , 1.00f );

    // Headers (collapsing headers, tree nodes)
    colors[ImGuiCol_Header] = ImVec4( 33.f/255.f , 27.f/255.f , 69.f/255.f , 1.00f );
    colors[ImGuiCol_HeaderHovered] = ImVec4( accent_r , accent_g , accent_b , 0.50f );
    colors[ImGuiCol_HeaderActive] = ImVec4( accent_r , accent_g , accent_b , 0.80f );

    // Separators - invisible
    colors[ImGuiCol_Separator] = ImVec4( 60.f/255.f , 53.f/255.f , 93.f/255.f , 0.30f );
    colors[ImGuiCol_SeparatorHovered] = ImVec4( accent_r , accent_g , accent_b , 0.50f );
    colors[ImGuiCol_SeparatorActive] = ImVec4( accent_r , accent_g , accent_b , 1.00f );

    // Resize grip
    colors[ImGuiCol_ResizeGrip] = ImVec4( 60.f/255.f , 53.f/255.f , 93.f/255.f , 0.25f );
    colors[ImGuiCol_ResizeGripHovered] = ImVec4( accent_r , accent_g , accent_b , 0.50f );
    colors[ImGuiCol_ResizeGripActive] = ImVec4( accent_r , accent_g , accent_b , 0.95f );

    // Tabs - transparent active tab for underline effect
    colors[ImGuiCol_Tab] = ImVec4( 33.f/255.f , 27.f/255.f , 69.f/255.f , 0.86f );
    colors[ImGuiCol_TabHovered] = ImVec4( accent_r , accent_g , accent_b , 0.50f );
    colors[ImGuiCol_TabSelected] = ImVec4( 0.0f , 0.0f , 0.0f , 0.0f );
    colors[ImGuiCol_TabActive] = ImVec4( 0.0f , 0.0f , 0.0f , 0.0f );
    colors[ImGuiCol_TabUnfocused] = ImVec4( 17.f/255.f , 15.f/255.f , 35.f/255.f , 0.97f );
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4( 25.f/255.f , 21.f/255.f , 52.f/255.f , 1.00f );

    // Plots
    colors[ImGuiCol_PlotLines] = ImVec4( 0.80f , 0.80f , 0.80f , 1.00f );
    colors[ImGuiCol_PlotLinesHovered] = ImVec4( accent_r , accent_g , accent_b , 1.00f );
    colors[ImGuiCol_PlotHistogram] = ImVec4( accent_r , accent_g , accent_b , 0.70f );
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4( accent_r , accent_g , accent_b , 1.00f );

    // Tables
    colors[ImGuiCol_TableHeaderBg] = ImVec4( 25.f/255.f , 21.f/255.f , 52.f/255.f , 1.00f );
    colors[ImGuiCol_TableBorderStrong] = ImVec4( 60.f/255.f , 53.f/255.f , 93.f/255.f , 1.00f );
    colors[ImGuiCol_TableBorderLight] = ImVec4( 60.f/255.f , 53.f/255.f , 93.f/255.f , 0.50f );
    colors[ImGuiCol_TableRowBg] = ImVec4( 0.00f , 0.00f , 0.00f , 0.00f );
    colors[ImGuiCol_TableRowBgAlt] = ImVec4( 1.00f , 1.00f , 1.00f , 0.03f );

    // Misc
    colors[ImGuiCol_TextSelectedBg] = ImVec4( accent_r , accent_g , accent_b , 0.35f );
    colors[ImGuiCol_DragDropTarget] = ImVec4( accent_r , accent_g , accent_b , 0.90f );
    colors[ImGuiCol_NavHighlight] = ImVec4( accent_r , accent_g , accent_b , 1.00f );
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4( 1.00f , 1.00f , 1.00f , 0.70f );
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4( 0.80f , 0.80f , 0.80f , 0.20f );
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4( 0.00f , 0.00f , 0.00f , 0.60f );
}

auto CAndromedaGUI::UpdateStyle() -> void
{
    ImGui::SetCurrentContext( m_pImGuiContext );
    SetNoturnalStyle();
}

auto CAndromedaGUI::SetExternalFonts( const unsigned char* montserrat_data , size_t montserrat_size , const unsigned char* fa_data , size_t fa_size , float size_px ) -> void
{
    m_pMontserratData = montserrat_data;
    m_uMontserratSize = montserrat_size;
    m_pFontAwesomeData = fa_data;
    m_uFontAwesomeSize = fa_size;
    m_fFontSizePx = size_px;
    InitFont();
}

auto CAndromedaGUI::LoadFontsFromBytes() -> void
{
    const unsigned char* montserrat = g_Montserrat_Regular;
    size_t montserrat_size = g_Montserrat_Regular_Size;
    const unsigned char* fa = g_FontAwesome_Solid;
    size_t fa_size = g_FontAwesome_Solid_Size;

#if HAS_JOSEFIN
    montserrat = josefin_sans.data();
    montserrat_size = josefin_sans.size();
#endif

    if ( (montserrat && montserrat_size) || (fa && fa_size) )
        SetExternalFonts( montserrat , montserrat_size , fa , fa_size , 18.0f );
}

bool CAndromedaGUI::FreeTypeBuild::PreNewFrame()
{
	if ( !WantRebuild )
		return false;

	ImFontAtlas* atlas = ImGui::GetIO().Fonts;

	for ( int n = 0; n < atlas->Sources.Size; n++ )
		( (ImFontConfig*)&atlas->Sources[n] )->RasterizerMultiply = RasterizerMultiply;

#ifdef IMGUI_ENABLE_FREETYPE
	if ( BuildMode == FontBuildMode::FreeType )
	{
		atlas->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();
		atlas->FontBuilderFlags = FreeTypeBuilderFlags;
	}
#endif

	atlas->Build();
	WantRebuild = false;

	return true;
}

auto CAndromedaGUI::ClearRenderTargetView() -> void
{
	if ( m_pRenderTargetView )
	{
		m_pRenderTargetView->Release();
		m_pRenderTargetView = nullptr;
	}
}

auto GetAndromedaGUI() -> CAndromedaGUI*
{
    return &g_AndromedaGUI;
}
