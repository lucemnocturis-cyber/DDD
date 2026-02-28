#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <queue>

namespace DDD {

/**
 * Event categories
 */
enum class EventCategory {
    Session,        // Game session events
    Gameplay,       // In-game actions
    Progression,    // Player progress
    Economy,        // Gold, purchases
    Combat,         // Battle events
    UI,             // Interface interactions
    Performance,    // Technical metrics
    Error,          // Errors and crashes
    Social,         // Multiplayer, sharing
    Custom          // User-defined
};

/**
 * Event priority for batching
 */
enum class EventPriority {
    Low,            // Can be batched
    Normal,         // Standard priority
    High,           // Send soon
    Critical        // Send immediately
};

/**
 * Analytics event
 */
struct AnalyticsEvent {
    std::string eventId;
    std::string eventName;
    EventCategory category;
    EventPriority priority;
    uint64_t timestamp;
    std::string sessionId;
    std::string userId;
    std::unordered_map<std::string, std::string> stringParams;
    std::unordered_map<std::string, int> intParams;
    std::unordered_map<std::string, float> floatParams;
};

/**
 * Session info
 */
struct SessionInfo {
    std::string sessionId;
    uint64_t startTime;
    uint64_t endTime;
    float durationSeconds;
    int eventsCount;
    std::string platform;
    std::string gameVersion;
    std::string osVersion;
    std::string language;
    std::string country;
    bool isFirstSession;
    int sessionNumber;
};

/**
 * User profile for analytics
 */
struct UserProfile {
    std::string odwId;
    uint64_t firstSeenTimestamp;
    int totalSessions;
    float totalPlayTime;
    int gamesPlayed;
    int gamesWon;
    int highestWave;
    std::string lastDifficulty;
    std::string acquisitionSource;
    bool isPremium;
};

/**
 * Funnel stage
 */
struct FunnelStage {
    std::string funnelId;
    std::string stageName;
    int stageIndex;
    uint64_t timestamp;
    float timeInStage;
};

/**
 * A/B test variant
 */
struct ABTestVariant {
    std::string testId;
    std::string testName;
    std::string variantId;
    std::string variantName;
    uint64_t assignedTimestamp;
};

/**
 * Analytics configuration
 */
struct AnalyticsConfig {
    bool enabled = true;
    bool collectPerformance = true;
    bool collectCrashes = true;
    bool collectUserBehavior = true;
    int batchSize = 50;
    float batchIntervalSeconds = 60.0f;
    int maxQueueSize = 1000;
    std::string endpoint = "";
    std::string apiKey = "";
    bool debugMode = false;
};

/**
 * AnalyticsSystem - game telemetry and analytics
 */
class AnalyticsSystem {
public:
    static AnalyticsSystem& Instance();
    
    void Initialize(const AnalyticsConfig& config);
    void Shutdown();
    void Update(float deltaTime);
    
    // Session management
    void StartSession();
    void EndSession();
    void PauseSession();
    void ResumeSession();
    SessionInfo GetCurrentSession() const { return m_currentSession; }
    
    // Event tracking
    void TrackEvent(const std::string& eventName, EventCategory category = EventCategory::Custom);
    void TrackEvent(const AnalyticsEvent& event);
    void TrackEventWithParams(const std::string& eventName, 
                              const std::unordered_map<std::string, std::string>& params);
    
    // Gameplay events
    void TrackGameStart(const std::string& difficulty, const std::string& gameMode);
    void TrackGameEnd(bool won, int wave, int score, float duration);
    void TrackWaveStart(int waveNumber);
    void TrackWaveEnd(int waveNumber, bool success, float duration);
    void TrackBossEncounter(const std::string& bossId, bool defeated, float duration);
    void TrackUnitSummon(const std::string& unitClass, const std::string& diceUsed);
    void TrackUnitDeath(const std::string& unitClass, const std::string& killedBy);
    void TrackAbilityUsed(const std::string& abilityId, const std::string& unitClass);
    void TrackItemCollected(const std::string& itemId);
    
