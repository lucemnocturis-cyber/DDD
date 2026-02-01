#include "CombatSystem.h"
#include "Unit.h"
#include "../Utils/Logger.h"

#include <algorithm>
#include <cstdlib>

namespace DDD {

CombatSystem::CombatSystem() = default;

CombatResult CombatSystem::CalculateAttack(const Unit& attacker, const Unit& defender) {
    CombatResult result;
    
    // Base damage: ATK - DEF, minimum 1
    int baseDamage = std::max(1, attacker.GetStats().atk - defender.GetStats().def);
    
    // Check for critical hit
    result.isCritical = (static_cast<float>(rand()) / RAND_MAX) < CalculateCritChance(attacker);
    
    // Apply critical multiplier
    if (result.isCritical) {
        result.damage = static_cast<int>(baseDamage * m_critMultiplier);
    } else {
        result.damage = baseDamage;
    }
    
    // Check if this would kill the defender
    result.isKill = (defender.GetStats().hp - result.damage) <= 0;
    
    // Calculate rewards if kill
    if (result.isKill) {
        result.expGained = CalculateExpReward(defender);
        result.goldGained = CalculateGoldReward(defender);
    }
    
    return result;
}

CombatResult CombatSystem::ExecuteAttack(std::shared_ptr<Unit> attacker, std::shared_ptr<Unit> defender) {
    CombatResult result = CalculateAttack(*attacker, *defender);
    
    // Apply damage
    defender->TakeDamage(result.damage);
    
    // Mark attacker as having attacked
    attacker->SetHasAttacked(true);
    
    // Award EXP to attacker
    if (result.isKill) {
        attacker->GainExp(result.expGained);
    }
    
    Logger::Info("{} {} {} for {} damage{}{}",
                 attacker->GetClassName(),
                 result.isCritical ? "CRITS" : "attacks",
                 defender->GetClassName(),
                 result.damage,
                 result.isCritical ? " (CRITICAL!)" : "",
                 result.isKill ? " - KILL!" : "");
    
    return result;
}

float CombatSystem::CalculateHitChance(const Unit& attacker, const Unit& defender) {
    // Base hit chance, could be modified by stats or abilities later
    return m_baseHitChance;
}

float CombatSystem::CalculateCritChance(const Unit& attacker) {
    // Base crit chance, could be modified by stats or abilities later
    // Rogues might have higher crit chance
    float critChance = m_baseCritChance;
    
    std::string className = attacker.GetClassName();
    if (className == "Rogue" || className == "Assassin" || className == "Ninja") {
        critChance += 0.10f;  // +10% for rogue classes
    }
    
    return critChance;
}

int CombatSystem::CalculateExpReward(const Unit& defeatedUnit) {
    // Base EXP: 10 + (level * 5)
    int baseExp = 10 + defeatedUnit.GetLevel() * 5;
    
    // Tier bonus: +50% per tier
    int tier = defeatedUnit.GetTier();
    if (tier > 0) {
        baseExp = static_cast<int>(baseExp * (1.0f + tier * 0.5f));
    }
    
    return baseExp;
}

int CombatSystem::CalculateGoldReward(const Unit& defeatedUnit) {
    // Base gold: 5 + (level * 3)
    int baseGold = 5 + defeatedUnit.GetLevel() * 3;
    
    // Tier bonus: +25% per tier
    int tier = defeatedUnit.GetTier();
    if (tier > 0) {
        baseGold = static_cast<int>(baseGold * (1.0f + tier * 0.25f));
    }
    
    return baseGold;
}

} // namespace DDD
