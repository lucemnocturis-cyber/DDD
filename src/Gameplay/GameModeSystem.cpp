#include "GameModeSystem.h"
#include "../Utils/Logger.h"
#include "../Utils/Random.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace DDD {

GameModeSystem& GameModeSystem::Instance() {
    static GameModeSystem instance;
    return instance;
}

void GameModeSystem::Initialize() {
    if (m_initialized) return;
    
    RegisterAllModes();
    RegisterAllModifiers();
    
    m_initialized = true;
    Logger::Info("GameModeSystem initialized with {} modes, {} modifiers", 
                 m_modes.size(), m_modifiers.size());
}

void GameModeSystem::RegisterMode(const GameModeDef& def) {
    m_modes[def.id] = def;
}

void GameModeSystem::RegisterModifier(const ChallengeModifier& mod) {
    m_modifiers[mod.id] = mod;
}

const GameModeDef* GameModeSystem::GetModeDef(GameModeType type) const {
    for (const auto& [id, def] : m_modes) {
        if (def.type == type) {
            return &def;
        }
    }
    return nullptr;
}

const GameModeDef* GameModeSystem::GetModeDef(const std::string& id) const {
    auto it = m_modes.find(id);
    return it != m_modes.end() ? &it->second : nullptr;
}

std::vector<GameModeType> GameModeSystem::GetAvailableModes() const {
    return {
        GameModeType::Campaign,
        GameModeType::Endless,
        GameModeType::DailyChallenge,
        GameModeType::SpeedRun,
        GameModeType::BossRush,
        GameModeType::Survival
    };
}

void GameModeSystem::SetActiveMode(GameModeType type) {
    m_activeMode = type;
    Logger::Info("Active game mode set to: {}", static_cast<int>(type));
}

void GameModeSystem::SetActiveMode(const std::string& id) {
    const GameModeDef* def = GetModeDef(id);
    if (def) {
        m_activeMode = def->type;
    }
}

const GameModeDef* GameModeSystem::GetActiveModeDef() const {
    return GetModeDef(m_activeMode);
}

const ChallengeModifier* GameModeSystem::GetModifier(const std::string& id) const {
    auto it = m_modifiers.find(id);
    return it != m_modifiers.end() ? &it->second : nullptr;
}

std::vector<std::string> GameModeSystem::GetAllModifiers() const {
    std::vector<std::string> result;
    for (const auto& [id, mod] : m_modifiers) {
        result.push_back(id);
    }
    return result;
}

void GameModeSystem::EnableModifier(const std::string& id) {
    if (std::find(m_activeModifiers.begin(), m_activeModifiers.end(), id) == m_activeModifiers.end()) {
        m_activeModifiers.push_back(id);
    }
}

void GameModeSystem::DisableModifier(const std::string& id) {
    m_activeModifiers.erase(
        std::remove(m_activeModifiers.begin(), m_activeModifiers.end(), id),
        m_activeModifiers.end()
    );
}

void GameModeSystem::ClearModifiers() {
    m_activeModifiers.clear();
}

DifficultyModifiers GameModeSystem::GetCombinedModifiers() const {
    DifficultyModifiers combined;
    
    // Start with mode base modifiers
    const GameModeDef* modeDef = GetActiveModeDef();
    if (modeDef) {
        combined = modeDef->baseModifiers;
    }
    
    // Apply challenge modifiers
    for (const auto& modId : m_activeModifiers) {
        const ChallengeModifier* mod = GetModifier(modId);
        if (mod) {
            combined.enemyHpMultiplier *= mod->modifiers.enemyHpMultiplier;
            combined.enemyDamageMultiplier *= mod->modifiers.enemyDamageMultiplier;
            combined.enemyCountMultiplier *= mod->modifiers.enemyCountMultiplier;
            combined.goldMultiplier *= mod->modifiers.goldMultiplier;
            combined.expMultiplier *= mod->modifiers.expMultiplier;
            combined.healingMultiplier *= mod->modifiers.healingMultiplier;
            
            if (mod->modifiers.permadeath) combined.permadeath = true;
            if (mod->modifiers.noShop) combined.noShop = true;
            if (mod->modifiers.elitesOnly) combined.elitesOnly = true;
            if (mod->modifiers.bossesOnly) combined.bossesOnly = true;
        }
    }
    
    return combined;
}

