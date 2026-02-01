#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cmath>

namespace DDD {

/**
 * Difficulty presets
 */
enum class DifficultyLevel {
    Easy,
    Normal,
    Hard,
    Nightmare
};

/**
 * Stat curve types for scaling
 */
enum class CurveType {
    Linear,         // y = base + wave * growth
    Exponential,    // y = base * (1 + growth)^wave
    Logarithmic,    // y = base + growth * ln(wave + 1)
    Sigmoid,        // S-curve, slow start, fast middle, slow end
    StepFunction    // Discrete jumps at thresholds
};

/**
 * Balance parameters for a specific aspect
 */
struct BalanceParam {
    std::string id;
    std::string name;
    std::string category;
    
    float baseValue;
    float growthRate;
    CurveType curveType;
    
    float minValue;
    float maxValue;
    
    // Per-difficulty multipliers
    float easyMult = 0.7f;
    float normalMult = 1.0f;
    float hardMult = 1.3f;
    float nightmareMult = 1.6f;
};

/**
 * Economy balance settings
 */
struct EconomyBalance {
    // Gold rewards
    int baseKillGold = 5;
    int goldPerEnemyTier = 3;
    int waveCompletionGold = 50;
    int waveGoldGrowth = 10;
    int bossKillGold = 100;
    
    // Shop prices
    float shopPriceMultiplier = 1.0f;
    float shopPriceWaveScaling = 0.05f;  // +5% per wave
    int shopRerollCost = 25;
    int shopRerollCostGrowth = 10;
    
    // Dice costs
    int baseDiceCost = 50;
    int diceCostPerRarity = 30;
    float diceCooldownReduction = 0.0f;
    
    // Item drops
    float itemDropChance = 0.15f;
    float itemDropChancePerWave = 0.01f;
    float rareItemChance = 0.1f;
};

/**
 * Combat balance settings
 */
struct CombatBalance {
    // Damage formula: (ATK * atkMultiplier - DEF * defMultiplier) * randomVariance
    float atkMultiplier = 1.0f;
    float defMultiplier = 0.5f;
    float minDamagePercent = 0.1f;  // Minimum 10% of raw damage
    float randomVarianceMin = 0.9f;
    float randomVarianceMax = 1.1f;
    
    // Critical hits
    float baseCritChance = 0.1f;
    float critDamageMultiplier = 1.5f;
    float critChancePerLevel = 0.01f;
    
    // Accuracy/Evasion
    float baseHitChance = 0.95f;
    float evasionCap = 0.5f;
    
    // Status effects
    float poisonDamagePercent = 0.1f;  // % of max HP
    float burnDamageFlat = 5;
    int stunMaxDuration = 2;
    float slowMovementReduction = 0.5f;
};

/**
 * Progression balance settings
 */
struct ProgressionBalance {
    // Experience
    int baseExpPerKill = 10;
    int expPerEnemyTier = 5;
    int expPerWave = 20;
    float expRequirementGrowth = 1.5f;  // Each level needs 50% more
    int baseExpToLevel = 100;
    
    // Promotion
    int promotionLevel = 5;
    int maxTier = 3;
    float promotionStatBonus = 0.2f;  // +20% stats on promote
    
    // Unit limits
    int maxUnitsOnBoard = 8;
    int startingDice = 3;
    int maxDiceInHand = 6;
};

/**
 * Wave balance settings
 */
struct WaveBalance {
    // Enemy count
    int baseEnemyCount = 3;
    float enemyCountGrowth = 0.5f;  // +0.5 per wave
    int maxEnemiesPerWave = 12;
    
    // Enemy stats scaling
    float enemyHpScaling = 0.1f;      // +10% per wave
    float enemyAtkScaling = 0.05f;    // +5% per wave
    float enemyDefScaling = 0.03f;    // +3% per wave
    
    // Elite/Champion spawns
    int eliteStartWave = 3;
    float eliteChanceBase = 0.1f;
    float eliteChanceGrowth = 0.05f;
    int championStartWave = 7;
    float championChanceBase = 0.05f;
    float championChanceGrowth = 0.03f;
    
    // Boss waves
    std::vector<int> bossWaves = {5, 10, 15, 20};
    float bossHpScaling = 0.15f;
    float bossAtkScaling = 0.08f;
};

/**
 * BalanceSystem - central repository for all game balance
 */
class BalanceSystem {
public:
    static BalanceSystem& Instance();
    
    void Initialize();
    
    // Difficulty
    void SetDifficulty(DifficultyLevel level);
    DifficultyLevel GetDifficulty() const { return m_difficulty; }
    float GetDifficultyMultiplier() const;
    
    // Parameter access
    float GetParam(const std::string& id) const;
    float GetScaledParam(const std::string& id, int wave) const;
    void SetParam(const std::string& id, float value);
    
    // Balance settings
    const EconomyBalance& GetEconomy() const { return m_economy; }
    const CombatBalance& GetCombat() const { return m_combat; }
    const ProgressionBalance& GetProgression() const { return m_progression; }
    const WaveBalance& GetWave() const { return m_wave; }
    
    // Calculation helpers
    int CalculateDamage(int attackerAtk, int defenderDef, bool isCrit = false) const;
    int CalculateExpRequired(int currentLevel) const;
    int CalculateGoldReward(int enemyTier, int waveNumber) const;
    int CalculateShopPrice(int basePrice, int waveNumber) const;
    float CalculateEnemyStatMultiplier(int waveNumber) const;
    int CalculateEnemyCount(int waveNumber) const;
    float CalculateEliteChance(int waveNumber) const;
    float CalculateChampionChance(int waveNumber) const;
    bool IsBossWave(int waveNumber) const;
    
    // Curve calculations
    static float ApplyCurve(float base, float growth, int wave, CurveType curve);
    
    // Debug/Testing
    void PrintBalanceReport() const;
    void ExportToJson(const std::string& filename) const;
    bool ImportFromJson(const std::string& filename);
    
private:
    BalanceSystem() = default;
    
    void RegisterParam(const BalanceParam& param);
    void RegisterAllParams();
    void ApplyDifficultyScaling();
    
    std::unordered_map<std::string, BalanceParam> m_params;
    
    EconomyBalance m_economy;
    CombatBalance m_combat;
    ProgressionBalance m_progression;
    WaveBalance m_wave;
    
    DifficultyLevel m_difficulty = DifficultyLevel::Normal;
    bool m_initialized = false;
};

/**
 * Balance presets for quick testing
 */
namespace BalancePresets {
    void ApplyEasyMode(BalanceSystem& system);
    void ApplyNormalMode(BalanceSystem& system);
    void ApplyHardMode(BalanceSystem& system);
    void ApplyNightmareMode(BalanceSystem& system);
    
    // Special presets
    void ApplyTutorialMode(BalanceSystem& system);
    void ApplySpeedRunMode(BalanceSystem& system);
    void ApplyEndlessMode(BalanceSystem& system);
}

} // namespace DDD
