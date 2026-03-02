#include "AnalyticsSystem.h"
#include "../Utils/Logger.h"

#include <algorithm>
#include <sstream>
#include <random>
#include <ctime>
#include <iomanip>

namespace DDD {

AnalyticsSystem& AnalyticsSystem::Instance() {
    static AnalyticsSystem instance;
    return instance;
}

void AnalyticsSystem::Initialize(const AnalyticsConfig& config) {
    if (m_initialized) return;
    
    m_config = config;
    DetectPlatformInfo();
    
    m_gameVersion = "1.0.0";
    
    m_initialized = true;
    Logger::Info("AnalyticsSystem initialized (enabled: {}, debug: {})", 
                 m_config.enabled, m_config.debugMode);
}

void AnalyticsSystem::Shutdown() {
    if (m_sessionActive) {
        EndSession();
    }
    
    // Flush remaining events
    FlushEvents();
    
    m_initialized = false;
}

void AnalyticsSystem::Update(float deltaTime) {
    if (!m_config.enabled || !m_consentGranted) return;
    
    // Update session time
    if (m_sessionActive && !m_sessionPaused) {
        m_sessionTimer += deltaTime;
    }
    
    // Process event batch
    m_batchTimer += deltaTime;
    if (m_batchTimer >= m_config.batchIntervalSeconds) {
        ProcessEventQueue();
        m_batchTimer = 0.0f;
    }
}

void AnalyticsSystem::StartSession() {
    m_currentSession = SessionInfo();
    m_currentSession.sessionId = GenerateSessionId();
    m_currentSession.startTime = GetTimestamp();
    m_currentSession.platform = m_platform;
    m_currentSession.gameVersion = m_gameVersion;
    m_currentSession.osVersion = m_osVersion;
    m_currentSession.language = m_language;
    m_currentSession.country = m_country;
    m_currentSession.sessionNumber = m_userProfile.totalSessions + 1;
    m_currentSession.isFirstSession = (m_userProfile.totalSessions == 0);
    
    m_sessionActive = true;
    m_sessionPaused = false;
    m_sessionTimer = 0.0f;
    
    // Track session start
    AnalyticsEvent event;
    event.eventName = "session_start";
    event.category = EventCategory::Session;
    event.priority = EventPriority::High;
    event.stringParams["session_id"] = m_currentSession.sessionId;
    event.intParams["session_number"] = m_currentSession.sessionNumber;
    event.stringParams["is_first_session"] = m_currentSession.isFirstSession ? "true" : "false";
    TrackEvent(event);
    
    Logger::Info("Analytics session started: {}", m_currentSession.sessionId);
}

void AnalyticsSystem::EndSession() {
    if (!m_sessionActive) return;
    
    m_currentSession.endTime = GetTimestamp();
    m_currentSession.durationSeconds = m_sessionTimer;
    
    // Track session end
    AnalyticsEvent event;
    event.eventName = "session_end";
    event.category = EventCategory::Session;
    event.priority = EventPriority::High;
    event.floatParams["duration_seconds"] = m_sessionTimer;
    event.intParams["events_count"] = m_currentSession.eventsCount;
    TrackEvent(event);
    
    // Flush events
    FlushEvents();
    
    m_sessionActive = false;
    m_userProfile.totalSessions++;
    m_userProfile.totalPlayTime += m_sessionTimer;
    
    Logger::Info("Analytics session ended: {} ({:.1f}s)", 
                 m_currentSession.sessionId, m_sessionTimer);
}

void AnalyticsSystem::PauseSession() {
    if (!m_sessionActive || m_sessionPaused) return;
    
    m_sessionPaused = true;
    
    AnalyticsEvent event;
    event.eventName = "session_pause";
    event.category = EventCategory::Session;
    TrackEvent(event);
}

void AnalyticsSystem::ResumeSession() {
    if (!m_sessionActive || !m_sessionPaused) return;
    
    m_sessionPaused = false;
    
    AnalyticsEvent event;
    event.eventName = "session_resume";
    event.category = EventCategory::Session;
    TrackEvent(event);
}

void AnalyticsSystem::TrackEvent(const std::string& eventName, EventCategory category) {
    AnalyticsEvent event;
    event.eventName = eventName;
    event.category = category;
    event.priority = EventPriority::Normal;
    TrackEvent(event);
}

void AnalyticsSystem::TrackEvent(const AnalyticsEvent& event) {
    if (!m_config.enabled || !m_consentGranted) return;
    
    AnalyticsEvent fullEvent = event;
    fullEvent.eventId = GenerateEventId();
    fullEvent.timestamp = GetTimestamp();
    fullEvent.sessionId = m_currentSession.sessionId;
    fullEvent.userId = m_userId;
    
    QueueEvent(fullEvent);
    
    m_currentSession.eventsCount++;
    
    if (m_config.debugMode) {
        Logger::Debug("Analytics event: {} ({})", event.eventName, 
                     static_cast<int>(event.category));
    }
}

void AnalyticsSystem::TrackEventWithParams(const std::string& eventName,
                                           const std::unordered_map<std::string, std::string>& params) {
    AnalyticsEvent event;
    event.eventName = eventName;
    event.category = EventCategory::Custom;
    event.stringParams = params;
    TrackEvent(event);
}

// ===========================================================================
// GAMEPLAY EVENTS
// ===========================================================================

void AnalyticsSystem::TrackGameStart(const std::string& difficulty, const std::string& gameMode) {
    AnalyticsEvent event;
    event.eventName = "game_start";
    event.category = EventCategory::Gameplay;
    event.priority = EventPriority::High;
    event.stringParams["difficulty"] = difficulty;
    event.stringParams["game_mode"] = gameMode;
    TrackEvent(event);
    
    m_userProfile.gamesPlayed++;
    m_userProfile.lastDifficulty = difficulty;
}

void AnalyticsSystem::TrackGameEnd(bool won, int wave, int score, float duration) {
    AnalyticsEvent event;
    event.eventName = won ? "game_won" : "game_lost";
    event.category = EventCategory::Gameplay;
    event.priority = EventPriority::High;
    event.stringParams["won"] = won ? "true" : "false";
    event.intParams["final_wave"] = wave;
    event.intParams["final_score"] = score;
    event.floatParams["duration_seconds"] = duration;
    TrackEvent(event);
    
    if (won) m_userProfile.gamesWon++;
    if (wave > m_userProfile.highestWave) m_userProfile.highestWave = wave;
}

void AnalyticsSystem::TrackWaveStart(int waveNumber) {
    AnalyticsEvent event;
    event.eventName = "wave_start";
    event.category = EventCategory::Gameplay;
    event.intParams["wave_number"] = waveNumber;
    TrackEvent(event);
}

void AnalyticsSystem::TrackWaveEnd(int waveNumber, bool success, float duration) {
    AnalyticsEvent event;
    event.eventName = "wave_end";
    event.category = EventCategory::Gameplay;
    event.intParams["wave_number"] = waveNumber;
    event.stringParams["success"] = success ? "true" : "false";
    event.floatParams["duration_seconds"] = duration;
    TrackEvent(event);
}

void AnalyticsSystem::TrackBossEncounter(const std::string& bossId, bool defeated, float duration) {
    AnalyticsEvent event;
    event.eventName = "boss_encounter";
    event.category = EventCategory::Combat;
    event.priority = EventPriority::High;
    event.stringParams["boss_id"] = bossId;
    event.stringParams["defeated"] = defeated ? "true" : "false";
    event.floatParams["duration_seconds"] = duration;
    TrackEvent(event);
}

void AnalyticsSystem::TrackUnitSummon(const std::string& unitClass, const std::string& diceUsed) {
    AnalyticsEvent event;
    event.eventName = "unit_summon";
    event.category = EventCategory::Gameplay;
    event.stringParams["unit_class"] = unitClass;
    event.stringParams["dice_used"] = diceUsed;
    TrackEvent(event);
}

void AnalyticsSystem::TrackUnitDeath(const std::string& unitClass, const std::string& killedBy) {
    AnalyticsEvent event;
    event.eventName = "unit_death";
    event.category = EventCategory::Combat;
    event.stringParams["unit_class"] = unitClass;
    event.stringParams["killed_by"] = killedBy;
    TrackEvent(event);
}

void AnalyticsSystem::TrackAbilityUsed(const std::string& abilityId, const std::string& unitClass) {
    AnalyticsEvent event;
    event.eventName = "ability_used";
    event.category = EventCategory::Gameplay;
    event.stringParams["ability_id"] = abilityId;
    event.stringParams["unit_class"] = unitClass;
    TrackEvent(event);
}

void AnalyticsSystem::TrackItemCollected(const std::string& itemId) {
    AnalyticsEvent event;
    event.eventName = "item_collected";
    event.category = EventCategory::Gameplay;
    event.stringParams["item_id"] = itemId;
    TrackEvent(event);
}

// ===========================================================================
// ECONOMY EVENTS
// ===========================================================================

void AnalyticsSystem::TrackGoldEarned(int amount, const std::string& source) {
    AnalyticsEvent event;
    event.eventName = "gold_earned";
    event.category = EventCategory::Economy;
    event.intParams["amount"] = amount;
    event.stringParams["source"] = source;
    TrackEvent(event);
}

void AnalyticsSystem::TrackGoldSpent(int amount, const std::string& item) {
    AnalyticsEvent event;
    event.eventName = "gold_spent";
    event.category = EventCategory::Economy;
    event.intParams["amount"] = amount;
    event.stringParams["item"] = item;
    TrackEvent(event);
}

void AnalyticsSystem::TrackPurchase(const std::string& itemId, const std::string& currency, float price) {
    AnalyticsEvent event;
    event.eventName = "purchase";
    event.category = EventCategory::Economy;
    event.priority = EventPriority::High;
    event.stringParams["item_id"] = itemId;
    event.stringParams["currency"] = currency;
    event.floatParams["price"] = price;
    TrackEvent(event);
}

// ===========================================================================
// PROGRESSION EVENTS
// ===========================================================================

void AnalyticsSystem::TrackLevelUp(const std::string& unitClass, int newLevel) {
    AnalyticsEvent event;
    event.eventName = "level_up";
    event.category = EventCategory::Progression;
    event.stringParams["unit_class"] = unitClass;
    event.intParams["new_level"] = newLevel;
    TrackEvent(event);
}

void AnalyticsSystem::TrackUnlock(const std::string& unlockType, const std::string& unlockId) {
    AnalyticsEvent event;
    event.eventName = "unlock";
    event.category = EventCategory::Progression;
    event.priority = EventPriority::High;
    event.stringParams["unlock_type"] = unlockType;
    event.stringParams["unlock_id"] = unlockId;
    TrackEvent(event);
}

void AnalyticsSystem::TrackAchievement(const std::string& achievementId) {
    AnalyticsEvent event;
    event.eventName = "achievement_unlocked";
    event.category = EventCategory::Progression;
    event.priority = EventPriority::High;
    event.stringParams["achievement_id"] = achievementId;
    TrackEvent(event);
}

void AnalyticsSystem::TrackTutorialStep(const std::string& stepId, bool completed) {
    AnalyticsEvent event;
    event.eventName = "tutorial_step";
    event.category = EventCategory::Progression;
    event.stringParams["step_id"] = stepId;
    event.stringParams["completed"] = completed ? "true" : "false";
    TrackEvent(event);
}

// ===========================================================================
// UI EVENTS
// ===========================================================================

void AnalyticsSystem::TrackScreenView(const std::string& screenName) {
    AnalyticsEvent event;
    event.eventName = "screen_view";
    event.category = EventCategory::UI;
    event.stringParams["screen_name"] = screenName;
    TrackEvent(event);
}

void AnalyticsSystem::TrackButtonClick(const std::string& buttonId, const std::string& screenName) {
    AnalyticsEvent event;
    event.eventName = "button_click";
    event.category = EventCategory::UI;
    event.stringParams["button_id"] = buttonId;
    event.stringParams["screen_name"] = screenName;
    TrackEvent(event);
}

void AnalyticsSystem::TrackSettingChanged(const std::string& settingName, const std::string& newValue) {
    AnalyticsEvent event;
    event.eventName = "setting_changed";
    event.category = EventCategory::UI;
    event.stringParams["setting_name"] = settingName;
    event.stringParams["new_value"] = newValue;
    TrackEvent(event);
}

// ===========================================================================
// PERFORMANCE EVENTS
// ===========================================================================

void AnalyticsSystem::TrackPerformance(float fps, float frameTime, int drawCalls, size_t memoryUsage) {
    if (!m_config.collectPerformance) return;
    
    AnalyticsEvent event;
    event.eventName = "performance_sample";
    event.category = EventCategory::Performance;
    event.priority = EventPriority::Low;
    event.floatParams["fps"] = fps;
    event.floatParams["frame_time_ms"] = frameTime;
    event.intParams["draw_calls"] = drawCalls;
    event.intParams["memory_mb"] = static_cast<int>(memoryUsage / 1024 / 1024);
    TrackEvent(event);
}

void AnalyticsSystem::TrackLoadTime(const std::string& loadType, float seconds) {
    AnalyticsEvent event;
    event.eventName = "load_time";
    event.category = EventCategory::Performance;
    event.stringParams["load_type"] = loadType;
    event.floatParams["seconds"] = seconds;
    TrackEvent(event);
}

void AnalyticsSystem::TrackError(const std::string& errorType, const std::string& message, const std::string& stackTrace) {
    AnalyticsEvent event;
    event.eventName = "error";
    event.category = EventCategory::Error;
    event.priority = EventPriority::High;
    event.stringParams["error_type"] = errorType;
    event.stringParams["message"] = message;
    if (!stackTrace.empty()) {
        event.stringParams["stack_trace"] = stackTrace;
    }
    TrackEvent(event);
}

void AnalyticsSystem::TrackCrash(const std::string& crashType, const std::string& details) {
    if (!m_config.collectCrashes) return;
    
    AnalyticsEvent event;
    event.eventName = "crash";
    event.category = EventCategory::Error;
    event.priority = EventPriority::Critical;
    event.stringParams["crash_type"] = crashType;
    event.stringParams["details"] = details;
    TrackEvent(event);
    
    // Immediately flush
    FlushEvents();
}

// ===========================================================================
// FUNNEL TRACKING
// ===========================================================================

void AnalyticsSystem::StartFunnel(const std::string& funnelId) {
    m_activeFunnels[funnelId].clear();
    
    FunnelStage stage;
    stage.funnelId = funnelId;
    stage.stageName = "start";
    stage.stageIndex = 0;
    stage.timestamp = GetTimestamp();
    stage.timeInStage = 0;
    m_activeFunnels[funnelId].push_back(stage);
    
    AnalyticsEvent event;
    event.eventName = "funnel_start";
    event.category = EventCategory::Progression;
    event.stringParams["funnel_id"] = funnelId;
    TrackEvent(event);
}

void AnalyticsSystem::AdvanceFunnel(const std::string& funnelId, const std::string& stageName) {
    auto it = m_activeFunnels.find(funnelId);
    if (it == m_activeFunnels.end()) return;
    
    FunnelStage stage;
    stage.funnelId = funnelId;
    stage.stageName = stageName;
    stage.stageIndex = static_cast<int>(it->second.size());
    stage.timestamp = GetTimestamp();
    
    if (!it->second.empty()) {
        stage.timeInStage = static_cast<float>(stage.timestamp - it->second.back().timestamp);
    }
    
    it->second.push_back(stage);
    
    AnalyticsEvent event;
    event.eventName = "funnel_step";
    event.category = EventCategory::Progression;
    event.stringParams["funnel_id"] = funnelId;
    event.stringParams["stage_name"] = stageName;
    event.intParams["stage_index"] = stage.stageIndex;
    event.floatParams["time_in_previous_stage"] = stage.timeInStage;
    TrackEvent(event);
}

void AnalyticsSystem::CompleteFunnel(const std::string& funnelId) {
    AdvanceFunnel(funnelId, "complete");
    
    AnalyticsEvent event;
    event.eventName = "funnel_complete";
    event.category = EventCategory::Progression;
    event.priority = EventPriority::High;
    event.stringParams["funnel_id"] = funnelId;
    
    auto it = m_activeFunnels.find(funnelId);
    if (it != m_activeFunnels.end()) {
        event.intParams["total_steps"] = static_cast<int>(it->second.size());
    }
    
    TrackEvent(event);
    m_activeFunnels.erase(funnelId);
}

void AnalyticsSystem::AbandonFunnel(const std::string& funnelId, const std::string& reason) {
    AnalyticsEvent event;
    event.eventName = "funnel_abandon";
    event.category = EventCategory::Progression;
    event.stringParams["funnel_id"] = funnelId;
    event.stringParams["reason"] = reason;
    
    auto it = m_activeFunnels.find(funnelId);
    if (it != m_activeFunnels.end()) {
        event.intParams["abandoned_at_step"] = static_cast<int>(it->second.size());
        if (!it->second.empty()) {
            event.stringParams["last_stage"] = it->second.back().stageName;
        }
    }
    
    TrackEvent(event);
    m_activeFunnels.erase(funnelId);
}

// ===========================================================================
// A/B TESTING
// ===========================================================================

void AnalyticsSystem::AssignABTest(const std::string& testId, const std::string& variantId) {
    ABTestVariant variant;
    variant.testId = testId;
    variant.variantId = variantId;
    variant.assignedTimestamp = GetTimestamp();
    m_abTests[testId] = variant;
    
    AnalyticsEvent event;
    event.eventName = "ab_test_assigned";
    event.category = EventCategory::Custom;
    event.stringParams["test_id"] = testId;
    event.stringParams["variant_id"] = variantId;
    TrackEvent(event);
}

std::string AnalyticsSystem::GetABTestVariant(const std::string& testId) const {
    auto it = m_abTests.find(testId);
    return it != m_abTests.end() ? it->second.variantId : "";
}

void AnalyticsSystem::TrackABTestConversion(const std::string& testId, const std::string& conversionEvent) {
    AnalyticsEvent event;
    event.eventName = "ab_test_conversion";
    event.category = EventCategory::Custom;
    event.priority = EventPriority::High;
    event.stringParams["test_id"] = testId;
    event.stringParams["conversion_event"] = conversionEvent;
    event.stringParams["variant_id"] = GetABTestVariant(testId);
    TrackEvent(event);
}

// ===========================================================================
// USER PROPERTIES
// ===========================================================================

void AnalyticsSystem::SetUserId(const std::string& userId) {
    m_userId = userId;
    m_userProfile.odwId = userId;
}

void AnalyticsSystem::SetUserProperty(const std::string& property, const std::string& value) {
    m_userProperties[property] = value;
}

void AnalyticsSystem::SetUserProperty(const std::string& property, int value) {
    m_userProperties[property] = std::to_string(value);
}

void AnalyticsSystem::IncrementUserProperty(const std::string& property, int amount) {
    auto it = m_userProperties.find(property);
    int current = (it != m_userProperties.end()) ? std::stoi(it->second) : 0;
    m_userProperties[property] = std::to_string(current + amount);
}

void AnalyticsSystem::SetAnalyticsConsent(bool granted) {
    m_consentGranted = granted;
    Logger::Info("Analytics consent: {}", granted ? "granted" : "denied");
}

void AnalyticsSystem::SetDataCollectionLevel(int level) {
    m_dataCollectionLevel = std::clamp(level, 0, 2);
}

std::string AnalyticsSystem::ExportSessionData() const {
    std::ostringstream ss;
    ss << "Session ID: " << m_currentSession.sessionId << "\n";
    ss << "Duration: " << m_sessionTimer << "s\n";
    ss << "Events: " << m_currentSession.eventsCount << "\n";
    return ss.str();
}

std::vector<AnalyticsEvent> AnalyticsSystem::GetRecentEvents(int count) const {
    std::vector<AnalyticsEvent> result;
    int start = std::max(0, static_cast<int>(m_recentEvents.size()) - count);
    for (int i = start; i < static_cast<int>(m_recentEvents.size()); ++i) {
        result.push_back(m_recentEvents[i]);
    }
    return result;
}

void AnalyticsSystem::EnableDebugMode(bool enabled) {
    m_config.debugMode = enabled;
}

void AnalyticsSystem::FlushEvents() {
    ProcessEventQueue();
}

// ===========================================================================
// PRIVATE METHODS
// ===========================================================================

std::string AnalyticsSystem::GenerateSessionId() const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    for (int i = 0; i < 32; ++i) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

std::string AnalyticsSystem::GenerateEventId() {
    return m_currentSession.sessionId + "_" + std::to_string(m_eventCounter++);
}

uint64_t AnalyticsSystem::GetTimestamp() const {
    return static_cast<uint64_t>(std::time(nullptr));
}

void AnalyticsSystem::QueueEvent(const AnalyticsEvent& event) {
    // Store in recent events
    m_recentEvents.push_back(event);
    if (m_recentEvents.size() > MAX_RECENT_EVENTS) {
        m_recentEvents.erase(m_recentEvents.begin());
    }
    
    // Queue for sending
    if (m_eventQueue.size() < static_cast<size_t>(m_config.maxQueueSize)) {
        m_eventQueue.push(event);
    }
    
    // Immediate send for critical events
    if (event.priority == EventPriority::Critical) {
        FlushEvents();
    }
}

void AnalyticsSystem::ProcessEventQueue() {
    if (m_eventQueue.empty()) return;
    
    std::vector<AnalyticsEvent> batch;
    while (!m_eventQueue.empty() && static_cast<int>(batch.size()) < m_config.batchSize) {
        batch.push_back(m_eventQueue.front());
        m_eventQueue.pop();
    }
    
    if (!batch.empty()) {
        SendBatch(batch);
    }
}

void AnalyticsSystem::SendBatch(const std::vector<AnalyticsEvent>& events) {
    // In production, would send to analytics endpoint
    // std::string payload = SerializeBatch(events);
    // HttpClient::Post(m_config.endpoint, payload);
    
    if (m_config.debugMode) {
        Logger::Debug("Analytics: sent batch of {} events", events.size());
    }
}

std::string AnalyticsSystem::SerializeEvent(const AnalyticsEvent& event) const {
    std::ostringstream ss;
    ss << "{";
    ss << "\"event_id\":\"" << event.eventId << "\",";
    ss << "\"event_name\":\"" << event.eventName << "\",";
    ss << "\"timestamp\":" << event.timestamp << ",";
    ss << "\"session_id\":\"" << event.sessionId << "\"";
    // Add params...
    ss << "}";
    return ss.str();
}

std::string AnalyticsSystem::SerializeBatch(const std::vector<AnalyticsEvent>& events) const {
    std::ostringstream ss;
    ss << "{\"events\":[";
    for (size_t i = 0; i < events.size(); ++i) {
        if (i > 0) ss << ",";
        ss << SerializeEvent(events[i]);
    }
    ss << "]}";
    return ss.str();
}

void AnalyticsSystem::DetectPlatformInfo() {
#ifdef _WIN32
    m_platform = "Windows";
    m_osVersion = "Windows 10+";
#elif __APPLE__
    m_platform = "macOS";
    m_osVersion = "macOS 10.15+";
#else
    m_platform = "Linux";
    m_osVersion = "Linux";
#endif
    
    m_language = "en";
    m_country = "US";
}

} // namespace DDD