DailySeed GameModeSystem::GetDailySeed() const {
    DailySeed daily;
    daily.date = GetCurrentDateString();
    daily.seed = GenerateDailySeed();
    
    // Rotate through modifier sets based on day
    int dayOfYear = std::time(nullptr) / 86400;
    int modSet = dayOfYear % 7;
    
    switch (modSet) {
        case 0:
            daily.activeModifiers = {"tough_enemies", "bonus_gold"};
            daily.targetScore = 15000;
            break;
        case 1:
            daily.activeModifiers = {"glass_cannon"};
            daily.targetScore = 12000;
            break;
        case 2:
            daily.activeModifiers = {"elite_forces", "reduced_healing"};
            daily.targetScore = 18000;
            break;
        case 3:
            daily.activeModifiers = {"swarm_mode"};
            daily.targetScore = 14000;
            break;
        case 4:
            daily.activeModifiers = {"no_shop", "bonus_exp"};
            daily.targetScore = 16000;
            break;
        case 5:
            daily.activeModifiers = {"tough_enemies", "glass_cannon"};
            daily.targetScore = 20000;
            break;
        default:
            daily.activeModifiers = {"bonus_gold", "bonus_exp"};
            daily.targetScore = 10000;
            break;
    }
    
    return daily;
}

bool GameModeSystem::IsDailyCompleted() const {
    return m_lastDailyCompleted == GetCurrentDateString();
}

void GameModeSystem::MarkDailyCompleted() {
    m_lastDailyCompleted = GetCurrentDateString();
}

void GameModeSystem::StartSpeedRunTimer() {
    m_speedRunActive = true;
    m_speedRunTime = 0.0f;
}

void GameModeSystem::StopSpeedRunTimer() {
    m_speedRunActive = false;
}

float GameModeSystem::GetSpeedRunTime() const {
    return m_speedRunTime;
}

void GameModeSystem::StartRun(uint32_t seed) {
    m_currentStats = RunStats();
    m_currentStats.mode = m_activeMode;
    m_currentStats.seed = seed != 0 ? seed : Random::Range(0, INT32_MAX);
    m_runActive = true;
    
    if (m_activeMode == GameModeType::SpeedRun) {
        StartSpeedRunTimer();
    }
    
    Logger::Info("Run started - Mode: {}, Seed: {}", 
                 static_cast<int>(m_activeMode), m_currentStats.seed);
}

void GameModeSystem::EndRun(bool victory) {
    if (!m_runActive) return;
    
    if (m_activeMode == GameModeType::SpeedRun) {
        StopSpeedRunTimer();
        m_currentStats.totalTime = m_speedRunTime;
    }
    
    // Calculate final score
    m_currentStats.finalScore = CalculateScore(m_currentStats);
    m_currentStats.rank = CalculateRank(m_currentStats.finalScore, m_activeMode);
    
    // Add to leaderboard
    LeaderboardEntry entry;
    entry.playerName = "Player"; // Would come from settings
    entry.score = m_currentStats.finalScore;
    entry.wavesCompleted = m_currentStats.wavesCompleted;
    entry.timeSeconds = m_currentStats.totalTime;
    entry.date = GetCurrentDateString();
    entry.seed = m_currentStats.seed;
    
    AddLeaderboardEntry(entry);
    
    if (m_activeMode == GameModeType::DailyChallenge && victory) {
        MarkDailyCompleted();
    }
    
    m_runActive = false;
    
    Logger::Info("Run ended - Score: {}, Rank: {}", 
                 m_currentStats.finalScore, m_currentStats.rank);
}