    // Economy events
    void TrackGoldEarned(int amount, const std::string& source);
    void TrackGoldSpent(int amount, const std::string& item);
    void TrackPurchase(const std::string& itemId, const std::string& currency, float price);
    
    // Progression events
    void TrackLevelUp(const std::string& unitClass, int newLevel);
    void TrackUnlock(const std::string& unlockType, const std::string& unlockId);
    void TrackAchievement(const std::string& achievementId);
    void TrackTutorialStep(const std::string& stepId, bool completed);
    
    // UI events
    void TrackScreenView(const std::string& screenName);
    void TrackButtonClick(const std::string& buttonId, const std::string& screenName);
    void TrackSettingChanged(const std::string& settingName, const std::string& newValue);
    
    // Performance events
    void TrackPerformance(float fps, float frameTime, int drawCalls, size_t memoryUsage);
    void TrackLoadTime(const std::string& loadType, float seconds);
    void TrackError(const std::string& errorType, const std::string& message, const std::string& stackTrace = "");
    void TrackCrash(const std::string& crashType, const std::string& details);
    
    // Funnel tracking
    void StartFunnel(const std::string& funnelId);
    void AdvanceFunnel(const std::string& funnelId, const std::string& stageName);
    void CompleteFunnel(const std::string& funnelId);
    void AbandonFunnel(const std::string& funnelId, const std::string& reason = "");
    
    // A/B testing
    void AssignABTest(const std::string& testId, const std::string& variantId);
    std::string GetABTestVariant(const std::string& testId) const;
    void TrackABTestConversion(const std::string& testId, const std::string& conversionEvent);
    
    // User properties
    void SetUserId(const std::string& userId);
    void SetUserProperty(const std::string& property, const std::string& value);
    void SetUserProperty(const std::string& property, int value);
    void IncrementUserProperty(const std::string& property, int amount = 1);
    
    // Consent management
    void SetAnalyticsConsent(bool granted);
    bool HasAnalyticsConsent() const { return m_consentGranted; }
    void SetDataCollectionLevel(int level);  // 0=none, 1=basic, 2=full
    
    // Data export
    std::string ExportSessionData() const;
    std::vector<AnalyticsEvent> GetRecentEvents(int count) const;
    
    // Debug
    void EnableDebugMode(bool enabled);
    int GetQueuedEventCount() const { return static_cast<int>(m_eventQueue.size()); }
    void FlushEvents();
    
private:
    AnalyticsSystem() = default;
    
    std::string GenerateSessionId() const;
    std::string GenerateEventId();  // FIXED: Removed const
    uint64_t GetTimestamp() const;
    void QueueEvent(const AnalyticsEvent& event);
    void ProcessEventQueue();
    void SendBatch(const std::vector<AnalyticsEvent>& events);
    std::string SerializeEvent(const AnalyticsEvent& event) const;
    std::string SerializeBatch(const std::vector<AnalyticsEvent>& events) const;
    void DetectPlatformInfo();
    
    // Configuration
    AnalyticsConfig m_config;
    bool m_consentGranted = true;
    int m_dataCollectionLevel = 2;
    
    // Session
    SessionInfo m_currentSession;
    bool m_sessionActive = false;
    bool m_sessionPaused = false;
    float m_sessionTimer = 0.0f;
    
    // User
    std::string m_userId;
    UserProfile m_userProfile;
    std::unordered_map<std::string, std::string> m_userProperties;
    
    // Events
    std::queue<AnalyticsEvent> m_eventQueue;
    std::vector<AnalyticsEvent> m_recentEvents;
    float m_batchTimer = 0.0f;
    mutable int m_eventCounter = 0;  // FIXED: Added mutable
    static constexpr int MAX_RECENT_EVENTS = 100;
    
    // Funnels
    std::unordered_map<std::string, std::vector<FunnelStage>> m_activeFunnels;
    
    // A/B tests
    std::unordered_map<std::string, ABTestVariant> m_abTests;
    
    // Platform info
    std::string m_platform;
    std::string m_gameVersion;
    std::string m_osVersion;
    std::string m_deviceId;
    std::string m_language;
    std::string m_country;
    
    bool m_initialized = false;
};

} // namespace DDD
