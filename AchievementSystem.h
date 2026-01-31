#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace DDD {

/**
 * Achievement category
 */
enum class AchievementCategory {
    Progression,    // Story/campaign progress
    Combat,         // Combat-related
    Collection,     // Collecting items/units/dice
    Challenge,      // Difficult feats
    Social,         // Multiplayer/social
    Secret,         // Hidden achievements
    Miscellaneous   // Other
};

/**
 * Achievement rarity (based on global unlock %)
 */
enum class AchievementRarity {
    Common,         // >50% of players
    Uncommon,       // 25-50%
    Rare,           // 10-25%
    VeryRare,       // 5-10%
    UltraRare       // <5%
};

/**
 * Achievement definition
 */
struct Achievement {
    std::string id;
    std::string apiName;        // Steam API name
    std::string name;
    std::string description;
    std::string hiddenDescription;  // Shown when locked for secret achievements
    std::string iconLocked;
    std::string iconUnlocked;
    
    AchievementCategory category;
    AchievementRarity rarity;
    bool isSecret = false;
    
    // Progress tracking (for incremental achievements)
    bool hasProgress = false;
    int progressCurrent = 0;
    int progressTarget = 0;
    std::string progressFormat;     // e.g., "{0}/{1} enemies defeated"
    
    // Unlock state
    bool unlocked = false;
    uint64_t unlockTime = 0;        // Unix timestamp
    
    // Rewards
    int xpReward = 0;
    std::string titleReward;        // Unlockable title
    std::string skinReward;         // Unlockable skin
};

/**
 * Achievement unlock notification
 */
struct AchievementNotification {
    std::string achievementId;
    std::string name;
    std::string description;
    float displayTime;
    float elapsed;
    bool showing;
};

/**
 * Stat tracking for achievement progress
 */
struct AchievementStat {
    std::string id;
    std::string name;
    int value = 0;
    int maxValue = 0;   // 0 = no max
    bool synced = true;
};

/**
 * AchievementSystem - tracks and unlocks achievements
 */
class AchievementSystem {
public:
    static AchievementSystem& Instance();
    
    void Initialize();
    void Shutdown();
    void Update(float deltaTime);
    
    // Achievement management
    void RegisterAchievement(const Achievement& achievement);
    const Achievement* GetAchievement(const std::string& id) const;
    std::vector<Achievement> GetAllAchievements() const;
    std::vector<Achievement> GetAchievementsByCategory(AchievementCategory category) const;
    std::vector<Achievement> GetUnlockedAchievements() const;
    std::vector<Achievement> GetLockedAchievements() const;
    
    // Unlock achievements
    bool UnlockAchievement(const std::string& id);
    bool IsAchievementUnlocked(const std::string& id) const;
    void SetAchievementProgress(const std::string& id, int progress);
    void IncrementAchievementProgress(const std::string& id, int amount = 1);
    
    // Stats
    void RegisterStat(const AchievementStat& stat);
    void SetStat(const std::string& id, int value);
    void IncrementStat(const std::string& id, int amount = 1);
    int GetStat(const std::string& id) const;
    
    // Bulk operations for game events
    void OnGameStart();
    void OnWaveComplete(int waveNumber);
    void OnBossDefeated(const std::string& bossId);
    void OnEnemyKilled(const std::string& enemyType);
    void OnUnitSummoned(const std::string& unitClass);
    void OnDiceRolled(const std::string& diceType);
    void OnItemCollected(const std::string& itemType);
    void OnGoldEarned(int amount);
    void OnDamageDealt(int amount);
    void OnDamageTaken(int amount);
    void OnCriticalHit();
    void OnPerfectWave();  // No damage taken
    void OnUnitDeath(bool isAlly);
    void OnGameWon(const std::string& difficulty, const std::string& gameMode);
    void OnGameLost();
    void OnSpeedrunComplete(float timeSeconds);
    
    // Queries
    int GetUnlockedCount() const;
    int GetTotalCount() const;
    float GetCompletionPercentage() const;
    int GetTotalXPEarned() const;
    
    // Notifications
    bool HasPendingNotification() const;
    AchievementNotification GetCurrentNotification() const;
    void DismissNotification();
    
    // Steam sync
    void SyncWithSteam();
    void ResetAllAchievements();  // Debug only
    
    // Callbacks
    using AchievementUnlockedCallback = std::function<void(const Achievement&)>;
    void SetUnlockedCallback(AchievementUnlockedCallback callback) { m_unlockedCallback = callback; }
    
private:
    AchievementSystem() = default;
    
    void RegisterAllAchievements();
    void RegisterProgressionAchievements();
    void RegisterCombatAchievements();
    void RegisterCollectionAchievements();
    void RegisterChallengeAchievements();
    void RegisterSecretAchievements();
    void RegisterAllStats();
    
    void CheckProgressAchievements();
    void ShowNotification(const Achievement& achievement);
    void LoadFromSteam();
    void SaveToSteam();
    
    // Achievements
    std::unordered_map<std::string, Achievement> m_achievements;
    
    // Stats
    std::unordered_map<std::string, AchievementStat> m_stats;
    
    // Notifications
    std::vector<AchievementNotification> m_notificationQueue;
    AchievementNotification m_currentNotification;
    static constexpr float NOTIFICATION_DURATION = 5.0f;
    
    // Tracking
    int m_totalXPEarned = 0;
    
    // Callback
    AchievementUnlockedCallback m_unlockedCallback;
    
    bool m_initialized = false;
};

} // namespace DDD
