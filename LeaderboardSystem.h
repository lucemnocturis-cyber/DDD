#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace DDD {

/**
 * Leaderboard types
 */
enum class LeaderboardType {
    HighScore,          // Highest score
    FastestTime,        // Fastest completion
    MostKills,          // Most enemies killed
    LongestStreak,      // Longest win streak
    HighestWave,        // Highest wave reached (endless)
    MostGold,           // Most gold earned in single run
    PerfectWaves,       // Most perfect waves in single run
    SpeedrunAny,        // Any% speedrun
    SpeedrunNoHits      // No-hit speedrun
};

/**
 * Leaderboard time range
 */
enum class LeaderboardRange {
    Global,             // All time, all players
    Friends,            // Friends only
    Daily,              // Today only
    Weekly,             // This week
    Monthly             // This month
};

/**
 * Sort method
 */
enum class LeaderboardSort {
    Ascending,          // Lower is better (time)
    Descending          // Higher is better (score)
};

/**
 * Upload result
 */
enum class UploadResult {
    Success,
    SuccessNewRecord,   // New personal best
    Failed,
    InvalidScore,
    NotConnected
};

/**
 * Individual leaderboard entry
 */
struct LeaderboardEntry {
    int globalRank;
    int friendRank;
    uint64_t steamId;
    std::string playerName;
    int score;
    int detailValue1;       // Extra data (e.g., wave number)
    int detailValue2;       // Extra data (e.g., units used)
    std::string detailString; // Extra data (e.g., difficulty)
    uint64_t timestamp;
    bool isCurrentUser;
    bool isFriend;
};

/**
 * Leaderboard definition
 */
struct LeaderboardDef {
    std::string id;
    std::string apiName;        // Steam API name
    std::string displayName;
    LeaderboardType type;
    LeaderboardSort sortMethod;
    std::string formatString;   // e.g., "{0} points", "{0}:{1:00}"
    bool perDifficulty;         // Separate boards per difficulty
    bool perGameMode;           // Separate boards per game mode
};

/**
 * Leaderboard query result
 */
struct LeaderboardQueryResult {
    bool success;
    std::string errorMessage;
    std::string leaderboardId;
    LeaderboardRange range;
    int totalEntries;
    int startRank;
    int endRank;
    std::vector<LeaderboardEntry> entries;
};

/**
 * Score upload data
 */
struct ScoreUpload {
    std::string leaderboardId;
    int score;
    std::string difficulty;
    std::string gameMode;
    int wave;
    int unitsUsed;
    float playTime;
    bool forceUpdate;           // Update even if not better
};

/**
 * LeaderboardSystem - Steam Leaderboard integration
 */
class LeaderboardSystem {
public:
    static LeaderboardSystem& Instance();
    
    void Initialize();
    void Shutdown();
    void Update(float deltaTime);
    
    // Leaderboard queries
    void QueryLeaderboard(const std::string& leaderboardId, LeaderboardRange range, int startRank, int count);
    void QueryAroundUser(const std::string& leaderboardId, int countAbove, int countBelow);
    void QueryFriends(const std::string& leaderboardId);
    
    // Get cached results
    LeaderboardQueryResult GetCachedResult(const std::string& leaderboardId) const;
    LeaderboardEntry GetUserEntry(const std::string& leaderboardId) const;
    int GetUserRank(const std::string& leaderboardId) const;
    int GetUserScore(const std::string& leaderboardId) const;
    
    // Score upload
    UploadResult UploadScore(const ScoreUpload& upload);
    UploadResult UploadHighScore(int score, const std::string& difficulty, const std::string& gameMode);
    UploadResult UploadSpeedrunTime(float seconds, const std::string& difficulty);
    UploadResult UploadEndlessWave(int wave, const std::string& difficulty);
    
    // Quick score uploads for common events
    void OnGameComplete(int score, int wave, float playTime, const std::string& difficulty, const std::string& gameMode);
    void OnEndlessGameOver(int wave, int score, const std::string& difficulty);
    
    // Leaderboard info
    std::vector<LeaderboardDef> GetAllLeaderboards() const;
    const LeaderboardDef* GetLeaderboardDef(const std::string& id) const;
    int GetTotalEntryCount(const std::string& leaderboardId) const;
    
    // Formatting
    std::string FormatScore(const std::string& leaderboardId, int score) const;
    std::string FormatTime(float seconds) const;
    std::string FormatRank(int rank) const;
    
    // Personal bests
    int GetPersonalBest(const std::string& leaderboardId) const;
    bool IsNewPersonalBest(const std::string& leaderboardId, int score) const;
    std::unordered_map<std::string, int> GetAllPersonalBests() const;
    
    // Callbacks
    using QueryCompleteCallback = std::function<void(const LeaderboardQueryResult&)>;
    using UploadCompleteCallback = std::function<void(UploadResult, const std::string& leaderboardId)>;
    
    void SetQueryCallback(QueryCompleteCallback callback) { m_queryCallback = callback; }
    void SetUploadCallback(UploadCompleteCallback callback) { m_uploadCallback = callback; }
    
    // Status
    bool IsQueryPending() const { return m_queryPending; }
    bool IsUploadPending() const { return m_uploadPending; }
    
private:
    LeaderboardSystem() = default;
    
    void RegisterLeaderboards();
    std::string GetLeaderboardApiName(const std::string& id, const std::string& difficulty, const std::string& gameMode) const;
    
    void OnQueryComplete(const LeaderboardQueryResult& result);
    void OnUploadComplete(UploadResult result, const std::string& leaderboardId, int score);
    
    // Leaderboard definitions
    std::unordered_map<std::string, LeaderboardDef> m_leaderboards;
    
    // Cached results
    std::unordered_map<std::string, LeaderboardQueryResult> m_cachedResults;
    std::unordered_map<std::string, LeaderboardEntry> m_userEntries;
    std::unordered_map<std::string, int> m_personalBests;
    std::unordered_map<std::string, int> m_totalEntryCounts;
    
    // Pending operations
    bool m_queryPending = false;
    bool m_uploadPending = false;
    std::string m_pendingQueryId;
    std::string m_pendingUploadId;
    
    // Callbacks
    QueryCompleteCallback m_queryCallback;
    UploadCompleteCallback m_uploadCallback;
    
    bool m_initialized = false;
};

} // namespace DDD
