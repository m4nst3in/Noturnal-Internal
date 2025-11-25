#include "CAndromedaMenu.hpp"

#include <ImGui/imgui.h>
#include <algorithm>
#include <vector>
#include <string>

#include <AndromedaClient/Settings/Settings.hpp>
#include <AndromedaClient/CAndromedaGUI.hpp>

static CAndromedaMenu g_CAndromedaMenu{};
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
static inline const char* Utf8( const char8_t* s ) { return reinterpret_cast<const char*>( s ); }
static float CalcItemWidth( const char* const* items , int count , int idx )
{
    if ( count <= 0 ) return 140.0f;
    int i = ( idx >= 0 && idx < count ) ? idx : 0;
    ImVec2 ts = ImGui::CalcTextSize( items[i] );
    return ts.x + 60.0f;
}

static bool AnimatedCheckbox( const char* label , bool* v )
{
    static std::unordered_map<ImGuiID,float> s_anim; const ImGuiID id = ImGui::GetID( label );
    bool changed = ImGui::Checkbox( label , v );
    float& a = s_anim[id]; float dt = ImGui::GetIO().DeltaTime; float speed = 6.0f;
    if ( *v ) a = std::min( 1.0f , a + dt * speed ); else a = 0.0f;
    ImVec2 min = ImGui::GetItemRectMin(); ImVec2 max = ImGui::GetItemRectMax();
    float h = max.y - min.y; float box = h; ImVec2 bmin = min; ImVec2 bmax = ImVec2( min.x + box , min.y + box );
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float inset = 3.0f;
    if ( *v && a < 0.999f )
    {
        ImVec2 coverMin = ImVec2( bmin.x + inset , bmin.y + inset );
        ImVec2 coverMax = ImVec2( bmin.x + inset + ( box - inset*2 ) * ( 1.0f - a ) , bmax.y - inset );
        ImU32 coverCol = ImGui::GetColorU32( ImGui::GetStyle().Colors[ ImGui::IsItemHovered() ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg ] );
        dl->AddRectFilled( coverMin , coverMax , coverCol , 2.0f );
    }
    return changed;
}

static bool ThinSliderFloat( const char* id , float* v , float v_min , float v_max , const char* format )
{
    ImGui::PushStyleVar( ImGuiStyleVar_FramePadding , ImVec2( 2.0f , 2.0f ) );
    ImGui::PushStyleVar( ImGuiStyleVar_GrabMinSize , 0.0f );
    ImGui::PushStyleVar( ImGuiStyleVar_GrabRounding , 100.0f );
    ImGui::PushStyleColor( ImGuiCol_SliderGrab , ImVec4( 0 , 0 , 0 , 0 ) );
    ImGui::PushStyleColor( ImGuiCol_SliderGrabActive , ImVec4( 0 , 0 , 0 , 0 ) );
    bool changed = ImGui::SliderFloat( id , v , v_min , v_max , format );
    ImVec2 rmin = ImGui::GetItemRectMin(); ImVec2 rmax = ImGui::GetItemRectMax();
    float frac = ( *v - v_min ) / ( v_max - v_min ); if ( frac < 0.f ) frac = 0.f; if ( frac > 1.f ) frac = 1.f;
    float padX = 6.0f;
    float cx = rmin.x + padX + frac * ( ( rmax.x - rmin.x ) - padX * 2.0f );
    float cy = ( rmin.y + rmax.y ) * 0.5f;
    float rad = 7.0f;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 knobCol = ImColor( 140 , 0 , 255 , 255 );
    dl->AddCircleFilled( ImVec2( cx , cy ) , rad , knobCol );
    dl->AddCircle( ImVec2( cx , cy ) , rad + 1.0f , ImColor( 255 , 255 , 255 , 60 ) );
    ImGui::PopStyleColor( 2 );
    ImGui::PopStyleVar( 3 );
    return changed;
}

struct GBContext { ImVec2 rectMin; float width; ImVec2 titleSize; std::string title; ImDrawList* dl; float rounding; };
static std::vector<GBContext> g_GBStack;

