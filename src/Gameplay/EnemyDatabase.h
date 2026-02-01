#pragma once

#include "UnitDatabase.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace DDD {

class Unit;

/**
 * Enemy difficulty tier
 */
enum class EnemyTier {
    Minion,     // Weak fodder enemies
    Regular,    // Standard enemies
    Elite,      // Stronger variants
    Champion,   // Mini-boss level
    Boss        // Wave boss
};

/**
 * Enemy behavior type
 */
enum class EnemyBehavior {
    Aggressive,     // Rushes toward player units
    Defensive,      // Holds position, attacks when in range
    Flanker,        // Tries to attack from sides/behind
    Supporter,      // Buffs/heals other enemies
    Assassin,       // Targets weak/injured units
    Berserker       // Gets stronger when damaged
};

/**
 * Enemy definition
 */
struct EnemyDef {
    std::string id;
    std::string name;
    std::string description;
    UnitArchetype archetype;
    EnemyTier tier;
    EnemyBehavior behavior;
    
    // Base stats
    int hp;
    int atk;
    int def;
    int mov;
    int rng;
    
    // Unfurl pattern
    std::array<int, 4> unfurlPattern;
    
    // Special ability (empty if none)
    std::string abilityName;
    
    // Rewards
    int expReward;
    int goldReward;
    
    // Visual
    SDL_Color tintColor;  // Color tint for variety
};

/**
 * Wave composition template
 */
struct WaveTemplate {
    std::string id;
    std::string name;
    int minWave;        // Earliest wave this can appear
    int maxWave;        // Latest wave (0 = no limit)
    
    // Enemy spawn weights
    std::vector<std::pair<std::string, int>> enemyWeights;
    
    // Total enemy count range
    int minEnemies;
    int maxEnemies;
    
    // Chance for elite/champion spawn
    int eliteChance;     // Percentage
    int championChance;  // Percentage
    
    // Boss wave?
    bool hasBoss;
    std::string bossId;
};

/**
 * EnemyDatabase - central repository of all enemy definitions
 */
class EnemyDatabase {
public:
    static EnemyDatabase& Instance();
    
    void Initialize();
    
    /**
     * Get enemy definition
     */
    const EnemyDef* GetEnemyDef(const std::string& id) const;
    
    /**
     * Get all enemies of a tier
     */
    std::vector<std::string> GetEnemiesByTier(EnemyTier tier) const;
    
    /**
     * Get wave template
     */
    const WaveTemplate* GetWaveTemplate(const std::string& id) const;
    
    /**
     * Get appropriate wave template for wave number
     */
    const WaveTemplate* GetTemplateForWave(int waveNumber) const;
    
    /**
     * Create an enemy unit
     */
    std::shared_ptr<Unit> CreateEnemy(const std::string& id) const;
    
    /**
     * Generate enemies for a wave
     */
    std::vector<std::shared_ptr<Unit>> GenerateWaveEnemies(int waveNumber) const;
    
    /**
     * Get tier color
     */
    static SDL_Color GetTierColor(EnemyTier tier);
    static std::string GetTierName(EnemyTier tier);
    
private:
    EnemyDatabase() = default;
    
    void RegisterEnemy(const EnemyDef& def);
    void RegisterWaveTemplate(const WaveTemplate& tmpl);
    
    // Enemy registration by category
    void RegisterMinionEnemies();
    void RegisterRegularEnemies();
    void RegisterEliteEnemies();
    void RegisterChampionEnemies();
    void RegisterBossEnemies();
    void RegisterWaveTemplates();
    
    std::unordered_map<std::string, EnemyDef> m_enemies;
    std::unordered_map<std::string, WaveTemplate> m_waveTemplates;
    bool m_initialized = false;
};

} // namespace DDD
