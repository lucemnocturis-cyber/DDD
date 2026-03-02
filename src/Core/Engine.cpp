#include "../UI/UIManager.h"
#include "Engine.h"
#include "Game.h"
#include "ResourceManager.h"
#include "Config.h"
#include "../Graphics/Renderer.h"
#include "../Audio/AudioManager.h"
#include "../Audio/SoundManager.h"
#include "../Audio/MusicManager.h"
#include "../Input/InputManager.h"
#include "../Utils/Logger.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#ifdef STEAM_ENABLED
#include <steam/steam_api.h>
#endif

namespace DDD {

Engine::Engine() = default;

Engine::~Engine() {
    if (m_isInitialized) {
        Shutdown();
    }
}

bool Engine::Initialize() {
    Logger::Info("Initializing engine...");
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        Logger::Error("SDL_Init failed: {}", SDL_GetError());
        return false;
    }
    Logger::Info("SDL initialized");
    
    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(imgFlags) & imgFlags) != imgFlags) {
        Logger::Error("SDL_image init failed: {}", IMG_GetError());
        return false;
    }
    Logger::Info("SDL_image initialized");
    
    // Initialize SDL_ttf
    if (TTF_Init() != 0) {
        Logger::Error("SDL_ttf init failed: {}", TTF_GetError());
        return false;
    }
    Logger::Info("SDL_ttf initialized");
    
    // Initialize SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        Logger::Error("SDL_mixer init failed: {}", Mix_GetError());
        return false;
    }
    Logger::Info("SDL_mixer initialized");
    
#ifdef STEAM_ENABLED
    // Initialize Steam
    if (!SteamAPI_Init()) {
        Logger::Warning("Steam API init failed - running without Steam");
    } else {
        Logger::Info("Steam API initialized");
    }
#endif
    
    // Create window
    m_window = SDL_CreateWindow(
        m_windowTitle.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        m_windowWidth,
        m_windowHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!m_window) {
        Logger::Error("Window creation failed: {}", SDL_GetError());
        return false;
    }
    Logger::Info("Window created: {}x{}", m_windowWidth, m_windowHeight);
    
    // Create SDL renderer
    m_sdlRenderer = SDL_CreateRenderer(
        m_window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!m_sdlRenderer) {
        Logger::Error("Renderer creation failed: {}", SDL_GetError());
        return false;
    }
    Logger::Info("SDL Renderer created");
    
    // Set render scale quality
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    
    // Initialize subsystems
    m_resourceManager = std::make_unique<ResourceManager>();
    if (!m_resourceManager->Initialize(m_sdlRenderer)) {
        Logger::Error("ResourceManager initialization failed");
        return false;
    }
    
    m_inputManager = std::make_unique<InputManager>();
    if (!m_inputManager->Initialize()) {
        Logger::Error("InputManager initialization failed");
        return false;
    }
    
    m_renderer = std::make_unique<Renderer>(m_sdlRenderer, m_windowWidth, m_windowHeight);
    if (!m_renderer->Initialize()) {
        Logger::Error("Renderer initialization failed");
        return false;
    }
    
    m_audioManager = std::make_unique<AudioManager>();
    if (!m_audioManager->Initialize()) {
        Logger::Error("AudioManager initialization failed");
        return false;
    }
    
    // Initialize sound manager (uses SDL audio directly)
    m_soundManager = std::make_unique<SoundManager>();
    if (!m_soundManager->Initialize()) {
        Logger::Warning("SoundManager initialization failed - continuing without sounds");
        // Not fatal, game can run without sound
    }
    
    // Initialize music manager
    m_musicManager = std::make_unique<MusicManager>();
    if (!m_musicManager->Initialize()) {
        Logger::Warning("MusicManager initialization failed - continuing without music");
    }
    
    // Create the game
    m_game = std::make_unique<Game>(*this);
    if (!m_game->Initialize()) {
        Logger::Error("Game initialization failed");
        return false;
    }
    
    m_isInitialized = true;
    m_isRunning = true;
    m_lastFrameTime = SDL_GetPerformanceCounter();
    
    Logger::Info("Engine initialization complete");
    return true;
}

