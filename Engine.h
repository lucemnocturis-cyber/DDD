#pragma once

#include <SDL2/SDL.h>
#include <memory>
#include <string>

namespace DDD {

// Forward declarations
class Game;
class ResourceManager;
class InputManager;
class Renderer;
class AudioManager;
class SoundManager;
class MusicManager;
class MusicManager;

/**
 * Main Engine class - handles initialization, main loop, and shutdown
 */
class Engine {
public:
    Engine();
    ~Engine();
    
    // Non-copyable
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    
    /**
     * Initialize the engine and all subsystems
     * @return true if successful, false otherwise
     */
    bool Initialize();
    
    /**
     * Run the main game loop
     */
    void Run();
    
    /**
     * Clean shutdown of all systems
     */
    void Shutdown();
    
    // Accessors for subsystems
    ResourceManager* GetResourceManager() const { return m_resourceManager.get(); }
    InputManager* GetInputManager() const { return m_inputManager.get(); }
    Renderer* GetRenderer() const { return m_renderer.get(); }
    AudioManager* GetAudioManager() const { return m_audioManager.get(); }
    SoundManager* GetSoundManager() const { return m_soundManager.get(); }
    MusicManager* GetMusicManager() const { return m_musicManager.get(); }
    Game* GetGame() const { return m_game.get(); }
    
    // Window info
    int GetWindowWidth() const { return m_windowWidth; }
    int GetWindowHeight() const { return m_windowHeight; }
    
    // Engine state
    bool IsRunning() const { return m_isRunning; }
    void RequestQuit() { m_isRunning = false; }
    void Stop() { m_isRunning = false; }
    
    // Timing
    float GetDeltaTime() const { return m_deltaTime; }
    uint64_t GetFrameCount() const { return m_frameCount; }
    
private:
    // Main loop phases
    void ProcessInput();
    void Update(float deltaTime);
    void Render();
    
    // SDL objects
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_sdlRenderer = nullptr;
    
    // Subsystems
    std::unique_ptr<ResourceManager> m_resourceManager;
    std::unique_ptr<InputManager> m_inputManager;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<AudioManager> m_audioManager;
    std::unique_ptr<SoundManager> m_soundManager;
    std::unique_ptr<MusicManager> m_musicManager;
    std::unique_ptr<Game> m_game;
    
    // Engine state
    bool m_isRunning = false;
    bool m_isInitialized = false;
    
    // Window properties
    int m_windowWidth = 1280;
    int m_windowHeight = 960;
    std::string m_windowTitle = "Dungeon Dice Duelists";
    
    // Timing
    uint64_t m_lastFrameTime = 0;
    float m_deltaTime = 0.0f;
    uint64_t m_frameCount = 0;
    
    // Target frame rate (60 FPS)
    static constexpr float TARGET_FRAME_TIME = 1.0f / 60.0f;
};

} // namespace DDD
