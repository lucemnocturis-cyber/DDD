#include "LeaderboardSystem.h"
#include "SteamManager.h"
#include "../Utils/Logger.h"

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace DDD {

LeaderboardSystem& LeaderboardSystem::Instance() {
    static LeaderboardSystem instance;
    return instance;
}

void LeaderboardSystem::Initialize() {
    if (m_initialized) return;
    
    RegisterLeaderboards();
    
    m_initialized = true;
    Logger::Info("LeaderboardSystem initialized with {} leaderboards", m_leaderboards.size());
}

void LeaderboardSystem::Shutdown() {
    m_leaderboards.clear();
    m_cachedResults.clear();
    m_userEntries.clear();
    m_personalBests.clear();
    m_initialized = false;
}

void LeaderboardSystem::Update(float deltaTime) {
    // In production, would process Steam callbacks here
}

void LeaderboardSystem::QueryLeaderboard(const std::string& leaderboardId, LeaderboardRange range, int startRank, int count) {
    if (!SteamManager::Instance().IsInitialized()) {
        Logger::Warning("Cannot query leaderboard - Steam not initialized");
        return;
    }
    
    m_queryPending = true;
    m_pendingQueryId = leaderboardId;
    
    Logger::Info("Querying leaderboard: {} (range: {}, start: {}, count: {})",
                 leaderboardId, static_cast<int>(range), startRank, count);
    
    // In production:
    // SteamAPICall_t call = SteamUserStats()->FindLeaderboard(apiName.c_str());
    // Then DownloadLeaderboardEntries in callback
    
    // Simulate response
    LeaderboardQueryResult result;
    result.success = true;
    result.leaderboardId = leaderboardId;
    result.range = range;
    result.totalEntries = 1000;
    result.startRank = startRank;
    result.endRank = startRank + count - 1;
    
    // Generate fake entries
    for (int i = 0; i < count; ++i) {
        LeaderboardEntry entry;
        entry.globalRank = startRank + i;
        entry.friendRank = (i < 5) ? i + 1 : 0;
        entry.steamId = 76561198000000000ULL + startRank + i;
        entry.playerName = "Player" + std::to_string(startRank + i);
        
        // Score based on type
        const LeaderboardDef* def = GetLeaderboardDef(leaderboardId);
        if (def && def->sortMethod == LeaderboardSort::Ascending) {
            entry.score = 300 + (startRank + i) * 10;  // Time-based
        } else {
            entry.score = 100000 - (startRank + i) * 100;  // Score-based
        }
        
        entry.detailValue1 = 20;  // Wave
        entry.detailValue2 = 6;   // Units
        entry.detailString = "Normal";
        entry.timestamp = static_cast<uint64_t>(std::time(nullptr)) - (startRank + i) * 3600;
        entry.isCurrentUser = (i == 5);
        entry.isFriend = (i < 5);
        
        result.entries.push_back(entry);
    }
    
    m_cachedResults[leaderboardId] = result;
    m_queryPending = false;
    
    OnQueryComplete(result);
}

void LeaderboardSystem::QueryAroundUser(const std::string& leaderboardId, int countAbove, int countBelow) {
    int userRank = GetUserRank(leaderboardId);
    if (userRank <= 0) userRank = 50;  // Default if unknown
    
    int startRank = std::max(1, userRank - countAbove);
    int count = countAbove + countBelow + 1;
    
    QueryLeaderboard(leaderboardId, LeaderboardRange::Global, startRank, count);
}

void LeaderboardSystem::QueryFriends(const std::string& leaderboardId) {
    QueryLeaderboard(leaderboardId, LeaderboardRange::Friends, 1, 100);
}

LeaderboardQueryResult LeaderboardSystem::GetCachedResult(const std::string& leaderboardId) const {
    auto it = m_cachedResults.find(leaderboardId);
    if (it != m_cachedResults.end()) {
        return it->second;
    }
    
    LeaderboardQueryResult empty;
    empty.success = false;
    empty.errorMessage = "No cached result";
    return empty;
}

LeaderboardEntry LeaderboardSystem::GetUserEntry(const std::string& leaderboardId) const {
    auto it = m_userEntries.find(leaderboardId);
    if (it != m_userEntries.end()) {
        return it->second;
    }
    
    LeaderboardEntry empty;
    empty.globalRank = 0;
    return empty;
}

int LeaderboardSystem::GetUserRank(const std::string& leaderboardId) const {
    auto entry = GetUserEntry(leaderboardId);
    return entry.globalRank;
}

int LeaderboardSystem::GetUserScore(const std::string& leaderboardId) const {
    auto entry = GetUserEntry(leaderboardId);
    return entry.score;
}

UploadResult LeaderboardSystem::UploadScore(const ScoreUpload& upload) {
    if (!SteamManager::Instance().IsInitialized()) {
        Logger::Warning("Cannot upload score - Steam not initialized");
        return UploadResult::NotConnected;
    }
    
    const LeaderboardDef* def = GetLeaderboardDef(upload.leaderboardId);
    if (!def) {
        Logger::Error("Unknown leaderboard: {}", upload.leaderboardId);
        return UploadResult::Failed;
    }
    
    m_uploadPending = true;
    m_pendingUploadId = upload.leaderboardId;
    
    std::string apiName = GetLeaderboardApiName(upload.leaderboardId, upload.difficulty, upload.gameMode);
    
    Logger::Info("Uploading score to {}: {} (difficulty: {}, mode: {})",
                 upload.leaderboardId, upload.score, upload.difficulty, upload.gameMode);
    
    // In production:
    // SteamAPICall_t call = SteamUserStats()->FindLeaderboard(apiName.c_str());
    // Then UploadLeaderboardScore in callback
    
    // Check for personal best
    bool isNewBest = IsNewPersonalBest(upload.leaderboardId, upload.score);
    
    if (isNewBest || upload.forceUpdate) {
        m_personalBests[upload.leaderboardId] = upload.score;
        
        // Update user entry
        LeaderboardEntry entry;
        entry.globalRank = 0;  // Will be updated by query
        entry.steamId = SteamManager::Instance().GetSteamId();
        entry.playerName = SteamManager::Instance().GetDisplayName();
        entry.score = upload.score;
        entry.detailValue1 = upload.wave;
        entry.detailValue2 = upload.unitsUsed;
        entry.detailString = upload.difficulty;
        entry.timestamp = static_cast<uint64_t>(std::time(nullptr));
        entry.isCurrentUser = true;
        m_userEntries[upload.leaderboardId] = entry;
    }
    
    m_uploadPending = false;
    
    UploadResult result = isNewBest ? UploadResult::SuccessNewRecord : UploadResult::Success;
    OnUploadComplete(result, upload.leaderboardId, upload.score);
    
    return result;
}

UploadResult LeaderboardSystem::UploadHighScore(int score, const std::string& difficulty, const std::string& gameMode) {
    ScoreUpload upload;
    upload.leaderboardId = "high_score";
    upload.score = score;
    upload.difficulty = difficulty;
    upload.gameMode = gameMode;
    return UploadScore(upload);
}

UploadResult LeaderboardSystem::UploadSpeedrunTime(float seconds, const std::string& difficulty) {
    ScoreUpload upload;
    upload.leaderboardId = "speedrun";
    upload.score = static_cast<int>(seconds * 1000);  // Store as milliseconds
    upload.difficulty = difficulty;
    return UploadScore(upload);
}

UploadResult LeaderboardSystem::UploadEndlessWave(int wave, const std::string& difficulty) {
    ScoreUpload upload;
    upload.leaderboardId = "endless_wave";
    upload.score = wave;
    upload.difficulty = difficulty;
    return UploadScore(upload);
}

void LeaderboardSystem::OnGameComplete(int score, int wave, float playTime, const std::string& difficulty, const std::string& gameMode) {
    // Upload high score
    ScoreUpload highScore;
    highScore.leaderboardId = "high_score";
    highScore.score = score;
    highScore.difficulty = difficulty;
    highScore.gameMode = gameMode;
    highScore.wave = wave;
    highScore.playTime = playTime;
    UploadScore(highScore);
    
    // Upload speedrun if campaign
    if (gameMode == "campaign") {
        ScoreUpload speedrun;
        speedrun.leaderboardId = "speedrun";
        speedrun.score = static_cast<int>(playTime * 1000);
        speedrun.difficulty = difficulty;
        UploadScore(speedrun);
    }
    
    Logger::Info("Game complete scores uploaded - Score: {}, Time: {:.1f}s", score, playTime);
}

void LeaderboardSystem::OnEndlessGameOver(int wave, int score, const std::string& difficulty) {
    // Upload wave reached
    ScoreUpload waveUpload;
    waveUpload.leaderboardId = "endless_wave";
    waveUpload.score = wave;
    waveUpload.difficulty = difficulty;
    UploadScore(waveUpload);
    
    // Upload endless score
    ScoreUpload scoreUpload;
    scoreUpload.leaderboardId = "endless_score";
    scoreUpload.score = score;
    scoreUpload.difficulty = difficulty;
    UploadScore(scoreUpload);
    
    Logger::Info("Endless scores uploaded - Wave: {}, Score: {}", wave, score);
}

std::vector<LeaderboardDef> LeaderboardSystem::GetAllLeaderboards() const {
    std::vector<LeaderboardDef> result;
    for (const auto& [id, def] : m_leaderboards) {
        result.push_back(def);
    }
    return result;
}

const LeaderboardDef* LeaderboardSystem::GetLeaderboardDef(const std::string& id) const {
    auto it = m_leaderboards.find(id);
    return it != m_leaderboards.end() ? &it->second : nullptr;
}

int LeaderboardSystem::GetTotalEntryCount(const std::string& leaderboardId) const {
    auto it = m_totalEntryCounts.find(leaderboardId);
    return it != m_totalEntryCounts.end() ? it->second : 0;
}

std::string LeaderboardSystem::FormatScore(const std::string& leaderboardId, int score) const {
    const LeaderboardDef* def = GetLeaderboardDef(leaderboardId);
    
    if (def) {
        if (def->type == LeaderboardType::FastestTime || 
            def->type == LeaderboardType::SpeedrunAny ||
            def->type == LeaderboardType::SpeedrunNoHits) {
            return FormatTime(score / 1000.0f);
        }
    }
    
    // Format with commas
    std::string str = std::to_string(score);
    int n = static_cast<int>(str.length()) - 3;
    while (n > 0) {
        str.insert(n, ",");
        n -= 3;
    }
    return str;
}

std::string LeaderboardSystem::FormatTime(float seconds) const {
    int totalSeconds = static_cast<int>(seconds);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int secs = totalSeconds % 60;
    int ms = static_cast<int>((seconds - totalSeconds) * 1000);
    
    std::ostringstream ss;
    if (hours > 0) {
        ss << hours << ":" << std::setfill('0') << std::setw(2) << minutes << ":"
           << std::setw(2) << secs << "." << std::setw(3) << ms;
    } else {
        ss << minutes << ":" << std::setfill('0') << std::setw(2) << secs 
           << "." << std::setw(3) << ms;
    }
    return ss.str();
}

std::string LeaderboardSystem::FormatRank(int rank) const {
    if (rank <= 0) return "-";
    
    std::string suffix;
    int lastTwo = rank % 100;
    
    if (lastTwo >= 11 && lastTwo <= 13) {
        suffix = "th";
    } else {
        switch (rank % 10) {
            case 1: suffix = "st"; break;
            case 2: suffix = "nd"; break;
            case 3: suffix = "rd"; break;
            default: suffix = "th"; break;
        }
    }
    
    return std::to_string(rank) + suffix;
}

int LeaderboardSystem::GetPersonalBest(const std::string& leaderboardId) const {
    auto it = m_personalBests.find(leaderboardId);
    return it != m_personalBests.end() ? it->second : 0;
}

bool LeaderboardSystem::IsNewPersonalBest(const std::string& leaderboardId, int score) const {
    const LeaderboardDef* def = GetLeaderboardDef(leaderboardId);
    if (!def) return false;
    
    int currentBest = GetPersonalBest(leaderboardId);
    if (currentBest == 0) return true;  // No previous score
    
    if (def->sortMethod == LeaderboardSort::Ascending) {
        return score < currentBest;  // Lower is better
    } else {
        return score > currentBest;  // Higher is better
    }
}

std::unordered_map<std::string, int> LeaderboardSystem::GetAllPersonalBests() const {
    return m_personalBests;
}

// ===========================================================================
// PRIVATE METHODS
// ===========================================================================

void LeaderboardSystem::RegisterLeaderboards() {
    // High Score (all modes)
    LeaderboardDef highScore;
    highScore.id = "high_score";
    highScore.apiName = "high_score";
    highScore.displayName = "High Score";
    highScore.type = LeaderboardType::HighScore;
    highScore.sortMethod = LeaderboardSort::Descending;
    highScore.formatString = "{0} pts";
    highScore.perDifficulty = true;
    highScore.perGameMode = true;
    m_leaderboards[highScore.id] = highScore;
    
    // Speedrun
    LeaderboardDef speedrun;
    speedrun.id = "speedrun";
    speedrun.apiName = "speedrun";
    speedrun.displayName = "Speedrun";
    speedrun.type = LeaderboardType::FastestTime;
    speedrun.sortMethod = LeaderboardSort::Ascending;
    speedrun.formatString = "{0}";
    speedrun.perDifficulty = true;
    speedrun.perGameMode = false;
    m_leaderboards[speedrun.id] = speedrun;
    
    // Endless Wave
    LeaderboardDef endlessWave;
    endlessWave.id = "endless_wave";
    endlessWave.apiName = "endless_wave";
    endlessWave.displayName = "Endless - Highest Wave";
    endlessWave.type = LeaderboardType::HighestWave;
    endlessWave.sortMethod = LeaderboardSort::Descending;
    endlessWave.formatString = "Wave {0}";
    endlessWave.perDifficulty = true;
    endlessWave.perGameMode = false;
    m_leaderboards[endlessWave.id] = endlessWave;
    
    // Endless Score
    LeaderboardDef endlessScore;
    endlessScore.id = "endless_score";
    endlessScore.apiName = "endless_score";
    endlessScore.displayName = "Endless - High Score";
    endlessScore.type = LeaderboardType::HighScore;
    endlessScore.sortMethod = LeaderboardSort::Descending;
    endlessScore.formatString = "{0} pts";
    endlessScore.perDifficulty = true;
    endlessScore.perGameMode = false;
    m_leaderboards[endlessScore.id] = endlessScore;
    
    // Most Kills (single run)
    LeaderboardDef mostKills;
    mostKills.id = "most_kills";
    mostKills.apiName = "most_kills";
    mostKills.displayName = "Most Kills";
    mostKills.type = LeaderboardType::MostKills;
    mostKills.sortMethod = LeaderboardSort::Descending;
    mostKills.formatString = "{0} kills";
    mostKills.perDifficulty = true;
    mostKills.perGameMode = false;
    m_leaderboards[mostKills.id] = mostKills;
    
    // Perfect Waves
    LeaderboardDef perfectWaves;
    perfectWaves.id = "perfect_waves";
    perfectWaves.apiName = "perfect_waves";
    perfectWaves.displayName = "Perfect Waves";
    perfectWaves.type = LeaderboardType::PerfectWaves;
    perfectWaves.sortMethod = LeaderboardSort::Descending;
    perfectWaves.formatString = "{0} waves";
    perfectWaves.perDifficulty = true;
    perfectWaves.perGameMode = false;
    m_leaderboards[perfectWaves.id] = perfectWaves;
    
    // Daily Challenge
    LeaderboardDef daily;
    daily.id = "daily_challenge";
    daily.apiName = "daily";
    daily.displayName = "Daily Challenge";
    daily.type = LeaderboardType::HighScore;
    daily.sortMethod = LeaderboardSort::Descending;
    daily.formatString = "{0} pts";
    daily.perDifficulty = false;
    daily.perGameMode = false;
    m_leaderboards[daily.id] = daily;
    
    // Weekly Challenge
    LeaderboardDef weekly;
    weekly.id = "weekly_challenge";
    weekly.apiName = "weekly";
    weekly.displayName = "Weekly Challenge";
    weekly.type = LeaderboardType::HighScore;
    weekly.sortMethod = LeaderboardSort::Descending;
    weekly.formatString = "{0} pts";
    weekly.perDifficulty = false;
    weekly.perGameMode = false;
    m_leaderboards[weekly.id] = weekly;
}

std::string LeaderboardSystem::GetLeaderboardApiName(const std::string& id, const std::string& difficulty, const std::string& gameMode) const {
    const LeaderboardDef* def = GetLeaderboardDef(id);
    if (!def) return id;
    
    std::string apiName = def->apiName;
    
    if (def->perDifficulty && !difficulty.empty()) {
        apiName += "_" + difficulty;
    }
    
    if (def->perGameMode && !gameMode.empty()) {
        apiName += "_" + gameMode;
    }
    
    return apiName;
}

void LeaderboardSystem::OnQueryComplete(const LeaderboardQueryResult& result) {
    Logger::Info("Leaderboard query complete: {} ({} entries)",
                 result.leaderboardId, result.entries.size());
    
    // Find and cache user entry
    for (const auto& entry : result.entries) {
        if (entry.isCurrentUser) {
            m_userEntries[result.leaderboardId] = entry;
            m_personalBests[result.leaderboardId] = entry.score;
            break;
        }
    }
    
    // Store total count
    m_totalEntryCounts[result.leaderboardId] = result.totalEntries;
    
    if (m_queryCallback) {
        m_queryCallback(result);
    }
}

void LeaderboardSystem::OnUploadComplete(UploadResult result, const std::string& leaderboardId, int score) {
    if (result == UploadResult::SuccessNewRecord) {
        Logger::Info("New personal best on {}: {}", leaderboardId, FormatScore(leaderboardId, score));
    }
    
    if (m_uploadCallback) {
        m_uploadCallback(result, leaderboardId);
    }
}

} // namespace DDD
