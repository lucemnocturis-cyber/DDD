#pragma once

#include <memory>
#include <functional>

namespace DDD {

class Unit;
class Board;

/**
 * Result of a combat action
 */
struct CombatResult {
    int damage = 0;
    bool isCritical = false;
    bool isKill = false;
    int expGained = 0;
    int goldGained = 0;
};

/**
 * CombatSystem - handles all combat calculations and effects
 */
class CombatSystem {
public:
    CombatSystem();
    ~CombatSystem() = default;
    
    /**
     * Calculate damage for an attack
     */
    CombatResult CalculateAttack(const Unit& attacker, const Unit& defender);
    
    /**
     * Execute an attack (modify units)
     */
    CombatResult ExecuteAttack(std::shared_ptr<Unit> attacker, std::shared_ptr<Unit> defender);
    
    /**
     * Calculate hit chance (0.0 - 1.0)
     */
    float CalculateHitChance(const Unit& attacker, const Unit& defender);
    
    /**
     * Calculate critical hit chance (0.0 - 1.0)
     */
    float CalculateCritChance(const Unit& attacker);
    
    /**
     * Get EXP reward for killing a unit
     */
    int CalculateExpReward(const Unit& defeatedUnit);
    
    /**
     * Get Gold reward for killing a unit
     */
    int CalculateGoldReward(const Unit& defeatedUnit);
    
    // Configuration
    void SetCritMultiplier(float mult) { m_critMultiplier = mult; }
    void SetBaseCritChance(float chance) { m_baseCritChance = chance; }
    void SetBaseHitChance(float chance) { m_baseHitChance = chance; }
    
private:
    float m_critMultiplier = 2.0f;
    float m_baseCritChance = 0.10f;  // 10%
    float m_baseHitChance = 0.95f;   // 95%
};

} // namespace DDD
