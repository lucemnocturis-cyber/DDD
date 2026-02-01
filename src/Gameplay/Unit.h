#pragma once

#include "../Utils/Math.h"

#include <string>
#include <vector>
#include <memory>
#include <array>

namespace DDD {

// Forward declarations
class Renderer;
class Ability;
enum class Owner;

/**
 * Unit stats
 */
struct UnitStats {
    int hp = 0;
    int maxHp = 0;
    int atk = 0;
    int def = 0;
    int mov = 0;    // Movement range
    int rng = 1;    // Attack range (1 for melee, higher for ranged)
};

/**
 * Unit - represents a character on the battlefield
 */
class Unit {
public:
    Unit();
    virtual ~Unit();
    
    // Non-copyable
    Unit(const Unit&) = delete;
    Unit& operator=(const Unit&) = delete;
    
    /**
     * Update unit state
     */
    virtual void Update(float deltaTime);
    
    /**
     * Render the unit
     */
    virtual void Render(Renderer& renderer, int screenX, int screenY, int cellSize);
    
    // Combat
    void TakeDamage(int damage);
    void Heal(int amount);
    bool IsDead() const { return m_stats.hp <= 0; }
    
    // Movement
    void MoveTo(const Position& newPos);
    
    // Experience and promotion
    void GainExp(int amount);
    bool CanPromote() const;
    void Promote(const std::string& newClassName);
    
    // Accessors
    const std::string& GetClassName() const { return m_className; }
    void SetClassName(const std::string& name) { m_className = name; }
    
    int GetTier() const { return m_tier; }
    void SetTier(int tier) { m_tier = tier; }
    
    const UnitStats& GetStats() const { return m_stats; }
    UnitStats& GetMutableStats() { return m_stats; }
    void SetStats(const UnitStats& stats) { m_stats = stats; }
    
    const Position& GetPosition() const { return m_position; }
    void SetPosition(const Position& pos) { m_position = pos; }
    
    Owner GetOwner() const { return m_owner; }
    void SetOwner(Owner owner) { m_owner = owner; }
    
    int GetExp() const { return m_exp; }
    int GetExperience() const { return m_exp; }
    int GetExpToPromote() const { return m_expToPromote; }
    int GetExpToNextLevel() const { return m_expToPromote; }
    void SetExpToPromote(int exp) { m_expToPromote = exp; }
    
    int GetLevel() const { return m_level; }
    void SetLevel(int level) { m_level = level; }
    
    int GetMaxHP() const { return m_stats.maxHp > 0 ? m_stats.maxHp : m_stats.hp; }
    
    std::string GetAbilityName() const;
    
    const std::vector<std::string>& GetPromotionOptions() const { return m_promotionOptions; }
    void SetPromotionOptions(const std::vector<std::string>& options) { m_promotionOptions = options; }
    
    // Abilities
    const std::vector<std::shared_ptr<Ability>>& GetAbilities() const { return m_abilities; }
    void AddAbility(std::shared_ptr<Ability> ability);
    void RemoveAbility(const std::string& abilityName);
    
    // Action state (for turn management)
    bool HasMoved() const { return m_hasMoved; }
    bool HasAttacked() const { return m_hasAttacked; }
    void SetHasMoved(bool moved) { m_hasMoved = moved; }
    void SetHasAttacked(bool attacked) { m_hasAttacked = attacked; }
    void ResetActions() { m_hasMoved = false; m_hasAttacked = false; }
    
    // Unfurl pattern
    const std::array<int, 4>& GetUnfurlPattern() const { return m_unfurlPattern; }
    void SetUnfurlPattern(const std::array<int, 4>& pattern) { m_unfurlPattern = pattern; }
    
    // Status effects
    bool IsStunned() const { return m_stunTurns > 0; }
    bool IsPoisoned() const { return m_poisonTurns > 0; }
    void ApplyStun(int turns) { m_stunTurns = turns; }
    void ApplyPoison(int damage, int turns) { m_poisonDamage = damage; m_poisonTurns = turns; }
    void ProcessStatusEffects();
    
protected:
    // Identity
    std::string m_className;
    int m_tier = 0;
    
    // Stats
    UnitStats m_stats;
    
    // Position
    Position m_position;
    Owner m_owner;
    
    // Progression
    int m_exp = 0;
    int m_level = 1;
    int m_expToPromote = 100;
    std::vector<std::string> m_promotionOptions;
    
    // Abilities
    std::vector<std::shared_ptr<Ability>> m_abilities;
    
    // Turn state
    bool m_hasMoved = false;
    bool m_hasAttacked = false;
    
    // Unfurl pattern [north, east, south, west]
    std::array<int, 4> m_unfurlPattern = {1, 1, 1, 1};
    
    // Status effects
    int m_stunTurns = 0;
    int m_poisonTurns = 0;
    int m_poisonDamage = 0;
    
    // Animation state
    float m_animationTime = 0.0f;
};

} // namespace DDD
