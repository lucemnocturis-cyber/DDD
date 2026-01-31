#!/bin/bash
# Build System Test Script for Dungeon Dice Duelists
# Tests compilation on the current platform

set -e  # Exit on error

echo "======================================================================="
echo "Dungeon Dice Duelists - Build System Test"
echo "======================================================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Functions
print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

print_section() {
    echo ""
    echo "-----------------------------------------------------------------------"
    echo "$1"
    echo "-----------------------------------------------------------------------"
}

# Check prerequisites
print_section "1. Checking Prerequisites"

# Check for CMake
if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -n1)
    print_success "CMake found: $CMAKE_VERSION"
else
    print_error "CMake not found. Please install CMake 3.15 or later."
    exit 1
fi

# Check for C++ compiler
if command -v g++ &> /dev/null; then
    GCC_VERSION=$(g++ --version | head -n1)
    print_success "G++ found: $GCC_VERSION"
elif command -v clang++ &> /dev/null; then
    CLANG_VERSION=$(clang++ --version | head -n1)
    print_success "Clang++ found: $CLANG_VERSION"
else
    print_error "No C++ compiler found. Please install g++ or clang++."
    exit 1
fi

# Check for pkg-config
if command -v pkg-config &> /dev/null; then
    print_success "pkg-config found"
else
    print_warning "pkg-config not found. May have issues finding SDL2."
fi

# Check for SDL2
print_section "2. Checking SDL2 Dependencies"

check_sdl_lib() {
    if pkg-config --exists $1 2>/dev/null; then
        VERSION=$(pkg-config --modversion $1)
        print_success "$1 found (version $VERSION)"
        return 0
    else
        print_error "$1 not found"
        return 1
    fi
}

SDL_FOUND=true
check_sdl_lib "sdl2" || SDL_FOUND=false
check_sdl_lib "SDL2_image" || SDL_FOUND=false
check_sdl_lib "SDL2_ttf" || SDL_FOUND=false
check_sdl_lib "SDL2_mixer" || SDL_FOUND=false

if [ "$SDL_FOUND" = false ]; then
    echo ""
    print_warning "Some SDL2 libraries are missing. Build may fail."
    echo "To install on Ubuntu/Debian:"
    echo "  sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev"
    echo ""
    echo "To install on macOS:"
    echo "  brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer"
    echo ""
fi

# Check file structure
print_section "3. Verifying Project Structure"

check_file() {
    if [ -f "$1" ]; then
        print_success "$1 exists"
        return 0
    else
        print_error "$1 missing"
        return 1
    fi
}

check_dir() {
    if [ -d "$1" ]; then
        print_success "$1/ exists"
        return 0
    else
        print_error "$1/ missing"
        return 1
    fi
}

# Check essential files
check_file "CMakeLists.txt"
check_file "src/main.cpp"
check_file "src/Graphics/BitmapFont.cpp"
check_file "src/Graphics/BitmapFont.h"

# Check essential directories
check_dir "src/Core"
check_dir "src/Graphics"
check_dir "src/Audio"
check_dir "src/Gameplay"
check_dir "src/UI"
check_dir "assets"

# Check asset directories
print_section "4. Verifying Assets"

check_dir "assets/sprites"
check_dir "assets/fonts"
check_dir "assets/audio"

# Count assets
SPRITE_COUNT=$(find assets/sprites -name "*.png" 2>/dev/null | wc -l)
FONT_COUNT=$(find assets/fonts -name "*.png" -o -name "*.json" 2>/dev/null | wc -l)
SOUND_COUNT=$(find assets/audio/sfx -name "*.wav" 2>/dev/null | wc -l)
MUSIC_COUNT=$(find assets/audio/music -name "*.wav" 2>/dev/null | wc -l)

echo "Asset counts:"
echo "  Sprites: $SPRITE_COUNT files"
echo "  Fonts: $FONT_COUNT files"
echo "  Sounds: $SOUND_COUNT files"
echo "  Music: $MUSIC_COUNT files"

if [ $SPRITE_COUNT -lt 100 ]; then
    print_warning "Expected ~140 sprite files, found $SPRITE_COUNT"
fi

if [ $SOUND_COUNT -lt 15 ]; then
    print_warning "Expected ~19 sound files, found $SOUND_COUNT"
fi

if [ $MUSIC_COUNT -lt 5 ]; then
    print_warning "Expected ~8 music files, found $MUSIC_COUNT"
fi

# Configure build
print_section "5. Configuring CMake Build"

# Clean previous build
if [ -d "build" ]; then
    print_warning "Removing previous build directory"
    rm -rf build
fi

mkdir -p build
cd build

echo "Running CMake configuration..."
if cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF -DENABLE_STEAM=OFF 2>&1 | tee cmake_config.log; then
    print_success "CMake configuration successful"
else
    print_error "CMake configuration failed. See cmake_config.log for details."
    exit 1
fi

# Build project
print_section "6. Compiling Project"

echo "Building project (this may take a few minutes)..."
CORES=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
echo "Using $CORES parallel jobs"

if cmake --build . --config Release -j$CORES 2>&1 | tee build.log; then
    print_success "Build successful"
else
    print_error "Build failed. See build.log for details."
    exit 1
fi

# Check output
print_section "7. Verifying Build Output"

if [ -f "bin/DungeonDiceDuelists" ]; then
    SIZE=$(du -h bin/DungeonDiceDuelists | cut -f1)
    print_success "Executable created: bin/DungeonDiceDuelists ($SIZE)"
    
    # Check if executable is valid
    if file bin/DungeonDiceDuelists | grep -q "executable"; then
        print_success "Executable is valid"
    else
        print_warning "Executable may not be valid"
    fi
else
    print_error "Executable not found"
    exit 1
fi

# Check assets copied
if [ -d "bin/assets" ]; then
    print_success "Assets copied to bin/"
    
    BIN_SPRITES=$(find bin/assets/sprites -name "*.png" 2>/dev/null | wc -l)
    BIN_SOUNDS=$(find bin/assets/audio/sfx -name "*.wav" 2>/dev/null | wc -l)
    
    echo "  Sprites in bin/: $BIN_SPRITES"
    echo "  Sounds in bin/: $BIN_SOUNDS"
else
    print_warning "Assets directory not found in bin/"
fi

# Summary
print_section "8. Build Summary"

echo "Build Type: Release"
echo "Compiler: $(cmake --version | grep cmake | cut -d' ' -f3)"
echo "Platform: $(uname -s)"
echo "Architecture: $(uname -m)"

# Check executable size
if [ -f "bin/DungeonDiceDuelists" ]; then
    EXE_SIZE=$(stat -f%z bin/DungeonDiceDuelists 2>/dev/null || stat -c%s bin/DungeonDiceDuelists 2>/dev/null)
    EXE_SIZE_MB=$((EXE_SIZE / 1024 / 1024))
    echo "Executable size: ${EXE_SIZE_MB}MB"
fi

# Check total build size
BUILD_SIZE=$(du -sh . | cut -f1)
echo "Total build size: $BUILD_SIZE"

print_section "9. Build Test Result"

echo ""
print_success "BUILD TEST PASSED!"
echo ""
echo "The project compiled successfully."
echo ""
echo "Next steps:"
echo "  1. Run the executable: ./bin/DungeonDiceDuelists"
echo "  2. Test gameplay functionality"
echo "  3. Verify asset loading"
echo "  4. Check for runtime errors"
echo ""
echo "To run the game:"
echo "  cd build/bin"
echo "  ./DungeonDiceDuelists"
echo ""

exit 0
