#include "UnitDatabase.h"
#include "../Utils/Logger.h"

namespace DDD {

UnitDatabase& UnitDatabase::Instance() {
    static UnitDatabase instance;
    return instance;
}

void UnitDatabase::Initialize() {
    if (m_initialized) return;
    
    RegisterTier1Classes();
    RegisterTier2Classes();
    RegisterTier3Classes();
    
    m_initialized = true;
    Logger::Info("UnitDatabase initialized with {} classes", m_classes.size());
}

void UnitDatabase::RegisterClass(const UnitClassDef& def) {
    m_classes[def.className] = def;
}

const UnitClassDef* UnitDatabase::GetClassDef(const std::string& className) const {
    auto it = m_classes.find(className);
    return it != m_classes.end() ? &it->second : nullptr;
}

std::vector<std::string> UnitDatabase::GetClassesByTier(int tier) const {
    std::vector<std::string> result;
    for (const auto& [name, def] : m_classes) {
        if (def.tier == tier) {
            result.push_back(name);
        }
    }
    return result;
}

std::vector<std::string> UnitDatabase::GetClassesByArchetype(UnitArchetype archetype) const {
    std::vector<std::string> result;
    for (const auto& [name, def] : m_classes) {
        if (def.archetype == archetype) {
            result.push_back(name);
        }
    }
    return result;
}

std::shared_ptr<Unit> UnitDatabase::CreateUnit(const std::string& className) const {
    const UnitClassDef* def = GetClassDef(className);
    if (!def) {
        Logger::Warning("Unknown unit class: {}", className);
        return nullptr;
    }
    
    auto unit = std::make_shared<Unit>();
    unit->SetClassName(def->className);
    unit->SetTier(def->tier);
    
    UnitStats stats;
    stats.hp = def->hp;
    stats.maxHp = def->hp;
    stats.atk = def->atk;
    stats.def = def->def;
    stats.mov = def->mov;
    stats.rng = def->rng;
    unit->SetStats(stats);
    
    unit->SetUnfurlPattern(def->unfurlPattern);
    unit->SetPromotionOptions(def->promotionOptions);
    unit->SetExpToPromote(def->expToPromote);
    
    return unit;
}

SDL_Color UnitDatabase::GetArchetypeColor(UnitArchetype archetype) {
    switch (archetype) {
        case UnitArchetype::Mage:    return {80, 100, 200, 255};
        case UnitArchetype::Soldier: return {200, 80, 80, 255};
        case UnitArchetype::Rogue:   return {80, 180, 100, 255};
        case UnitArchetype::Healer:  return {220, 200, 100, 255};
        case UnitArchetype::Tank:    return {120, 120, 140, 255};
        case UnitArchetype::Archer:  return {160, 80, 200, 255};
        default: return {200, 200, 200, 255};
    }
}

SDL_Color UnitDatabase::GetArchetypeColorLight(UnitArchetype archetype) {
    switch (archetype) {
        case UnitArchetype::Mage:    return {120, 140, 255, 255};
        case UnitArchetype::Soldier: return {255, 120, 120, 255};
        case UnitArchetype::Rogue:   return {120, 220, 140, 255};
        case UnitArchetype::Healer:  return {255, 240, 150, 255};
        case UnitArchetype::Tank:    return {160, 160, 180, 255};
        case UnitArchetype::Archer:  return {200, 120, 255, 255};
        default: return {240, 240, 240, 255};
    }
}

// ===========================================================================
// TIER 1 CLASSES (Starting units)
// ===========================================================================

void UnitDatabase::RegisterTier1Classes() {
    // MAGE - Magic damage dealer
    RegisterClass({
        "Mage",
        UnitArchetype::Mage,
        1,
        25, 8, 2, 2, 2,  // HP, ATK, DEF, MOV, RNG
        {1, 1, 1, 1},    // Unfurl: cross pattern
        {"Wizard", "Warlock"},
        "Fireball",
        50,
        "Ranged magic attacker. Can hit enemies from 2 tiles away."
    });
    
    // SOLDIER - Melee fighter
    RegisterClass({
        "Soldier",
        UnitArchetype::Soldier,
        1,
        35, 6, 4, 2, 1,
        {1, 1, 1, 1},
        {"Knight", "Berserker"},
        "",
        50,
        "Balanced melee fighter with solid defense."
    });
    
    // ROGUE - Fast striker
    RegisterClass({
        "Rogue",
        UnitArchetype::Rogue,
        1,
        22, 7, 2, 3, 1,
        {2, 0, 0, 0},    // Unfurl: forward only
        {"Assassin", "Ninja"},
        "Backstab",
        50,
        "Fast and deadly. Can move 3 tiles per turn."
    });
    
    // CLERIC - Healer
    RegisterClass({
        "Cleric",
        UnitArchetype::Healer,
        1,
        28, 3, 3, 2, 1,
        {1, 1, 1, 1},
        {"Priest", "Paladin"},
        "Heal",
        50,
        "Support unit that can heal allies."
    });
    
    // GUARD - Defender
    RegisterClass({
        "Guard",
        UnitArchetype::Tank,
        1,
        45, 4, 6, 1, 1,
        {0, 1, 0, 1},    // Unfurl: sides only
        {"Sentinel", "Champion"},
        "Taunt",
        50,
        "Slow but sturdy. High HP and defense."
    });
    
    // SCOUT - Ranged
    RegisterClass({
        "Scout",
        UnitArchetype::Archer,
        1,
        20, 6, 1, 3, 3,
        {2, 0, 0, 0},
        {"Sniper", "Ranger"},
        "",
        50,
        "Long-range attacker with excellent mobility."
    });
}

// ===========================================================================
// TIER 2 CLASSES (First promotion)
// ===========================================================================

void UnitDatabase::RegisterTier2Classes() {
    // MAGE promotions
    RegisterClass({
        "Wizard",
        UnitArchetype::Mage,
        2,
        35, 12, 3, 2, 3,
        {2, 1, 1, 1},
        {"Archmage"},
        "Chain Lightning",
        100,
        "Powerful mage with extended range and AoE potential."
    });
    
    RegisterClass({
        "Warlock",
        UnitArchetype::Mage,
        2,
        40, 10, 4, 2, 2,
        {1, 2, 1, 2},
        {"Lich"},
        "Life Drain",
        100,
        "Dark mage that heals when dealing damage."
    });
    
    // SOLDIER promotions
    RegisterClass({
        "Knight",
        UnitArchetype::Soldier,
        2,
        50, 8, 7, 2, 1,
        {1, 2, 1, 2},
        {"Lord"},
        "Shield Bash",
        100,
        "Heavily armored warrior with a stunning attack."
    });
    
    RegisterClass({
        "Berserker",
        UnitArchetype::Soldier,
        2,
        45, 12, 3, 3, 1,
        {2, 2, 0, 0},
        {"Warlord"},
        "Rampage",
        100,
        "Ferocious fighter that gets stronger when hurt."
    });
    
    // ROGUE promotions
    RegisterClass({
        "Assassin",
        UnitArchetype::Rogue,
        2,
        28, 14, 2, 4, 1,
        {3, 0, 0, 0},
        {"Phantom"},
        "Execute",
        100,
        "Master of critical strikes. Deadly vs low-health targets."
    });
    
    RegisterClass({
        "Ninja",
        UnitArchetype::Rogue,
        2,
        30, 10, 3, 4, 2,
        {2, 1, 0, 1},
        {"Shadow"},
        "Smoke Bomb",
        100,
        "Can attack from range and evade counterattacks."
    });
    
    // CLERIC promotions
    RegisterClass({
        "Priest",
        UnitArchetype::Healer,
        2,
        35, 5, 4, 2, 2,
        {1, 2, 1, 2},
        {"High Priest"},
        "Mass Heal",
        100,
        "Powerful healer that can heal multiple allies."
    });
    
    RegisterClass({
        "Paladin",
        UnitArchetype::Healer,
        2,
        45, 8, 6, 2, 1,
        {2, 1, 2, 1},
        {"Holy Knight"},
        "Divine Shield",
        100,
        "Holy warrior combining healing and combat."
    });
    
    // GUARD promotions
    RegisterClass({
        "Sentinel",
        UnitArchetype::Tank,
        2,
        65, 5, 9, 1, 1,
        {0, 2, 0, 2},
        {"Fortress"},
        "Fortify",
        100,
        "Immovable wall. Highest defense in the game."
    });
    
    RegisterClass({
        "Champion",
        UnitArchetype::Tank,
        2,
        55, 9, 7, 2, 1,
        {1, 1, 1, 1},
        {"Titan"},
        "Inspire",
        100,
        "Tank that boosts nearby allies."
    });
    
    // SCOUT promotions
    RegisterClass({
        "Sniper",
        UnitArchetype::Archer,
        2,
        25, 12, 1, 2, 5,
        {3, 0, 0, 0},
        {"Deadeye"},
        "Headshot",
        100,
        "Extreme range. Deals bonus damage to full-health enemies."
    });
    
    RegisterClass({
        "Ranger",
        UnitArchetype::Archer,
        2,
        35, 8, 3, 4, 3,
        {2, 1, 0, 1},
        {"Warden"},
        "Multishot",
        100,
        "Mobile archer that can attack multiple enemies."
    });
}

// ===========================================================================
// TIER 3 CLASSES (Final promotion)
// ===========================================================================

void UnitDatabase::RegisterTier3Classes() {
    // MAGE final
    RegisterClass({
        "Archmage",
        UnitArchetype::Mage,
        3,
        45, 18, 4, 2, 4,
        {2, 2, 2, 2},
        {},
        "Meteor",
        0,
        "Master of arcane arts. Devastating AoE damage."
    });
    
    RegisterClass({
        "Lich",
        UnitArchetype::Mage,
        3,
        55, 14, 5, 2, 3,
        {2, 2, 2, 2},
        {},
        "Soul Harvest",
        0,
        "Undying mage that revives upon death once."
    });
    
    // SOLDIER final
    RegisterClass({
        "Lord",
        UnitArchetype::Soldier,
        3,
        70, 12, 10, 2, 1,
        {2, 2, 2, 2},
        {},
        "Royal Guard",
        0,
        "Legendary knight. Protects adjacent allies from damage."
    });
    
    RegisterClass({
        "Warlord",
        UnitArchetype::Soldier,
        3,
        60, 18, 5, 3, 1,
        {3, 2, 0, 2},
        {},
        "Fury",
        0,
        "Unstoppable warrior. Extra attack when killing."
    });
    
    // ROGUE final
    RegisterClass({
        "Phantom",
        UnitArchetype::Rogue,
        3,
        35, 20, 3, 5, 1,
        {4, 0, 0, 0},
        {},
        "Death Mark",
        0,
        "Silent killer. Guaranteed critical on marked targets."
    });
    
    RegisterClass({
        "Shadow",
        UnitArchetype::Rogue,
        3,
        40, 16, 4, 5, 2,
        {3, 1, 0, 1},
        {},
        "Shadowstep",
        0,
        "Can teleport behind enemies before attacking."
    });
    
    // CLERIC final
    RegisterClass({
        "High Priest",
        UnitArchetype::Healer,
        3,
        50, 8, 5, 2, 3,
        {2, 2, 2, 2},
        {},
        "Resurrection",
        0,
        "Can bring fallen allies back to battle."
    });
    
    RegisterClass({
        "Holy Knight",
        UnitArchetype::Healer,
        3,
        60, 12, 8, 2, 1,
        {2, 2, 2, 2},
        {},
        "Judgment",
        0,
        "Holy warrior dealing bonus damage to enemies."
    });
    
    // GUARD final
    RegisterClass({
        "Fortress",
        UnitArchetype::Tank,
        3,
        90, 6, 12, 1, 1,
        {1, 2, 1, 2},
        {},
        "Unbreakable",
        0,
        "Living fortress. Reduces all damage taken by 50%."
    });
    
    RegisterClass({
        "Titan",
        UnitArchetype::Tank,
        3,
        75, 14, 9, 2, 1,
        {2, 2, 2, 2},
        {},
        "Earthquake",
        0,
        "Massive warrior with AoE ground slam."
    });
    
    // SCOUT final
    RegisterClass({
        "Deadeye",
        UnitArchetype::Archer,
        3,
        30, 22, 2, 2, 6,
        {4, 0, 0, 0},
        {},
        "Perfect Shot",
        0,
        "Master marksman. Ignores defense completely."
    });
    
    RegisterClass({
        "Warden",
        UnitArchetype::Archer,
        3,
        50, 14, 5, 4, 4,
        {3, 2, 0, 2},
        {},
        "Arrow Storm",
        0,
        "Forest guardian with devastating volley attacks."
    });
}

} // namespace DDD
