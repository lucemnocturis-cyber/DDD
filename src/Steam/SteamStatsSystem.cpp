#include "SteamStatsSystem.h"
#include "SteamManager.h"
#include "../Utils/Logger.h"

#include <algorithm>
#include <ctime>

namespace DDD {

SteamStatsSystem& SteamStatsSystem::Instance() {
    static SteamStatsSystem instance;
    return instance;
}

void SteamStatsSystem::Initialize() {
    if (m_initialized) return;
    
    RegisterAllStats();
    
    // Initialize card sets (simulated data)
    TradingCardSet mainSet;
    mainSet.setId = "main";
    mainSet.setName = "Dungeon Dice Duelists";
    mainSet.totalCards = 8;
    mainSet.ownedCards = 0;
    mainSet.cardIds = {"card_mage", "card_soldier", "card_rogue", "card_healer",
                       "card_dragon", "card_necro", "card_titan", "card_shadow"};
    mainSet.complete = false;
    m_cardSets.push_back(mainSet);
    
    // Initialize badges
    Badge normalBadge;
    normalBadge.badgeId = "badge_normal";
    normalBadge.name = "Dungeon Dice Duelists";
    normalBadge.description = "Collected the complete card set";
    normalBadge.level = 0;
    normalBadge.isFoil = false;
    normalBadge.xpEarned = 0;
    m_badges.push_back(normalBadge);
    
    Badge foilBadge;
    foilBadge.badgeId = "badge_foil";
    foilBadge.name = "Dungeon Dice Duelists (Foil)";
    foilBadge.description = "Collected the complete foil card set";
    foilBadge.level = 0;
    foilBadge.isFoil = true;
    foilBadge.xpEarned = 0;
    m_badges.push_back(foilBadge);
    
    // Download stats from Steam
    DownloadStats();
    
    m_initialized = true;
    Logger::Info("SteamStatsSystem initialized with {} stats", m_statDefs.size());
}

void SteamStatsSystem::Shutdown() {
    if (m_sessionActive) {
        EndSession();
    }
    
    // Final upload
    UploadStats();
    
    m_statDefs.clear();
    m_statValues.clear();
    m_initialized = false;
}

void SteamStatsSystem::Update(float deltaTime) {
    // Update session time
    if (m_sessionActive) {
        m_sessionPlayTime += deltaTime;
        
        // Check for card drops
        CheckCardDrop();
    }
    
    // Periodic stat upload
    if (m_pendingUpload) {
        m_uploadTimer += deltaTime;
        if (m_uploadTimer >= UPLOAD_INTERVAL) {
            UploadStats();
            m_uploadTimer = 0.0f;
        }
    }
}

void SteamStatsSystem::RegisterStat(const StatDef& stat) {
    m_statDefs[stat.id] = stat;
    
    // Initialize value
    StatValue value;
    value.id = stat.id;
    value.type = stat.type;
    value.intValue = static_cast<int>(stat.defaultValue);
    value.floatValue = stat.defaultValue;
    value.dirty = false;
    m_statValues[stat.id] = value;
}

const StatDef* SteamStatsSystem::GetStatDef(const std::string& id) const {
    auto it = m_statDefs.find(id);
    return it != m_statDefs.end() ? &it->second : nullptr;
}

std::vector<StatDef> SteamStatsSystem::GetAllStats() const {
    std::vector<StatDef> result;
    for (const auto& [id, def] : m_statDefs) {
        result.push_back(def);
    }
    return result;
}

void SteamStatsSystem::SetStatInt(const std::string& id, int value) {
    auto it = m_statValues.find(id);
    if (it == m_statValues.end()) return;
    
    const StatDef* def = GetStatDef(id);
    if (def) {
        value = std::max(static_cast<int>(def->minValue), 
                        std::min(static_cast<int>(def->maxValue), value));
        
        if (def->incrementOnly && value < it->second.intValue) {
            return;  // Can't decrease
        }
    }
    
    it->second.intValue = value;
    it->second.dirty = true;
    m_pendingUpload = true;
}

void SteamStatsSystem::SetStatFloat(const std::string& id, float value) {
    auto it = m_statValues.find(id);
    if (it == m_statValues.end()) return;
    
    const StatDef* def = GetStatDef(id);
    if (def) {
        value = std::max(def->minValue, std::min(def->maxValue, value));
        
        if (def->incrementOnly && value < it->second.floatValue) {
            return;
        }
    }
    
    it->second.floatValue = value;
    it->second.dirty = true;
    m_pendingUpload = true;
}

void SteamStatsSystem::IncrementStat(const std::string& id, int amount) {
    int current = GetStatInt(id);
    SetStatInt(id, current + amount);
}

void SteamStatsSystem::IncrementStatFloat(const std::string& id, float amount) {
    float current = GetStatFloat(id);
    SetStatFloat(id, current + amount);
}

void SteamStatsSystem::UpdateAvgRateStat(const std::string& id, float value, float sessionLength) {
    // In production, Steam handles averaging
    // For now, just set the value
    SetStatFloat(id, value);
}

int SteamStatsSystem::GetStatInt(const std::string& id) const {
    auto it = m_statValues.find(id);
    return it != m_statValues.end() ? it->second.intValue : 0;
}

float SteamStatsSystem::GetStatFloat(const std::string& id) const {
    auto it = m_statValues.find(id);
    return it != m_statValues.end() ? it->second.floatValue : 0.0f;
}

void SteamStatsSystem::UploadStats() {
    if (!SteamManager::Instance().IsInitialized()) return;
    
    // In production:
    // for (const auto& [id, value] : m_statValues) {
    //     if (value.dirty) {
    //         const StatDef* def = GetStatDef(id);
    //         if (def->type == StatType::Int) {
    //             SteamUserStats()->SetStat(def->apiName.c_str(), value.intValue);
    //         } else {
    //             SteamUserStats()->SetStat(def->apiName.c_str(), value.floatValue);
    //         }
    //     }
    // }
    // SteamUserStats()->StoreStats();
    
    // Clear dirty flags
    for (auto& [id, value] : m_statValues) {
        value.dirty = false;
    }
    
    m_pendingUpload = false;
    Logger::Info("Stats uploaded to Steam");
}

void SteamStatsSystem::DownloadStats() {
    if (!SteamManager::Instance().IsInitialized()) {
        if (m_statsCallback) m_statsCallback(false);
        return;
    }
    
    // In production:
    // SteamUserStats()->RequestCurrentStats();
    // Then in callback, read each stat
    
    // Simulate success
    if (m_statsCallback) m_statsCallback(true);
    Logger::Info("Stats downloaded from Steam");
}

void SteamStatsSystem::ResetStat(const std::string& id) {
    auto it = m_statValues.find(id);
    if (it == m_statValues.end()) return;
    
    const StatDef* def = GetStatDef(id);
    if (def) {
        it->second.intValue = static_cast<int>(def->defaultValue);
        it->second.floatValue = def->defaultValue;
        it->second.dirty = true;
        m_pendingUpload = true;
    }
}

void SteamStatsSystem::ResetAllStats() {
    for (auto& [id, value] : m_statValues) {
        const StatDef* def = GetStatDef(id);
        if (def) {
            value.intValue = static_cast<int>(def->defaultValue);
            value.floatValue = def->defaultValue;
            value.dirty = true;
        }
    }
    m_pendingUpload = true;
    
    // In production:
    // SteamUserStats()->ResetAllStats(false);  // false = don't reset achievements
    
    Logger::Info("All stats reset");
}

PlayTimeMilestone SteamStatsSystem::GetPlayTimeMilestone() const {
    PlayTimeMilestone milestone;
    milestone.hoursPlayed = GetTotalPlayTime() / 3600.0f;
    milestone.cardsDropped = m_totalCardsDropped;
    milestone.maxDrops = m_maxCardDrops;
    
    if (m_totalCardsDropped < m_maxCardDrops) {
        float hoursForNextDrop = (m_totalCardsDropped + 1) * m_hoursPerDrop;
        milestone.nextDropAt = std::max(0.0f, hoursForNextDrop - milestone.hoursPlayed);
    } else {
        milestone.nextDropAt = -1.0f;  // No more drops
    }
    
    return milestone;
}

float SteamStatsSystem::GetHoursUntilNextDrop() const {
    return GetPlayTimeMilestone().nextDropAt;
}

std::vector<TradingCardDrop> SteamStatsSystem::GetRecentDrops() const {
    return m_recentDrops;
}

TradingCardDrop SteamStatsSystem::GetLastDrop() const {
    if (m_recentDrops.empty()) {
        TradingCardDrop empty;
        empty.cardId = "";
        return empty;
    }
    return m_recentDrops.back();
}

void SteamStatsSystem::AcknowledgeDrop() {
    m_pendingDrop = false;
}

std::vector<TradingCardSet> SteamStatsSystem::GetCardSets() const {
    return m_cardSets;
}

TradingCardSet SteamStatsSystem::GetCardSet(const std::string& setId) const {
    for (const auto& set : m_cardSets) {
        if (set.setId == setId) return set;
    }
    TradingCardSet empty;
    empty.setId = "";
    return empty;
}

int SteamStatsSystem::GetTotalCardsOwned() const {
    int total = 0;
    for (const auto& set : m_cardSets) {
        total += set.ownedCards;
    }
    return total;
}

bool SteamStatsSystem::HasCompletedSet(const std::string& setId) const {
    for (const auto& set : m_cardSets) {
        if (set.setId == setId) return set.complete;
    }
    return false;
}

std::vector<Badge> SteamStatsSystem::GetBadges() const {
    return m_badges;
}

Badge SteamStatsSystem::GetBadge(const std::string& badgeId) const {
    for (const auto& badge : m_badges) {
        if (badge.badgeId == badgeId) return badge;
    }
    Badge empty;
    empty.badgeId = "";
    return empty;
}

int SteamStatsSystem::GetTotalBadgeXP() const {
    int total = 0;
    for (const auto& badge : m_badges) {
        total += badge.xpEarned;
    }
    return total;
}

// ===========================================================================
// GAME EVENT HANDLERS
// ===========================================================================

void SteamStatsSystem::OnGameStart() {
    IncrementStat("games_played", 1);
}

void SteamStatsSystem::OnGameEnd(bool won, float playTimeSeconds) {
    if (won) {
        IncrementStat("games_won", 1);
    }
    IncrementStatFloat("total_play_time", playTimeSeconds);
}

void SteamStatsSystem::OnWaveComplete(int wave, bool perfect) {
    IncrementStat("waves_completed", 1);
    
    int highest = GetStatInt("highest_wave");
    if (wave > highest) {
        SetStatInt("highest_wave", wave);
    }
    
    if (perfect) {
        IncrementStat("perfect_waves", 1);
    }
}

void SteamStatsSystem::OnEnemyKilled(const std::string& enemyType) {
    IncrementStat("enemies_killed", 1);
}

void SteamStatsSystem::OnBossKilled(const std::string& bossId) {
    IncrementStat("bosses_killed", 1);
    
    // Track specific bosses
    if (bossId == "ancient_dragon") IncrementStat("dragons_killed", 1);
    else if (bossId == "necrolord") IncrementStat("necrolords_killed", 1);
    else if (bossId == "titan") IncrementStat("titans_killed", 1);
    else if (bossId == "shadow_king") IncrementStat("shadow_kings_killed", 1);
}

void SteamStatsSystem::OnUnitSummoned(const std::string& unitClass) {
    IncrementStat("units_summoned", 1);
}

void SteamStatsSystem::OnAbilityUsed(const std::string& abilityId) {
    IncrementStat("abilities_used", 1);
}

void SteamStatsSystem::OnItemCollected(const std::string& itemId) {
    IncrementStat("items_collected", 1);
}

void SteamStatsSystem::OnGoldEarned(int amount) {
    IncrementStat("gold_earned", amount);
}

void SteamStatsSystem::OnGoldSpent(int amount) {
    IncrementStat("gold_spent", amount);
}

void SteamStatsSystem::OnDamageDealt(int amount, bool critical) {
    IncrementStat("damage_dealt", amount);
    if (critical) {
        IncrementStat("critical_hits", 1);
    }
}

void SteamStatsSystem::OnDamageTaken(int amount) {
    IncrementStat("damage_taken", amount);
}

void SteamStatsSystem::OnUnitLost() {
    IncrementStat("units_lost", 1);
}

void SteamStatsSystem::OnDiceRolled(const std::string& diceType, int result) {
    IncrementStat("dice_rolled", 1);
    
    // Track high rolls
    if (result == 6) {
        IncrementStat("perfect_rolls", 1);
    }
}

void SteamStatsSystem::StartSession() {
    m_sessionActive = true;
    m_sessionPlayTime = 0.0f;
    m_totalPlayTimeAtStart = GetStatFloat("total_play_time");
    
    Logger::Info("Session started");
}

void SteamStatsSystem::EndSession() {
    if (!m_sessionActive) return;
    
    m_sessionActive = false;
    
    // Update total play time
    IncrementStatFloat("total_play_time", m_sessionPlayTime);
    
    // Upload final stats
    UploadStats();
    
    Logger::Info("Session ended (duration: {:.1f}s)", m_sessionPlayTime);
}

float SteamStatsSystem::GetTotalPlayTime() const {
    float total = GetStatFloat("total_play_time");
    if (m_sessionActive) {
        total += m_sessionPlayTime;
    }
    return total;
}

// ===========================================================================
// PRIVATE METHODS
// ===========================================================================

void SteamStatsSystem::RegisterAllStats() {
    // Game stats
    RegisterStat({"games_played", "STAT_GAMES_PLAYED", "Games Played", 
                  StatType::Int, 0, 0, 999999, true, "{0}"});
    RegisterStat({"games_won", "STAT_GAMES_WON", "Games Won",
                  StatType::Int, 0, 0, 999999, true, "{0}"});
    RegisterStat({"total_play_time", "STAT_PLAY_TIME", "Total Play Time",
                  StatType::Float, 0, 0, 999999999, true, "{0} hours"});
    
    // Wave stats
    RegisterStat({"waves_completed", "STAT_WAVES", "Waves Completed",
                  StatType::Int, 0, 0, 999999, true, "{0}"});
    RegisterStat({"highest_wave", "STAT_HIGHEST_WAVE", "Highest Wave",
                  StatType::Int, 0, 0, 999, true, "{0}"});
    RegisterStat({"perfect_waves", "STAT_PERFECT_WAVES", "Perfect Waves",
                  StatType::Int, 0, 0, 999999, true, "{0}"});
    
    // Combat stats
    RegisterStat({"enemies_killed", "STAT_ENEMIES", "Enemies Killed",
                  StatType::Int, 0, 0, 99999999, true, "{0}"});
    RegisterStat({"bosses_killed", "STAT_BOSSES", "Bosses Killed",
                  StatType::Int, 0, 0, 99999, true, "{0}"});
    RegisterStat({"damage_dealt", "STAT_DAMAGE_DEALT", "Damage Dealt",
                  StatType::Int, 0, 0, 999999999, true, "{0}"});
    RegisterStat({"damage_taken", "STAT_DAMAGE_TAKEN", "Damage Taken",
                  StatType::Int, 0, 0, 999999999, true, "{0}"});
    RegisterStat({"critical_hits", "STAT_CRITS", "Critical Hits",
                  StatType::Int, 0, 0, 9999999, true, "{0}"});
    
    // Boss-specific stats
    RegisterStat({"dragons_killed", "STAT_DRAGONS", "Dragons Slain",
                  StatType::Int, 0, 0, 9999, true, "{0}"});
    RegisterStat({"necrolords_killed", "STAT_NECROS", "Necrolords Vanquished",
                  StatType::Int, 0, 0, 9999, true, "{0}"});
    RegisterStat({"titans_killed", "STAT_TITANS", "Titans Felled",
                  StatType::Int, 0, 0, 9999, true, "{0}"});
    RegisterStat({"shadow_kings_killed", "STAT_SHADOWS", "Shadow Kings Defeated",
                  StatType::Int, 0, 0, 9999, true, "{0}"});
    
    // Unit stats
    RegisterStat({"units_summoned", "STAT_UNITS", "Units Summoned",
                  StatType::Int, 0, 0, 9999999, true, "{0}"});
    RegisterStat({"units_lost", "STAT_UNITS_LOST", "Units Lost",
                  StatType::Int, 0, 0, 9999999, true, "{0}"});
    RegisterStat({"abilities_used", "STAT_ABILITIES", "Abilities Used",
                  StatType::Int, 0, 0, 9999999, true, "{0}"});
    
    // Economy stats
    RegisterStat({"gold_earned", "STAT_GOLD_EARNED", "Gold Earned",
                  StatType::Int, 0, 0, 999999999, true, "{0}"});
    RegisterStat({"gold_spent", "STAT_GOLD_SPENT", "Gold Spent",
                  StatType::Int, 0, 0, 999999999, true, "{0}"});
    RegisterStat({"items_collected", "STAT_ITEMS", "Items Collected",
                  StatType::Int, 0, 0, 999999, true, "{0}"});
    
    // Dice stats
    RegisterStat({"dice_rolled", "STAT_DICE", "Dice Rolled",
                  StatType::Int, 0, 0, 9999999, true, "{0}"});
    RegisterStat({"perfect_rolls", "STAT_PERFECT_ROLLS", "Perfect Rolls (6)",
                  StatType::Int, 0, 0, 9999999, true, "{0}"});
    
    // Average stats
    RegisterStat({"avg_score", "STAT_AVG_SCORE", "Average Score",
                  StatType::AvgRate, 0, 0, 999999, false, "{0}"});
    RegisterStat({"avg_wave", "STAT_AVG_WAVE", "Average Wave Reached",
                  StatType::AvgRate, 0, 0, 100, false, "{0:.1f}"});
}

void SteamStatsSystem::CheckCardDrop() {
    if (!m_tradingCardsEnabled) return;
    if (m_totalCardsDropped >= m_maxCardDrops) return;
    
    float totalHours = GetTotalPlayTime() / 3600.0f;
    float hoursNeeded = (m_totalCardsDropped + 1) * m_hoursPerDrop;
    
    if (totalHours >= hoursNeeded) {
        SimulateCardDrop();
    }
}

void SteamStatsSystem::SimulateCardDrop() {
    // In production, Steam handles card drops
    // This simulates the behavior
    
    m_totalCardsDropped++;
    
    TradingCardDrop drop;
    drop.dropNumber = m_totalCardsDropped;
    drop.timestamp = static_cast<uint64_t>(std::time(nullptr));
    drop.foil = (rand() % 100) < 2;  // 2% foil chance
    
    // Random card from set
    if (!m_cardSets.empty() && !m_cardSets[0].cardIds.empty()) {
        int cardIndex = rand() % m_cardSets[0].cardIds.size();
        drop.cardId = m_cardSets[0].cardIds[cardIndex];
        drop.cardName = "Card " + std::to_string(cardIndex + 1);
        drop.setName = m_cardSets[0].setName;
    }
    
    m_recentDrops.push_back(drop);
    m_pendingDrop = true;
    
    Logger::Info("Trading card dropped: {} (foil: {})", drop.cardName, drop.foil);
    
    if (m_cardDropCallback) {
        m_cardDropCallback(drop);
    }
}

} // namespace DDD