static bool BeginGroupBox( const char* id , const char* title , ImVec2 size )
{
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float width = size.x <= 0.0f ? avail.x : size.x;
    ImVec2 rectMin = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float rounding = 12.0f;

    GBContext ctx{ rectMin , width , ImGui::CalcTextSize( title ) , std::string( title ) , dl , rounding };
    g_GBStack.push_back( ctx );

    dl->ChannelsSplit( 2 );
    dl->ChannelsSetCurrent( 1 );

    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding , ImVec2( 18.0f , 18.0f ) );
    ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing , ImVec2( style.ItemSpacing.x , 8.0f ) );
    ImGui::PushStyleVar( ImGuiStyleVar_FramePadding , ImVec2( 3.0f , 3.0f ) );
    ImGui::PushStyleVar( ImGuiStyleVar_GrabMinSize , 6.0f );
    ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding , 3.0f );
    ImGui::PushStyleVar( ImGuiStyleVar_GrabRounding , 100.0f );
    ImGui::PushStyleVar( ImGuiStyleVar_PopupRounding , 6.0f );

    ImGui::Dummy( ImVec2( 0.0f , ctx.titleSize.y + 6.0f ) );
    ImGui::Indent( 12.0f );
    ImGui::BeginGroup();
    return true;
}

static void EndGroupBox()
{
    ImGui::EndGroup();
    GBContext ctx = g_GBStack.back();
    g_GBStack.pop_back();

    ImVec2 rectMax = ImGui::GetCursorScreenPos();
    rectMax.x = ctx.rectMin.x + ctx.width;
    rectMax.y += 24.0f; // bottom padding

    ImU32 bg = ImColor( 0 , 0 , 0 , 220 );
    ImU32 border = ImColor( 34 , 39 , 46 , 255 );
    ImU32 glow1 = ImColor( 140 , 0 , 255 , 60 );
    ImU32 glow2 = ImColor( 140 , 0 , 255 , 36 );
    ImU32 glow3 = ImColor( 140 , 0 , 255 , 18 );

    ctx.dl->ChannelsSetCurrent( 0 );
    ctx.dl->AddRectFilled( ctx.rectMin , rectMax , bg , ctx.rounding );
    ImVec2 g1min = ImVec2( ctx.rectMin.x - 2.0f , ctx.rectMin.y - 2.0f );
    ImVec2 g1max = ImVec2( rectMax.x + 2.0f , rectMax.y + 2.0f );
    ImVec2 g2min = ImVec2( ctx.rectMin.x - 5.0f , ctx.rectMin.y - 5.0f );
    ImVec2 g2max = ImVec2( rectMax.x + 5.0f , rectMax.y + 5.0f );
    ImVec2 g3min = ImVec2( ctx.rectMin.x - 8.0f , ctx.rectMin.y - 8.0f );
    ImVec2 g3max = ImVec2( rectMax.x + 8.0f , rectMax.y + 8.0f );
    ctx.dl->AddRect( g1min , g1max , glow1 , ctx.rounding + 2.0f , 0 , 2.0f );
    ctx.dl->AddRect( g2min , g2max , glow2 , ctx.rounding + 5.0f , 0 , 2.0f );
    ctx.dl->AddRect( g3min , g3max , glow3 , ctx.rounding + 8.0f , 0 , 1.0f );
    ctx.dl->AddRect( ctx.rectMin , rectMax , border , ctx.rounding , 0 , 2.0f );

    ImVec2 center = ImVec2( ctx.rectMin.x + ( ctx.width * 0.5f ) , ctx.rectMin.y + 12.0f );
    ImVec2 textPos = ImVec2( center.x - ctx.titleSize.x * 0.5f , center.y - ctx.titleSize.y * 0.5f );
    ctx.dl->AddText( textPos , ImColor( 240 , 240 , 255 , 255 ) , ctx.title.c_str() );

    ctx.dl->ChannelsMerge();
    ImGui::Unindent( 12.0f );
    ImGui::PopStyleVar(7);
}

