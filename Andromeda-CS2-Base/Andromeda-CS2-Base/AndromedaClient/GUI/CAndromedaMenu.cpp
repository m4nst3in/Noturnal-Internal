#include "CAndromedaMenu.hpp"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>
#include <algorithm>
#include <vector>
#include <string>
#include <cmath>
#include <unordered_map>

#include <AndromedaClient/Settings/Settings.hpp>
#include <AndromedaClient/CAndromedaGUI.hpp>
#include <AndromedaClient/Features/CInventory/InventoryChanger.hpp>
#include <AndromedaClient/Features/CInventory/CSkinDatabase.hpp>
#include <AndromedaClient/Features/CInventory/SkinApiClient.hpp>
#include <AndromedaClient/Features/CInventory/SkinImageCache.hpp>

static CAndromedaMenu g_CAndromedaMenu{};
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

// ============================================================================
// Fatality-inspired color constants
// ============================================================================
namespace Theme
{
    // Accent color (purple)
    constexpr ImVec4 Accent       = ImVec4( 140.f/255.f , 90.f/255.f , 190.f/255.f , 1.00f );
    constexpr ImVec4 AccentDim    = ImVec4( 140.f/255.f , 90.f/255.f , 190.f/255.f , 0.65f );
    
    // Text colors
    constexpr ImVec4 Text         = ImVec4( 0.96f , 0.96f , 0.96f , 1.00f );
    constexpr ImVec4 TextMid      = ImVec4( 0.43f , 0.39f , 0.59f , 1.00f );
    constexpr ImVec4 TextDark     = ImVec4( 0.35f , 0.31f , 0.48f , 1.00f );
    
    // Background colors
    constexpr ImVec4 BgBottom     = ImVec4( 17.f/255.f , 15.f/255.f , 35.f/255.f , 1.00f );
    constexpr ImVec4 BgBlock      = ImVec4( 25.f/255.f , 21.f/255.f , 52.f/255.f , 1.00f );
    constexpr ImVec4 BgBlockLight = ImVec4( 33.f/255.f , 27.f/255.f , 69.f/255.f , 1.00f );
    
    // Outline colors
    constexpr ImVec4 Outline      = ImVec4( 60.f/255.f , 53.f/255.f , 93.f/255.f , 1.00f );
    constexpr ImVec4 OutlineLight = ImVec4( 60.f/255.f , 53.f/255.f , 93.f/255.f , 0.60f );
    
    inline ImU32 GetAccentU32( float alpha = 1.0f ) { return ImColor( Accent.x , Accent.y , Accent.z , Accent.w * alpha ); }
    inline ImU32 GetBgBlockU32() { return ImColor( BgBlock ); }
    inline ImU32 GetBgBottomU32() { return ImColor( BgBottom ); }
    inline ImU32 GetOutlineU32( float alpha = 1.0f ) { return ImColor( Outline.x , Outline.y , Outline.z , Outline.w * alpha ); }
    inline ImU32 GetTextU32() { return ImColor( Text ); }
    inline ImU32 GetTextMidU32() { return ImColor( TextMid ); }
    
    inline ImVec4 Lerp( const ImVec4& a , const ImVec4& b , float t )
    {
        return ImVec4( a.x + ( b.x - a.x ) * t , a.y + ( b.y - a.y ) * t , a.z + ( b.z - a.z ) * t , a.w + ( b.w - a.w ) * t );
    }
}

static inline const char* Utf8( const char8_t* s ) { return reinterpret_cast<const char*>( s ); }

// ============================================================================
// Animation helpers
// ============================================================================
static std::unordered_map<ImGuiID , float> g_AnimState;

static float GetAnimValue( ImGuiID id , bool active , float speed = 8.0f )
{
    float& val = g_AnimState[id];
    float dt = ImGui::GetIO().DeltaTime;
    float target = active ? 1.0f : 0.0f;
    val += ( target - val ) * ( std::min )( 1.0f , dt * speed );
    return val;
}

// ============================================================================
// Custom Checkbox - Compact Fatality style (no double checkmark)
// ============================================================================
static bool StyledCheckbox( const char* label , bool* v )
{
    ImGuiID id = ImGui::GetID( label );
    
    // Get animation state before drawing
    float anim = GetAnimValue( id , *v , 10.0f );
    float hover_anim = GetAnimValue( id + 999999 , false , 12.0f );
    
    // Calculate sizes - BIGGER checkbox
    float box_size = 18.0f;
    ImVec2 label_size = ImGui::CalcTextSize( label );
    float total_width = box_size + 8.0f + label_size.x;
    float height = ( std::max )( box_size , label_size.y );
    
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 box_min( pos.x , pos.y + ( height - box_size ) * 0.5f );
    ImVec2 box_max( pos.x + box_size , pos.y + ( height - box_size ) * 0.5f + box_size );
    
    // Invisible button for interaction
    ImGui::InvisibleButton( label , ImVec2( total_width , height ) );
    bool hovered = ImGui::IsItemHovered();
    bool clicked = ImGui::IsItemClicked();
    
    if ( clicked )
        *v = !( *v );
    
    hover_anim = GetAnimValue( id + 999999 , hovered , 12.0f );
    
    ImDrawList* dl = ImGui::GetWindowDrawList();
    
    // Background
    ImU32 bg_col = ImGui::ColorConvertFloat4ToU32( Theme::Lerp( Theme::BgBottom , Theme::Accent , anim * 0.2f ) );
    dl->AddRectFilled( box_min , box_max , bg_col , 3.0f );
    
    // Border
    ImU32 border_col = ImGui::ColorConvertFloat4ToU32( Theme::Lerp( Theme::Outline , Theme::Accent , ( std::max )( anim , hover_anim * 0.5f ) ) );
    dl->AddRect( box_min , box_max , border_col , 3.0f , 0 , 1.0f );
    
    // Checkmark - bigger
    if ( anim > 0.01f )
    {
        ImVec2 center( ( box_min.x + box_max.x ) * 0.5f , ( box_min.y + box_max.y ) * 0.5f );
        float scale = anim;
        
        ImVec2 p1( center.x - 5.0f * scale , center.y + 1.0f * scale );
        ImVec2 p2( center.x - 1.0f * scale , center.y + 4.5f * scale );
        ImVec2 p3( center.x + 5.5f * scale , center.y - 3.5f * scale );
        
        dl->AddLine( p1 , p2 , Theme::GetAccentU32( anim ) , 2.5f );
        dl->AddLine( p2 , p3 , Theme::GetAccentU32( anim ) , 2.5f );
    }
    
    // Label
    ImVec2 text_pos( pos.x + box_size + 8.0f , pos.y + ( height - label_size.y ) * 0.5f );
    dl->AddText( text_pos , Theme::GetTextU32() , label );
    
    return clicked;
}

