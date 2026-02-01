#pragma once

#include <memory>
#include <vector>
#include <functional>

namespace DDD {

// Forward declarations
class Game;
class Unit;
class AIController;

/**
 * Turn phases
 */
enum class TurnPhase {
    PlacementPhase,
    ActionPhase,
    EnemyTurn,
    WaveComplete,
    GameOver
};

/**
 * TurnManager - handles turn order and phases
 */
class TurnManager {
public:
    explicit TurnManager(Game& game);
    ~TurnManager();
    
    /**
     * Initialize AI controller
     */
    void Initialize();
    
    /**
     * Start a new battle
     */
    void StartBattle();
    
    /**
     * Update turn logic
     */
    void Update(float deltaTime);
    
    /**
     * End the current turn
     */
    void EndTurn();
    
    /**
     * Get current turn phase
     */
    TurnPhase GetPhase() const { return m_phase; }
    
    /**
     * Check if it's the player's turn
     */
    bool IsPlayerTurn() const { return m_isPlayerTurn; }
    
    /**
     * Get turn number
     */
    int GetTurnNumber() const { return m_turnNumber; }
    
    /**
     * Called when a unit dies
     */
    void OnUnitDied(const std::shared_ptr<Unit>& unit);
    
    // Callbacks
    using TurnChangedCallback = std::function<void(bool isPlayerTurn, int turnNumber)>;
    void SetOnTurnChanged(TurnChangedCallback cb) { m_onTurnChanged = cb; }
    
    using EnemyActionCallback = std::function<void(std::shared_ptr<Unit>, std::shared_ptr<Unit>, int)>;
    void SetOnEnemyAttack(EnemyActionCallback cb) { m_onEnemyAttack = cb; }
    
private:
    void StartPlayerTurn();
    void StartEnemyTurn();
    void ProcessEnemyTurn(float deltaTime);
    void CheckWaveComplete();
    
    Game& m_game;
    std::unique_ptr<AIController> m_aiController;
    
    TurnPhase m_phase = TurnPhase::PlacementPhase;
    bool m_isPlayerTurn = true;
    int m_turnNumber = 1;
    
    float m_enemyTurnTimer = 0.0f;
    int m_currentEnemyIndex = 0;
    
    // Callbacks
    TurnChangedCallback m_onTurnChanged;
    EnemyActionCallback m_onEnemyAttack;
    
    static constexpr float ENEMY_ACTION_DELAY = 0.7f;  // Slightly slower for readability
};

} // namespace DDD
