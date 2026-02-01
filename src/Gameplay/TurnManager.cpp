#include "TurnManager.h"
#include "../Core/Game.h"
#include "Board.h"
#include "Unit.h"
#include "AI/AIController.h"
#include "../Utils/Logger.h"

namespace DDD {

TurnManager::TurnManager(Game& game)
    : m_game(game)
{
}

TurnManager::~TurnManager() = default;

void TurnManager::Initialize() {
    m_aiController = std::make_unique<AIController>(m_game);
    
    // Set up AI callbacks
    m_aiController->SetOnAttack([this](std::shared_ptr<Unit> attacker, 
                                        std::shared_ptr<Unit> target, int damage) {
        // Notify for visual effects
        if (m_onEnemyAttack) {
            m_onEnemyAttack(attacker, target, damage);
        }
        
        // Check for death
        if (target->IsDead()) {
            OnUnitDied(target);
        }
    });
    
    m_aiController->SetOnMove([this](std::shared_ptr<Unit> unit, Position newPos) {
        // Could add move animation callback here
    });
    
    Logger::Info("TurnManager initialized with AI controller");
}

void TurnManager::StartBattle() {
    m_phase = TurnPhase::PlacementPhase;
    m_isPlayerTurn = true;
    m_turnNumber = 1;
    Logger::Info("Battle started - Turn 1, Player's turn");
    
    if (m_onTurnChanged) {
        m_onTurnChanged(true, m_turnNumber);
    }
}

void TurnManager::Update(float deltaTime) {
    switch (m_phase) {
        case TurnPhase::EnemyTurn:
            ProcessEnemyTurn(deltaTime);
            break;
            
        case TurnPhase::WaveComplete:
        case TurnPhase::GameOver:
            // Handled by Game
            break;
            
        default:
            break;
    }
}

void TurnManager::EndTurn() {
    if (m_isPlayerTurn) {
        StartEnemyTurn();
    } else {
        m_turnNumber++;
        StartPlayerTurn();
    }
}

void TurnManager::StartPlayerTurn() {
    m_isPlayerTurn = true;
    m_phase = TurnPhase::ActionPhase;
    
    // Reset player unit actions
    auto* board = m_game.GetBoard();
    auto playerUnits = board->GetUnitsByOwner(Owner::Player);
    for (auto& unit : playerUnits) {
        unit->ResetActions();
        unit->ProcessStatusEffects();
    }
    
    Logger::Info("Turn {} - Player's turn ({} units)", m_turnNumber, playerUnits.size());
    
    if (m_onTurnChanged) {
        m_onTurnChanged(true, m_turnNumber);
    }
}

void TurnManager::StartEnemyTurn() {
    m_isPlayerTurn = false;
    m_phase = TurnPhase::EnemyTurn;
    m_enemyTurnTimer = 0.0f;
    m_currentEnemyIndex = 0;
    
    // Reset enemy unit actions
    auto* board = m_game.GetBoard();
    auto enemyUnits = board->GetUnitsByOwner(Owner::Enemy);
    for (auto& unit : enemyUnits) {
        unit->ResetActions();
        unit->ProcessStatusEffects();
    }
    
    Logger::Info("Turn {} - Enemy's turn ({} units)", m_turnNumber, enemyUnits.size());
    
    if (m_onTurnChanged) {
        m_onTurnChanged(false, m_turnNumber);
    }
}

void TurnManager::ProcessEnemyTurn(float deltaTime) {
    m_enemyTurnTimer += deltaTime;
    
    // Wait between actions for visual clarity
    if (m_enemyTurnTimer < ENEMY_ACTION_DELAY) {
        return;
    }
    
    auto* board = m_game.GetBoard();
    auto enemyUnits = board->GetUnitsByOwner(Owner::Enemy);
    
    // Process one enemy at a time
    if (m_currentEnemyIndex < static_cast<int>(enemyUnits.size())) {
        auto& enemy = enemyUnits[m_currentEnemyIndex];
        
        // Skip stunned units
        if (enemy->IsStunned()) {
            Logger::Info("{} is stunned, skipping", enemy->GetClassName());
            m_currentEnemyIndex++;
            m_enemyTurnTimer = ENEMY_ACTION_DELAY * 0.5f;  // Shorter delay for skips
            return;
        }
        
        // Skip dead units (might have died this turn)
        if (enemy->IsDead()) {
            m_currentEnemyIndex++;
            return;
        }
        
        // Use AI to decide and execute action
        if (m_aiController && (!enemy->HasMoved() || !enemy->HasAttacked())) {
            auto action = m_aiController->DecideAction(enemy);
            
            if (action.type != AIActionType::None && action.type != AIActionType::Wait) {
                m_aiController->ExecuteAction(enemy, action);
                
                // If unit can still do something, don't move to next unit yet
                if (!enemy->HasMoved() || !enemy->HasAttacked()) {
                    m_enemyTurnTimer = 0.0f;
                    return;
                }
            }
        }
        
        m_currentEnemyIndex++;
        m_enemyTurnTimer = 0.0f;
    } else {
        // All enemies have acted
        Logger::Info("Enemy turn complete");
        
        CheckWaveComplete();
        if (m_phase != TurnPhase::WaveComplete && m_phase != TurnPhase::GameOver) {
            EndTurn();  // This will call StartPlayerTurn via EndTurn()
        }
    }
}

void TurnManager::OnUnitDied(const std::shared_ptr<Unit>& unit) {
    auto* board = m_game.GetBoard();
    
    // Award rewards for enemy kills
    if (unit->GetOwner() == Owner::Enemy) {
        int expReward = 10 + unit->GetLevel() * 5 + unit->GetTier() * 15;
        int goldReward = 5 + unit->GetLevel() * 3 + unit->GetTier() * 5;
        
        // Give EXP to nearby player units
        auto playerUnits = board->GetUnitsByOwner(Owner::Player);
        for (auto& player : playerUnits) {
            int dist = board->GetDistance(player->GetPosition(), unit->GetPosition());
            if (dist <= 4) {  // Nearby units get partial EXP
                int exp = (dist <= 1) ? expReward : expReward / 2;
                player->GainExp(exp);
            }
        }
        
        // Award gold
        m_game.AddGold(goldReward);
        m_game.AddScore(25 + unit->GetLevel() * 10 + unit->GetTier() * 25);
        
        Logger::Info("{} defeated! +{} Gold, +{} EXP", unit->GetClassName(), goldReward, expReward);
    }
    
    // Remove unit from board
    board->RemoveUnit(unit);
    
    // Check win/lose conditions
    CheckWaveComplete();
}

void TurnManager::CheckWaveComplete() {
    auto* board = m_game.GetBoard();
    auto enemyUnits = board->GetUnitsByOwner(Owner::Enemy);
    auto playerUnits = board->GetUnitsByOwner(Owner::Player);
    
    if (enemyUnits.empty()) {
        m_phase = TurnPhase::WaveComplete;
        m_game.OnWaveComplete();
        Logger::Info("Wave complete! All enemies defeated.");
    } else if (playerUnits.empty()) {
        m_phase = TurnPhase::GameOver;
        m_game.OnGameOver(false);
        Logger::Info("Game over - All player units defeated");
    }
}

} // namespace DDD
