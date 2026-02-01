#include "EnemyDatabase.h"
#include "Unit.h"
#include "../Utils/Logger.h"
#include "../Utils/Random.h"

#include <algorithm>

namespace DDD {

EnemyDatabase& EnemyDatabase::Instance() {
    static EnemyDatabase instance;
    return instance;
}

void EnemyDatabase::Initialize() {
    if (m_initialized) return;
    
    RegisterMinionEnemies();
    RegisterRegularEnemies();
    RegisterEliteEnemies();
    RegisterChampionEnemies();
    RegisterBossEnemies();
    RegisterWaveTemplates();
    
    m_initialized = true;
    Logger::Info("EnemyDatabase initialized with {} enemies, {} wave templates", 
                 m_enemies.size(), m_waveTemplates.size());
}

void EnemyDatabase::RegisterEnemy(const EnemyDef& def) {
    m_enemies[def.id] = def;
}

void EnemyDatabase::RegisterWaveTemplate(const WaveTemplate& tmpl) {
    m_waveTemplates[tmpl.id] = tmpl;
}

const EnemyDef* EnemyDatabase::GetEnemyDef(const std::string& id) const {
    auto it = m_enemies.find(id);
    return it != m_enemies.end() ? &it->second : nullptr;
}

std::vector<std::string> EnemyDatabase::GetEnemiesByTier(EnemyTier tier) const {
    std::vector<std::string> result;
    for (const auto& [id, def] : m_enemies) {
        if (def.tier == tier) {
            result.push_back(id);
        }
    }
    return result;
}

const WaveTemplate* EnemyDatabase::GetWaveTemplate(const std::string& id) const {
    auto it = m_waveTemplates.find(id);
    return it != m_waveTemplates.end() ? &it->second : nullptr;
}

const WaveTemplate* EnemyDatabase::GetTemplateForWave(int waveNumber) const {
    // Find suitable templates
    std::vector<const WaveTemplate*> suitable;
    
    for (const auto& [id, tmpl] : m_waveTemplates) {
        if (waveNumber >= tmpl.minWave && 
            (tmpl.maxWave == 0 || waveNumber <= tmpl.maxWave)) {
            suitable.push_back(&tmpl);
        }
    }
    
    if (suitable.empty()) {
        // Fallback to any template
        if (!m_waveTemplates.empty()) {
            return &m_waveTemplates.begin()->second;
        }
        return nullptr;
    }
    
    // Pick random suitable template
    return suitable[Random::Range(0, static_cast<int>(suitable.size()) - 1)];
}

std::shared_ptr<Unit> EnemyDatabase::CreateEnemy(const std::string& id) const {
    const EnemyDef* def = GetEnemyDef(id);
    if (!def) {
        Logger::Warning("Unknown enemy ID: {}", id);
        return nullptr;
    }
    
    auto unit = std::make_shared<Unit>();
    unit->SetClassName(def->name);
    unit->SetOwner(Owner::Enemy);
    
    // Set tier based on enemy tier
    int tier = 1;
    if (def->tier == EnemyTier::Elite) tier = 2;
    else if (def->tier == EnemyTier::Champion || def->tier == EnemyTier::Boss) tier = 3;
    unit->SetTier(tier);
    
    UnitStats stats;
    stats.hp = def->hp;
    stats.maxHp = def->hp;
    stats.atk = def->atk;
    stats.def = def->def;
    stats.mov = def->mov;
    stats.rng = def->rng;
    unit->SetStats(stats);
    
    unit->SetUnfurlPattern(def->unfurlPattern);
    
    return unit;
}

std::vector<std::shared_ptr<Unit>> EnemyDatabase::GenerateWaveEnemies(int waveNumber) const {
    std::vector<std::shared_ptr<Unit>> enemies;
    
    const WaveTemplate* tmpl = GetTemplateForWave(waveNumber);
    if (!tmpl) {
        Logger::Warning("No wave template found for wave {}", waveNumber);
        return enemies;
    }
    
    // Determine enemy count (scales slightly with wave)
    int baseCount = Random::Range(tmpl->minEnemies, tmpl->maxEnemies);
    int bonusCount = waveNumber / 5;  // +1 enemy every 5 waves
    int totalCount = std::min(baseCount + bonusCount, 10);  // Cap at 10
    
    // Build weighted enemy pool
    std::vector<std::string> pool;
    for (const auto& [enemyId, weight] : tmpl->enemyWeights) {
        for (int i = 0; i < weight; ++i) {
            pool.push_back(enemyId);
        }
    }
    
    if (pool.empty()) {
        Logger::Warning("Empty enemy pool for wave {}", waveNumber);
        return enemies;
    }
    
    // Spawn regular enemies
    for (int i = 0; i < totalCount; ++i) {
        std::string enemyId = pool[Random::Range(0, static_cast<int>(pool.size()) - 1)];
        
        // Check for elite upgrade
        if (Random::Range(0, 100) < tmpl->eliteChance) {
            // Try to find elite version
            std::string eliteId = enemyId + "_elite";
            if (GetEnemyDef(eliteId)) {
                enemyId = eliteId;
            }
        }
        
        // Check for champion upgrade
        if (Random::Range(0, 100) < tmpl->championChance) {
            std::string champId = enemyId + "_champion";
            if (GetEnemyDef(champId)) {
                enemyId = champId;
            }
        }
        
        auto enemy = CreateEnemy(enemyId);
        if (enemy) {
            enemies.push_back(enemy);
        }
    }
    
    // Boss wave handling
    if (tmpl->hasBoss && !tmpl->bossId.empty()) {
        auto boss = CreateEnemy(tmpl->bossId);
        if (boss) {
            enemies.push_back(boss);
        }
    }
    
    // Scale stats based on wave number
    float statMultiplier = 1.0f + (waveNumber - 1) * 0.1f;  // +10% per wave
    for (auto& enemy : enemies) {
        auto& stats = enemy->GetMutableStats();
        stats.hp = static_cast<int>(stats.hp * statMultiplier);
        stats.maxHp = stats.hp;
        stats.atk = static_cast<int>(stats.atk * (1.0f + (waveNumber - 1) * 0.05f));
    }
    
    return enemies;
}

SDL_Color EnemyDatabase::GetTierColor(EnemyTier tier) {
    switch (tier) {
        case EnemyTier::Minion:   return {150, 150, 150, 255};  // Gray
        case EnemyTier::Regular:  return {200, 100, 100, 255};  // Red
        case EnemyTier::Elite:    return {200, 150, 50, 255};   // Orange
        case EnemyTier::Champion: return {180, 80, 200, 255};   // Purple
        case EnemyTier::Boss:     return {255, 50, 50, 255};    // Bright red
        default: return {200, 200, 200, 255};
    }
}

std::string EnemyDatabase::GetTierName(EnemyTier tier) {
    switch (tier) {
        case EnemyTier::Minion:   return "Minion";
        case EnemyTier::Regular:  return "Regular";
        case EnemyTier::Elite:    return "Elite";
        case EnemyTier::Champion: return "Champion";
        case EnemyTier::Boss:     return "Boss";
        default: return "Unknown";
    }
}

// ===========================================================================
// MINION ENEMIES (Weak fodder)
// ===========================================================================

void EnemyDatabase::RegisterMinionEnemies() {
    RegisterEnemy({
        "goblin", "Goblin", "Weak but numerous",
        UnitArchetype::Rogue, EnemyTier::Minion, EnemyBehavior::Aggressive,
        12, 3, 1, 2, 1,
        {1, 0, 0, 0},
        "",
        5, 2,
        {100, 150, 100, 255}
    });
    
    RegisterEnemy({
        "skeleton", "Skeleton", "Brittle undead warrior",
        UnitArchetype::Soldier, EnemyTier::Minion, EnemyBehavior::Aggressive,
        15, 4, 1, 2, 1,
        {1, 1, 0, 0},
        "",
        5, 2,
        {200, 200, 180, 255}
    });
    
    RegisterEnemy({
        "imp", "Imp", "Tiny fire demon",
        UnitArchetype::Mage, EnemyTier::Minion, EnemyBehavior::Flanker,
        10, 4, 0, 2, 2,
        {1, 0, 0, 0},
        "Firebolt",
        6, 3,
        {200, 100, 100, 255}
    });
    
    RegisterEnemy({
        "slime", "Slime", "Slow but resilient blob",
        UnitArchetype::Tank, EnemyTier::Minion, EnemyBehavior::Defensive,
        20, 2, 2, 1, 1,
        {0, 1, 0, 1},
        "",
        4, 2,
        {100, 200, 100, 255}
    });
}

// ===========================================================================
// REGULAR ENEMIES (Standard threats)
// ===========================================================================

void EnemyDatabase::RegisterRegularEnemies() {
    RegisterEnemy({
        "orc_warrior", "Orc Warrior", "Brutal melee fighter",
        UnitArchetype::Soldier, EnemyTier::Regular, EnemyBehavior::Aggressive,
        30, 7, 3, 2, 1,
        {1, 1, 1, 1},
        "",
        15, 5,
        {100, 150, 100, 255}
    });
    
    RegisterEnemy({
        "dark_mage", "Dark Mage", "Corrupted spellcaster",
        UnitArchetype::Mage, EnemyTier::Regular, EnemyBehavior::Defensive,
        22, 9, 2, 2, 2,
        {1, 1, 1, 1},
        "Shadow Bolt",
        18, 6,
        {100, 80, 150, 255}
    });
    
    RegisterEnemy({
        "bandit", "Bandit", "Quick and deadly outlaw",
        UnitArchetype::Rogue, EnemyTier::Regular, EnemyBehavior::Flanker,
        20, 8, 2, 3, 1,
        {2, 0, 0, 0},
        "Backstab",
        16, 8,
        {150, 120, 100, 255}
    });
    
    RegisterEnemy({
        "cultist", "Cultist", "Dark magic devotee",
        UnitArchetype::Healer, EnemyTier::Regular, EnemyBehavior::Supporter,
        25, 4, 2, 2, 2,
        {1, 1, 1, 1},
        "Dark Heal",
        14, 5,
        {80, 50, 100, 255}
    });
    
    RegisterEnemy({
        "armored_skeleton", "Armored Skeleton", "Heavily armored undead",
        UnitArchetype::Tank, EnemyTier::Regular, EnemyBehavior::Defensive,
        35, 5, 5, 1, 1,
        {0, 1, 0, 1},
        "",
        12, 4,
        {180, 180, 160, 255}
    });
    
    RegisterEnemy({
        "goblin_archer", "Goblin Archer", "Annoying ranged pest",
        UnitArchetype::Archer, EnemyTier::Regular, EnemyBehavior::Defensive,
        18, 6, 1, 2, 3,
        {2, 0, 0, 0},
        "",
        12, 5,
        {120, 160, 100, 255}
    });
}

// ===========================================================================
// ELITE ENEMIES (Stronger variants)
// ===========================================================================

void EnemyDatabase::RegisterEliteEnemies() {
    RegisterEnemy({
        "orc_berserker", "Orc Berserker", "Raging orc elite",
        UnitArchetype::Soldier, EnemyTier::Elite, EnemyBehavior::Berserker,
        40, 10, 2, 3, 1,
        {2, 1, 0, 1},
        "Rampage",
        30, 12,
        {150, 100, 100, 255}
    });
    
    RegisterEnemy({
        "necromancer", "Necromancer", "Master of undeath",
        UnitArchetype::Mage, EnemyTier::Elite, EnemyBehavior::Supporter,
        30, 11, 3, 2, 3,
        {1, 2, 1, 2},
        "Raise Dead",
        35, 15,
        {60, 40, 80, 255}
    });
    
    RegisterEnemy({
        "shadow_assassin", "Shadow Assassin", "Deadly killer from darkness",
        UnitArchetype::Rogue, EnemyTier::Elite, EnemyBehavior::Assassin,
        28, 14, 2, 4, 1,
        {3, 0, 0, 0},
        "Execute",
        32, 14,
        {50, 50, 80, 255}
    });
    
    RegisterEnemy({
        "dark_priest", "Dark Priest", "Heals and curses",
        UnitArchetype::Healer, EnemyTier::Elite, EnemyBehavior::Supporter,
        35, 6, 4, 2, 2,
        {1, 2, 1, 2},
        "Mass Heal",
        28, 10,
        {100, 50, 120, 255}
    });
    
    RegisterEnemy({
        "golem", "Stone Golem", "Animated stone guardian",
        UnitArchetype::Tank, EnemyTier::Elite, EnemyBehavior::Defensive,
        55, 8, 8, 1, 1,
        {1, 1, 1, 1},
        "Fortify",
        25, 10,
        {140, 140, 150, 255}
    });
    
    RegisterEnemy({
        "dark_ranger", "Dark Ranger", "Undead marksman",
        UnitArchetype::Archer, EnemyTier::Elite, EnemyBehavior::Assassin,
        25, 12, 2, 2, 4,
        {2, 1, 0, 1},
        "Poison Arrow",
        30, 12,
        {80, 100, 80, 255}
    });
}

// ===========================================================================
// CHAMPION ENEMIES (Mini-boss level)
// ===========================================================================

void EnemyDatabase::RegisterChampionEnemies() {
    RegisterEnemy({
        "orc_warchief", "Orc Warchief", "Leader of the warband",
        UnitArchetype::Soldier, EnemyTier::Champion, EnemyBehavior::Aggressive,
        60, 14, 6, 2, 1,
        {2, 2, 1, 2},
        "War Cry",
        60, 25,
        {120, 80, 80, 255}
    });
    
    RegisterEnemy({
        "lich", "Lich", "Undead sorcerer supreme",
        UnitArchetype::Mage, EnemyTier::Champion, EnemyBehavior::Defensive,
        45, 16, 4, 2, 3,
        {2, 2, 2, 2},
        "Soul Harvest",
        70, 30,
        {80, 80, 140, 255}
    });
    
    RegisterEnemy({
        "phantom_lord", "Phantom Lord", "Ethereal master assassin",
        UnitArchetype::Rogue, EnemyTier::Champion, EnemyBehavior::Assassin,
        40, 18, 3, 5, 1,
        {3, 1, 0, 1},
        "Death Mark",
        65, 28,
        {100, 100, 150, 255}
    });
    
    RegisterEnemy({
        "demon_lord", "Demon Lord", "Powerful demon commander",
        UnitArchetype::Tank, EnemyTier::Champion, EnemyBehavior::Berserker,
        80, 12, 7, 2, 1,
        {2, 2, 2, 2},
        "Hellfire",
        75, 35,
        {180, 60, 60, 255}
    });
}

// ===========================================================================
// BOSS ENEMIES
// ===========================================================================

void EnemyDatabase::RegisterBossEnemies() {
    RegisterEnemy({
        "boss_dragon", "Ancient Dragon", "Legendary wyrm of destruction",
        UnitArchetype::Mage, EnemyTier::Boss, EnemyBehavior::Aggressive,
        150, 20, 8, 2, 3,
        {3, 3, 3, 3},
        "Dragon Breath",
        200, 100,
        {200, 50, 50, 255}
    });
    
    RegisterEnemy({
        "boss_necrolord", "The Necrolord", "Master of all undead",
        UnitArchetype::Mage, EnemyTier::Boss, EnemyBehavior::Supporter,
        120, 18, 6, 1, 4,
        {2, 3, 2, 3},
        "Army of the Dead",
        180, 90,
        {50, 30, 80, 255}
    });
    
    RegisterEnemy({
        "boss_titan", "Titan", "Ancient giant of immense power",
        UnitArchetype::Tank, EnemyTier::Boss, EnemyBehavior::Berserker,
        200, 15, 10, 1, 1,
        {2, 2, 2, 2},
        "Earthquake",
        220, 110,
        {150, 140, 120, 255}
    });
    
    RegisterEnemy({
        "boss_shadow_king", "Shadow King", "Ruler of the dark realm",
        UnitArchetype::Rogue, EnemyTier::Boss, EnemyBehavior::Assassin,
        100, 25, 5, 4, 2,
        {4, 2, 0, 2},
        "Shadow Realm",
        190, 95,
        {40, 40, 60, 255}
    });
}

// ===========================================================================
// WAVE TEMPLATES
// ===========================================================================

void EnemyDatabase::RegisterWaveTemplates() {
    // Early game (waves 1-3)
    RegisterWaveTemplate({
        "early_goblin", "Goblin Raid",
        1, 3,
        {{"goblin", 5}, {"skeleton", 3}, {"slime", 2}},
        2, 4,
        5, 0,
        false, ""
    });
    
    // Early-mid game (waves 2-5)
    RegisterWaveTemplate({
        "undead_swarm", "Undead Swarm",
        2, 5,
        {{"skeleton", 4}, {"imp", 3}, {"goblin", 2}},
        3, 5,
        10, 0,
        false, ""
    });
    
    // Mid game (waves 4-7)
    RegisterWaveTemplate({
        "orc_warband", "Orc Warband",
        4, 7,
        {{"orc_warrior", 4}, {"goblin", 3}, {"goblin_archer", 2}},
        4, 6,
        15, 5,
        false, ""
    });
    
    RegisterWaveTemplate({
        "dark_coven", "Dark Coven",
        4, 7,
        {{"dark_mage", 3}, {"cultist", 3}, {"bandit", 2}},
        3, 5,
        15, 5,
        false, ""
    });
    
    // Mid-late game (waves 6-9)
    RegisterWaveTemplate({
        "elite_assault", "Elite Assault",
        6, 9,
        {{"orc_berserker", 2}, {"shadow_assassin", 2}, {"dark_ranger", 2}, {"golem", 1}},
        4, 6,
        25, 10,
        false, ""
    });
    
    // Late game (waves 8+)
    RegisterWaveTemplate({
        "champion_challenge", "Champion's Challenge",
        8, 0,
        {{"orc_warchief", 1}, {"lich", 1}, {"necromancer", 2}, {"golem", 2}},
        4, 6,
        30, 20,
        false, ""
    });
    
    // Boss waves
    RegisterWaveTemplate({
        "boss_wave_5", "Dragon's Lair",
        5, 5,
        {{"orc_warrior", 2}, {"dark_mage", 2}},
        3, 4,
        0, 0,
        true, "boss_dragon"
    });
    
    RegisterWaveTemplate({
        "boss_wave_10", "The Necrolord's Domain",
        10, 10,
        {{"necromancer", 2}, {"armored_skeleton", 3}},
        4, 5,
        0, 0,
        true, "boss_necrolord"
    });
    
    RegisterWaveTemplate({
        "boss_wave_15", "Titan's Awakening",
        15, 15,
        {{"golem", 2}, {"demon_lord", 1}},
        3, 4,
        0, 0,
        true, "boss_titan"
    });
    
    RegisterWaveTemplate({
        "boss_wave_20", "Shadow King's Throne",
        20, 20,
        {{"phantom_lord", 1}, {"shadow_assassin", 2}, {"dark_priest", 2}},
        4, 5,
        0, 0,
        true, "boss_shadow_king"
    });
}

} // namespace DDD
