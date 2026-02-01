#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace DDD {

/**
 * Stat data type
 */
enum class StatType {
    Int,
    Float,
    AvgRate    // Average rate (Steam handles averaging)
};

/**
 * Stat definition
 */
struct StatDef {
    std::string id;
    std::string apiName;        // Steam API name
    std::string displayName;
    StatType type;
    float defaultValue;
    float minValue;
    float maxValue;
    bool incrementOnly;         // Can only increase
    std::string formatString;   // Display format
};

/**
 * Current stat value
 */
struct StatValue {
    std::string id;
    StatType type;
    int intValue;
    float floatValue;
    bool dirty;                 // Needs upload
};

/**
 * Trading card drop info
 */
struct TradingCardDrop {
    std::string cardId;
    std::string cardName;
    std::string setName;
    int dropNumber;             // Which drop this was
    uint64_t timestamp;
    bool foil;
};

/**
 * Trading card set info
 */
struct TradingCardSet {
    std::string setId;
    std::string setName;
    int totalCards;
    int ownedCards;
    std::vector<std::string> cardIds;
    bool complete;
};

/**
 * Badge info
 */
struct Badge {
    std::string badgeId;
    std::string name;
    std::string description;
    int level;                  // 1-5 for normal, 1 for foil
    bool isFoil;
    std::string iconUrl;
    int xpEarned;
};

/**
 * Play time milestone for card drops
 */
struct PlayTimeMilestone {
    float hoursPlayed;
    int cardsDropped;
    int maxDrops;
    float nextDropAt;           // Hours until next drop
};

/**
 * SteamStatsSystem - Steam Stats and Trading Cards
 */
class SteamStatsSystem {
public:
    static SteamStatsSystem& Instance();
    
    void Initialize();
    void Shutdown();
    void Update(float deltaTime);
    
    // Stats management
    void RegisterStat(const StatDef& stat);
    const StatDef* GetStatDef(const std::string& id) const;
    std::vector<StatDef> GetAllStats() const;
    
    // Stat values
    void SetStatInt(const std::string& id, int value);
    void SetStatFloat(const std::string& id, float value);
    void IncrementStat(const std::string& id, int amount = 1);
    void IncrementStatFloat(const std::string& id, float amount);
    void UpdateAvgRateStat(const std::string& id, float value, float sessionLength);
    
    int GetStatInt(const std::string& id) const;
    float GetStatFloat(const std::string& id) const;
    
    // Stat sync
    void UploadStats();
    void DownloadStats();
    bool HasPendingUpload() const { return m_pendingUpload; }
    
    // Reset (debug only)
    void ResetStat(const std::string& id);
    void ResetAllStats();
    
    // Trading cards
    bool AreTradingCardsEnabled() const { return m_tradingCardsEnabled; }
    PlayTimeMilestone GetPlayTimeMilestone() const;
    float GetHoursUntilNextDrop() const;
    int GetTotalCardsDropped() const { return m_totalCardsDropped; }
    int GetMaxCardDrops() const { return m_maxCardDrops; }
    
    // Card drops
    std::vector<TradingCardDrop> GetRecentDrops() const;
    TradingCardDrop GetLastDrop() const;
    bool HasPendingDrop() const { return m_pendingDrop; }
    void AcknowledgeDrop();
    
    // Card collection (from Steam inventory)
    std::vector<TradingCardSet> GetCardSets() const;
    TradingCardSet GetCardSet(const std::string& setId) const;
    int GetTotalCardsOwned() const;
    bool HasCompletedSet(const std::string& setId) const;
    
    // Badges
    std::vector<Badge> GetBadges() const;
    Badge GetBadge(const std::string& badgeId) const;
    int GetTotalBadgeXP() const;
    
    // Game events for stat tracking
    void OnGameStart();
    void OnGameEnd(bool won, float playTimeSeconds);
    void OnWaveComplete(int wave, bool perfect);
    void OnEnemyKilled(const std::string& enemyType);
    void OnBossKilled(const std::string& bossId);
    void OnUnitSummoned(const std::string& unitClass);
    void OnAbilityUsed(const std::string& abilityId);
    void OnItemCollected(const std::string& itemId);
    void OnGoldEarned(int amount);
    void OnGoldSpent(int amount);
    void OnDamageDealt(int amount, bool critical);
    void OnDamageTaken(int amount);
    void OnUnitLost();
    void OnDiceRolled(const std::string& diceType, int result);
    
    // Session tracking
    void StartSession();
    void EndSession();
    float GetSessionPlayTime() const { return m_sessionPlayTime; }
    float GetTotalPlayTime() const;
    
    // Callbacks
    using StatsReceivedCallback = std::function<void(bool success)>;
    using CardDropCallback = std::function<void(const TradingCardDrop& drop)>;
    
    void SetStatsReceivedCallback(StatsReceivedCallback callback) { m_statsCallback = callback; }
    void SetCardDropCallback(CardDropCallback callback) { m_cardDropCallback = callback; }
    
private:
    SteamStatsSystem() = default;
    
    void RegisterAllStats();
    void CheckCardDrop();
    void SimulateCardDrop();
    
    // Stats
    std::unordered_map<std::string, StatDef> m_statDefs;
    std::unordered_map<std::string, StatValue> m_statValues;
    bool m_pendingUpload = false;
    float m_uploadTimer = 0.0f;
    static constexpr float UPLOAD_INTERVAL = 60.0f;
    
    // Trading cards
    bool m_tradingCardsEnabled = true;
    int m_totalCardsDropped = 0;
    int m_maxCardDrops = 5;             // Typical Steam limit
    float m_hoursPerDrop = 2.0f;        // Hours of play per drop
    bool m_pendingDrop = false;
    std::vector<TradingCardDrop> m_recentDrops;
    
    // Session
    float m_sessionPlayTime = 0.0f;
    float m_totalPlayTimeAtStart = 0.0f;
    bool m_sessionActive = false;
    
    // Card sets (simulated)
    std::vector<TradingCardSet> m_cardSets;
    std::vector<Badge> m_badges;
    
    // Callbacks
    StatsReceivedCallback m_statsCallback;
    CardDropCallback m_cardDropCallback;
    
    bool m_initialized = false;
};

} // namespace DDD
