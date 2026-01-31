#pragma once

#include "../Utils/Math.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <ctime>

namespace DDD {

/**
 * Available game modes
 */
enum class GameModeType {
    Campaign,       // Standard story progression
    Endless,        // Infinite waves, scaling difficulty
    DailyChallenge, // Same seed for all players daily
    SpeedRun,       // Timed run with leaderboards
    BossRush,       // Only boss battles
    Survival,       // Limited resources, no shop
    Custom          // Player-defined modifiers
};

/**
 * Difficulty modifiers
 */
struct DifficultyModifiers {
    float enemyHpMultiplier = 1.0f;
    float enemyDamageMultiplier = 1.0f;
    float enemyCountMultiplier = 1.0f;
    float goldMultiplier = 1.0f;
    float expMultiplier = 1.0f;
    float healingMultiplier = 1.0f;
    int startingGold = 100;
    int startingLives = 3;
    bool permadeath = false;
    bool noShop = false;
    bool elitesOnly = false;
    bool bossesOnly = false;
};

/**
 * Challenge modifier (stackable effects)
 */
struct ChallengeModifier {
    std::string id;
    std::string name;
    std::string description;
    int scoreMultiplier;    // % bonus to score
    DifficultyModifiers modifiers;
    bool isPositive;        // Helps or hinders player
};

/**
 * Game mode definition
 */
struct GameModeDef {
    GameModeType type;
    std::string id;
    std::string name;
    std::string description;
    
    // Victory/end conditions
    int targetWaves;        // 0 = infinite
    int timeLimit;          // Seconds, 0 = none
    bool trackTime;
    bool trackScore;
    
    // Base modifiers
    DifficultyModifiers baseModifiers;
    
    // Allowed features
    bool allowShop;
    bool allowSave;
    bool allowPause;
    
    // Scoring
    int baseScorePerWave;
    int baseScorePerKill;
    int baseScorePerGold;
    float timeBonusMultiplier;
};

/**
 * Daily challenge seed data
 */
struct DailySeed {
    uint32_t seed;
    std::string date;       // YYYY-MM-DD
    std::vector<std::string> activeModifiers;
    int targetScore;        // Par score for the day
};

/**
 * Run statistics for scoring
 */
struct RunStats {
    GameModeType mode;
    uint32_t seed;
    
    // Progress
    int wavesCompleted;
    int enemiesKilled;
    int bossesKilled;
    int unitsLost;
    
    // Resources
    int goldEarned;
    int goldSpent;
    int itemsUsed;
    int diceRolled;
    
    // Combat
    int totalDamageDealt;
    int totalDamageTaken;
    int totalHealing;
    int criticalHits;
    
    // Time
    float totalTime;
    float fastestWave;
    float slowestWave;
    
    // Calculated
    int finalScore;
    std::string rank;       // S, A, B, C, D, F
};

/**
 * Leaderboard entry
 */
struct LeaderboardEntry {
    std::string playerName;
    int score;
    int wavesCompleted;
    float timeSeconds;
    std::string date;
    uint32_t seed;
};

/**
 * GameModeSystem - manages game modes and scoring
 */
class GameModeSystem {
public:
    static GameModeSystem& Instance();
    
    void Initialize();
    
    // Mode management
    const GameModeDef* GetModeDef(GameModeType type) const;
    const GameModeDef* GetModeDef(const std::string& id) const;
    std::vector<GameModeType> GetAvailableModes() const;
    
    // Active mode
    void SetActiveMode(GameModeType type);
    void SetActiveMode(const std::string& id);
    GameModeType GetActiveMode() const { return m_activeMode; }
    const GameModeDef* GetActiveModeDef() const;
    
    // Modifiers
    const ChallengeModifier* GetModifier(const std::string& id) const;
    std::vector<std::string> GetAllModifiers() const;
    void EnableModifier(const std::string& id);
    void DisableModifier(const std::string& id);
    void ClearModifiers();
    const std::vector<std::string>& GetActiveModifiers() const { return m_activeModifiers; }
    
    // Get combined difficulty
    DifficultyModifiers GetCombinedModifiers() const;
    
    // Daily challenge
    DailySeed GetDailySeed() const;
    bool IsDailyCompleted() const;
    void MarkDailyCompleted();
    
    // Speed run
    void StartSpeedRunTimer();
    void StopSpeedRunTimer();
    float GetSpeedRunTime() const;
    bool IsSpeedRunActive() const { return m_speedRunActive; }
    
    // Run tracking
    void StartRun(uint32_t seed = 0);
    void EndRun(bool victory);
    RunStats& GetCurrentStats() { return m_currentStats; }
    const RunStats& GetCurrentStats() const { return m_currentStats; }
    
    // Scoring
    int CalculateScore(const RunStats& stats) const;
    std::string CalculateRank(int score, GameModeType mode) const;
    void AddKillScore(int enemyTier);
    void AddWaveScore(int waveNumber, float timeSeconds);
    void AddBonusScore(int amount, const std::string& reason);
    
    // Leaderboards (local)
    void AddLeaderboardEntry(const LeaderboardEntry& entry);
    std::vector<LeaderboardEntry> GetLeaderboard(GameModeType mode, int count = 10) const;
    int GetPlayerRank(GameModeType mode, int score) const;
    
    // Utility
    static uint32_t GenerateDailySeed();
    static std::string GetCurrentDateString();
    
private:
    GameModeSystem() = default;
    
    void RegisterMode(const GameModeDef& def);
    void RegisterModifier(const ChallengeModifier& mod);
    void RegisterAllModes();
    void RegisterAllModifiers();
    
    std::unordered_map<std::string, GameModeDef> m_modes;
    std::unordered_map<std::string, ChallengeModifier> m_modifiers;
    std::unordered_map<GameModeType, std::vector<LeaderboardEntry>> m_leaderboards;
    
    GameModeType m_activeMode = GameModeType::Campaign;
    std::vector<std::string> m_activeModifiers;
    
    RunStats m_currentStats;
    bool m_runActive = false;
    
    // Speed run
    bool m_speedRunActive = false;
    float m_speedRunTime = 0.0f;
    
    // Daily tracking
    std::string m_lastDailyCompleted;
    
    bool m_initialized = false;
};

} // namespace DDD
