# Dungeon Dice Duelists

A tactical roguelike strategy game combining Fire Emblem-style tactical grid combat, Slay the Spire-inspired roguelike progression, and dice-based unit summoning mechanics.

## Features

- **Tactical Combat**: 13x19 grid battlefield with territory control
- **Dice Mechanics**: Each unit is represented by a 6-sided die with different stat configurations
- **Class System**: 39 unique classes across 4 tiers with branching promotion paths
- **Roguelike Progression**: Wave-based gameplay with permanent progression
- **Steam Integration**: Achievements, leaderboards, and cloud saves

## Building

### Prerequisites

- CMake 3.15+
- C++17 compatible compiler (GCC 8+, Clang 7+, MSVC 2019+)
- SDL2, SDL2_image, SDL2_ttf, SDL2_mixer

### Linux (Ubuntu/Debian)

```bash
# Install dependencies
sudo apt update
sudo apt install build-essential cmake git
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev

# Clone and build
git clone https://github.com/yourusername/DungeonDiceDuelists.git
cd DungeonDiceDuelists
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run
./bin/DungeonDiceDuelists
```

### Windows (Visual Studio)

```powershell
# Install vcpkg if not already installed
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

# Install SDL2 libraries
.\vcpkg install sdl2:x64-windows sdl2-image:x64-windows sdl2-ttf:x64-windows sdl2-mixer:x64-windows

# Build with CMake
cd path\to\DungeonDiceDuelists
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]\scripts\buildsystems\vcpkg.cmake
cmake --build . --config Release
```

### macOS

```bash
# Install dependencies via Homebrew
brew install cmake sdl2 sdl2_image sdl2_ttf sdl2_mixer

# Build
cd DungeonDiceDuelists
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

## Building with Steam Support

1. Download the Steamworks SDK from https://partner.steamgames.com/
2. Extract to `third_party/steamworks/`
3. Build with Steam enabled:

```bash
cmake .. -DENABLE_STEAM=ON
make
```

## Project Structure

```
DungeonDiceDuelists/
├── CMakeLists.txt      # Build configuration
├── assets/             # Game assets
│   ├── sprites/        # Unit and UI sprites
│   ├── audio/          # Music and sound effects
│   ├── fonts/          # TTF fonts
│   └── data/           # JSON configuration files
├── src/
│   ├── Core/           # Engine and game core
│   ├── Graphics/       # Rendering system
│   ├── Audio/          # Audio management
│   ├── Input/          # Input handling
│   ├── UI/             # User interface
│   ├── Gameplay/       # Game mechanics
│   │   └── AI/         # AI controllers
│   ├── Data/           # Data loading
│   ├── Steam/          # Steamworks integration
│   └── Utils/          # Utility classes
├── tests/              # Unit tests
└── docs/               # Documentation
```

## Controls

| Action | Keyboard | Mouse |
|--------|----------|-------|
| Select | Enter | Left Click |
| Cancel | Escape | Right Click |
| Move Camera | WASD / Arrow Keys | - |
| End Turn | Space | End Turn Button |
| Open Shop | Tab | Shop Button |
| Pause | Escape | - |

## Game Mechanics

### Dice System
- Each unit is a 6-sided die with different stat combinations per face
- Rolling determines your unit's HP, ATK, DEF, MOVE, and RANGE
- Strategic depth comes from adapting to random rolls

### Territory Control (Unfurling)
- When a die is placed, it claims territory in a cross pattern
- The pattern varies per die face (e.g., [2, 3, 1, 4] = 2 North, 3 East, 1 South, 4 West)
- Enemy units on converted territory take damage

### Combat
- Damage = Attacker ATK - Defender DEF (minimum 1)
- Melee units attack adjacent cells
- Ranged units require line of sight
- Abilities add tactical options

### Progression
- Units gain EXP from kills
- At 100 EXP, units can promote to a higher tier
- 4 tiers total with 3 promotion choices per unit

## License

Copyright © 2026 Your Studio. All rights reserved.

## Credits

- **Engine**: SDL2, nlohmann/json, spdlog
- **Inspired By**: Fire Emblem, Slay the Spire, Into the Breach
