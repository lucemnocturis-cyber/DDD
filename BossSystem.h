#pragma once

#include "../Utils/Math.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace DDD {

class Unit;
class Board;
class Renderer;

/**
 * Boss phase definition
 */
struct BossPhase {
    std::string name;
    float healthThreshold;      // Triggers when HP falls below this percentage
    std::string abilityName;    // Special ability for this phase
    int abilityDamage;
    int abilityCooldown;
    float statMultiplier;       // Stat boost for this phase
    std::string announcement;   // Text to show when phase starts
};

/**
 * Boss attack patterns
 */
enum class BossAttackType {
    SingleTarget,       // High damage to one unit
    Cleave,            // Hits adjacent units too
    AreaOfEffect,      // Damages all units in radius
    LineAttack,        // Damages all units in a line
    SummonMinions,     // Spawns additional enemies
    Buff,              // Buffs self or allies
    Debuff,            // Debuffs player units
    Heal               // Heals self
};

/**
 * Boss special attack definition
 */
struct BossAttack {
    std::string name;
    BossAttackType type;
    int damage;
    int range;
    int radius;         // For AoE attacks
    int cooldown;       // Turns between uses
    int currentCooldown;
    std::string effectName;  // Visual/sound effect
    std::string description;
};

/**
 * Boss definition
 */
struct BossDef {
    std::string id;
    std::string name;
    std::string title;          // e.g., "Ancient Dragon" + "Wyrm of Destruction"
    
    // Phases (triggered by HP thresholds)
    std::vector<BossPhase> phases;
    
    // Special attacks
    std::vector<BossAttack> attacks;
    
    // Summon data (for summoner bosses)
    std::vector<std::string> summonIds;
    int maxSummons;
    
    // Enrage mechanics
    bool hasEnrage;
    int enrageTurn;         // Turn number when boss enrages
    float enrageMultiplier; // Damage multiplier when enraged
};

/**
 * Active boss state during combat
 */
struct ActiveBoss {
    std::shared_ptr<Unit> unit;
    const BossDef* definition;
    int currentPhase;
    bool isEnraged;
    int turnCount;
    std::vector<int> attackCooldowns;
    std::vector<std::shared_ptr<Unit>> summonedUnits;
    
    // Animation state
    float phaseTransitionTimer;
    bool showingPhaseTransition;
    std::string phaseAnnouncement;
};

/**
 * BossSystem - manages boss encounters
 */
class BossSystem {
public:
    static BossSystem& Instance();
    
    void Initialize();
    
    /**
     * Get boss definition
     */
    const BossDef* GetBossDef(const std::string& id) const;
    
    /**
     * Start a boss encounter
     */
    void StartBossEncounter(std::shared_ptr<Unit> bossUnit, const std::string& bossId);
    
    /**
     * End boss encounter
     */
    void EndBossEncounter();
    
    /**
     * Check if boss encounter is active
     */
    bool IsBossActive() const { return m_activeBoss.unit != nullptr; }
    
    /**
     * Get active boss
     */
    const ActiveBoss& GetActiveBoss() const { return m_activeBoss; }
    ActiveBoss& GetActiveBoss() { return m_activeBoss; }
    
    /**
     * Update boss state (call after boss takes damage)
     */
    void UpdateBossState();
    
    /**
     * Process boss turn - select and execute attack
     */
    struct BossAction {
        BossAttackType type;
        std::string attackName;
        Position targetPos;
        std::vector<Position> affectedCells;
        int damage;
        std::vector<std::string> summonIds;
    };
    BossAction SelectBossAction(Board& board);
    
    /**
     * Process boss turn end
     */
    void OnBossTurnEnd();
    
    /**
     * Update animations
     */
    void Update(float deltaTime);
    
    /**
     * Render boss UI elements
     */
    void Render(Renderer& renderer, int screenWidth, int screenHeight);
    
    /**
     * Get cells that will be affected by boss's next attack (for preview)
     */
    std::vector<Position> GetThreatZone(Board& board) const;
    
private:
    BossSystem() = default;
    
    void RegisterBoss(const BossDef& def);
    void RegisterAllBosses();
    
    void CheckPhaseTransition();
    int SelectBestAttack(Board& board);
    Position FindBestTarget(Board& board, const BossAttack& attack);
    std::vector<Position> CalculateAffectedCells(const BossAttack& attack, const Position& target, Board& board);
    
    std::unordered_map<std::string, BossDef> m_bosses;
    ActiveBoss m_activeBoss;
    bool m_initialized = false;
    
    // UI animation
    float m_healthBarPulse = 0.0f;
    float m_titleGlow = 0.0f;
};

} // namespace DDD
