# Dungeon Dice Duelists - Asset Manifest

**Generated**: 2026-01-30
**Status**: Placeholder Assets Complete
**Total Assets**: 150+ files

---

## Directory Structure

```
assets/
├── sprites/
│   ├── units/          # Unit sprites (25 files)
│   ├── dice/           # Dice sprites (38 files)
│   ├── ui/             # UI elements (31 files)
│   ├── terrain/        # Terrain tiles (16 files)
│   ├── particles/      # Particle effects (24 files)
│   └── effects/        # Animation frames (8 files)
├── fonts/              # Font files (0 files - needs TTF)
└── data/               # JSON data files (2 files)
```

---

## Unit Sprites (24 classes + 1 spritesheet)

### Spritesheet
- `units_spritesheet.png` (384x256) - All units in 6x4 grid

### Individual Sprites (64x64 each)

#### Warrior Archetype (Red, Square)
- squire.png
- knight.png
- paladin.png
- champion.png

#### Mage Archetype (Blue, Diamond)
- apprentice.png
- sorcerer.png
- archmage.png
- mystic.png

#### Ranger Archetype (Green, Triangle)
- scout.png
- hunter.png
- sniper.png
- marksman.png

#### Rogue Archetype (Purple, Hexagon)
- thief.png
- assassin.png
- shadowblade.png
- phantom.png

#### Cleric Archetype (Yellow, Circle)
- acolyte.png
- priest.png
- bishop.png
- saint.png

#### Tank Archetype (Gray, Octagon)
- guard.png
- defender.png
- fortress.png
- juggernaut.png

---

## Dice Sprites (32 types + 6 faces + 1 spritesheet)

### Spritesheet
- `dice_spritesheet.png` (1024x512) - All dice in 8x4 grid

### Dice by Rarity (128x128 each)

#### Common (White border)
- warrior_d6.png
- mage_d6.png
- ranger_d6.png
- rogue_d6.png
- cleric_d6.png
- tank_d6.png

#### Uncommon (Green border)
- heavy_warrior.png
- heavy_mage.png
- heavy_ranger.png
- heavy_rogue.png
- balanced_die.png

#### Rare (Blue border)
- lucky_warrior.png
- lucky_mage.png
- lucky_ranger.png
- lucky_cleric.png
- cursed_die.png
- blessed_die.png
- loaded_die.png

#### Epic (Purple border)
- fire_die.png
- ice_die.png
- lightning_die.png
- earth_die.png
- holy_die.png
- shadow_die.png
- wild_die.png
- chaos_die.png

#### Legendary (Gold border)
- dragon_die.png
- phoenix_die.png
- titan_die.png
- celestial_die.png
- prismatic_die.png
- void_die.png

### Dice Face Sprites (64x64 each)
- face_1.png through face_6.png

---

## UI Sprites (31 files)

### Buttons (200x50, 3 states each = 12 files)
- button_primary_{normal, hover, pressed}.png (Blue)
- button_success_{normal, hover, pressed}.png (Green)
- button_danger_{normal, hover, pressed}.png (Red)
- button_warning_{normal, hover, pressed}.png (Yellow)

### Panels (3 sizes)
- panel_small.png (300x400)
- panel_medium.png (500x600)
- panel_large.png (800x600)

### Backgrounds (1280x720)
- background_arena.png (Stone floor with grid)
- background_menu.png (Gradient background)

### Icons (32x32 each)
- icon_heart.png (HP)
- icon_sword.png (Attack)
- icon_shield.png (Defense)
- icon_boot.png (Movement)
- icon_target.png (Range)
- icon_coin.png (Gold)

### Other UI Elements
- cursor.png (32x32 - Custom cursor)
- healthbar_100.png (Full health)
- healthbar_75.png (75% health)
- healthbar_50.png (50% health)
- healthbar_25.png (25% health)

---

## Terrain Sprites (15 types + 1 spritesheet)

### Spritesheet
- `terrain_spritesheet.png` (320x192) - All terrain in 5x3 grid

### Individual Tiles (64x64 each)
- normal.png (Brown stone)
- grass.png (Green with grass blades)
- water.png (Blue with waves)
- lava.png (Red-orange with cracks)
- ice.png (Light blue with crystals)
- forest.png (Dark green)
- desert.png (Tan/yellow)
- mountain.png (Gray rock)
- swamp.png (Murky green)
- void.png (Dark purple)
- sanctified.png (Glowing yellow-white)
- corrupted.png (Purple-black)
- wall.png (Gray with brick pattern)
- bridge.png (Brown wood)
- chasm.png (Black)

---

## Particle Effects (24 files, 3 sizes each)

