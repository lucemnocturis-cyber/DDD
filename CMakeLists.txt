cmake_minimum_required(VERSION 3.15)
project(DungeonDiceDuelists VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Options
option(BUILD_TESTS "Build unit tests" ON)
option(ENABLE_STEAM "Enable Steamworks integration" OFF)
option(ENABLE_IMGUI "Enable Dear ImGui debug tools" ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find packages
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_mixer REQUIRED)

# Fetch nlohmann_json
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# Fetch spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)

# Source files
set(SOURCES
    src/main.cpp
    
    # Core
    src/Core/Engine.cpp
    src/Core/Game.cpp
    src/Core/ResourceManager.cpp
    src/Core/AccessibilitySystem.cpp
    src/Core/LocalizationSystem.cpp
    src/Core/PerformanceSystem.cpp
    src/Core/AnalyticsSystem.cpp
    src/Core/BuildSystem.cpp
    src/Core/Config.cpp
    
    # Graphics
    src/Graphics/Renderer.cpp
    src/Graphics/TextRenderer.cpp
    src/Graphics/BitmapFont.cpp
    src/Graphics/Sprite.cpp
    src/Graphics/SpriteSheet.cpp
    src/Graphics/Animation.cpp
    src/Graphics/UnitRenderer.cpp
    src/Graphics/Camera.cpp
    src/Graphics/ParticleSystem.cpp
    src/Graphics/AdvancedParticleSystem.cpp
    src/Graphics/PostProcessing.cpp
    src/Graphics/ScreenEffects.cpp
    src/Graphics/TransitionManager.cpp
    
    # Audio
    src/Audio/AudioManager.cpp
    src/Audio/MusicPlayer.cpp
    src/Audio/EnhancedSoundSystem.cpp
    src/Audio/DynamicMusicSystem.cpp
    src/Audio/VoiceSystem.cpp
    src/Audio/SoundManager.cpp
    src/Audio/MusicManager.cpp
    
    # Steam
    src/Steam/SteamManager.cpp
    src/Steam/AchievementSystem.cpp
    src/Steam/CloudSaveSystem.cpp
    src/Steam/LeaderboardSystem.cpp
    src/Steam/SteamStatsSystem.cpp
    
    # Input
    src/Input/InputManager.cpp
    src/Input/InputMapping.cpp
    
    # UI
    src/UI/UIManager.cpp
    src/UI/UIElement.cpp
    src/UI/Button.cpp
    src/UI/Panel.cpp
    src/UI/DiceCard.cpp
    src/UI/DicePanel.cpp
    src/UI/HUD.cpp
    src/UI/MainMenu.cpp
    src/UI/UnitInfoPanel.cpp
    src/UI/GameOverScreen.cpp
    src/UI/TurnBanner.cpp
    src/UI/SettingsPanel.cpp
    src/UI/TutorialSystem.cpp
    src/UI/LoadingScreen.cpp
    src/UI/PromotionScreen.cpp
    
    # Gameplay
    src/Gameplay/GameState.cpp
    src/Gameplay/Board.cpp
    src/Gameplay/Cell.cpp
    src/Gameplay/Unit.cpp
    src/Gameplay/Dice.cpp
    src/Gameplay/Ability.cpp
    src/Gameplay/TurnManager.cpp
    src/Gameplay/WaveManager.cpp
    src/Gameplay/SelectionManager.cpp
    src/Gameplay/UnitDatabase.cpp
    src/Gameplay/DiceDatabase.cpp
    src/Gameplay/EnemyDatabase.cpp
    src/Gameplay/BossSystem.cpp
    src/Gameplay/AbilityDatabase.cpp
    src/Gameplay/TerrainSystem.cpp
    src/Gameplay/ItemSystem.cpp
    src/Gameplay/GameModeSystem.cpp
    src/Gameplay/BalanceSystem.cpp
    src/Gameplay/CombatSystem.cpp
    src/Gameplay/AI/AIController.cpp
    src/Gameplay/AI/AIStrategy.cpp
    src/Gameplay/AI/Pathfinding.cpp
    
    # Data
    src/Data/UnitData.cpp
    src/Data/AbilityData.cpp
    src/Data/DataLoader.cpp
    
    # Utils
    src/Utils/Math.cpp
    src/Utils/Random.cpp
    src/Utils/Timer.cpp
    src/Utils/Logger.cpp
)

# Header files (for IDEs)
set(HEADERS
    src/Core/Engine.h
    src/Core/Game.h
    src/Core/ResourceManager.h
    src/Core/AccessibilitySystem.h
    src/Core/LocalizationSystem.h
    src/Core/PerformanceSystem.h
    src/Core/AnalyticsSystem.h
    src/Core/BuildSystem.h
    src/Core/Config.h
    src/Graphics/Renderer.h
    src/Graphics/TextRenderer.h
    src/Graphics/BitmapFont.h
    src/Graphics/Sprite.h
    src/Graphics/SpriteSheet.h
    src/Graphics/Animation.h
    src/Graphics/Camera.h
    src/Graphics/ParticleSystem.h
    src/Graphics/AdvancedParticleSystem.h
    src/Graphics/PostProcessing.h
    src/Graphics/ScreenEffects.h
    src/Graphics/TransitionManager.h
    src/Graphics/ColorPalette.h
    src/Audio/AudioManager.h
    src/Audio/MusicPlayer.h
    src/Audio/EnhancedSoundSystem.h
    src/Audio/DynamicMusicSystem.h
    src/Audio/VoiceSystem.h
    src/Audio/SoundManager.h
    src/Audio/MusicManager.h
    src/Steam/SteamManager.h
    src/Steam/AchievementSystem.h
    src/Steam/CloudSaveSystem.h
    src/Steam/LeaderboardSystem.h
    src/Steam/SteamStatsSystem.h
    src/Input/InputManager.h
    src/Input/InputMapping.h
    src/UI/UIManager.h
    src/UI/UIElement.h
    src/UI/Button.h
    src/UI/Panel.h
    src/UI/DiceCard.h
    src/UI/TurnBanner.h
    src/UI/SettingsPanel.h
    src/UI/TutorialSystem.h
    src/UI/LoadingScreen.h
    src/UI/PromotionScreen.h
    src/Gameplay/GameState.h
    src/Gameplay/Board.h
    src/Gameplay/Cell.h
    src/Gameplay/Unit.h
    src/Gameplay/Dice.h
    src/Gameplay/Ability.h
    src/Gameplay/TurnManager.h
    src/Gameplay/WaveManager.h
    src/Gameplay/UnitDatabase.h
    src/Gameplay/DiceDatabase.h
    src/Gameplay/EnemyDatabase.h
    src/Gameplay/BossSystem.h
    src/Gameplay/AbilityDatabase.h
    src/Gameplay/TerrainSystem.h
    src/Gameplay/ItemSystem.h
    src/Gameplay/GameModeSystem.h
    src/Gameplay/BalanceSystem.h
    src/Gameplay/AI/AIController.h
    src/Gameplay/AI/AIStrategy.h
    src/Gameplay/AI/Pathfinding.h
    src/Data/UnitData.h
    src/Data/AbilityData.h
    src/Data/DataLoader.h
    src/Utils/Math.h
    src/Utils/Random.h
    src/Utils/Timer.h
    src/Utils/Tween.h
    src/Utils/Logger.h
)

# Steam integration (optional)
if(ENABLE_STEAM)
    set(SOURCES ${SOURCES}
        src/Steam/SteamManager.cpp
        src/Steam/Achievements.cpp
        src/Steam/Leaderboards.cpp
        src/Steam/CloudSave.cpp
    )
    set(HEADERS ${HEADERS}
        src/Steam/SteamManager.h
        src/Steam/Achievements.h
        src/Steam/Leaderboards.h
        src/Steam/CloudSave.h
    )
    add_definitions(-DSTEAM_ENABLED)
endif()

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${SDL2_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL2::SDL2
    SDL2::SDL2main
    SDL2_image::SDL2_image
    SDL2_ttf::SDL2_ttf
    SDL2_mixer::SDL2_mixer
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

# Steam library (if enabled)
if(ENABLE_STEAM)
    if(WIN32)
        target_link_libraries(${PROJECT_NAME} PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party/steamworks/redistributable_bin/win64/steam_api64.lib
        )
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(${PROJECT_NAME} PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party/steamworks/redistributable_bin/linux64/libsteam_api.so
        )
    elseif(APPLE)
        target_link_libraries(${PROJECT_NAME} PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party/steamworks/redistributable_bin/osx/libsteam_api.dylib
        )
    endif()
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/steamworks/public
    )
endif()

# Copy assets to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMENT "Copying assets to build directory"
)

