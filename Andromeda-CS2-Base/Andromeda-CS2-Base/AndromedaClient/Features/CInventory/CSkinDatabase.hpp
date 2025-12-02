#pragma once

#include <Common/Common.hpp>
#include <ImGui/imgui.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <GameClient/CL_ItemDefinition.hpp>

// Enum for skin wear levels
enum class ESkinWear : int
{
    FactoryNew = 0,
    MinimalWear,
    FieldTested,
    WellWorn,
    BattleScarred,
    Max
};

// Enum for skin rarity
enum class ESkinRarity : int
{
    Common = 0,      // Consumer Grade (White)
    Uncommon = 1,    // Industrial Grade (Light Blue)
    Rare = 2,        // Mil-Spec (Blue)
    Mythical = 3,    // Restricted (Purple)
    Legendary = 4,   // Classified (Pink)
    Ancient = 5,     // Covert (Red)
    Contraband = 6,  // Contraband (Orange) - Dragon Lore, etc.
    Max
};

// Skin data structure
struct SkinData_t
{
    int m_nPaintKitID = 0;
    std::string m_sName;
    std::string m_sDescription;
    ESkinRarity m_eRarity = ESkinRarity::Common;
    ImU32 m_uRarityColor = 0;
    float m_fMinFloat = 0.0f;
    float m_fMaxFloat = 1.0f;
    std::string m_sImageUrl;
    std::vector<int> m_vCompatibleWeapons; // Weapon DefIndex list

    // Constructor
    SkinData_t() = default;
    SkinData_t( int id , const std::string& name , const std::string& desc , ESkinRarity rarity )
        : m_nPaintKitID( id ) , m_sName( name ) , m_sDescription( desc ) , m_eRarity( rarity ) {}
};

// Weapon category for UI organization
enum class EWeaponCategory : int
{
    Pistols = 0,
    SMGs,
    Rifles,
    Shotguns,
    Snipers,
    MachineGuns,
    Knives,
    Gloves,
    Max
};

// Weapon data for skin changer
struct WeaponSkinData_t
{
    int m_nDefIndex = 0;
    std::string m_sName;
    std::string m_sIconName;
    EWeaponCategory m_eCategory = EWeaponCategory::Pistols;
    bool m_bCanHaveSkin = true;

    WeaponSkinData_t() = default;
    WeaponSkinData_t( int defIndex , const std::string& name , const std::string& iconName , EWeaponCategory category , bool canHaveSkin = true )
        : m_nDefIndex( defIndex ) , m_sName( name ) , m_sIconName( iconName ) , m_eCategory( category ) , m_bCanHaveSkin( canHaveSkin ) {}
};

// Skin Database class - manages all skins
class CSkinDatabase
{
public:
    static auto Get() -> CSkinDatabase*;

    // Initialize database from game memory
    auto Initialize() -> void;
    auto IsInitialized() const -> bool { return m_bInitialized; }

    // Get all skins
    auto GetAllSkins() const -> const std::vector<SkinData_t>& { return m_vSkins; }
    
    // Get skins for specific weapon
    auto GetSkinsForWeapon( int defIndex ) const -> std::vector<const SkinData_t*>;
    
    // Get skin by paint kit ID
    auto GetSkinByPaintKit( int paintKitID ) const -> const SkinData_t*;
    
    // Get all weapons
    auto GetAllWeapons() const -> const std::vector<WeaponSkinData_t>& { return m_vWeapons; }
    
    // Get weapons by category
    auto GetWeaponsByCategory( EWeaponCategory category ) const -> std::vector<const WeaponSkinData_t*>;
    
    // Get weapon by def index
    auto GetWeaponByDefIndex( int defIndex ) const -> const WeaponSkinData_t*;
    
    // Filter skins by search query
    auto FilterSkins( const std::vector<const SkinData_t*>& skins , const std::string& query ) const -> std::vector<const SkinData_t*>;
    
    // Get rarity color
    static auto GetRarityColor( ESkinRarity rarity ) -> ImU32;
    static auto GetRarityName( ESkinRarity rarity ) -> const char*;
    
    // Get wear name
    static auto GetWearName( ESkinWear wear ) -> const char*;
    static auto GetWearRange( ESkinWear wear ) -> std::pair<float , float>;

    auto UpdateFromApi(const std::vector<SkinData_t>& skins, const std::vector<WeaponSkinData_t>& weapons) -> void;

private:
    CSkinDatabase() = default;
    
    auto LoadSkinsFromGame() -> void;
    auto LoadWeaponsFromGame() -> void;
    auto BuildCompatibilityMap() -> void;
    
    // Static skin data - comprehensive CS2 skin database
    auto LoadStaticSkinDatabase() -> void;

private:
    bool m_bInitialized = false;
    std::vector<SkinData_t> m_vSkins;
    std::vector<WeaponSkinData_t> m_vWeapons;
    std::unordered_map<int , std::vector<int>> m_mWeaponToSkins; // DefIndex -> PaintKit IDs
    std::unordered_map<int , const SkinData_t*> m_mPaintKitToSkin;
    std::unordered_map<int , const WeaponSkinData_t*> m_mDefIndexToWeapon;
};

// Global accessor
auto GetSkinDatabase() -> CSkinDatabase*;
