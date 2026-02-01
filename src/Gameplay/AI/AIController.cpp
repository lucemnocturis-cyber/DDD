#include "AIController.h"
#include "../Board.h"
#include "../Unit.h"
#include "../../Core/Game.h"
#include "../../Utils/Logger.h"

#include <algorithm>
#include <climits>

namespace DDD {

AIController::AIController(Game& game) : m_game(game) {}

AIAction AIController::DecideAction(std::shared_ptr<Unit> unit) {
    auto actions = GeneratePossibleActions(unit);
    
    if (actions.empty()) {
        AIAction waitAction;
        waitAction.type = AIActionType::Wait;
        waitAction.description = "No valid actions";
        return waitAction;
    }
    
    return EvaluateBestAction(actions);
}

void AIController::ExecuteAction(std::shared_ptr<Unit> unit, const AIAction& action) {
    Board* board = m_game.GetBoard();
    
    switch (action.type) {
        case AIActionType::Attack: {
            if (!action.targetUnit || unit->HasAttacked()) break;
            
            // Calculate damage
            int damage = std::max(1, unit->GetStats().atk - action.targetUnit->GetStats().def);
            
            // Apply damage
            action.targetUnit->TakeDamage(damage);
            unit->SetHasAttacked(true);
            
            Logger::Info("AI: {} attacks {} for {} damage",
                        unit->GetClassName(), action.targetUnit->GetClassName(), damage);
            
            // Callback for visual effects
            if (m_onAttack) {
                m_onAttack(unit, action.targetUnit, damage);
            }
            break;
        }
        
        case AIActionType::Move: {
            if (unit->HasMoved()) break;
            
            Position oldPos = unit->GetPosition();
            board->MoveUnit(unit, action.targetPos);
            unit->SetHasMoved(true);
            
            Logger::Info("AI: {} moves from ({},{}) to ({},{})",
                        unit->GetClassName(), oldPos.x, oldPos.y, 
                        action.targetPos.x, action.targetPos.y);
            
            // Callback for visual effects
            if (m_onMove) {
                m_onMove(unit, action.targetPos);
            }
            break;
        }
        
        case AIActionType::Wait:
        case AIActionType::None:
        default:
            break;
    }
}

std::vector<AIAction> AIController::GeneratePossibleActions(std::shared_ptr<Unit> unit) {
    std::vector<AIAction> actions;
    
    // Generate attack actions first (higher priority)
    if (!unit->HasAttacked()) {
        GenerateAttackActions(unit, actions);
    }
    
    // Generate move actions
    if (!unit->HasMoved()) {
        GenerateMoveActions(unit, actions);
    }
    
    return actions;
}

AIAction AIController::EvaluateBestAction(const std::vector<AIAction>& actions) {
    if (actions.empty()) {
        return AIAction{};
    }
    
    // Find highest priority action
    auto best = std::max_element(actions.begin(), actions.end(),
        [](const AIAction& a, const AIAction& b) {
            return a.priority < b.priority;
        });
    
    return *best;
}

void AIController::GenerateAttackActions(std::shared_ptr<Unit> unit, std::vector<AIAction>& actions) {
    Board* board = m_game.GetBoard();
    auto targets = board->GetValidAttackCells(*unit);
    
    for (const auto& pos : targets) {
        auto target = board->GetUnitAt(pos.x, pos.y);
        if (target && target->GetOwner() == Owner::Player) {
            AIAction action;
            action.type = AIActionType::Attack;
            action.targetPos = pos;
            action.targetUnit = target;
            action.priority = ScoreAttackTarget(unit, target);
            action.description = "Attack " + target->GetClassName();
            actions.push_back(action);
        }
    }
}

void AIController::GenerateMoveActions(std::shared_ptr<Unit> unit, std::vector<AIAction>& actions) {
    Board* board = m_game.GetBoard();
    auto moveCells = board->GetValidMoveCells(*unit);
    
    for (const auto& pos : moveCells) {
        AIAction action;
        action.type = AIActionType::Move;
        action.targetPos = pos;
        action.priority = ScoreMovePosition(unit, pos);
        action.description = "Move to (" + std::to_string(pos.x) + "," + std::to_string(pos.y) + ")";
        actions.push_back(action);
    }
}

int AIController::ScoreAttackTarget(std::shared_ptr<Unit> attacker, std::shared_ptr<Unit> target) {
    int score = 100;  // Base attack score (always prioritize attacking)
    
    // Calculate potential damage
    int damage = std::max(1, attacker->GetStats().atk - target->GetStats().def);
    
    // Bonus for killing the target
    if (target->GetStats().hp <= damage) {
        score += 200;  // High priority to finish off enemies
    }
    
    // Bonus for attacking low HP targets
    float hpPercent = static_cast<float>(target->GetStats().hp) / target->GetMaxHP();
    score += static_cast<int>((1.0f - hpPercent) * 50);
    
    // Bonus for attacking high-value targets (higher ATK)
    score += target->GetStats().atk * 2;
    
    // Bonus for attacking ranged units (they're squishy and dangerous)
    if (target->GetStats().rng > 1) {
        score += 30;
    }
    
    return score;
}

int AIController::ScoreMovePosition(std::shared_ptr<Unit> unit, const Position& pos) {
    Board* board = m_game.GetBoard();
    int score = 50;  // Base move score
    
    // Find nearest player unit
    auto playerUnits = board->GetUnitsByOwner(Owner::Player);
    if (playerUnits.empty()) return score;
    
    int currentMinDist = INT_MAX;
    int newMinDist = INT_MAX;
    
    for (const auto& player : playerUnits) {
        int currentDist = board->GetDistance(unit->GetPosition(), player->GetPosition());
        int newDist = board->GetDistance(pos, player->GetPosition());
        
        currentMinDist = std::min(currentMinDist, currentDist);
        newMinDist = std::min(newMinDist, newDist);
    }
    
    // Bonus for getting closer to enemies
    if (newMinDist < currentMinDist) {
        score += (currentMinDist - newMinDist) * 10;
    }
    
    // Check if move puts unit in attack range
    int attackRange = unit->GetStats().rng;
    if (newMinDist <= attackRange) {
        score += 40;  // Good position to attack next
    }
    
    // Penalty for moving away from enemies
    if (newMinDist > currentMinDist) {
        score -= (newMinDist - currentMinDist) * 5;
    }
    
    // Bonus for staying on friendly territory
    const auto& cell = board->GetCell(pos.x, pos.y);
    if (cell.owner == Owner::Enemy) {
        score += 10;
    }
    
    return score;
}

std::shared_ptr<Unit> AIController::FindNearestEnemy(std::shared_ptr<Unit> unit) {
    Board* board = m_game.GetBoard();
    auto playerUnits = board->GetUnitsByOwner(Owner::Player);
    
    std::shared_ptr<Unit> nearest = nullptr;
    int minDist = INT_MAX;
    
    for (const auto& player : playerUnits) {
        int dist = board->GetDistance(unit->GetPosition(), player->GetPosition());
        if (dist < minDist) {
            minDist = dist;
            nearest = player;
        }
    }
    
    return nearest;
}

std::shared_ptr<Unit> AIController::FindWeakestEnemy(std::shared_ptr<Unit> unit) {
    Board* board = m_game.GetBoard();
    auto playerUnits = board->GetUnitsByOwner(Owner::Player);
    
    std::shared_ptr<Unit> weakest = nullptr;
    int minHP = INT_MAX;
    
    for (const auto& player : playerUnits) {
        if (player->GetStats().hp < minHP) {
            minHP = player->GetStats().hp;
            weakest = player;
        }
    }
    
    return weakest;
}

bool AIController::IsInDanger(std::shared_ptr<Unit> unit) {
    Board* board = m_game.GetBoard();
    auto playerUnits = board->GetUnitsByOwner(Owner::Player);
    
    for (const auto& player : playerUnits) {
        int dist = board->GetDistance(unit->GetPosition(), player->GetPosition());
        if (dist <= player->GetStats().rng) {
            return true;  // Player can attack this unit
        }
    }
    
    return false;
}

} // namespace DDD
