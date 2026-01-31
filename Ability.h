#pragma once

#include "../Utils/Math.h"

#include <string>
#include <vector>
#include <memory>

namespace DDD {

// Forward declarations
class Unit;
class Board;

/**
 * Ability types
 */
enum class AbilityType {
    Attack,      // Deals damage
    Heal,        // Restores HP
    Buff,        // Enhances allies
    Debuff,      // Weakens enemies
    Movement,    // Affects positioning
    Summon,      // Creates units
    Utility      // Other effects
};

/**
 * Targeting modes
 */
enum class TargetType {
    Self,           // Only self
    SingleEnemy,    // One enemy unit
    SingleAlly,     // One friendly unit
    AllEnemies,     // All enemy units
    AllAllies,      // All friendly units
    Area,           // Area of effect
    Line,           // Line attack
    None            // No target needed
};

/**
 * Ability - base class for all unit abilities
 */
class Ability {
public:
    Ability();
    virtual ~Ability();
    
    /**
     * Check if the ability can be used
     */
    virtual bool CanUse(const Unit& caster, const Board& board) const;
    
    /**
     * Execute the ability
     */
    virtual void Execute(Unit& caster, const std::vector<Unit*>& targets, Board& board) = 0;
    
    /**
     * Get valid target positions
     */
    virtual std::vector<Position> GetValidTargets(const Unit& caster, const Board& board) const;
    
    /**
     * Reduce cooldown at start of turn
     */
    void TickCooldown();
    
    /**
     * Check if ability is ready
     */
    bool IsReady() const { return m_currentCooldown == 0; }
    
    // Accessors
    const std::string& GetName() const { return m_name; }
    const std::string& GetDescription() const { return m_description; }
    AbilityType GetType() const { return m_type; }
    TargetType GetTargetType() const { return m_targetType; }
    int GetCooldown() const { return m_cooldown; }
    int GetCurrentCooldown() const { return m_currentCooldown; }
    int GetRange() const { return m_range; }
    int GetAoeRadius() const { return m_aoeRadius; }
    
protected:
    std::string m_name;
    std::string m_description;
    AbilityType m_type = AbilityType::Attack;
    TargetType m_targetType = TargetType::SingleEnemy;
    int m_cooldown = 1;
    int m_currentCooldown = 0;
    int m_range = 1;
    int m_aoeRadius = 0;  // 0 = single target
};

// ============================================================================
// Concrete Ability Classes
// ============================================================================

/**
 * Fireball - 2x2 AoE fire damage
 */
class FireballAbility : public Ability {
public:
    FireballAbility();
    void Execute(Unit& caster, const std::vector<Unit*>& targets, Board& board) override;
};

/**
 * Ice Shard - Single target damage + freeze
 */
class IceShardAbility : public Ability {
public:
    IceShardAbility();
    void Execute(Unit& caster, const std::vector<Unit*>& targets, Board& board) override;
};

/**
 * Shield Bash - Damage + push
 */
class ShieldBashAbility : public Ability {
public:
    ShieldBashAbility();
    void Execute(Unit& caster, const std::vector<Unit*>& targets, Board& board) override;
};

/**
 * Backstab - High damage from behind
 */
class BackstabAbility : public Ability {
public:
    BackstabAbility();
    void Execute(Unit& caster, const std::vector<Unit*>& targets, Board& board) override;
};

/**
 * Poison - Damage over time
 */
class PoisonAbility : public Ability {
public:
    PoisonAbility();
    void Execute(Unit& caster, const std::vector<Unit*>& targets, Board& board) override;
};

/**
 * Taunt - Force enemies to attack this unit
 */
class TauntAbility : public Ability {
public:
    TauntAbility();
    void Execute(Unit& caster, const std::vector<Unit*>& targets, Board& board) override;
};

/**
 * Defensive Stance - Increase defense
 */
class DefensiveStanceAbility : public Ability {
public:
    DefensiveStanceAbility();
    void Execute(Unit& caster, const std::vector<Unit*>& targets, Board& board) override;
};

/**
 * AbilityFactory - creates abilities by name
 */
class AbilityFactory {
public:
    static std::shared_ptr<Ability> CreateAbility(const std::string& name);
};

} // namespace DDD
