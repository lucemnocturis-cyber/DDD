#pragma once

#include "Ability.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

namespace DDD {

class Unit;
class Board;
class ParticleSystem;

/**
 * Status effect types
 */
enum class StatusEffect {
    None,
    Stun,           // Cannot act
    Poison,         // Damage over time
    Burn,           // Fire damage over time
    Freeze,         // Cannot move
    Slow,           // Reduced movement
    Weaken,         // Reduced attack
    Vulnerability,  // Increased damage taken
    Regeneration,   // Heal over time
    Shield,         // Damage absorption
    AttackBoost,    // Increased attack
    DefenseBoost,   // Increased defense
    Haste,          // Increased movement
    Taunt,          // Must be attacked
    Stealth,        // Cannot be targeted
    Marked          // Takes bonus damage
};

/**
 * Ability effect definition
 */
struct AbilityDef {
    std::string id;
    std::string name;
    std::string description;
    std::string iconName;       // For UI
    
    AbilityType type;
    TargetType targetType;
    
    int baseDamage;             // Base damage (0 for non-damage)
    int baseHeal;               // Base healing (0 for non-heal)
    float atkScaling;           // Damage scales with ATK
    
    int range;
    int aoeRadius;              // 0 = single target
    int cooldown;
    int manaCost;               // If mana system added
    
    // Status effect
    StatusEffect appliesEffect;
    int effectDuration;
    int effectPower;            // Damage for DoTs, etc
    
    // Special flags
    bool canTargetSelf;
    bool canTargetAllies;
    bool canTargetEnemies;
    bool ignoresDefense;
    bool guaranteedCrit;
    float critBonus;            // Extra crit chance
    
    // Visual
    std::string particleEffect;
    std::string soundEffect;
};

/**
 * Ability result for UI feedback
 */
struct AbilityResult {
    bool success;
    std::vector<std::pair<Unit*, int>> damageDealt;  // Unit, amount
    std::vector<std::pair<Unit*, int>> healingDone;
    std::vector<std::pair<Unit*, StatusEffect>> effectsApplied;
    std::string message;
};

/**
 * AbilityDatabase - central repository of all ability definitions
 */
class AbilityDatabase {
public:
    static AbilityDatabase& Instance();
    
    void Initialize();
    
    /**
     * Get ability definition
     */
    const AbilityDef* GetAbilityDef(const std::string& id) const;
    
    /**
     * Get abilities by type
     */
    std::vector<std::string> GetAbilitiesByType(AbilityType type) const;
    
    /**
     * Execute an ability
     */
    AbilityResult ExecuteAbility(const std::string& abilityId, 
                                  Unit& caster, 
                                  const Position& targetPos,
                                  Board& board);
    
    /**
     * Get valid targets for an ability
     */
    std::vector<Position> GetValidTargets(const std::string& abilityId,
                                          const Unit& caster,
                                          const Board& board) const;
    
    /**
     * Get affected cells for AoE preview
     */
    std::vector<Position> GetAffectedCells(const std::string& abilityId,
                                           const Position& casterPos,
                                           const Position& targetPos,
                                           const Board& board) const;
    
    /**
     * Check if ability can be used
     */
    bool CanUseAbility(const std::string& abilityId,
                       const Unit& caster,
                       const Board& board) const;
    
    /**
     * Get status effect name
     */
    static std::string GetEffectName(StatusEffect effect);
    static SDL_Color GetEffectColor(StatusEffect effect);
    
private:
    AbilityDatabase() = default;
    
    void RegisterAbility(const AbilityDef& def);
    
    // Category registration
    void RegisterMageAbilities();
    void RegisterSoldierAbilities();
    void RegisterRogueAbilities();
    void RegisterHealerAbilities();
    void RegisterTankAbilities();
    void RegisterArcherAbilities();
    void RegisterBossAbilities();
    
    // Helper methods
    int CalculateDamage(const AbilityDef& def, const Unit& caster, const Unit& target) const;
    int CalculateHeal(const AbilityDef& def, const Unit& caster) const;
    void ApplyStatusEffect(Unit& target, StatusEffect effect, int duration, int power);
    
    std::unordered_map<std::string, AbilityDef> m_abilities;
    bool m_initialized = false;
};

} // namespace DDD