auto CAndromedaMenu::OnRenderMenu() -> void
{
    static float s_fade = 0.0f;
    if ( !GetAndromedaGUI()->IsVisible() ) s_fade = 0.0f;
    s_fade = std::clamp( s_fade + ImGui::GetIO().DeltaTime * 3.0f , 0.0f , 1.0f );
    ImGui::PushStyleVar( ImGuiStyleVar_Alpha , s_fade );
    ImGui::SetNextWindowSize( ImVec2( 960 , 640 ) , ImGuiCond_Always );
    const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
    if ( ImGui::Begin( XorStr( CHEAT_NAME ) , 0 , windowFlags ) )
    {
        const char* title = "Noturnal";
        ImFont* font = GetAndromedaGUI()->GetTitleFont() ? GetAndromedaGUI()->GetTitleFont() : ImGui::GetFont();
        float titleSizePx = font->FontSize; 
        ImVec2 cursorStart = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImU32 colStart = ImColor( 140 , 0 , 255 , 255 );
        ImU32 colEnd = ImColor( 255 , 255 , 255 , 255 );
        const char* p = title;
        ImVec2 pos = ImVec2( floorf( cursorStart.x ) , floorf( cursorStart.y + 4.0f ) );
        float x = pos.x;
        float total = font->CalcTextSizeA( titleSizePx , FLT_MAX , 0.0f , title , nullptr , nullptr ).x;
        while ( *p )
        {
            const char* chStart = p;
            ImWchar c = (ImWchar)*p;
            if ( (unsigned char)c < 0x80 ) { p++; }
            else { p++; while ( (unsigned char)*p && ((unsigned char)*p & 0xC0) == 0x80 ) p++; }
            ImVec2 chSize = font->CalcTextSizeA( titleSizePx , FLT_MAX , 0.0f , chStart , p , nullptr );
            float t = ( x - pos.x ) / ( total > 0.f ? total : 1.f );
            ImVec4 a = ImGui::ColorConvertU32ToFloat4( colStart );
            ImVec4 b = ImGui::ColorConvertU32ToFloat4( colEnd );
            ImVec4 ccol = ImVec4( a.x + ( b.x - a.x ) * t , a.y + ( b.y - a.y ) * t , a.z + ( b.z - a.z ) * t , a.w + ( b.w - a.w ) * t );
            ImU32 col = ImGui::ColorConvertFloat4ToU32( ccol );
            dl->AddText( font , titleSizePx , ImVec2( floorf( x ) , pos.y ) , col , chStart , p );
            x += chSize.x;
        }
        float titleAdvance = total;
        ImGui::SetCursorScreenPos( ImVec2( cursorStart.x + titleAdvance + 8.0f , cursorStart.y + 1.0f ) );
        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding , ImVec2( 12.0f , 8.0f ) );
        ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize , 0.0f );
        if ( ImGui::BeginTabBar( XorStr( "MainTabs" ) , ImGuiTabBarFlags_NoCloseWithMiddleMouseButton ) )
        {
            ImVec2 afterTabPos = ImGui::GetCursorScreenPos();
            ImVec2 coverMin = ImVec2( afterTabPos.x , afterTabPos.y - 10.0f );
            ImVec2 coverMax = ImVec2( afterTabPos.x + ImGui::GetContentRegionAvail().x , afterTabPos.y + 2.0f );
            ImU32 coverCol = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_WindowBg] );
            ImGui::GetWindowDrawList()->AddRectFilled( coverMin , coverMax , coverCol );
            if ( ImGui::BeginTabItem( Utf8( u8"\uf06d  Rage" ) ) )
            {
                ImGui::SetCursorPosY( ImGui::GetCursorPosY() - 2.0f );
                ImGui::EndTabItem();
            }
            if ( ImGui::BeginTabItem( Utf8( u8"\uf140  Legit" ) ) )
            {
                ImGui::SetCursorPosY( ImGui::GetCursorPosY() - 2.0f );
                ImGui::EndTabItem();
            }
            if ( ImGui::BeginTabItem( Utf8( u8"\uf06e  Visuals" ) ) )
            {
                ImGui::SetCursorPosY( ImGui::GetCursorPosY() - 2.0f );
                ImGui::Columns( 3 , 0 , false );
                BeginGroupBox( XorStr( "Group_Visuals_ESP" ) , XorStr( "ESP" ) , ImVec2( 0 , 220 ) );
                AnimatedCheckbox( XorStr( "Enable" ) , &Settings::Visual::Active );
                AnimatedCheckbox( XorStr( "Show Allies" ) , &Settings::Visual::Team );
                AnimatedCheckbox( XorStr( "Show Enemies" ) , &Settings::Visual::Enemy );
                AnimatedCheckbox( XorStr( "Show Box" ) , &Settings::Visual::PlayerBox );
                {
                    const char* boxTypes[] = { "Box" , "Outline Box" , "Coal Box" , "Outline Coal Box" };
                    ImGui::PushItemWidth( CalcItemWidth( boxTypes , IM_ARRAYSIZE( boxTypes ) , Settings::Visual::PlayerBoxType ) );
                    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding , ImVec2( 4.0f , 4.0f ) );
                    ImGui::Combo( XorStr( "Box type" ) , &Settings::Visual::PlayerBoxType , boxTypes , IM_ARRAYSIZE( boxTypes ) );
                    ImGui::PopStyleVar();
                    ImGui::PopItemWidth();
                }
                {
                    const char* modes[] = { "Team default" , "Custom" , "Gradient" , "RGB" };
                    ImGui::PushItemWidth( CalcItemWidth( modes , IM_ARRAYSIZE( modes ) , Settings::Visual::BoxColorMode ) );
                    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding , ImVec2( 4.0f , 4.0f ) );
                    ImGui::Combo( XorStr( "Box color mode" ) , &Settings::Visual::BoxColorMode , modes , IM_ARRAYSIZE( modes ) );
                    ImGui::PopStyleVar();
                    ImGui::PopItemWidth();
                }
                if ( Settings::Visual::BoxColorMode == 1 )
                    ImGui::ColorEdit4( XorStr( "Custom color" ) , Settings::Visual::BoxCustomColor , ImGuiColorEditFlags_NoInputs );
                else if ( Settings::Visual::BoxColorMode == 2 )
                {
                    ImGui::ColorEdit4( XorStr( "Top" ) , Settings::Visual::BoxGradientTop , ImGuiColorEditFlags_NoInputs );
                    ImGui::ColorEdit4( XorStr( "Bottom" ) , Settings::Visual::BoxGradientBottom , ImGuiColorEditFlags_NoInputs );
                }
                else if ( Settings::Visual::BoxColorMode == 3 )
                    ThinSliderFloat( XorStr( "##BoxRGBSpeed" ) , &Settings::Visual::BoxRGBSpeed , 0.1f , 5.0f , XorStr( "%.2fx" ) );
                AnimatedCheckbox( XorStr( "Show Name" ) , &Settings::Visual::ShowName );
                {
                    const char* posOpts[] = { "Above" , "Below" };
                    ImGui::PushItemWidth( CalcItemWidth( posOpts , IM_ARRAYSIZE( posOpts ) , Settings::Visual::NamePosition ) );
                    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding , ImVec2( 4.0f , 4.0f ) );
                    ImGui::Combo( XorStr( "Name position" ) , &Settings::Visual::NamePosition , posOpts , IM_ARRAYSIZE( posOpts ) );
                    ImGui::PopStyleVar();
                    ImGui::PopItemWidth();
                }
                
                AnimatedCheckbox( XorStr( "Show Weapon" ) , &Settings::Visual::ShowWeapon );
                AnimatedCheckbox( XorStr( "Show Dropped" ) , &Settings::Visual::ShowWorldWeapons );
                AnimatedCheckbox( XorStr( "Show Dropped" ) , &Settings::Visual::ShowWorldWeapons );
                AnimatedCheckbox( XorStr( "Show Defuser Icon" ) , &Settings::Visual::ShowDefuserIcon );
                {
                    const char* weaponLabelOpts[] = { "Text" , "Icon" };
                    ImGui::PushItemWidth( CalcItemWidth( weaponLabelOpts , IM_ARRAYSIZE( weaponLabelOpts ) , Settings::Visual::WeaponLabelMode ) );
                    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding , ImVec2( 4.0f , 4.0f ) );
                    ImGui::Combo( XorStr( "Weapon display" ) , &Settings::Visual::WeaponLabelMode , weaponLabelOpts , IM_ARRAYSIZE( weaponLabelOpts ) );
                    ImGui::PopStyleVar();
                    ImGui::PopItemWidth();
                }
                {
                    const char* posOpts[] = { "Above" , "Below" };
                    ImGui::PushItemWidth( CalcItemWidth( posOpts , IM_ARRAYSIZE( posOpts ) , Settings::Visual::WeaponPosition ) );
                    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding , ImVec2( 4.0f , 4.0f ) );
                    ImGui::Combo( XorStr( "Weapon position" ) , &Settings::Visual::WeaponPosition , posOpts , IM_ARRAYSIZE( posOpts ) );
                    ImGui::PopStyleVar();
                    ImGui::PopItemWidth();
                }
                
                EndGroupBox();
                ImGui::NextColumn();
                BeginGroupBox( XorStr( "Group_Visuals_2" ) , XorStr( "HUD" ) , ImVec2( 0 , 220 ) );
                AnimatedCheckbox( XorStr( "Show Health Bar" ) , &Settings::Visual::ShowHealthBar );
                ImGui::ColorEdit4( XorStr( "Health top" ) , Settings::Visual::HealthGradientTop , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "Health bottom" ) , Settings::Visual::HealthGradientBottom , ImGuiColorEditFlags_NoInputs );
                ImGui::Text( XorStr( "Health width" ) );
                ThinSliderFloat( XorStr( "##HealthWidth" ) , &Settings::Visual::HealthBarWidth , 1.0f , 6.0f , XorStr( "%.1f px" ) );
                AnimatedCheckbox( XorStr( "Show Reload" ) , &Settings::Visual::ShowReload );
                ImGui::ColorEdit4( XorStr( "Reload color" ) , Settings::Visual::ReloadColor , ImGuiColorEditFlags_NoInputs );
                AnimatedCheckbox( XorStr( "Show Bomb (C4)" ) , &Settings::Visual::ShowBomb );
                ImGui::ColorEdit4( XorStr( "Bomb color" ) , Settings::Visual::BombColor , ImGuiColorEditFlags_NoInputs );
                AnimatedCheckbox( XorStr( "Show C4 Timer" ) , &Settings::Visual::ShowC4Timer );
                
                
                EndGroupBox();
                ImGui::NextColumn();
                BeginGroupBox( XorStr( "Group_Visuals_3" ) , XorStr( "Team Colors" ) , ImVec2( 0 , 220 ) );
                ImGui::ColorEdit4( XorStr( "TT box" ) , Settings::Colors::Visual::PlayerBoxTT , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "TT visible" ) , Settings::Colors::Visual::PlayerBoxTT_Visible , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "CT box" ) , Settings::Colors::Visual::PlayerBoxCT , ImGuiColorEditFlags_NoInputs );
                ImGui::ColorEdit4( XorStr( "CT visible" ) , Settings::Colors::Visual::PlayerBoxCT_Visible , ImGuiColorEditFlags_NoInputs );
                EndGroupBox();
                ImGui::Columns( 1 );
                ImGui::EndTabItem();
            }
            if ( ImGui::BeginTabItem( Utf8( u8"\uf4fe  Inventory" ) ) )
            {
                ImGui::SetCursorPosY( ImGui::GetCursorPosY() - 2.0f );
                ImGui::EndTabItem();
            }
            if ( ImGui::BeginTabItem( Utf8( u8"\uf7d9  Misc" ) ) )
            {
                ImGui::Columns( 1 , 0 , false );
                BeginGroupBox( XorStr( "Group_Misc_2" ) , XorStr( "Misc" ) , ImVec2( 0 , 160 ) );
                AnimatedCheckbox( XorStr( "Third Person" ) , &Settings::Visual::ThirdPerson );
                EndGroupBox();
                ImGui::Columns( 1 );
                ImGui::EndTabItem();
            }
            if ( ImGui::BeginTabItem( Utf8( u8"\uf013  Settings" ) ) )
            {
                ImGui::SetCursorPosY( ImGui::GetCursorPosY() - 2.0f );
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
            ImGui::PopStyleVar(2);
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

auto GetAndromedaMenu() -> CAndromedaMenu*
{
    return &g_CAndromedaMenu;
}