int GameModeSystem::CalculateScore(const RunStats& stats) const {
    int score = 0;
    
    const GameModeDef* modeDef = GetModeDef(stats.mode);
    if (!modeDef) return 0;
    
    // Base wave score
    score += stats.wavesCompleted * modeDef->baseScorePerWave;
    
    // Kill score
    score += stats.enemiesKilled * modeDef->baseScorePerKill;
    score += stats.bossesKilled * modeDef->baseScorePerKill * 10;
    
    // Gold bonus
    score += stats.goldEarned * modeDef->baseScorePerGold;
    
    // Time bonus (for speed run)
    if (modeDef->trackTime && stats.totalTime > 0) {
        // Bonus for faster times
        float timeBonus = std::max(0.0f, 600.0f - stats.totalTime); // 10 minute baseline
        score += static_cast<int>(timeBonus * modeDef->timeBonusMultiplier);
    }
    
    // Efficiency bonuses
    if (stats.unitsLost == 0) {
        score += 1000; // No casualties bonus
    }
    
    // Critical hit bonus
    score += stats.criticalHits * 10;
    
    // Apply modifier multipliers
    int totalMultiplier = 100;
    for (const auto& modId : m_activeModifiers) {
        const ChallengeModifier* mod = GetModifier(modId);
        if (mod) {
            totalMultiplier += mod->scoreMultiplier;
        }
    }
    
    score = score * totalMultiplier / 100;
    
    return score;
}

std::string GameModeSystem::CalculateRank(int score, GameModeType mode) const {
    // Rank thresholds vary by mode
    int sThreshold, aThreshold, bThreshold, cThreshold, dThreshold;
    
    switch (mode) {
        case GameModeType::Endless:
            sThreshold = 50000;
            aThreshold = 30000;
            bThreshold = 15000;
            cThreshold = 8000;
            dThreshold = 3000;
            break;
            
        case GameModeType::SpeedRun:
            sThreshold = 20000;
            aThreshold = 15000;
            bThreshold = 10000;
            cThreshold = 5000;
            dThreshold = 2000;
            break;
            
        case GameModeType::BossRush:
            sThreshold = 25000;
            aThreshold = 18000;
            bThreshold = 12000;
            cThreshold = 6000;
            dThreshold = 2500;
            break;
            
        default:
            sThreshold = 30000;
            aThreshold = 20000;
            bThreshold = 12000;
            cThreshold = 6000;
            dThreshold = 2000;
            break;
    }
    
    if (score >= sThreshold) return "S";
    if (score >= aThreshold) return "A";
    if (score >= bThreshold) return "B";
    if (score >= cThreshold) return "C";
    if (score >= dThreshold) return "D";
    return "F";
}

void GameModeSystem::AddKillScore(int enemyTier) {
    if (!m_runActive) return;
    
    m_currentStats.enemiesKilled++;
    
    // Tier-based score
    int baseScore = 10 * (enemyTier + 1);
    // Score added directly to stats will be calculated at end
}

void GameModeSystem::AddWaveScore(int waveNumber, float timeSeconds) {
    if (!m_runActive) return;
    
    m_currentStats.wavesCompleted = waveNumber;
    
    if (m_currentStats.fastestWave == 0 || timeSeconds < m_currentStats.fastestWave) {
        m_currentStats.fastestWave = timeSeconds;
    }
    if (timeSeconds > m_currentStats.slowestWave) {
        m_currentStats.slowestWave = timeSeconds;
    }
}

void GameModeSystem::AddBonusScore(int amount, const std::string& reason) {
    if (!m_runActive) return;
    
    Logger::Info("Bonus score: +{} ({})", amount, reason);
}

