#include "AchievementSystem.h"
#include "SteamManager.h"
#include "../Utils/Logger.h"

#include <algorithm>
#include <ctime>

namespace DDD {

AchievementSystem& AchievementSystem::Instance() {
    static AchievementSystem instance;
    return instance;
}

void AchievementSystem::Initialize() {
    if (m_initialized) return;
    
    RegisterAllAchievements();
    RegisterAllStats();
    LoadFromSteam();
    
    m_initialized = true;
    Logger::Info("AchievementSystem initialized with {} achievements, {} stats",
                 m_achievements.size(), m_stats.size());
}

void AchievementSystem::Shutdown() {
    SaveToSteam();
    m_achievements.clear();
    m_stats.clear();
    m_initialized = false;
}

void AchievementSystem::Update(float deltaTime) {
    // Update notification
    if (m_currentNotification.showing) {
        m_currentNotification.elapsed += deltaTime;
        if (m_currentNotification.elapsed >= m_currentNotification.displayTime) {
            m_currentNotification.showing = false;
        }
    }
    
    // Show next notification if available
    if (!m_currentNotification.showing && !m_notificationQueue.empty()) {
        m_currentNotification = m_notificationQueue.front();
        m_notificationQueue.erase(m_notificationQueue.begin());
        m_currentNotification.showing = true;
        m_currentNotification.elapsed = 0.0f;
    }
}

void AchievementSystem::RegisterAchievement(const Achievement& achievement) {
    m_achievements[achievement.id] = achievement;
}

const Achievement* AchievementSystem::GetAchievement(const std::string& id) const {
    auto it = m_achievements.find(id);
    return it != m_achievements.end() ? &it->second : nullptr;
}

std::vector<Achievement> AchievementSystem::GetAllAchievements() const {
    std::vector<Achievement> result;
    for (const auto& [id, ach] : m_achievements) {
        result.push_back(ach);
    }
    return result;
}

std::vector<Achievement> AchievementSystem::GetAchievementsByCategory(AchievementCategory category) const {
    std::vector<Achievement> result;
    for (const auto& [id, ach] : m_achievements) {
        if (ach.category == category) {
            result.push_back(ach);
        }
    }
    return result;
}

std::vector<Achievement> AchievementSystem::GetUnlockedAchievements() const {
    std::vector<Achievement> result;
    for (const auto& [id, ach] : m_achievements) {
        if (ach.unlocked) {
            result.push_back(ach);
        }
    }
    return result;
}

std::vector<Achievement> AchievementSystem::GetLockedAchievements() const {
    std::vector<Achievement> result;
    for (const auto& [id, ach] : m_achievements) {
        if (!ach.unlocked) {
            result.push_back(ach);
        }
    }
    return result;
}

bool AchievementSystem::UnlockAchievement(const std::string& id) {
    auto it = m_achievements.find(id);
    if (it == m_achievements.end()) return false;
    
    if (it->second.unlocked) return false;  // Already unlocked
    
    it->second.unlocked = true;
    it->second.unlockTime = static_cast<uint64_t>(std::time(nullptr));
    
    // Add XP reward
    m_totalXPEarned += it->second.xpReward;
    
    // Show notification
    ShowNotification(it->second);
    
    // Callback
    if (m_unlockedCallback) {
        m_unlockedCallback(it->second);
    }
    
    // Sync with Steam
    // In production: SteamUserStats()->SetAchievement(it->second.apiName.c_str());
    
    Logger::Info("Achievement unlocked: {} - {}", it->second.name, it->second.description);
    
    return true;
}

bool AchievementSystem::IsAchievementUnlocked(const std::string& id) const {
    auto it = m_achievements.find(id);
    return it != m_achievements.end() && it->second.unlocked;
}

void AchievementSystem::SetAchievementProgress(const std::string& id, int progress) {
    auto it = m_achievements.find(id);
    if (it == m_achievements.end() || !it->second.hasProgress) return;
    
    it->second.progressCurrent = std::min(progress, it->second.progressTarget);
    
    // Check for unlock
    if (it->second.progressCurrent >= it->second.progressTarget) {
        UnlockAchievement(id);
    }
}

void AchievementSystem::IncrementAchievementProgress(const std::string& id, int amount) {
    auto it = m_achievements.find(id);
    if (it == m_achievements.end() || !it->second.hasProgress) return;
    
    SetAchievementProgress(id, it->second.progressCurrent + amount);
}

void AchievementSystem::RegisterStat(const AchievementStat& stat) {
    m_stats[stat.id] = stat;
}

void AchievementSystem::SetStat(const std::string& id, int value) {
    auto it = m_stats.find(id);
    if (it == m_stats.end()) return;
    
    it->second.value = value;
    if (it->second.maxValue > 0) {
        it->second.value = std::min(it->second.value, it->second.maxValue);
    }
    it->second.synced = false;
    
    CheckProgressAchievements();
}

void AchievementSystem::IncrementStat(const std::string& id, int amount) {
    auto it = m_stats.find(id);
    if (it != m_stats.end()) {
        SetStat(id, it->second.value + amount);
    }
}

int AchievementSystem::GetStat(const std::string& id) const {
    auto it = m_stats.find(id);
    return it != m_stats.end() ? it->second.value : 0;
}

// ===========================================================================
// GAME EVENT HANDLERS
// ===========================================================================

void AchievementSystem::OnGameStart() {
    IncrementStat("games_started", 1);
}

void AchievementSystem::OnWaveComplete(int waveNumber) {
    IncrementStat("waves_completed", 1);
    
    if (waveNumber == 5) UnlockAchievement("wave_5");
    if (waveNumber == 10) UnlockAchievement("wave_10");
    if (waveNumber == 15) UnlockAchievement("wave_15");
    if (waveNumber == 20) UnlockAchievement("wave_20");
}

void AchievementSystem::OnBossDefeated(const std::string& bossId) {
    IncrementStat("bosses_defeated", 1);
    
    if (bossId == "ancient_dragon") UnlockAchievement("defeat_dragon");
    if (bossId == "necrolord") UnlockAchievement("defeat_necrolord");
    if (bossId == "titan") UnlockAchievement("defeat_titan");
    if (bossId == "shadow_king") UnlockAchievement("defeat_shadow_king");
    
    // All bosses
    if (IsAchievementUnlocked("defeat_dragon") &&
        IsAchievementUnlocked("defeat_necrolord") &&
        IsAchievementUnlocked("defeat_titan") &&
        IsAchievementUnlocked("defeat_shadow_king")) {
        UnlockAchievement("boss_slayer");
    }
}

void AchievementSystem::OnEnemyKilled(const std::string& enemyType) {
    IncrementStat("enemies_killed", 1);
    IncrementAchievementProgress("kill_100", 1);
    IncrementAchievementProgress("kill_1000", 1);
    IncrementAchievementProgress("kill_10000", 1);
}

void AchievementSystem::OnUnitSummoned(const std::string& unitClass) {
    IncrementStat("units_summoned", 1);
    
    // Track unique classes
    std::string statId = "summoned_" + unitClass;
    SetStat(statId, 1);
    
    // Check all classes summoned
    int classCount = 0;
    std::vector<std::string> classes = {"mage", "soldier", "rogue", "healer", "tank", "archer"};
    for (const auto& c : classes) {
        if (GetStat("summoned_" + c) > 0) classCount++;
    }
    if (classCount >= 6) UnlockAchievement("all_classes");
}

void AchievementSystem::OnDiceRolled(const std::string& diceType) {
    IncrementStat("dice_rolled", 1);
    IncrementAchievementProgress("roll_1000", 1);
}

void AchievementSystem::OnItemCollected(const std::string& itemType) {
    IncrementStat("items_collected", 1);
    IncrementAchievementProgress("collector", 1);
}

void AchievementSystem::OnGoldEarned(int amount) {
    IncrementStat("gold_earned", amount);
    IncrementAchievementProgress("gold_10000", amount);
    IncrementAchievementProgress("gold_100000", amount);
}

void AchievementSystem::OnDamageDealt(int amount) {
    IncrementStat("damage_dealt", amount);
    IncrementAchievementProgress("damage_100000", amount);
}

void AchievementSystem::OnDamageTaken(int amount) {
    IncrementStat("damage_taken", amount);
}

void AchievementSystem::OnCriticalHit() {
    IncrementStat("critical_hits", 1);
    IncrementAchievementProgress("crit_100", 1);
    IncrementAchievementProgress("crit_1000", 1);
}

void AchievementSystem::OnPerfectWave() {
    IncrementStat("perfect_waves", 1);
    UnlockAchievement("perfect_wave");
    
    if (GetStat("perfect_waves") >= 5) UnlockAchievement("flawless_5");
    if (GetStat("perfect_waves") >= 10) UnlockAchievement("flawless_10");
}

void AchievementSystem::OnUnitDeath(bool isAlly) {
    if (isAlly) {
        IncrementStat("allies_lost", 1);
    }
}

void AchievementSystem::OnGameWon(const std::string& difficulty, const std::string& gameMode) {
    IncrementStat("games_won", 1);
    UnlockAchievement("first_victory");
    
    if (difficulty == "easy") UnlockAchievement("win_easy");
    if (difficulty == "normal") UnlockAchievement("win_normal");
    if (difficulty == "hard") UnlockAchievement("win_hard");
    if (difficulty == "nightmare") UnlockAchievement("win_nightmare");
    
    if (gameMode == "endless") {
        IncrementStat("endless_wins", 1);
    }
    
    if (GetStat("games_won") >= 10) UnlockAchievement("veteran");
    if (GetStat("games_won") >= 50) UnlockAchievement("champion");
    if (GetStat("games_won") >= 100) UnlockAchievement("legend");
}

void AchievementSystem::OnGameLost() {
    IncrementStat("games_lost", 1);
    
    // Persistence achievement
    if (GetStat("games_lost") >= 10) UnlockAchievement("persistent");
}

void AchievementSystem::OnSpeedrunComplete(float timeSeconds) {
    if (timeSeconds < 1800.0f) UnlockAchievement("speedrun_30");  // 30 minutes
    if (timeSeconds < 1200.0f) UnlockAchievement("speedrun_20");  // 20 minutes
    if (timeSeconds < 900.0f) UnlockAchievement("speedrun_15");   // 15 minutes
}

// ===========================================================================
// QUERIES
// ===========================================================================

int AchievementSystem::GetUnlockedCount() const {
    int count = 0;
    for (const auto& [id, ach] : m_achievements) {
        if (ach.unlocked) count++;
    }
    return count;
}

int AchievementSystem::GetTotalCount() const {
    return static_cast<int>(m_achievements.size());
}

float AchievementSystem::GetCompletionPercentage() const {
    if (m_achievements.empty()) return 0.0f;
    return (static_cast<float>(GetUnlockedCount()) / GetTotalCount()) * 100.0f;
}

int AchievementSystem::GetTotalXPEarned() const {
    return m_totalXPEarned;
}

bool AchievementSystem::HasPendingNotification() const {
    return m_currentNotification.showing || !m_notificationQueue.empty();
}

AchievementNotification AchievementSystem::GetCurrentNotification() const {
    return m_currentNotification;
}

void AchievementSystem::DismissNotification() {
    m_currentNotification.showing = false;
}

void AchievementSystem::SyncWithSteam() {
    if (!SteamManager::Instance().IsInitialized()) return;
    
    // In production:
    // SteamUserStats()->StoreStats();
    
    Logger::Info("Achievements synced with Steam");
}

void AchievementSystem::ResetAllAchievements() {
    for (auto& [id, ach] : m_achievements) {
        ach.unlocked = false;
        ach.unlockTime = 0;
        ach.progressCurrent = 0;
    }
    
    for (auto& [id, stat] : m_stats) {
        stat.value = 0;
    }
    
    m_totalXPEarned = 0;
    
    // In production:
    // SteamUserStats()->ResetAllStats(true);
    
    Logger::Info("All achievements reset");
}

// ===========================================================================
// PRIVATE METHODS
// ===========================================================================

void AchievementSystem::ShowNotification(const Achievement& achievement) {
    AchievementNotification notif;
    notif.achievementId = achievement.id;
    notif.name = achievement.name;
    notif.description = achievement.description;
    notif.displayTime = NOTIFICATION_DURATION;
    notif.elapsed = 0.0f;
    notif.showing = false;
    
    m_notificationQueue.push_back(notif);
}

void AchievementSystem::CheckProgressAchievements() {
    // Check all progress-based achievements
    for (auto& [id, ach] : m_achievements) {
        if (ach.hasProgress && !ach.unlocked) {
            if (ach.progressCurrent >= ach.progressTarget) {
                UnlockAchievement(id);
            }
        }
    }
}

void AchievementSystem::LoadFromSteam() {
    if (!SteamManager::Instance().IsInitialized()) return;
    
    // In production:
    // SteamUserStats()->RequestCurrentStats();
    // Then in callback, iterate achievements and load state
}

void AchievementSystem::SaveToSteam() {
    if (!SteamManager::Instance().IsInitialized()) return;
    
    // In production:
    // SteamUserStats()->StoreStats();
}

// ===========================================================================
// ACHIEVEMENT REGISTRATION
// ===========================================================================

void AchievementSystem::RegisterAllAchievements() {
    RegisterProgressionAchievements();
    RegisterCombatAchievements();
    RegisterCollectionAchievements();
    RegisterChallengeAchievements();
    RegisterSecretAchievements();
}

void AchievementSystem::RegisterProgressionAchievements() {
    // Wave progress
    RegisterAchievement({"wave_5", "ACH_WAVE_5", "Getting Started", 
        "Complete wave 5", "", "", "", AchievementCategory::Progression, 
        AchievementRarity::Common, false, false, 0, 0, "", false, 0, 10});
    
    RegisterAchievement({"wave_10", "ACH_WAVE_10", "Halfway There",
        "Complete wave 10", "", "", "", AchievementCategory::Progression,
        AchievementRarity::Common, false, false, 0, 0, "", false, 0, 25});
    
    RegisterAchievement({"wave_15", "ACH_WAVE_15", "Almost There",
        "Complete wave 15", "", "", "", AchievementCategory::Progression,
        AchievementRarity::Uncommon, false, false, 0, 0, "", false, 0, 50});
    
    RegisterAchievement({"wave_20", "ACH_WAVE_20", "Victory!",
        "Complete wave 20", "", "", "", AchievementCategory::Progression,
        AchievementRarity::Uncommon, false, false, 0, 0, "", false, 0, 100});
    
    // First victory
    RegisterAchievement({"first_victory", "ACH_FIRST_WIN", "First Blood",
        "Win your first game", "", "", "", AchievementCategory::Progression,
        AchievementRarity::Common, false, false, 0, 0, "", false, 0, 50});
    
    // Difficulty completions
    RegisterAchievement({"win_easy", "ACH_WIN_EASY", "Easy Peasy",
        "Win on Easy difficulty", "", "", "", AchievementCategory::Progression,
        AchievementRarity::Common, false, false, 0, 0, "", false, 0, 25});
    
    RegisterAchievement({"win_normal", "ACH_WIN_NORMAL", "Standard Bearer",
        "Win on Normal difficulty", "", "", "", AchievementCategory::Progression,
        AchievementRarity::Uncommon, false, false, 0, 0, "", false, 0, 50});
    
    RegisterAchievement({"win_hard", "ACH_WIN_HARD", "Hardened Veteran",
        "Win on Hard difficulty", "", "", "", AchievementCategory::Progression,
        AchievementRarity::Rare, false, false, 0, 0, "", false, 0, 100});
    
    RegisterAchievement({"win_nightmare", "ACH_WIN_NIGHTMARE", "Nightmare Slayer",
        "Win on Nightmare difficulty", "", "", "", AchievementCategory::Progression,
        AchievementRarity::VeryRare, false, false, 0, 0, "", false, 0, 200});
    
    // Veteran achievements
    RegisterAchievement({"veteran", "ACH_VETERAN", "Veteran",
        "Win 10 games", "", "", "", AchievementCategory::Progression,
        AchievementRarity::Uncommon, false, false, 0, 0, "", false, 0, 100});
    
    RegisterAchievement({"champion", "ACH_CHAMPION", "Champion",
        "Win 50 games", "", "", "", AchievementCategory::Progression,
        AchievementRarity::Rare, false, false, 0, 0, "", false, 0, 250});
    
    RegisterAchievement({"legend", "ACH_LEGEND", "Living Legend",
        "Win 100 games", "", "", "", AchievementCategory::Progression,
        AchievementRarity::UltraRare, false, false, 0, 0, "", false, 0, 500});
}

void AchievementSystem::RegisterCombatAchievements() {
    // Kill achievements
    Achievement kill100;
    kill100.id = "kill_100";
    kill100.apiName = "ACH_KILL_100";
    kill100.name = "Exterminator";
    kill100.description = "Defeat 100 enemies";
    kill100.category = AchievementCategory::Combat;
    kill100.rarity = AchievementRarity::Common;
    kill100.hasProgress = true;
    kill100.progressTarget = 100;
    kill100.progressFormat = "{0}/100 enemies";
    kill100.xpReward = 50;
    RegisterAchievement(kill100);
    
    Achievement kill1000;
    kill1000.id = "kill_1000";
    kill1000.apiName = "ACH_KILL_1000";
    kill1000.name = "Mass Extinction";
    kill1000.description = "Defeat 1,000 enemies";
    kill1000.category = AchievementCategory::Combat;
    kill1000.rarity = AchievementRarity::Uncommon;
    kill1000.hasProgress = true;
    kill1000.progressTarget = 1000;
    kill1000.xpReward = 150;
    RegisterAchievement(kill1000);
    
    Achievement kill10000;
    kill10000.id = "kill_10000";
    kill10000.apiName = "ACH_KILL_10000";
    kill10000.name = "Apocalypse";
    kill10000.description = "Defeat 10,000 enemies";
    kill10000.category = AchievementCategory::Combat;
    kill10000.rarity = AchievementRarity::VeryRare;
    kill10000.hasProgress = true;
    kill10000.progressTarget = 10000;
    kill10000.xpReward = 500;
    RegisterAchievement(kill10000);
    
    // Critical hits
    Achievement crit100;
    crit100.id = "crit_100";
    crit100.apiName = "ACH_CRIT_100";
    crit100.name = "Critical Thinker";
    crit100.description = "Land 100 critical hits";
    crit100.category = AchievementCategory::Combat;
    crit100.rarity = AchievementRarity::Common;
    crit100.hasProgress = true;
    crit100.progressTarget = 100;
    crit100.xpReward = 50;
    RegisterAchievement(crit100);
    
    Achievement crit1000;
    crit1000.id = "crit_1000";
    crit1000.apiName = "ACH_CRIT_1000";
    crit1000.name = "Precision Master";
    crit1000.description = "Land 1,000 critical hits";
    crit1000.category = AchievementCategory::Combat;
    crit1000.rarity = AchievementRarity::Rare;
    crit1000.hasProgress = true;
    crit1000.progressTarget = 1000;
    crit1000.xpReward = 200;
    RegisterAchievement(crit1000);
    
    // Damage dealt
    Achievement damage100k;
    damage100k.id = "damage_100000";
    damage100k.apiName = "ACH_DMG_100K";
    damage100k.name = "Devastator";
    damage100k.description = "Deal 100,000 total damage";
    damage100k.category = AchievementCategory::Combat;
    damage100k.rarity = AchievementRarity::Uncommon;
    damage100k.hasProgress = true;
    damage100k.progressTarget = 100000;
    damage100k.xpReward = 150;
    RegisterAchievement(damage100k);
    
    // Boss achievements
    RegisterAchievement({"defeat_dragon", "ACH_DRAGON", "Dragon Slayer",
        "Defeat the Ancient Dragon", "", "", "", AchievementCategory::Combat,
        AchievementRarity::Uncommon, false, false, 0, 0, "", false, 0, 100});
    
    RegisterAchievement({"defeat_necrolord", "ACH_NECRO", "Death to the Dead",
        "Defeat the Necrolord", "", "", "", AchievementCategory::Combat,
        AchievementRarity::Uncommon, false, false, 0, 0, "", false, 0, 100});
    
    RegisterAchievement({"defeat_titan", "ACH_TITAN", "Giant Killer",
        "Defeat the Titan", "", "", "", AchievementCategory::Combat,
        AchievementRarity::Uncommon, false, false, 0, 0, "", false, 0, 100});
    
    RegisterAchievement({"defeat_shadow_king", "ACH_SHADOW", "Light in Darkness",
        "Defeat the Shadow King", "", "", "", AchievementCategory::Combat,
        AchievementRarity::Uncommon, false, false, 0, 0, "", false, 0, 100});
    
    RegisterAchievement({"boss_slayer", "ACH_ALL_BOSSES", "Boss Slayer",
        "Defeat all bosses", "", "", "", AchievementCategory::Combat,
        AchievementRarity::Rare, false, false, 0, 0, "", false, 0, 250});
}

void AchievementSystem::RegisterCollectionAchievements() {
    // All classes
    RegisterAchievement({"all_classes", "ACH_ALL_CLASSES", "Diverse Army",
        "Summon units of all 6 classes", "", "", "", AchievementCategory::Collection,
        AchievementRarity::Uncommon, false, false, 0, 0, "", false, 0, 75});
    
    // Gold
    Achievement gold10k;
    gold10k.id = "gold_10000";
    gold10k.apiName = "ACH_GOLD_10K";
    gold10k.name = "Wealthy";
    gold10k.description = "Earn 10,000 gold total";
    gold10k.category = AchievementCategory::Collection;
    gold10k.rarity = AchievementRarity::Common;
    gold10k.hasProgress = true;
    gold10k.progressTarget = 10000;
    gold10k.xpReward = 50;
    RegisterAchievement(gold10k);
    
    Achievement gold100k;
    gold100k.id = "gold_100000";
    gold100k.apiName = "ACH_GOLD_100K";
    gold100k.name = "Dragon's Hoard";
    gold100k.description = "Earn 100,000 gold total";
    gold100k.category = AchievementCategory::Collection;
    gold100k.rarity = AchievementRarity::Rare;
    gold100k.hasProgress = true;
    gold100k.progressTarget = 100000;
    gold100k.xpReward = 200;
    RegisterAchievement(gold100k);
    
    // Dice
    Achievement roll1000;
    roll1000.id = "roll_1000";
    roll1000.apiName = "ACH_ROLL_1000";
    roll1000.name = "Dice Master";
    roll1000.description = "Roll dice 1,000 times";
    roll1000.category = AchievementCategory::Collection;
    roll1000.rarity = AchievementRarity::Uncommon;
    roll1000.hasProgress = true;
    roll1000.progressTarget = 1000;
    roll1000.xpReward = 100;
    RegisterAchievement(roll1000);
    
    // Items
    Achievement collector;
    collector.id = "collector";
    collector.apiName = "ACH_COLLECTOR";
    collector.name = "Collector";
    collector.description = "Collect 100 items";
    collector.category = AchievementCategory::Collection;
    collector.rarity = AchievementRarity::Uncommon;
    collector.hasProgress = true;
    collector.progressTarget = 100;
    collector.xpReward = 100;
    RegisterAchievement(collector);
}

void AchievementSystem::RegisterChallengeAchievements() {
    // Perfect wave
    RegisterAchievement({"perfect_wave", "ACH_PERFECT", "Untouchable",
        "Complete a wave without taking damage", "", "", "", AchievementCategory::Challenge,
        AchievementRarity::Rare, false, false, 0, 0, "", false, 0, 100});
    
    RegisterAchievement({"flawless_5", "ACH_FLAWLESS_5", "Flawless Fighter",
        "Complete 5 waves without damage", "", "", "", AchievementCategory::Challenge,
        AchievementRarity::VeryRare, false, false, 0, 0, "", false, 0, 200});
    
    RegisterAchievement({"flawless_10", "ACH_FLAWLESS_10", "Invincible",
        "Complete 10 waves without damage", "", "", "", AchievementCategory::Challenge,
        AchievementRarity::UltraRare, false, false, 0, 0, "", false, 0, 400});
    
    // Speedrun
    RegisterAchievement({"speedrun_30", "ACH_SPEED_30", "Quick Victory",
        "Win in under 30 minutes", "", "", "", AchievementCategory::Challenge,
        AchievementRarity::Rare, false, false, 0, 0, "", false, 0, 150});
    
    RegisterAchievement({"speedrun_20", "ACH_SPEED_20", "Speed Demon",
        "Win in under 20 minutes", "", "", "", AchievementCategory::Challenge,
        AchievementRarity::VeryRare, false, false, 0, 0, "", false, 0, 250});
    
    RegisterAchievement({"speedrun_15", "ACH_SPEED_15", "Lightning Fast",
        "Win in under 15 minutes", "", "", "", AchievementCategory::Challenge,
        AchievementRarity::UltraRare, false, false, 0, 0, "", false, 0, 500});
    
    // Persistence
    RegisterAchievement({"persistent", "ACH_PERSISTENT", "Persistence",
        "Lose 10 games but keep trying", "", "", "", AchievementCategory::Challenge,
        AchievementRarity::Common, false, false, 0, 0, "", false, 0, 25});
}

void AchievementSystem::RegisterSecretAchievements() {
    Achievement secret1;
    secret1.id = "secret_overkill";
    secret1.apiName = "ACH_SECRET_1";
    secret1.name = "Overkill";
    secret1.description = "Deal 500+ damage in a single hit";
    secret1.hiddenDescription = "???";
    secret1.category = AchievementCategory::Secret;
    secret1.rarity = AchievementRarity::VeryRare;
    secret1.isSecret = true;
    secret1.xpReward = 200;
    RegisterAchievement(secret1);
    
    Achievement secret2;
    secret2.id = "secret_pacifist";
    secret2.apiName = "ACH_SECRET_2";
    secret2.name = "Pacifist Wave";
    secret2.description = "Win a wave using only healers";
    secret2.hiddenDescription = "???";
    secret2.category = AchievementCategory::Secret;
    secret2.rarity = AchievementRarity::UltraRare;
    secret2.isSecret = true;
    secret2.xpReward = 300;
    RegisterAchievement(secret2);
    
    Achievement secret3;
    secret3.id = "secret_lucky";
    secret3.apiName = "ACH_SECRET_3";
    secret3.name = "Lucky Seven";
    secret3.description = "Roll 7 critical hits in a row";
    secret3.hiddenDescription = "???";
    secret3.category = AchievementCategory::Secret;
    secret3.rarity = AchievementRarity::UltraRare;
    secret3.isSecret = true;
    secret3.xpReward = 250;
    RegisterAchievement(secret3);
}

void AchievementSystem::RegisterAllStats() {
    RegisterStat({"games_started", "Games Started", 0, 0, true});
    RegisterStat({"games_won", "Games Won", 0, 0, true});
    RegisterStat({"games_lost", "Games Lost", 0, 0, true});
    RegisterStat({"waves_completed", "Waves Completed", 0, 0, true});
    RegisterStat({"bosses_defeated", "Bosses Defeated", 0, 0, true});
    RegisterStat({"enemies_killed", "Enemies Killed", 0, 0, true});
    RegisterStat({"units_summoned", "Units Summoned", 0, 0, true});
    RegisterStat({"dice_rolled", "Dice Rolled", 0, 0, true});
    RegisterStat({"items_collected", "Items Collected", 0, 0, true});
    RegisterStat({"gold_earned", "Gold Earned", 0, 0, true});
    RegisterStat({"damage_dealt", "Damage Dealt", 0, 0, true});
    RegisterStat({"damage_taken", "Damage Taken", 0, 0, true});
    RegisterStat({"critical_hits", "Critical Hits", 0, 0, true});
    RegisterStat({"perfect_waves", "Perfect Waves", 0, 0, true});
    RegisterStat({"allies_lost", "Allies Lost", 0, 0, true});
    RegisterStat({"endless_wins", "Endless Mode Wins", 0, 0, true});
    
    // Class tracking
    RegisterStat({"summoned_mage", "Mages Summoned", 0, 1, true});
    RegisterStat({"summoned_soldier", "Soldiers Summoned", 0, 1, true});
    RegisterStat({"summoned_rogue", "Rogues Summoned", 0, 1, true});
    RegisterStat({"summoned_healer", "Healers Summoned", 0, 1, true});
    RegisterStat({"summoned_tank", "Tanks Summoned", 0, 1, true});
    RegisterStat({"summoned_archer", "Archers Summoned", 0, 1, true});
}

} // namespace DDD
