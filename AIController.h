#pragma once

#include "../Unit.h"
#include "../../Utils/Math.h"

#include <memory>
#include <vector>
#include <functional>

namespace DDD {

class Board;
class Unit;
class Game;

/**
 * Possible AI actions
 */
enum class AIActionType {
    None,
    Move,
    Attack,
    UseAbility,
    Wait
};

/**
 * Represents a single AI action
 */
struct AIAction {
    AIActionType type = AIActionType::None;
    Position targetPos;
    std::shared_ptr<Unit> targetUnit;
    int priority = 0;  // Higher = more important
    std::string description;
};

/**
 * AIController - controls enemy unit behavior
 */
class AIController {
public:
    AIController(Game& game);
    ~AIController() = default;
    
    /**
     * Decide the best action for a unit
     */
    AIAction DecideAction(std::shared_ptr<Unit> unit);
    
    /**
     * Execute an action
     */
    void ExecuteAction(std::shared_ptr<Unit> unit, const AIAction& action);
    
    /**
     * Set callback for when AI attacks
     */
    using AttackCallback = std::function<void(std::shared_ptr<Unit>, std::shared_ptr<Unit>, int)>;
    void SetOnAttack(AttackCallback cb) { m_onAttack = cb; }
    
    /**
     * Set callback for when AI moves
     */
    using MoveCallback = std::function<void(std::shared_ptr<Unit>, Position)>;
    void SetOnMove(MoveCallback cb) { m_onMove = cb; }
    
private:
    // Decision making
    std::vector<AIAction> GeneratePossibleActions(std::shared_ptr<Unit> unit);
    AIAction EvaluateBestAction(const std::vector<AIAction>& actions);
    
    // Action generation
    void GenerateAttackActions(std::shared_ptr<Unit> unit, std::vector<AIAction>& actions);
    void GenerateMoveActions(std::shared_ptr<Unit> unit, std::vector<AIAction>& actions);
    
    // Scoring helpers
    int ScoreAttackTarget(std::shared_ptr<Unit> attacker, std::shared_ptr<Unit> target);
    int ScoreMovePosition(std::shared_ptr<Unit> unit, const Position& pos);
    
    // Utility
    std::shared_ptr<Unit> FindNearestEnemy(std::shared_ptr<Unit> unit);
    std::shared_ptr<Unit> FindWeakestEnemy(std::shared_ptr<Unit> unit);
    bool IsInDanger(std::shared_ptr<Unit> unit);
    
    Game& m_game;
    AttackCallback m_onAttack;
    MoveCallback m_onMove;
};

} // namespace DDD