void GameModeSystem::AddLeaderboardEntry(const LeaderboardEntry& entry) {
    auto& leaderboard = m_leaderboards[m_activeMode];
    leaderboard.push_back(entry);
    
    // Sort by score (descending)
    std::sort(leaderboard.begin(), leaderboard.end(),
        [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
            return a.score > b.score;
        });
    
    // Keep only top 100
    if (leaderboard.size() > 100) {
        leaderboard.resize(100);
    }
}

std::vector<LeaderboardEntry> GameModeSystem::GetLeaderboard(GameModeType mode, int count) const {
    auto it = m_leaderboards.find(mode);
    if (it == m_leaderboards.end()) {
        return {};
    }
    
    const auto& leaderboard = it->second;
    int actualCount = std::min(count, static_cast<int>(leaderboard.size()));
    
    return std::vector<LeaderboardEntry>(leaderboard.begin(), leaderboard.begin() + actualCount);
}

int GameModeSystem::GetPlayerRank(GameModeType mode, int score) const {
    auto it = m_leaderboards.find(mode);
    if (it == m_leaderboards.end()) {
        return 1;
    }
    
    int rank = 1;
    for (const auto& entry : it->second) {
        if (score >= entry.score) break;
        rank++;
    }
    
    return rank;
}

uint32_t GameModeSystem::GenerateDailySeed() {
    // Generate consistent seed based on current date
    std::time_t now = std::time(nullptr);
    std::tm* utc = std::gmtime(&now);
    
    uint32_t seed = (utc->tm_year + 1900) * 10000 + 
                    (utc->tm_mon + 1) * 100 + 
                    utc->tm_mday;
    
    // Mix it up a bit
    seed = seed * 1103515245 + 12345;
    
    return seed;
}

std::string GameModeSystem::GetCurrentDateString() {
    std::time_t now = std::time(nullptr);
    std::tm* utc = std::gmtime(&now);
    
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(4) << (utc->tm_year + 1900) << "-"
       << std::setw(2) << (utc->tm_mon + 1) << "-"
       << std::setw(2) << utc->tm_mday;
    
    return ss.str();
}

// ===========================================================================
// GAME MODE DEFINITIONS
// ===========================================================================

void GameModeSystem::RegisterAllModes() {
    // Campaign Mode
    RegisterMode({
        GameModeType::Campaign,
        "campaign",
        "Campaign",
        "Progress through 20 waves with increasing difficulty. Defeat the Shadow King!",
        20, 0, false, true,
        {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 100, 3, false, false, false, false},
        true, true, true,
        500, 10, 1, 0.0f
    });
    
    // Endless Mode
    RegisterMode({
        GameModeType::Endless,
        "endless",
        "Endless",
        "Survive as long as possible! Difficulty scales infinitely.",
        0, 0, true, true,
        {1.0f, 1.0f, 1.0f, 1.2f, 1.2f, 1.0f, 150, 3, false, false, false, false},
        true, false, true,
        1000, 15, 2, 0.0f
    });
    
    // Daily Challenge
    RegisterMode({
        GameModeType::DailyChallenge,
        "daily",
        "Daily Challenge",
        "A unique challenge every day! Same seed for all players. One attempt!",
        10, 0, true, true,
        {1.2f, 1.1f, 1.0f, 1.0f, 1.0f, 1.0f, 100, 1, true, false, false, false},
        true, false, true,
        750, 12, 1, 0.0f
    });
    
    // Speed Run
    RegisterMode({
        GameModeType::SpeedRun,
        "speedrun",
        "Speed Run",
        "Complete 10 waves as fast as possible! Time is your score.",
        10, 600, true, true,
        {0.8f, 0.8f, 0.8f, 1.5f, 1.5f, 1.2f, 200, 5, false, false, false, false},
        true, false, false,
        200, 5, 0, 10.0f
    });
    
    // Boss Rush
    RegisterMode({
        GameModeType::BossRush,
        "bossrush",
        "Boss Rush",
        "Face all bosses back-to-back! No regular waves.",
        4, 0, true, true,
        {1.0f, 1.0f, 1.0f, 2.0f, 2.0f, 0.8f, 300, 2, false, false, false, true},
        true, false, true,
        2500, 0, 5, 0.0f
    });
    
    // Survival
    RegisterMode({
        GameModeType::Survival,
        "survival",
        "Survival",
        "No shop, no healing between waves. How long can you last?",
        0, 0, true, true,
        {0.9f, 1.0f, 1.2f, 0.5f, 1.0f, 0.5f, 0, 1, true, true, false, false},
        false, false, true,
        1500, 20, 3, 0.0f
    });
}

