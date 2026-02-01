#include "BalanceSystem.h"
#include "../Utils/Logger.h"
#include "../Utils/Random.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace DDD {

BalanceSystem& BalanceSystem::Instance() {
    static BalanceSystem instance;
    return instance;
}

void BalanceSystem::Initialize() {
    if (m_initialized) return;
    
    RegisterAllParams();
    ApplyDifficultyScaling();
    
    m_initialized = true;
    Logger::Info("BalanceSystem initialized with {} parameters", m_params.size());
}

void BalanceSystem::RegisterParam(const BalanceParam& param) {
    m_params[param.id] = param;
}

void BalanceSystem::SetDifficulty(DifficultyLevel level) {
    m_difficulty = level;
    ApplyDifficultyScaling();
    Logger::Info("Difficulty set to: {}", static_cast<int>(level));
}

float BalanceSystem::GetDifficultyMultiplier() const {
    switch (m_difficulty) {
        case DifficultyLevel::Easy: return 0.7f;
        case DifficultyLevel::Normal: return 1.0f;
        case DifficultyLevel::Hard: return 1.3f;
        case DifficultyLevel::Nightmare: return 1.6f;
        default: return 1.0f;
    }
}

float BalanceSystem::GetParam(const std::string& id) const {
    auto it = m_params.find(id);
    if (it != m_params.end()) {
        return it->second.baseValue;
    }
    return 0.0f;
}

float BalanceSystem::GetScaledParam(const std::string& id, int wave) const {
    auto it = m_params.find(id);
    if (it != m_params.end()) {
        const BalanceParam& param = it->second;
        float value = ApplyCurve(param.baseValue, param.growthRate, wave, param.curveType);
        return std::clamp(value, param.minValue, param.maxValue);
    }
    return 0.0f;
}

void BalanceSystem::SetParam(const std::string& id, float value) {
    auto it = m_params.find(id);
    if (it != m_params.end()) {
        it->second.baseValue = value;
    }
}

float BalanceSystem::ApplyCurve(float base, float growth, int wave, CurveType curve) {
    switch (curve) {
        case CurveType::Linear:
            return base + wave * growth;
            
        case CurveType::Exponential:
            return base * std::pow(1.0f + growth, wave);
            
        case CurveType::Logarithmic:
            return base + growth * std::log(static_cast<float>(wave + 1));
            
        case CurveType::Sigmoid: {
            // S-curve centered around wave 10
            float x = (wave - 10.0f) / 5.0f;
            float sigmoid = 1.0f / (1.0f + std::exp(-x));
            return base + growth * sigmoid * wave;
        }
        
        case CurveType::StepFunction: {
            // Steps at waves 5, 10, 15, 20
            int steps = wave / 5;
            return base + growth * steps;
        }
        
        default:
            return base;
    }
}

// ===========================================================================
// CALCULATION HELPERS
// ===========================================================================

int BalanceSystem::CalculateDamage(int attackerAtk, int defenderDef, bool isCrit) const {
    // Base damage formula
    float rawDamage = attackerAtk * m_combat.atkMultiplier;
    float mitigation = defenderDef * m_combat.defMultiplier;
    float damage = rawDamage - mitigation;
    
    // Minimum damage (percentage of raw)
    float minDamage = rawDamage * m_combat.minDamagePercent;
    damage = std::max(damage, minDamage);
    
    // Random variance
    float variance = Random::Range(m_combat.randomVarianceMin, m_combat.randomVarianceMax);
    damage *= variance;
    
    // Critical hit
    if (isCrit) {
        damage *= m_combat.critDamageMultiplier;
    }
    
    // Difficulty scaling (enemies deal more damage on hard)
    // This would be applied contextually by caller
    
    return std::max(1, static_cast<int>(damage));
}

int BalanceSystem::CalculateExpRequired(int currentLevel) const {
    // Exponential growth
    float exp = m_progression.baseExpToLevel * 
                std::pow(m_progression.expRequirementGrowth, currentLevel - 1);
    return static_cast<int>(exp);
}

int BalanceSystem::CalculateGoldReward(int enemyTier, int waveNumber) const {
    int base = m_economy.baseKillGold + (enemyTier * m_economy.goldPerEnemyTier);
    
    // Wave scaling (slight increase over time)
    float waveBonus = 1.0f + (waveNumber * 0.02f);
    
    return static_cast<int>(base * waveBonus);
}