# Copy Steam DLL (if enabled)
if(ENABLE_STEAM)
    if(WIN32)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/third_party/steamworks/redistributable_bin/win64/steam_api64.dll
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMENT "Copying Steam API DLL"
        )
    endif()
endif()

# Unit tests
if(BUILD_TESTS)
    enable_testing()
    
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
    
    # Test executable
    add_executable(${PROJECT_NAME}_tests
        tests/test_board.cpp
        tests/test_combat.cpp
        tests/test_ai.cpp
        
        # Include necessary source files
        src/Gameplay/Board.cpp
        src/Gameplay/Cell.cpp
        src/Gameplay/Unit.cpp
        src/Gameplay/Dice.cpp
        src/Utils/Math.cpp
        src/Utils/Random.cpp
    )
    
    target_include_directories(${PROJECT_NAME}_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    
    target_link_libraries(${PROJECT_NAME}_tests PRIVATE
        GTest::gtest_main
        nlohmann_json::nlohmann_json
        spdlog::spdlog
    )
    
    include(GoogleTest)
    gtest_discover_tests(${PROJECT_NAME}_tests)
endif()

# Install rules
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(DIRECTORY assets/ DESTINATION bin/assets)

# CPack for creating installers
set(CPACK_PACKAGE_NAME "DungeonDiceDuelists")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "YourStudio")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A tactical roguelike strategy game")
include(CPack)