// ===========================================================================
// CHALLENGE MODIFIERS
// ===========================================================================

void GameModeSystem::RegisterAllModifiers() {
    // Difficulty modifiers (negative for player)
    RegisterModifier({
        "tough_enemies",
        "Tough Enemies",
        "Enemies have 50% more HP",
        25,
        {1.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 100, 3, false, false, false, false},
        false
    });
    
    RegisterModifier({
        "deadly_enemies",
        "Deadly Enemies",
        "Enemies deal 50% more damage",
        30,
        {1.0f, 1.5f, 1.0f, 1.0f, 1.0f, 1.0f, 100, 3, false, false, false, false},
        false
    });
    
    RegisterModifier({
        "swarm_mode",
        "Swarm Mode",
        "50% more enemies per wave",
        20,
        {1.0f, 1.0f, 1.5f, 1.0f, 1.0f, 1.0f, 100, 3, false, false, false, false},
        false
    });
    
    RegisterModifier({
        "elite_forces",
        "Elite Forces",
        "All enemies are elites",
        50,
        {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 100, 3, false, false, true, false},
        false
    });
    
    RegisterModifier({
        "reduced_healing",
        "Reduced Healing",
        "Healing is 50% less effective",
        15,
        {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 100, 3, false, false, false, false},
        false
    });
    
    RegisterModifier({
        "no_shop",
        "No Shop",
        "Shop is disabled",
        35,
        {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 100, 3, false, true, false, false},
        false
    });
    
    RegisterModifier({
        "glass_cannon",
        "Glass Cannon",
        "Deal 50% more damage, take 50% more damage",
        40,
        {1.0f, 1.5f, 1.0f, 1.0f, 1.0f, 1.0f, 100, 3, false, false, false, false},
        false
    });
    
    RegisterModifier({
        "permadeath",
        "Permadeath",
        "No extra lives - one defeat ends the run",
        60,
        {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 100, 1, true, false, false, false},
        false
    });
    
    // Positive modifiers
    RegisterModifier({
        "bonus_gold",
        "Golden Touch",
        "Earn 50% more gold",
        -10,
        {1.0f, 1.0f, 1.0f, 1.5f, 1.0f, 1.0f, 100, 3, false, false, false, false},
        true
    });
    
    RegisterModifier({
        "bonus_exp",
        "Quick Learner",
        "Earn 50% more experience",
        -10,
        {1.0f, 1.0f, 1.0f, 1.0f, 1.5f, 1.0f, 100, 3, false, false, false, false},
        true
    });
    
    RegisterModifier({
        "bonus_healing",
        "Blessed",
        "Healing is 50% more effective",
        -15,
        {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.5f, 100, 3, false, false, false, false},
        true
    });
    
    RegisterModifier({
        "weak_enemies",
        "Weakened Foes",
        "Enemies have 25% less HP",
        -20,
        {0.75f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 100, 3, false, false, false, false},
        true
    });
    
    RegisterModifier({
        "extra_lives",
        "Nine Lives",
        "Start with 5 extra lives",
        -25,
        {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 100, 8, false, false, false, false},
        true
    });
    
    RegisterModifier({
        "rich_start",
        "Trust Fund",
        "Start with 500 gold",
        -15,
        {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 500, 3, false, false, false, false},
        true
    });
}

} // namespace DDD