// ============================================================================
// Custom Slider - Label on top, thin bar, circular grab
// ============================================================================
static bool StyledSlider( const char* label , float* v , float v_min , float v_max , const char* format = "%.1f" )
{
    ImGuiID id = ImGui::GetID( label );
    ImDrawList* dl = ImGui::GetWindowDrawList();
    
    // Calculate sizes
    ImVec2 label_size = ImGui::CalcTextSize( label );
    float slider_width = ImGui::CalcItemWidth();
    float slider_height = 4.0f;
    float grab_radius = 5.0f;
    
    // Format value text
    char value_buf[32];
    snprintf( value_buf , sizeof( value_buf ) , format , *v );
    ImVec2 value_size = ImGui::CalcTextSize( value_buf );
    
    // Total height: label row + gap + slider track + grab space
    float total_height = label_size.y + 6.0f + slider_height + grab_radius;
    
    ImVec2 pos = ImGui::GetCursorScreenPos();
    
    // Invisible button for interaction FIRST (reserves space)
    ImGui::InvisibleButton( label , ImVec2( slider_width , total_height ) );
    bool hovered = ImGui::IsItemHovered();
    bool active = ImGui::IsItemActive();
    
    // Now draw everything on top of the reserved area
    // Draw label on left
    dl->AddText( pos , Theme::GetTextU32() , label );
    
    // Draw value on right (same row as label)
    dl->AddText( ImVec2( pos.x + slider_width - value_size.x , pos.y ) , Theme::GetTextMidU32() , value_buf );
    
    // Slider track position (below the label)
    float track_y = pos.y + label_size.y + 6.0f + slider_height * 0.5f;
    ImVec2 track_min( pos.x , track_y - slider_height * 0.5f );
    ImVec2 track_max( pos.x + slider_width , track_y + slider_height * 0.5f );
    
    // Handle drag
    if ( active )
    {
        float mouse_x = ImGui::GetIO().MousePos.x;
        float t = ( mouse_x - track_min.x ) / ( track_max.x - track_min.x );
        t = t < 0.0f ? 0.0f : ( t > 1.0f ? 1.0f : t );
        *v = v_min + t * ( v_max - v_min );
    }
    
    float t = ( *v - v_min ) / ( v_max - v_min );
    float grab_x = track_min.x + t * ( track_max.x - track_min.x );
    
    // Draw track background
    dl->AddRectFilled( track_min , track_max , Theme::GetBgBottomU32() , 2.0f );
    
    // Draw filled portion
    if ( t > 0.01f )
    {
        ImVec2 fill_max( grab_x , track_max.y );
        dl->AddRectFilled( track_min , fill_max , Theme::GetAccentU32( 0.8f ) , 2.0f );
    }
    
    // Draw circular grab
    float hover_anim = GetAnimValue( id + 500000 , hovered || active , 12.0f );
    float effective_radius = grab_radius + hover_anim * 1.5f;
    dl->AddCircleFilled( ImVec2( grab_x , track_y ) , effective_radius , Theme::GetAccentU32() );
    
    // Outline on hover
    if ( hover_anim > 0.01f )
        dl->AddCircle( ImVec2( grab_x , track_y ) , effective_radius + 1.0f , IM_COL32( 255 , 255 , 255 , (int)( 60 * hover_anim ) ) , 0 , 1.5f );
    
    return active;
}

// ============================================================================
// Compact Color Edit - Same size as checkbox (18px)
// ============================================================================
static bool StyledColorEdit( const char* label , float col[4] )
{
    float box_size = 18.0f;
    ImVec2 label_size = ImGui::CalcTextSize( label );
    float total_width = box_size + 8.0f + label_size.x;
    float height = ( std::max )( box_size , label_size.y );
    
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 box_min( pos.x , pos.y + ( height - box_size ) * 0.5f );
    ImVec2 box_max( pos.x + box_size , pos.y + ( height - box_size ) * 0.5f + box_size );
    
    ImGui::PushID( label );
    
    // Invisible button for interaction area
    ImGui::InvisibleButton( "##colorarea" , ImVec2( total_width , height ) );
    bool hovered = ImGui::IsItemHovered();
    
    ImDrawList* dl = ImGui::GetWindowDrawList();
    
    // Draw color box
    ImU32 color_u32 = ImGui::ColorConvertFloat4ToU32( ImVec4( col[0] , col[1] , col[2] , col[3] ) );
    dl->AddRectFilled( box_min , box_max , color_u32 , 3.0f );
    
    // Border
    ImU32 border_col = hovered ? Theme::GetAccentU32() : Theme::GetOutlineU32();
    dl->AddRect( box_min , box_max , border_col , 3.0f , 0 , 1.0f );
    
    // Draw label
    ImVec2 text_pos( pos.x + box_size + 8.0f , pos.y + ( height - label_size.y ) * 0.5f );
    dl->AddText( text_pos , Theme::GetTextU32() , label );
    
    // Open popup on click
    bool changed = false;
    if ( ImGui::IsItemClicked() )
        ImGui::OpenPopup( "##colorpicker" );
    
    // Color picker popup
    if ( ImGui::BeginPopup( "##colorpicker" ) )
    {
        ImGui::PushStyleColor( ImGuiCol_FrameBg , Theme::BgBlockLight );
        changed = ImGui::ColorPicker4( "##picker" , col , 
            ImGuiColorEditFlags_AlphaBar |
            ImGuiColorEditFlags_AlphaPreview |
            ImGuiColorEditFlags_NoSidePreview |
            ImGuiColorEditFlags_NoSmallPreview );
        ImGui::PopStyleColor();
        ImGui::EndPopup();
    }
    
    ImGui::PopID();
    
    return changed;
}