int BalanceSystem::CalculateShopPrice(int basePrice, int waveNumber) const {
    float multiplier = m_economy.shopPriceMultiplier;
    multiplier += waveNumber * m_economy.shopPriceWaveScaling;
    
    return static_cast<int>(basePrice * multiplier);
}

float BalanceSystem::CalculateEnemyStatMultiplier(int waveNumber) const {
    // Exponential scaling that starts gentle
    return 1.0f + (waveNumber - 1) * m_wave.enemyHpScaling;
}

int BalanceSystem::CalculateEnemyCount(int waveNumber) const {
    float count = m_wave.baseEnemyCount + (waveNumber * m_wave.enemyCountGrowth);
    count *= GetDifficultyMultiplier();
    
    return std::min(static_cast<int>(count), m_wave.maxEnemiesPerWave);
}

float BalanceSystem::CalculateEliteChance(int waveNumber) const {
    if (waveNumber < m_wave.eliteStartWave) return 0.0f;
    
    float chance = m_wave.eliteChanceBase + 
                   (waveNumber - m_wave.eliteStartWave) * m_wave.eliteChanceGrowth;
    
    return std::min(chance, 0.5f);  // Cap at 50%
}

float BalanceSystem::CalculateChampionChance(int waveNumber) const {
    if (waveNumber < m_wave.championStartWave) return 0.0f;
    
    float chance = m_wave.championChanceBase + 
                   (waveNumber - m_wave.championStartWave) * m_wave.championChanceGrowth;
    
    return std::min(chance, 0.25f);  // Cap at 25%
}

bool BalanceSystem::IsBossWave(int waveNumber) const {
    return std::find(m_wave.bossWaves.begin(), m_wave.bossWaves.end(), waveNumber) 
           != m_wave.bossWaves.end();
}

void BalanceSystem::ApplyDifficultyScaling() {
    float mult = GetDifficultyMultiplier();
    
    // Scale enemy stats
    m_wave.enemyHpScaling = 0.1f * mult;
    m_wave.enemyAtkScaling = 0.05f * mult;
    
    // Scale economy (easier = more gold)
    m_economy.shopPriceMultiplier = 1.0f + (mult - 1.0f) * 0.5f;
    
    // Scale progression (easier = more exp)
    float expMult = 1.0f + (1.0f - mult) * 0.3f;
    m_progression.baseExpPerKill = static_cast<int>(10 * expMult);
}

// ===========================================================================
// DEBUG AND EXPORT
// ===========================================================================

void BalanceSystem::PrintBalanceReport() const {
    Logger::Info("=== BALANCE REPORT ===");
    Logger::Info("Difficulty: {}", static_cast<int>(m_difficulty));
    Logger::Info("");
    
    Logger::Info("-- Economy --");
    Logger::Info("Base Kill Gold: {}", m_economy.baseKillGold);
    Logger::Info("Wave Completion Gold: {}", m_economy.waveCompletionGold);
    Logger::Info("Shop Price Mult: {:.2f}", m_economy.shopPriceMultiplier);
    Logger::Info("");
    
    Logger::Info("-- Combat --");
    Logger::Info("ATK Multiplier: {:.2f}", m_combat.atkMultiplier);
    Logger::Info("DEF Multiplier: {:.2f}", m_combat.defMultiplier);
    Logger::Info("Crit Chance: {:.0f}%", m_combat.baseCritChance * 100);
    Logger::Info("Crit Damage: {:.0f}%", m_combat.critDamageMultiplier * 100);
    Logger::Info("");
    
    Logger::Info("-- Wave Scaling (at wave 10) --");
    Logger::Info("Enemy Count: {}", CalculateEnemyCount(10));
    Logger::Info("Enemy HP Mult: {:.2f}x", CalculateEnemyStatMultiplier(10));
    Logger::Info("Elite Chance: {:.0f}%", CalculateEliteChance(10) * 100);
    Logger::Info("Champion Chance: {:.0f}%", CalculateChampionChance(10) * 100);
    Logger::Info("");
    
    Logger::Info("-- Progression --");
    Logger::Info("EXP to Level 2: {}", CalculateExpRequired(1));
    Logger::Info("EXP to Level 5: {}", CalculateExpRequired(4));
    Logger::Info("EXP to Level 10: {}", CalculateExpRequired(9));
}

