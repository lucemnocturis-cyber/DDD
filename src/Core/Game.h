#pragma once

#include <memory>
#include <stack>

namespace DDD {

// Forward declarations
class Engine;
class Renderer;
class Board;
class TurnManager;
class WaveManager;
class UIManager;
class SelectionManager;
class TutorialSystem;

// From TransitionManager.h
enum class TransitionType;

/**
 * Game state types
 */
enum class GameStateType {
    MainMenu,
    DeckSelection,
    Battle,
    WaveReward,
    Promotion,
    Shop,
    GameOver,
    Settings,
    Paused
};

/**
 * Main Game class - manages game states and gameplay systems
 */
class Game {
public:
    explicit Game(Engine& engine);
    ~Game();
    
    // Non-copyable
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;
    
    /**
     * Initialize the game
     */
    bool Initialize();
    
    /**
     * Update game logic
     */
    void Update(float deltaTime);
    
    /**
     * Render the game
     */
    void Render(Renderer& renderer);
    
    /**
     * Handle mouse input
     */
    void OnMouseMove(int x, int y);
    void OnMouseClick(int button);
    
    /**
     * Handle keyboard input
     */
    void OnKeyPress(int key);
    
    /**
     * Change game state
     */
    void ChangeState(GameStateType newState);
    
    /**
     * Change state with transition effect
     */
    void TransitionToState(GameStateType newState, TransitionType transition);
    void TransitionToState(GameStateType newState);  // Overload with no transition    
    /**
     * Push a state onto the stack (for overlays like pause menu)
     */
    void PushState(GameStateType state);
    
    /**
     * Pop the current state
     */
    void PopState();
    
    // Accessors
    Engine& GetEngine() { return m_engine; }
    GameStateType GetCurrentState() const { return m_currentState; }
    Board* GetBoard() const { return m_board.get(); }
    TurnManager* GetTurnManager() const { return m_turnManager.get(); }
    WaveManager* GetWaveManager() const { return m_waveManager.get(); }
    UIManager* GetUIManager() const { return m_uiManager.get(); }
    SelectionManager* GetSelectionManager() const { return m_selectionManager.get(); }
    
    // Game progression
    int GetCurrentWave() const { return m_currentWave; }
    int GetPlayerGold() const { return m_playerGold; }
    int GetScore() const { return m_score; }
    
    void AddGold(int amount) { m_playerGold += amount; }
    bool SpendGold(int amount);
    void AddScore(int amount) { m_score += amount; }
    
    // Wave management
    void StartNewWave();
    void OnWaveComplete();
    void OnGameOver(bool victory);
    
    // Reset game for new playthrough
    void ResetGame();
    
private:
    // State management
    void EnterState(GameStateType state);
    void ExitState(GameStateType state);
    void UpdateState(float deltaTime);
    void RenderState(Renderer& renderer);
    
    // Selection callbacks
    void SetupSelectionCallbacks();
    void SetupTurnCallbacks();
    
    // Reference to engine
    Engine& m_engine;
    
    // Game state
    GameStateType m_currentState = GameStateType::MainMenu;
    std::stack<GameStateType> m_stateStack;
    
    // Core gameplay systems
    std::unique_ptr<Board> m_board;
    std::unique_ptr<TurnManager> m_turnManager;
    std::unique_ptr<WaveManager> m_waveManager;
    std::unique_ptr<UIManager> m_uiManager;
    std::unique_ptr<SelectionManager> m_selectionManager;
    std::unique_ptr<class ParticleSystem> m_particleSystem;
    std::unique_ptr<class ScreenEffects> m_screenEffects;
    std::unique_ptr<class TransitionManager> m_transitionManager;
    std::unique_ptr<TutorialSystem> m_tutorialSystem;
    
    // Mouse position
    int m_mouseX = 0;
    int m_mouseY = 0;
    
    // Game progression
    int m_currentWave = 1;
    int m_playerGold = 5;  // Starting gold
    int m_score = 0;
    bool m_lastGameVictory = false;  // Track if last game was victory
    
    // Constants
    static constexpr int STARTING_GOLD = 5;
public:
    static constexpr int MAX_WAVES = 20;
    
    // Particle system access
    class ParticleSystem* GetParticleSystem() const { return m_particleSystem.get(); }
    
    // Screen effects access
    class ScreenEffects* GetScreenEffects() const { return m_screenEffects.get(); }
    
    // Transition manager access
    class TransitionManager* GetTransitionManager() const { return m_transitionManager.get(); }
    
    // Tutorial system access
    TutorialSystem* GetTutorialSystem() const { return m_tutorialSystem.get(); }
};

} // namespace DDD