// ============================================================================
// Custom Combo - Compact Fatality style dropdown
// ============================================================================
static bool StyledCombo( const char* label , int* current_item , const char* const items[] , int items_count )
{
    ImGui::PushStyleVar( ImGuiStyleVar_FramePadding , ImVec2( 4.0f , 2.0f ) );
    ImGui::PushStyleVar( ImGuiStyleVar_PopupRounding , 2.0f );
    ImGui::PushStyleColor( ImGuiCol_FrameBg , Theme::BgBlockLight );
    ImGui::PushStyleColor( ImGuiCol_PopupBg , Theme::BgBlock );
    ImGui::PushStyleColor( ImGuiCol_Border , Theme::Outline );
    ImGui::PushStyleColor( ImGuiCol_Header , Theme::AccentDim );
    ImGui::PushStyleColor( ImGuiCol_HeaderHovered , Theme::Accent );
    
    const char* preview = ( *current_item >= 0 && *current_item < items_count ) ? items[*current_item] : "";
    bool changed = false;
    
    if ( ImGui::BeginCombo( label , preview , ImGuiComboFlags_None ) )
    {
        for ( int i = 0; i < items_count; i++ )
        {
            bool is_selected = ( *current_item == i );
            
            if ( ImGui::Selectable( items[i] , is_selected ) )
            {
                *current_item = i;
                changed = true;
            }
            
            if ( is_selected )
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    
    ImGui::PopStyleColor( 5 );
    ImGui::PopStyleVar( 2 );
    return changed;
}

// ============================================================================
// GroupBox - Fatality style with centered title
// ============================================================================
struct GroupBoxContext
{
    ImVec2 rectMin;
    ImVec2 size;
    ImVec2 titleSize;
    std::string title;
    ImDrawList* dl;
    ImVec2 cursorStart;
};

static std::vector<GroupBoxContext> g_GroupBoxStack;

static bool BeginGroupBox( const char* id , const char* title , ImVec2 size = ImVec2( 0 , 0 ) )
{
    ImVec2 avail = ImGui::GetContentRegionAvail();
    if ( size.x <= 0.0f ) size.x = avail.x;
    if ( size.y <= 0.0f ) size.y = avail.y;
    
    ImVec2 rectMin = ImGui::GetCursorScreenPos();
    ImVec2 titleSize = ImGui::CalcTextSize( title );
    ImDrawList* dl = ImGui::GetWindowDrawList();
    
    GroupBoxContext ctx{ rectMin , size , titleSize , std::string( title ) , dl , ImGui::GetCursorPos() };
    g_GroupBoxStack.push_back( ctx );
    
    // Split channels for proper layering
    dl->ChannelsSplit( 2 );
    dl->ChannelsSetCurrent( 1 );
    
    // Content area starts below title - more compact
    float titleAreaHeight = titleSize.y + 6.0f;
    ImGui::SetCursorPosY( ImGui::GetCursorPosY() + titleAreaHeight );
    ImGui::Indent( 6.0f );
    ImGui::BeginGroup();
    
    // Good spacing between items
    ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing , ImVec2( 4.0f , 8.0f ) );
    
    return true;
}

static void EndGroupBox()
{
    if ( g_GroupBoxStack.empty() ) return;
    
    ImGui::PopStyleVar();
    ImGui::EndGroup();
    ImGui::Unindent( 6.0f );
    
    GroupBoxContext ctx = g_GroupBoxStack.back();
    g_GroupBoxStack.pop_back();
    
    // Calculate actual height - compact padding
    float contentBottom = ImGui::GetCursorScreenPos().y;
    float height = ( std::max )( ctx.size.y , contentBottom - ctx.rectMin.y + 6.0f );
    ImVec2 rectMax( ctx.rectMin.x + ctx.size.x , ctx.rectMin.y + height );
    
    // Draw background on lower channel
    ctx.dl->ChannelsSetCurrent( 0 );
    
    // Main background
    ctx.dl->AddRectFilled( ctx.rectMin , rectMax , Theme::GetBgBlockU32() , 4.0f );
    
    // Subtle border
    ctx.dl->AddRect( ctx.rectMin , rectMax , Theme::GetOutlineU32( 0.6f ) , 4.0f , 0 , 1.0f );
    
    // Title text - compact (no background strip)
    ImVec2 textPos( ctx.rectMin.x + ( ctx.size.x - ctx.titleSize.x ) * 0.5f , ctx.rectMin.y + 3.0f );
    ctx.dl->AddText( textPos , Theme::GetTextU32() , ctx.title.c_str() );
    
    ctx.dl->ChannelsMerge();
    
    // Minimal spacing after groupbox
    ImGui::Dummy( ImVec2( 0 , 4.0f ) );
}

// ============================================================================
// Tab underline animation state
// ============================================================================
static float g_TabUnderX = 0.0f , g_TabUnderW = 0.0f;
static float g_TabTargetX = 0.0f , g_TabTargetW = 0.0f;
static float g_TabStartX = 0.0f , g_TabStartW = 0.0f;
static float g_TabAnimT = 1.0f;
static float g_TabBaseY = 0.0f;

static void UpdateTabAnimation( ImVec2 tabMin , ImVec2 tabMax )
{
    float newX = tabMin.x;
    float newW = tabMax.x - tabMin.x;
    
    if ( newX != g_TabTargetX || newW != g_TabTargetW )
    {
        g_TabStartX = g_TabUnderX;
        g_TabStartW = g_TabUnderW;
        g_TabAnimT = 0.0f;
    }
    
    g_TabTargetX = newX;
    g_TabTargetW = newW;
}

static void RenderTabUnderline()
{
    float dt = ImGui::GetIO().DeltaTime;
    g_TabAnimT = ( std::min )( g_TabAnimT + dt * 6.0f , 1.0f );
    
    // Ease out cubic
    float ease = 1.0f - powf( 1.0f - g_TabAnimT , 3.0f );
    
    g_TabUnderX = g_TabStartX + ( g_TabTargetX - g_TabStartX ) * ease;
    g_TabUnderW = g_TabStartW + ( g_TabTargetW - g_TabStartW ) * ease;
    
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled( 
        ImVec2( g_TabUnderX , g_TabBaseY - 4.0f ) , 
        ImVec2( g_TabUnderX + g_TabUnderW , g_TabBaseY - 1.0f ) , 
        Theme::GetAccentU32() , 
        1.5f 
    );
}

auto CAndromedaMenu::OnRenderMenu() -> void
{
    static float s_fade = 0.0f;
    if ( !GetAndromedaGUI()->IsVisible() ) s_fade = 0.0f;
    float delta_fade = s_fade + ImGui::GetIO().DeltaTime * 4.0f;
    s_fade = delta_fade < 0.0f ? 0.0f : ( delta_fade > 1.0f ? 1.0f : delta_fade );
    
    ImGui::PushStyleVar( ImGuiStyleVar_Alpha , s_fade );
    ImGui::SetNextWindowSize( ImVec2( 848 , 588 ) , ImGuiCond_Always );
    
    const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;
    
    if ( ImGui::Begin( XorStr( CHEAT_NAME ) , 0 , windowFlags ) )
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();
        
        // Draw accent line at top of window
        dl->AddRectFilled( windowPos , ImVec2( windowPos.x + windowSize.x , windowPos.y + 2.0f ) , Theme::GetAccentU32() );
        
        // Title with gradient effect
        const char* title = "Noturnal";
        ImFont* font = GetAndromedaGUI()->GetTitleFont() ? GetAndromedaGUI()->GetTitleFont() : ImGui::GetFont();
        float titleSizePx = font->FontSize;
        ImVec2 cursorStart = ImGui::GetCursorScreenPos();
        
        // Animated gradient title
        float tt = fmodf( (float)ImGui::GetTime() * 0.5f , 1.0f );
        const char* p = title;
        float x = cursorStart.x;
        float total = font->CalcTextSizeA( titleSizePx , FLT_MAX , 0.0f , title , nullptr , nullptr ).x;
        
        while ( *p )
        {
            const char* chStart = p;
            if ( (unsigned char)*p < 0x80 ) { p++; }
            else { p++; while ( *p && ((unsigned char)*p & 0xC0) == 0x80 ) p++; }
            
            ImVec2 chSize = font->CalcTextSizeA( titleSizePx , FLT_MAX , 0.0f , chStart , p , nullptr );
            float t = ( x - cursorStart.x ) / ( total > 0.f ? total : 1.f );
            
            // Gradient from accent to lighter
            ImVec4 colA = Theme::Accent;
            ImVec4 colB = ImVec4( 1.0f , 0.7f , 0.85f , 1.0f );
            ImVec4 ccol = ImLerp( colA , colB , t );
            
            dl->AddText( font , titleSizePx , ImVec2( x , cursorStart.y + 4.0f ) , ImGui::ColorConvertFloat4ToU32( ccol ) , chStart , p );
            x += chSize.x;
        }
        
        float titleAdvance = total;
        ImGui::SetCursorScreenPos( ImVec2( cursorStart.x + titleAdvance + 16.0f , cursorStart.y + 2.0f ) );
        
        // Tab bar styling
        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding , ImVec2( 14.0f , 10.0f ) );
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing , ImVec2( 4.0f , 0.0f ) );
        ImGui::PushStyleColor( ImGuiCol_Tab , ImVec4( 0 , 0 , 0 , 0 ) );
        ImGui::PushStyleColor( ImGuiCol_TabActive , ImVec4( 0 , 0 , 0 , 0 ) );
        ImGui::PushStyleColor( ImGuiCol_TabHovered , Theme::AccentDim );
        
        if ( ImGui::BeginTabBar( XorStr( "MainTabs" ) , ImGuiTabBarFlags_NoCloseWithMiddleMouseButton ) )
        {
            ImVec2 afterTabPos = ImGui::GetCursorScreenPos();
            g_TabBaseY = afterTabPos.y;
            
            // Cover the default tab line
            ImVec2 coverMin = ImVec2( windowPos.x , afterTabPos.y - 8.0f );
            ImVec2 coverMax = ImVec2( windowPos.x + windowSize.x , afterTabPos.y + 2.0f );
            dl->AddRectFilled( coverMin , coverMax , Theme::GetBgBottomU32() );
            
            // === RAGE TAB ===
            if ( ImGui::BeginTabItem( Utf8( u8"\uf06d  Rage" ) ) )
            {
                UpdateTabAnimation( ImGui::GetItemRectMin() , ImGui::GetItemRectMax() );
                ImGui::Dummy( ImVec2( 0 , 4.0f ) );
                
                BeginGroupBox( "rage_todo" , "RAGE" , ImVec2( 0 , 80 ) );
                ImGui::TextColored( Theme::TextMid , "TODO: Ragebot not implemented yet" );
                ImGui::TextColored( Theme::TextDark , "Features: Aimbot, Silent, Autofire, etc." );
                EndGroupBox();
                
                ImGui::EndTabItem();
            }
            
            // === LEGIT TAB ===
            if ( ImGui::BeginTabItem( Utf8( u8"\uf140  Legit" ) ) )
            {
                UpdateTabAnimation( ImGui::GetItemRectMin() , ImGui::GetItemRectMax() );
                ImGui::Dummy( ImVec2( 0 , 4.0f ) );
                
                ImGui::Columns( 2 , nullptr , false );
                ImGui::SetColumnWidth( 0 , 400.0f );
                
                // Triggerbot Group
                BeginGroupBox( "legit_triggerbot" , "TRIGGERBOT" , ImVec2( 0 , 280 ) );
                StyledCheckbox( "Enable" , &Settings::Legit::TriggerbotEnabled );
                
                // Hotkey selector - simplified combo
                const char* keyNames[] = { "Mouse Left" , "Mouse Right" , "Mouse Middle" , "Mouse 4" , "Mouse 5" , "Shift" , "Ctrl" , "Alt" };
                static int selectedKey = 0; // Default to Mouse Left
                if ( StyledCombo( "Hotkey" , &selectedKey , keyNames , IM_ARRAYSIZE( keyNames ) ) )
                {
                    // Map selected index to ImGuiKey
                    switch ( selectedKey )
                    {
                        case 0: Settings::Legit::TriggerbotKey = ImGuiKey_MouseLeft; break;
                        case 1: Settings::Legit::TriggerbotKey = ImGuiKey_MouseRight; break;
                        case 2: Settings::Legit::TriggerbotKey = ImGuiKey_MouseMiddle; break;
                        case 3: Settings::Legit::TriggerbotKey = ImGuiKey_MouseX1; break;
                        case 4: Settings::Legit::TriggerbotKey = ImGuiKey_MouseX2; break;
                        case 5: Settings::Legit::TriggerbotKey = ImGuiKey_LeftShift; break;
                        case 6: Settings::Legit::TriggerbotKey = ImGuiKey_LeftCtrl; break;
                        case 7: Settings::Legit::TriggerbotKey = ImGuiKey_LeftAlt; break;
                    }
                }
                
                ImGui::PushItemWidth( 160.0f );
                float delayFloat = static_cast<float>( Settings::Legit::TriggerbotDelayMs );
                if ( StyledSlider( "Delay" , &delayFloat , 0.f , 500.f , "%.0f ms" ) )
                    Settings::Legit::TriggerbotDelayMs = static_cast<int>( delayFloat );
                ImGui::PopItemWidth();
                
                StyledCheckbox( "Visible Only" , &Settings::Legit::TriggerbotVisibleOnly );
                StyledCheckbox( "Auto Scope" , &Settings::Legit::TriggerbotAutoScope );
                
                // Hitbox filter
                const char* hitboxFilters[] = { "All" , "Head Only" , "Body Only" };
                StyledCombo( "Hitbox Filter" , &Settings::Legit::TriggerbotHitboxFilter , hitboxFilters , IM_ARRAYSIZE( hitboxFilters ) );
                
                StyledCheckbox( "Burst Mode" , &Settings::Legit::TriggerbotBurstMode );
                
                if ( Settings::Legit::TriggerbotBurstMode )
                {
                    ImGui::PushItemWidth( 160.0f );
                    float burstFloat = static_cast<float>( Settings::Legit::TriggerbotBurstCount );
                    if ( StyledSlider( "Burst Count" , &burstFloat , 1.f , 10.f , "%.0f" ) )
                        Settings::Legit::TriggerbotBurstCount = static_cast<int>( burstFloat );
                    ImGui::PopItemWidth();
                }
                
                EndGroupBox();
                
                ImGui::NextColumn();
                
                // Autowall Group
                BeginGroupBox( "legit_autowall" , "AUTOWALL" , ImVec2( 0 , 120 ) );
                StyledCheckbox( "Enable" , &Settings::Legit::AutowallEnabled );
                
                ImGui::PushItemWidth( 160.0f );
                StyledSlider( "Min Damage" , &Settings::Legit::AutowallMinDamage , 1.f , 100.f , "%.0f HP" );
                ImGui::PopItemWidth();
                
                StyledCheckbox( "Friendly Fire" , &Settings::Legit::AutowallFriendlyFire );
                EndGroupBox();
                
                ImGui::Columns( 1 );
                ImGui::EndTabItem();
            }
            
            // === VISUALS TAB ===
            if ( ImGui::BeginTabItem( Utf8( u8"\uf06e  Visuals" ) ) )
            {
                UpdateTabAnimation( ImGui::GetItemRectMin() , ImGui::GetItemRectMax() );
                ImGui::Dummy( ImVec2( 0 , 4.0f ) );
                
                ImGui::Columns( 3 , nullptr , false );
                ImGui::SetColumnWidth( 0 , 270.0f );
                ImGui::SetColumnWidth( 1 , 270.0f );
                
                // ESP Group
                BeginGroupBox( "visuals_esp" , "ESP" , ImVec2( 0 , 180 ) );
                StyledCheckbox( "Enable" , &Settings::Visual::Active );
                StyledCheckbox( "Show Allies" , &Settings::Visual::Team );
                StyledCheckbox( "Show Enemies" , &Settings::Visual::Enemy );
                StyledCheckbox( "Show Box" , &Settings::Visual::PlayerBox );
                
                const char* boxTypes[] = { "Box" , "Outline Box" , "Coal Box" , "Outline Coal" };
                StyledCombo( "Box Type" , &Settings::Visual::PlayerBoxType , boxTypes , IM_ARRAYSIZE( boxTypes ) );
                
                const char* colorModes[] = { "Team Default" , "Custom" , "Gradient" , "RGB" };
                StyledCombo( "Color Mode" , &Settings::Visual::BoxColorMode , colorModes , IM_ARRAYSIZE( colorModes ) );
                
                if ( Settings::Visual::BoxColorMode == 1 )
                    StyledColorEdit( "Custom Color" , Settings::Visual::BoxCustomColor );
                else if ( Settings::Visual::BoxColorMode == 2 )
                {
                    StyledColorEdit( "Gradient Top" , Settings::Visual::BoxGradientTop );
                    StyledColorEdit( "Gradient Bottom" , Settings::Visual::BoxGradientBottom );
                }
                else if ( Settings::Visual::BoxColorMode == 3 )
                {
                    ImGui::PushItemWidth( 160.0f );
                    StyledSlider( "RGB Speed" , &Settings::Visual::BoxRGBSpeed , 0.1f , 5.0f , "%.2fx" );
                    ImGui::PopItemWidth();
                }
                
                StyledCheckbox( "Show Name" , &Settings::Visual::ShowName );
                StyledCheckbox( "Show Weapon" , &Settings::Visual::ShowWeapon );
                StyledCheckbox( "Show Dropped" , &Settings::Visual::ShowWorldWeapons );
                EndGroupBox();
                
                ImGui::NextColumn();
                
                // HUD Group
                BeginGroupBox( "visuals_hud" , "HUD" , ImVec2( 0 , 180 ) );
                StyledCheckbox( "Health Bar" , &Settings::Visual::ShowHealthBar );
                StyledColorEdit( "Health Top" , Settings::Visual::HealthGradientTop );
                StyledColorEdit( "Health Bottom" , Settings::Visual::HealthGradientBottom );
                
                ImGui::PushItemWidth( 160.0f );
                StyledSlider( "Health Width" , &Settings::Visual::HealthBarWidth , 1.0f , 6.0f , "%.1f px" );
                ImGui::PopItemWidth();
                
                StyledCheckbox( "Show Reload" , &Settings::Visual::ShowReload );
                StyledColorEdit( "Reload Color" , Settings::Visual::ReloadColor );
                StyledCheckbox( "Show Bomb (C4)" , &Settings::Visual::ShowBomb );
                StyledColorEdit( "Bomb Color" , Settings::Visual::BombColor );
                StyledCheckbox( "Show C4 Timer" , &Settings::Visual::ShowC4Timer );
                EndGroupBox();
                
                ImGui::NextColumn();
                
                // Team Colors Group
                BeginGroupBox( "visuals_colors" , "TEAM COLORS" , ImVec2( 0 , 100 ) );
                StyledColorEdit( "TT Box" , Settings::Colors::Visual::PlayerBoxTT );
                StyledColorEdit( "TT Visible" , Settings::Colors::Visual::PlayerBoxTT_Visible );
                StyledColorEdit( "CT Box" , Settings::Colors::Visual::PlayerBoxCT );
                StyledColorEdit( "CT Visible" , Settings::Colors::Visual::PlayerBoxCT_Visible );
                EndGroupBox();
                
                ImGui::Columns( 1 );
                
                // Chams row
                ImGui::Columns( 2 , nullptr , false );
                ImGui::SetColumnWidth( 0 , 400.0f );
                
                BeginGroupBox( "visuals_chams" , "CHAMS" , ImVec2( 0 , 110 ) );
                StyledCheckbox( "Enable Chams" , &Settings::Visual::ChamsEnabled );
                StyledCheckbox( "Glow" , &Settings::Visual::ChamsGlow );
                StyledCheckbox( "Ignore Z" , &Settings::Visual::ChamsIgnoreZ );
                StyledColorEdit( "Visible Color" , Settings::Visual::ChamsVisibleColor );
                StyledColorEdit( "Invisible Color" , Settings::Visual::ChamsInvisibleColor );
                StyledCheckbox( "Body Fill" , &Settings::Visual::ChamsOverlay );
                EndGroupBox();
                
                ImGui::NextColumn();
                
                BeginGroupBox( "visuals_viewmodel" , "VIEWMODEL CHAMS" , ImVec2( 0 , 60 ) );
                StyledCheckbox( "Enable Viewmodel" , &Settings::Visual::ViewmodelChamsEnabled );
                StyledColorEdit( "Viewmodel Color" , Settings::Visual::ViewmodelChamsColor );
                EndGroupBox();
                
                ImGui::Columns( 1 );
                ImGui::EndTabItem();
            }
            
            // === SKINS TAB ===
            if ( ImGui::BeginTabItem( Utf8( u8"\uf219  Skins" ) ) )
            {
                UpdateTabAnimation( ImGui::GetItemRectMin() , ImGui::GetItemRectMax() );
                ImGui::Dummy( ImVec2( 0 , 4.0f ) );
                static int team = 0; // 0=CT,1=T
                static int category = 0; // EWeaponCategory
                static int selectedDefIndex = -1;
                static char searchBuf[64] = {0};

                // Layout: 4 colunas para ocupar todo espaço e dividir DETAILS em dois
                float fullW = ImGui::GetContentRegionAvail().x;
                ImGui::Columns( 4 , nullptr , false );
                float col0W = 130.0f;
                float col1W = 260.0f;
                float remainingW = fullW - col0W - col1W;
                if ( remainingW < 200.0f ) remainingW = 200.0f;
                float col2W = remainingW * 0.50f;
                float col3W = remainingW - col2W;
                ImGui::SetColumnWidth( 0 , col0W );
                ImGui::SetColumnWidth( 1 , col1W );
                ImGui::SetColumnWidth( 2 , col2W );
                ImGui::SetColumnWidth( 3 , col3W );

                // === COLUNA 1: TEAM + WEAPONS ===
                BeginGroupBox( "skins_team" , "TEAM" , ImVec2( 0 , 50 ) );
                const char* teams[] = { "CT", "T" };
                ImGui::PushItemWidth( -1 );
                StyledCombo( "##Team" , &team , teams , IM_ARRAYSIZE( teams ) );
                ImGui::PopItemWidth();
                EndGroupBox();

                BeginGroupBox( "skins_weapons" , "WEAPONS" , ImVec2( 0 , 380 ) );
                const char* cats[] = { "Pistols", "SMGs", "Rifles", "Shotguns", "Snipers", "MGs", "Knives", "Gloves" };
                ImGui::PushItemWidth( -1 );
                StyledCombo( "##Category" , &category , cats , IM_ARRAYSIZE( cats ) );
                ImGui::PopItemWidth();

                auto isAllowedForTeam = [&]( int defIndex , bool isCT ) -> bool
                {
                    static const int ct_only[] = { 10 , 8 , 16 , 60 , 61 , 32 , 34 , 27 , 35 , 25 };
                    static const int t_only[]  = { 7 , 39 , 13 , 17 , 30 , 4 , 29 };
                    auto in = [&]( const int* arr , size_t n ) { for( size_t i=0;i<n;i++ ) if ( arr[i] == defIndex ) return true; return false; };
                    if ( isCT ) return !in( t_only , sizeof(t_only)/sizeof(t_only[0]) );
                    else        return !in( ct_only , sizeof(ct_only)/sizeof(ct_only[0]) );
                };

                auto catList = GetSkinDatabase()->GetWeaponsByCategory( (EWeaponCategory)category );
                float oneRowH = ImGui::GetTextLineHeightWithSpacing();
                ImGui::BeginChild( "weapon_list" , ImVec2( 0 , 300.0f - oneRowH ) , false );
                for ( auto* w : catList )
                {
                    if ( !isAllowedForTeam( w->m_nDefIndex , team == 0 ) ) continue;
                    bool sel = selectedDefIndex == w->m_nDefIndex;
                    if ( ImGui::Selectable( w->m_sName.c_str() , sel ) ) selectedDefIndex = w->m_nDefIndex;
                }
                ImGui::EndChild();
                EndGroupBox();

                ImGui::NextColumn();

                // === COLUNA 2: SKINS LIST ===
                BeginGroupBox( "skins_list" , "SKINS" , ImVec2( 0 , 440 ) );
                ImGui::PushItemWidth( -1 );
                ImGui::InputText( "##Search" , searchBuf , sizeof( searchBuf ) );
                ImGui::PopItemWidth();
                std::vector<const SkinData_t*> skins;
                if ( selectedDefIndex != -1 ) skins = GetSkinDatabase()->GetSkinsForWeapon( selectedDefIndex );
                auto filtered = GetSkinDatabase()->FilterSkins( skins , searchBuf );
                ImGui::BeginChild( "skins_child" , ImVec2( 0 , 380 ) , false );
                static int selectedPaintKit = 0;
                for ( auto* s : filtered )
                {
                    ImU32 col = s->m_uRarityColor ? s->m_uRarityColor : CSkinDatabase::GetRarityColor( s->m_eRarity );
                    ImGui::PushStyleColor( ImGuiCol_Text , col );
                    bool sel = selectedPaintKit == s->m_nPaintKitID;
                    if ( ImGui::Selectable( s->m_sName.c_str() , sel ) ) selectedPaintKit = s->m_nPaintKitID;
                    ImGui::PopStyleColor();
                }
                ImGui::EndChild();
                EndGroupBox();

                ImGui::NextColumn();

                // === COLUNA 3: DETAILS (controles) ===
                float detailsTopY = ImGui::GetCursorPosY();
                BeginGroupBox( "skins_details" , "DETAILS" , ImVec2( 0 , 440 ) );
                ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing , ImVec2( 4.0f , 4.0f ) );
                ImGui::PushStyleVar( ImGuiStyleVar_FramePadding , ImVec2( 6.0f , 3.0f ) );
                static SkinSelection current{};
                current.paintkit = selectedPaintKit;
                
                ImGui::PushItemWidth( 100.0f );
                StyledSlider( "Seed" , (float*)&current.seed , 0.f , 999.f , "%.0f" );
                float wear = current.wear; StyledSlider( "Wear" , &wear , 0.00f , 1.00f , "%.2f" ); current.wear = wear;
                StyledSlider( "StatTrak" , (float*)&current.stattrak , -1.f , 999.f , "%.0f" );
                ImGui::PopItemWidth();
                
                ImGui::PushItemWidth( 120.0f );
                char nameBuf[64] = {0};
                if ( !current.custom_name.empty() ) strncpy_s( nameBuf , sizeof( nameBuf ) , current.custom_name.c_str() , _TRUNCATE );
                ImGui::InputText( "##Custom" , nameBuf , sizeof( nameBuf ) );
                current.custom_name = nameBuf;
                ImGui::PopItemWidth();
                
                ImGui::Dummy( ImVec2( 0 , 4 ) );
                {
                    ImVec2 tsz = ImGui::CalcTextSize( "Apply Active" );
                    float bh = ImGui::GetFrameHeight();
                    ImVec2 bw( tsz.x + 16.0f , bh );
                    if ( ImGui::Button( "Apply Active" , bw ) && selectedDefIndex != -1 )
                    {
                        GetInventoryChanger()->SetSelectedTeamCT( team == 0 );
                        GetInventoryChanger()->SetSelection( selectedDefIndex , current );
                        GetInventoryChanger()->ApplyNowForActive();
                    }
                }
                ImGui::Dummy( ImVec2( 0 , 2 ) );
                {
                    ImVec2 tsz = ImGui::CalcTextSize( "Apply Loadout" );
                    float bh = ImGui::GetFrameHeight();
                    ImVec2 bw( tsz.x + 16.0f , bh );
                    if ( ImGui::Button( "Apply Loadout" , bw ) && selectedDefIndex != -1 )
                    {
                        bool ok = GetInventoryChanger()->ApplyToLoadout( selectedDefIndex , team == 0 , current );
                        ImGui::TextColored( ok ? Theme::TextMid : Theme::TextDark , ok ? "Equipped" : "Failed" );
                    }
                }
                
                {
                    ImVec2 tsz = ImGui::CalcTextSize( "Save" );
                    float bh = ImGui::GetFrameHeight();
                    ImVec2 bw( tsz.x + 16.0f , bh );
                    if ( ImGui::Button( "Save" , bw ) )
                    {
                        GetInventoryChanger()->SaveProfiles( "inventory_changer.json" );
                    }
                }
                ImGui::Dummy( ImVec2( 0 , 2 ) );
                {
                    ImVec2 tsz = ImGui::CalcTextSize( "Load" );
                    float bh = ImGui::GetFrameHeight();
                    ImVec2 bw( tsz.x + 16.0f , bh );
                    if ( ImGui::Button( "Load" , bw ) )
                    {
                        GetInventoryChanger()->LoadProfiles( "inventory_changer.json" );
                    }
                }
                // Preview e info foram movidos para a 4ª coluna
                ImGui::PopStyleVar(2);
                EndGroupBox();
                
                // === COLUNA 4: PREVIEW (preview/info) ===
                ImGui::NextColumn();
                ImGui::SetCursorPosY( detailsTopY );
                BeginGroupBox( "skins_details_preview" , "PREVIEW" , ImVec2( 0 , 440 ) );
                if ( selectedPaintKit > 0 )
                {
                    auto* sd = GetSkinDatabase()->GetSkinByPaintKit( selectedPaintKit );
                    if ( sd )
                    {
                        ImGui::TextColored( Theme::TextMid , "Rarity: %s" , CSkinDatabase::GetRarityName( sd->m_eRarity ) );
                        ImGui::TextColored( Theme::TextDark , "Float: %.2f - %.2f" , sd->m_fMinFloat , sd->m_fMaxFloat );
                        if ( !sd->m_sImageUrl.empty() )
                        {
                            SkinPreview prev{};
                            if ( GetSkinImageCache()->GetOrFetch( sd->m_nPaintKitID , sd->m_sImageUrl , prev ) && prev.srv )
                            {
                                float maxW = ImGui::GetContentRegionAvail().x - 10.0f;
                                float maxH = 220.0f;
                                float sx = ( prev.width  > 0 ) ? ( maxW / (float)prev.width  ) : 1.0f;
                                float sy = ( prev.height > 0 ) ? ( maxH / (float)prev.height ) : 1.0f;
                                float scale = sx < sy ? sx : sy;
                                ImGui::Image( (ImTextureID)prev.srv , ImVec2( prev.width * scale , prev.height * scale ) );
                            }
                        }
                    }
                }
                EndGroupBox();

                ImGui::Columns( 1 );
                ImGui::EndTabItem();
            }
            
            // === MISC TAB ===
            if ( ImGui::BeginTabItem( Utf8( u8"\uf7d9  Misc" ) ) )
            {
                UpdateTabAnimation( ImGui::GetItemRectMin() , ImGui::GetItemRectMax() );
                ImGui::Dummy( ImVec2( 0 , 4.0f ) );
                
                ImGui::Columns( 2 , nullptr , false );
                ImGui::SetColumnWidth( 0 , 400.0f );
                
                BeginGroupBox( "misc_camera" , "CAMERA" , ImVec2( 0 , 80 ) );
                StyledCheckbox( "Third Person" , &Settings::Visual::ThirdPerson );
                
                ImGui::PushItemWidth( 160.0f );
                StyledSlider( "Distance" , &Settings::Visual::ThirdPersonDistance , 60.f , 250.f , "%.0f u" );
                ImGui::PopItemWidth();
                EndGroupBox();
                
                ImGui::NextColumn();
                
                BeginGroupBox( "misc_todo" , "OTHER" , ImVec2( 0 , 80 ) );
                ImGui::TextColored( Theme::TextMid , "TODO: Misc features" );
                ImGui::TextColored( Theme::TextDark , "Bhop, Autostrafe, etc." );
                EndGroupBox();
                
                ImGui::Columns( 1 );
                ImGui::EndTabItem();
            }
            
            // Render animated tab underline
            
            ImGui::EndTabBar();
        }
        
        ImGui::PopStyleColor( 3 );
        ImGui::PopStyleVar( 2 );
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

auto GetAndromedaMenu() -> CAndromedaMenu*
{
    return &g_CAndromedaMenu;
}