void Engine::Run() {
    Logger::Info("Starting main game loop");
    
    while (m_isRunning) {
        // Calculate delta time
        uint64_t currentTime = SDL_GetPerformanceCounter();
        m_deltaTime = static_cast<float>(currentTime - m_lastFrameTime) / 
                      static_cast<float>(SDL_GetPerformanceFrequency());
        m_lastFrameTime = currentTime;
        
        // Cap delta time to prevent spiral of death
        if (m_deltaTime > 0.25f) {
            m_deltaTime = 0.25f;
        }
        
        // Process input
        ProcessInput();
        
        // Update game state
        Update(m_deltaTime);
        
        // Render
        Render();
        
#ifdef STEAM_ENABLED
        // Run Steam callbacks
        SteamAPI_RunCallbacks();
#endif
        
        m_frameCount++;
    }
}

void Engine::ProcessInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                m_isRunning = false;
                break;
                
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    m_windowWidth = event.window.data1;
                    m_windowHeight = event.window.data2;
                    m_renderer->OnWindowResize(m_windowWidth, m_windowHeight);
                    Logger::Info("Window resized to {}x{}", m_windowWidth, m_windowHeight);
                }
                break;
                
            case SDL_MOUSEMOTION:
                if (m_game) {
                    m_game->OnMouseMove(event.motion.x, event.motion.y);
                    // Also forward to UIManager
                    if (m_game->GetUIManager()) {
                        m_game->GetUIManager()->OnMouseMove(event.motion.x, event.motion.y);
                    }
                }
                break;
                
            case SDL_MOUSEBUTTONDOWN:
                if (m_game) {
                    m_game->OnMouseClick(event.button.button);
                    if (m_game->GetUIManager()) {
                        m_game->GetUIManager()->OnMouseDown();
                    }
                }
                break;
                
            case SDL_MOUSEBUTTONUP:
                if (m_game && m_game->GetUIManager()) {
                    m_game->GetUIManager()->OnMouseUp();
                }
                break;
                
            case SDL_KEYDOWN:
                if (!event.key.repeat && m_game) {
                    // Handle escape to quit from main menu
                    if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && 
                        m_game->GetCurrentState() == GameStateType::MainMenu) {
                        m_isRunning = false;
                    } else {
                        m_game->OnKeyPress(event.key.keysym.scancode);
                    }
                }
                break;
                
            default:
                // Pass event to input manager
                m_inputManager->ProcessEvent(event);
                break;
        }
    }
    
    // Update input manager state
    m_inputManager->Update();
}

void Engine::Update(float deltaTime) {
    // Update music manager for fading
    if (m_musicManager) {
        m_musicManager->Update(deltaTime);
    }
    
    m_game->Update(deltaTime);
}

void Engine::Render() {
    // Clear screen
    m_renderer->Clear();
    
    // Render game
    m_game->Render(*m_renderer);
    
    // Present
    m_renderer->Present();
}

void Engine::Shutdown() {
    Logger::Info("Shutting down engine...");
    
    // Shutdown in reverse order of initialization
    m_game.reset();
    m_audioManager.reset();
    m_renderer.reset();
    m_inputManager.reset();
    m_resourceManager.reset();
    
#ifdef STEAM_ENABLED
    SteamAPI_Shutdown();
    Logger::Info("Steam API shutdown");
#endif
    
    if (m_sdlRenderer) {
        SDL_DestroyRenderer(m_sdlRenderer);
        m_sdlRenderer = nullptr;
    }
    
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    
    Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    
    m_isInitialized = false;
    Logger::Info("Engine shutdown complete");
}

} // namespace DDD