void BalanceSystem::ExportToJson(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        Logger::Error("Failed to export balance to: {}", filename);
        return;
    }
    
    file << "{\n";
    file << "  \"difficulty\": " << static_cast<int>(m_difficulty) << ",\n";
    file << "  \"economy\": {\n";
    file << "    \"baseKillGold\": " << m_economy.baseKillGold << ",\n";
    file << "    \"waveCompletionGold\": " << m_economy.waveCompletionGold << ",\n";
    file << "    \"shopPriceMultiplier\": " << m_economy.shopPriceMultiplier << "\n";
    file << "  },\n";
    file << "  \"combat\": {\n";
    file << "    \"atkMultiplier\": " << m_combat.atkMultiplier << ",\n";
    file << "    \"defMultiplier\": " << m_combat.defMultiplier << ",\n";
    file << "    \"critChance\": " << m_combat.baseCritChance << ",\n";
    file << "    \"critDamage\": " << m_combat.critDamageMultiplier << "\n";
    file << "  },\n";
    file << "  \"wave\": {\n";
    file << "    \"baseEnemyCount\": " << m_wave.baseEnemyCount << ",\n";
    file << "    \"enemyHpScaling\": " << m_wave.enemyHpScaling << ",\n";
    file << "    \"eliteStartWave\": " << m_wave.eliteStartWave << "\n";
    file << "  }\n";
    file << "}\n";
    
    file.close();
    Logger::Info("Balance exported to: {}", filename);
}

bool BalanceSystem::ImportFromJson(const std::string& filename) {
    // Simplified import - would use nlohmann/json in full implementation
    Logger::Info("Balance import not fully implemented");
    return false;
}

// ===========================================================================
// PARAMETER REGISTRATION
// ===========================================================================

void BalanceSystem::RegisterAllParams() {
    // Enemy HP scaling
    RegisterParam({
        "enemy_hp_base", "Enemy Base HP", "enemies",
        15.0f, 0.1f, CurveType::Linear,
        10.0f, 500.0f,
        0.8f, 1.0f, 1.2f, 1.5f
    });
    
    // Enemy ATK scaling
    RegisterParam({
        "enemy_atk_base", "Enemy Base ATK", "enemies",
        5.0f, 0.05f, CurveType::Linear,
        3.0f, 50.0f,
        0.8f, 1.0f, 1.2f, 1.5f
    });
    
    // Gold per kill
    RegisterParam({
        "gold_per_kill", "Gold Per Kill", "economy",
        5.0f, 0.5f, CurveType::Logarithmic,
        3.0f, 50.0f,
        1.3f, 1.0f, 0.8f, 0.6f
    });
    
    // EXP per kill
    RegisterParam({
        "exp_per_kill", "EXP Per Kill", "progression",
        10.0f, 1.0f, CurveType::Linear,
        5.0f, 100.0f,
        1.2f, 1.0f, 0.9f, 0.8f
    });
    
    // Shop prices
    RegisterParam({
        "shop_price_mult", "Shop Price Multiplier", "economy",
        1.0f, 0.05f, CurveType::Linear,
        0.5f, 3.0f,
        0.8f, 1.0f, 1.2f, 1.5f
    });
    
    // Crit chance
    RegisterParam({
        "crit_chance", "Critical Hit Chance", "combat",
        0.1f, 0.01f, CurveType::Logarithmic,
        0.05f, 0.5f,
        1.0f, 1.0f, 1.0f, 1.0f
    });
    
    // Healing effectiveness
    RegisterParam({
        "heal_effectiveness", "Healing Effectiveness", "combat",
        1.0f, 0.0f, CurveType::Linear,
        0.5f, 2.0f,
        1.2f, 1.0f, 0.8f, 0.6f
    });
}

// ===========================================================================
// BALANCE PRESETS
// ===========================================================================

