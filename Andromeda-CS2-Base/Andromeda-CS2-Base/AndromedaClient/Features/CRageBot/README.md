# Rage Bot System

Complete implementation of a Rage Bot for CS2 with auto-fire, target selection, multipoint, and all advanced features.

## Features

### 1. Auto-Fire System
- Automatically fires when a valid target is found
- No need to manually aim or pull trigger
- Integrated with weapon ready checks

### 2. Target Selection
- **FOV-based targeting**: Only considers targets within configured FOV
- **Distance filtering**: Max distance configuration
- **Priority modes**:
  - FOV: Closest to crosshair
  - Distance: Nearest target
  - HP: Lowest health first
  - Threat: Targets aiming at you (simplified implementation)
- **Team filtering**: Automatically ignores teammates

### 3. Multipoint System
- Generates multiple aim points per hitbox for better hit probability
- **Supported hitboxes**:
  - Head (6 points: center, left, right, forward, top)
  - Chest (5 points: center, left, right, upper, lower)
  - Pelvis (3 points: center, left, right)
- **Configurable scale** (0-100%): Adjusts spread of multipoints
- **Safe points**: More conservative points when target is stationary
- **Fallback to center** when multipoint is disabled

### 4. Hitchance System
- Per-weapon hitchance configuration
- Separate values for visible targets and wallbangs
- **Adaptive hitchance**:
  - Reduces for fast-moving targets
  - Increases for stationary targets
  - Distance-based adjustments
- **6 weapon groups** with individual configs:
  - Pistol
  - Heavy Pistol (Deagle, R8)
  - SMG
  - Rifle
  - Sniper (AWP, SSG08)
  - Auto-Sniper (G3SG1, SCAR-20)

### 5. Min Damage System
- Configurable minimum damage per weapon
- Separate values for visible/wallbang
- **Damage override**: Force higher damage with keybind (configurable)
- Ensures shots only taken when sufficient damage can be dealt

### 6. No Spread / Recoil Compensation
- Removes weapon spread for perfect accuracy
- Always aims at exact point
- Can be toggled on/off

### 7. Auto-Scope
- Automatically scopes when using sniper rifles
- Configurable per weapon group
- Only triggers when necessary

### 8. Auto-Stop System
- Stops player movement for better accuracy
- **Multiple modes**:
  - Full: Complete stop
  - Quick: Counter-strafing for rapid stops
  - Min Speed: Stop to minimum velocity for accuracy
  - Predictive: Predicts when target will stop
- Configurable per weapon

### 9. Silent Aim
- Doesn't move the player's camera/view
- Sets aim angles directly in user commands
- Invisible to spectators (server-side only)

## Configuration

All settings are exposed via the `Settings::RageBot` namespace:

### General Settings
```cpp
Settings::RageBot::Enabled = true;           // Enable/disable rage bot
Settings::RageBot::SilentAim = true;        // Silent aim mode
Settings::RageBot::AutoFire = true;         // Auto-fire on target
Settings::RageBot::AutoScope = true;        // Auto-scope for snipers
Settings::RageBot::NoSpread = true;         // Remove spread
Settings::RageBot::Penetration = true;      // Allow wallbangs
Settings::RageBot::MaxFOV = 180.0f;         // Maximum FOV
Settings::RageBot::MaxDistance = 8192.0f;   // Maximum distance
Settings::RageBot::DelayShot = 0;           // Delay between shots (ms)
```

### Target Priority
```cpp
Settings::RageBot::TargetPriority = 0;  // 0=FOV, 1=Distance, 2=HP, 3=Threat
```

### Multipoint Settings
```cpp
Settings::RageBot::MultipointEnabled = true;
Settings::RageBot::MultipointScale = 75.0f;     // 0-100%
Settings::RageBot::SafePoints = true;
Settings::RageBot::SafePointOnLimbs = false;
Settings::RageBot::BodyAimFallback = true;
```

### Hitbox Selection
```cpp
Settings::RageBot::TargetHead = true;
Settings::RageBot::TargetChest = true;
Settings::RageBot::TargetPelvis = false;
```

### Per-Weapon Configuration Example (Rifle)
```cpp
Settings::RageBot::RifleMinDamage = 30;             // Min damage
Settings::RageBot::RifleMinDamageVisible = 40;      // Min damage when visible
Settings::RageBot::RifleHitchance = 55;             // Hitchance %
Settings::RageBot::RifleHitchanceVisible = 65;      // Hitchance % when visible
Settings::RageBot::RifleAutoScope = false;          // Auto-scope
Settings::RageBot::RifleAutoStop = true;            // Auto-stop
```

### Overrides
```cpp
Settings::RageBot::DamageOverrideValue = 100;       // Override damage value
Settings::RageBot::DamageOverrideActive = false;    // Enable damage override
Settings::RageBot::SafePointOverrideActive = false; // Force safe points
```

## Integration

The rage bot is automatically integrated into the CreateMove hook:

```cpp
// In CAndromedaClient::OnCreateMove
GetRageBot()->OnCreateMove( pInput , pUserCmd );
```

## Usage

1. Enable the rage bot: `Settings::RageBot::Enabled = true`
2. Configure your preferences (FOV, hitboxes, per-weapon settings)
3. The rage bot will automatically:
   - Scan for targets in your FOV
   - Select the best target based on priority
   - Generate multipoints on the target
   - Calculate hitchance and damage
   - Stop movement if needed
   - Scope if needed
   - Aim at the best point
   - Fire automatically

## Technical Details

### Files
- `Config.hpp`: Configuration structures and enums
- `RageBot.hpp`: Main class declaration
- `RageBot.cpp`: Implementation (~500+ lines)
- `CRageBot.hpp`: Convenience header

### Dependencies
- CS2 SDK (entities, weapons, math)
- GameClient utilities (bones, visibility, players, weapons)
- AndromedaClient settings system

### Performance
- Executes every tick in CreateMove
- Uses entity cache for efficient iteration
- Minimal overhead when disabled
- Lock-based thread safety for entity access

## Notes

- This is a **minimal implementation** focused on core functionality
- Some advanced features (like backtracking, advanced damage calculation) are simplified
- The system is designed to be modular and easily extensible
- All calculations consider game tick/latency
- Safe points are prioritized for stationary targets

## Future Enhancements

Potential areas for improvement:
- Advanced damage calculation with penetration physics
- Backtracking system for lag compensation
- More sophisticated threat detection
- Animated hitbox prediction
- Advanced spread pattern compensation
- Per-situation auto-stop mode selection
- History-based adaptive hitchance
- Advanced body aim conditions