### Effect Types (16, 32, 64 pixel versions)
- **fire_{16,32,64}.png** - Red-orange-yellow gradient
- **ice_{16,32,64}.png** - Light blue with sparkle
- **lightning_{16,32,64}.png** - Yellow-white electric
- **heal_{16,32,64}.png** - Green with cross
- **smoke_{16,32,64}.png** - Gray wisps
- **sparkle_{16,32,64}.png** - White star burst
- **blood_{16,32,64}.png** - Dark red splatter
- **poison_{16,32,64}.png** - Purple-green toxic

---

## Animation Frames (8 files)

### Explosion Animation (64x64 each)
- explosion_frame_0.png through explosion_frame_7.png
- 8 frames showing expanding explosion from yellow to red to fade

---

## Data Files (JSON)

### units.json
- Contains all 24 unit class definitions
- Base stats, abilities, promotion trees
- ~200 lines

### abilities.json
- Contains 30+ ability definitions
- Effects, cooldowns, costs
- ~150 lines

---

## Missing Assets (To Be Added)

### Critical Priority
- [ ] **Font file** (game_font.ttf) - REQUIRED for text rendering
- [ ] **Sound effects** (~40 sounds needed)
- [ ] **Music tracks** (8 tracks needed)

### High Priority
- [ ] **Enemy sprites** (25+ enemy types)
- [ ] **Boss sprites** (4 bosses with multiple forms)
- [ ] **Effect animations** (more VFX like shield, buff indicators)
- [ ] **Projectile sprites** (arrows, magic bolts, etc.)

### Medium Priority
- [ ] **UI element polish** (better buttons, borders, decorations)
- [ ] **Character portraits** (for story/dialogue)
- [ ] **Title screen logo**
- [ ] **Achievement icons** (40+ icons)

---

## Asset Usage in Code

### Loading Assets

The game expects assets in these locations:

```cpp
// Units
"assets/sprites/units/units_spritesheet.png"
"assets/sprites/units/{class_name}.png"

// Dice
"assets/sprites/dice/dice_spritesheet.png"
"assets/sprites/dice/{dice_type}.png"

// UI
"assets/sprites/ui/button_{type}_{state}.png"
"assets/sprites/ui/icon_{icon_name}.png"
"assets/sprites/ui/background_{scene}.png"

// Terrain
"assets/sprites/terrain/terrain_spritesheet.png"
"assets/sprites/terrain/{terrain_type}.png"

// Particles
"assets/sprites/particles/{effect}_{size}.png"

// Fonts
"assets/fonts/game_font.ttf"

// Data
"assets/data/units.json"
"assets/data/abilities.json"
```

### Asset Manager

The `ResourceManager` class handles loading:
```cpp
ResourceManager::LoadTexture("texture_id", "path/to/texture.png");
ResourceManager::LoadFont("font_id", "path/to/font.ttf", size);
ResourceManager::LoadJSON("data_id", "path/to/data.json");
```

---

## Generation Scripts

### Python Scripts (Included)
1. `generate_unit_sprites.py` - Creates all unit sprites
2. `generate_dice_sprites.py` - Creates all dice sprites
3. `generate_ui_sprites.py` - Creates all UI elements
4. `generate_terrain_particles.py` - Creates terrain and particles

### Running Scripts
```bash
cd DungeonDiceDuelists
python3 generate_unit_sprites.py
python3 generate_dice_sprites.py
python3 generate_ui_sprites.py
python3 generate_terrain_particles.py
```

---

## File Formats

- **Images**: PNG with transparency (RGBA)
- **Spritesheets**: Power-of-2 dimensions where possible
- **Fonts**: TrueType Font (.ttf)
- **Data**: JSON UTF-8

---

## Art Style Notes

### Color Palette
- **Warrior**: Red tones (180-255, 20-50, 20-50)
- **Mage**: Blue tones (20-50, 20-50, 180-255)
- **Ranger**: Green tones (20-50, 180-255, 20-50)
- **Rogue**: Purple tones (150-220, 20-50, 150-220)
- **Cleric**: Yellow tones (200-255, 200-255, 20-50)
- **Tank**: Gray tones (120-180, 120-180, 120-180)

### Rarity Colors
- **Common**: White/Gray (200, 200, 200)
- **Uncommon**: Green (100, 200, 100)
- **Rare**: Blue (100, 150, 255)
- **Epic**: Purple (200, 100, 200)
- **Legendary**: Gold (255, 215, 0)

### Visual Design
- Geometric shapes for unit differentiation
- Bold outlines for visibility
- High contrast for readability
- Consistent sizing across asset types

---

## Next Steps

1. ✅ Generate placeholder sprites (COMPLETE)
2. ⏭️ Add font file (step 47)
3. ⏭️ Generate sound effects (step 48)
4. ⏭️ Create music tracks (step 49)
5. ⏭️ Test build system (step 50)

---

*Last Updated: 2026-01-30*
*Step 46 Complete: Placeholder Assets Created*