namespace BalancePresets {

void ApplyEasyMode(BalanceSystem& system) {
    system.SetDifficulty(DifficultyLevel::Easy);
    
    // More forgiving combat
    auto& combat = const_cast<CombatBalance&>(system.GetCombat());
    combat.defMultiplier = 0.6f;  // Defense more effective
    combat.baseCritChance = 0.15f;  // More crits for player
    
    // More resources
    auto& econ = const_cast<EconomyBalance&>(system.GetEconomy());
    econ.baseKillGold = 7;
    econ.itemDropChance = 0.2f;
    
    // Slower enemy scaling
    auto& wave = const_cast<WaveBalance&>(system.GetWave());
    wave.enemyHpScaling = 0.07f;
    wave.eliteStartWave = 5;
    wave.championStartWave = 10;
    
    Logger::Info("Applied Easy mode preset");
}

void ApplyNormalMode(BalanceSystem& system) {
    system.SetDifficulty(DifficultyLevel::Normal);
    // Default values are normal mode
    Logger::Info("Applied Normal mode preset");
}

void ApplyHardMode(BalanceSystem& system) {
    system.SetDifficulty(DifficultyLevel::Hard);
    
    // Tougher combat
    auto& combat = const_cast<CombatBalance&>(system.GetCombat());
    combat.defMultiplier = 0.4f;  // Defense less effective
    combat.baseCritChance = 0.08f;
    
    // Scarcer resources
    auto& econ = const_cast<EconomyBalance&>(system.GetEconomy());
    econ.baseKillGold = 4;
    econ.shopPriceMultiplier = 1.2f;
    
    // Faster enemy scaling
    auto& wave = const_cast<WaveBalance&>(system.GetWave());
    wave.enemyHpScaling = 0.12f;
    wave.eliteStartWave = 2;
    wave.championStartWave = 5;
    
    Logger::Info("Applied Hard mode preset");
}

void ApplyNightmareMode(BalanceSystem& system) {
    system.SetDifficulty(DifficultyLevel::Nightmare);
    
    // Brutal combat
    auto& combat = const_cast<CombatBalance&>(system.GetCombat());
    combat.defMultiplier = 0.3f;
    combat.baseCritChance = 0.05f;
    combat.randomVarianceMax = 1.2f;  // More variance (spiky damage)
    
    // Very scarce resources
    auto& econ = const_cast<EconomyBalance&>(system.GetEconomy());
    econ.baseKillGold = 3;
    econ.shopPriceMultiplier = 1.5f;
    econ.itemDropChance = 0.1f;
    
    // Aggressive enemy scaling
    auto& wave = const_cast<WaveBalance&>(system.GetWave());
    wave.baseEnemyCount = 4;
    wave.enemyHpScaling = 0.15f;
    wave.enemyAtkScaling = 0.08f;
    wave.eliteStartWave = 1;
    wave.championStartWave = 3;
    wave.eliteChanceBase = 0.2f;
    
    Logger::Info("Applied Nightmare mode preset");
}

void ApplyTutorialMode(BalanceSystem& system) {
    system.SetDifficulty(DifficultyLevel::Easy);
    
    // Very easy enemies
    auto& wave = const_cast<WaveBalance&>(system.GetWave());
    wave.baseEnemyCount = 2;
    wave.enemyHpScaling = 0.0f;
    wave.enemyAtkScaling = 0.0f;
    wave.eliteStartWave = 999;  // No elites
    wave.championStartWave = 999;
    
    // Generous resources
    auto& econ = const_cast<EconomyBalance&>(system.GetEconomy());
    econ.baseKillGold = 20;
    
    Logger::Info("Applied Tutorial mode preset");
}

void ApplySpeedRunMode(BalanceSystem& system) {
    system.SetDifficulty(DifficultyLevel::Normal);
    
    // Weaker enemies for faster clears
    auto& wave = const_cast<WaveBalance&>(system.GetWave());
    wave.enemyHpScaling = 0.05f;
    wave.baseEnemyCount = 2;
    
    // More gold for faster shopping
    auto& econ = const_cast<EconomyBalance&>(system.GetEconomy());
    econ.baseKillGold = 10;
    econ.waveCompletionGold = 100;
    
    Logger::Info("Applied Speed Run mode preset");
}

void ApplyEndlessMode(BalanceSystem& system) {
    system.SetDifficulty(DifficultyLevel::Normal);
    
    // Exponential scaling for endless
    auto& wave = const_cast<WaveBalance&>(system.GetWave());
    wave.enemyHpScaling = 0.08f;  // Compounds over many waves
    wave.enemyAtkScaling = 0.04f;
    wave.maxEnemiesPerWave = 15;
    
    // Add more boss waves for endless
    wave.bossWaves = {5, 10, 15, 20, 25, 30, 35, 40, 45, 50};
    
    // Better rewards to keep up
    auto& econ = const_cast<EconomyBalance&>(system.GetEconomy());
    econ.waveGoldGrowth = 15;
    
    Logger::Info("Applied Endless mode preset");
}

} // namespace BalancePresets

} // namespace DDD
