#include "DiceDatabase.h"
#include "UnitDatabase.h"
#include "../Utils/Logger.h"
#include "../Utils/Random.h"

#include <algorithm>
#include <random>

namespace DDD {

DiceDatabase& DiceDatabase::Instance() {
    static DiceDatabase instance;
    return instance;
}

void DiceDatabase::Initialize() {
    if (m_initialized) return;
    
    RegisterMageDice();
    RegisterSoldierDice();
    RegisterRogueDice();
    RegisterHealerDice();
    RegisterTankDice();
    RegisterArcherDice();
    RegisterSpecialDice();
    
    m_initialized = true;
    Logger::Info("DiceDatabase initialized with {} dice", m_dice.size());
}

void DiceDatabase::RegisterDice(const DiceDef& def) {
    m_dice[def.id] = def;
}

const DiceDef* DiceDatabase::GetDiceDef(const std::string& id) const {
    auto it = m_dice.find(id);
    return it != m_dice.end() ? &it->second : nullptr;
}

std::vector<std::string> DiceDatabase::GetDiceByRarity(DiceRarity rarity) const {
    std::vector<std::string> result;
    for (const auto& [id, def] : m_dice) {
        if (def.rarity == rarity) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<std::string> DiceDatabase::GetDiceByArchetype(UnitArchetype archetype) const {
    std::vector<std::string> result;
    for (const auto& [id, def] : m_dice) {
        if (def.archetype == archetype) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<std::string> DiceDatabase::GetStarterDice() const {
    // Return 4 basic dice for starting the game
    return {"mage_common", "soldier_common", "rogue_common", "cleric_common"};
}

std::shared_ptr<Dice> DiceDatabase::CreateDice(const std::string& id) const {
    const DiceDef* def = GetDiceDef(id);
    if (!def) {
        Logger::Warning("Unknown dice ID: {}", id);
        return nullptr;
    }
    
    // Get base class stats from UnitDatabase
    auto& unitDb = UnitDatabase::Instance();
    const UnitClassDef* classDef = unitDb.GetClassDef(def->className);
    if (!classDef) {
        Logger::Warning("Unknown unit class for dice: {}", def->className);
        return nullptr;
    }
    
    auto dice = std::make_shared<Dice>();
    
    // Create faces with stat variations
    std::vector<DiceFace> faces;
    for (int i = 0; i < 6; ++i) {
        DiceFace face;
        
        // Base stats from class + dice bonuses + face variation
        int variation = (i % 3) - 1;  // -1, 0, +1 variation
        
        face.hp = classDef->hp + def->hpBonus + variation * 2;
        face.atk = classDef->atk + def->atkBonus + (i % 2 == 0 ? 1 : 0);
        face.def = classDef->def + def->defBonus + (i % 2 == 1 ? 1 : 0);
        face.move = classDef->mov + def->movBonus;
        face.range = classDef->rng + def->rngBonus;
        
        // Unfurl pattern with slight variations
        face.unfurl = classDef->unfurlPattern;
        if (i >= 4) {
            // Faces 5-6 have slightly better unfurl
            for (int j = 0; j < 4; ++j) {
                if (face.unfurl[j] > 0 && Random::Range(0, 100) < 30) {
                    face.unfurl[j]++;
                }
            }
        }
        
        // Abilities
        if (!classDef->abilityName.empty()) {
            face.abilities.push_back(classDef->abilityName);
        }
        
        faces.push_back(face);
    }
    
    dice->Initialize(def->className, classDef->tier, def->cost, faces, classDef->promotionOptions);
    
    return dice;
}

std::vector<std::string> DiceDatabase::GetShopDice(int waveNumber, int count) const {
    std::vector<std::string> pool;
    
    // Determine rarity weights based on wave
    int commonWeight = std::max(10, 50 - waveNumber * 3);
    int uncommonWeight = std::min(40, 20 + waveNumber * 2);
    int rareWeight = std::min(30, waveNumber * 2);
    int epicWeight = std::min(15, std::max(0, waveNumber - 5) * 2);
    int legendaryWeight = std::min(5, std::max(0, waveNumber - 10));
    
    // Build weighted pool
    auto commons = GetDiceByRarity(DiceRarity::Common);
    auto uncommons = GetDiceByRarity(DiceRarity::Uncommon);
    auto rares = GetDiceByRarity(DiceRarity::Rare);
    auto epics = GetDiceByRarity(DiceRarity::Epic);
    auto legendaries = GetDiceByRarity(DiceRarity::Legendary);
    
    for (int i = 0; i < commonWeight && !commons.empty(); ++i) {
        pool.push_back(commons[Random::Range(0, static_cast<int>(commons.size()) - 1)]);
    }
    for (int i = 0; i < uncommonWeight && !uncommons.empty(); ++i) {
        pool.push_back(uncommons[Random::Range(0, static_cast<int>(uncommons.size()) - 1)]);
    }
    for (int i = 0; i < rareWeight && !rares.empty(); ++i) {
        pool.push_back(rares[Random::Range(0, static_cast<int>(rares.size()) - 1)]);
    }
    for (int i = 0; i < epicWeight && !epics.empty(); ++i) {
        pool.push_back(epics[Random::Range(0, static_cast<int>(epics.size()) - 1)]);
    }
    for (int i = 0; i < legendaryWeight && !legendaries.empty(); ++i) {
        pool.push_back(legendaries[Random::Range(0, static_cast<int>(legendaries.size()) - 1)]);
    }
    
    // Shuffle and pick
    std::shuffle(pool.begin(), pool.end(), std::mt19937(std::random_device()()));
    
    std::vector<std::string> result;
    for (int i = 0; i < count && i < static_cast<int>(pool.size()); ++i) {
        // Avoid duplicates
        if (std::find(result.begin(), result.end(), pool[i]) == result.end()) {
            result.push_back(pool[i]);
        }
    }
    
    return result;
}

SDL_Color DiceDatabase::GetRarityColor(DiceRarity rarity) {
    switch (rarity) {
        case DiceRarity::Common:    return {180, 180, 180, 255};  // Gray
        case DiceRarity::Uncommon:  return {100, 200, 100, 255};  // Green
        case DiceRarity::Rare:      return {80, 140, 220, 255};   // Blue
        case DiceRarity::Epic:      return {180, 80, 220, 255};   // Purple
        case DiceRarity::Legendary: return {255, 180, 50, 255};   // Gold
        default: return {200, 200, 200, 255};
    }
}

std::string DiceDatabase::GetRarityName(DiceRarity rarity) {
    switch (rarity) {
        case DiceRarity::Common:    return "Common";
        case DiceRarity::Uncommon:  return "Uncommon";
        case DiceRarity::Rare:      return "Rare";
        case DiceRarity::Epic:      return "Epic";
        case DiceRarity::Legendary: return "Legendary";
        default: return "Unknown";
    }
}

// ===========================================================================
// MAGE DICE
// ===========================================================================

void DiceDatabase::RegisterMageDice() {
    // Common Mage
    RegisterDice({
        "mage_common", "Mage", UnitArchetype::Mage, DiceRarity::Common,
        2, 2, "", "Basic magic user with ranged attacks.",
        0, 0, 0, 0, 0
    });
    
    // Uncommon Mage
    RegisterDice({
        "mage_uncommon", "Mage", UnitArchetype::Mage, DiceRarity::Uncommon,
        3, 2, "", "Trained mage with improved magic power.",
        3, 1, 0, 0, 0
    });
    
    // Rare Wizard
    RegisterDice({
        "wizard_rare", "Wizard", UnitArchetype::Mage, DiceRarity::Rare,
        5, 3, "Spell Surge", "Powerful wizard with chain lightning.",
        5, 2, 1, 0, 0
    });
    
    // Epic Warlock
    RegisterDice({
        "warlock_epic", "Warlock", UnitArchetype::Mage, DiceRarity::Epic,
        7, 3, "Soul Link", "Dark mage that drains life from enemies.",
        8, 3, 2, 0, 0
    });
    
    // Legendary Archmage
    RegisterDice({
        "archmage_legendary", "Archmage", UnitArchetype::Mage, DiceRarity::Legendary,
        10, 4, "Meteor Storm", "Master of arcane destruction.",
        10, 5, 2, 0, 1
    });
}

// ===========================================================================
// SOLDIER DICE
// ===========================================================================

void DiceDatabase::RegisterSoldierDice() {
    // Common Soldier
    RegisterDice({
        "soldier_common", "Soldier", UnitArchetype::Soldier, DiceRarity::Common,
        2, 2, "", "Reliable melee fighter.",
        0, 0, 0, 0, 0
    });
    
    // Uncommon Soldier
    RegisterDice({
        "soldier_uncommon", "Soldier", UnitArchetype::Soldier, DiceRarity::Uncommon,
        3, 2, "", "Veteran soldier with better equipment.",
        5, 1, 1, 0, 0
    });
    
    // Rare Knight
    RegisterDice({
        "knight_rare", "Knight", UnitArchetype::Soldier, DiceRarity::Rare,
        5, 3, "Shield Wall", "Armored knight with stunning attacks.",
        8, 1, 2, 0, 0
    });
    
    // Epic Berserker
    RegisterDice({
        "berserker_epic", "Berserker", UnitArchetype::Soldier, DiceRarity::Epic,
        7, 3, "Blood Rage", "Ferocious warrior, stronger when wounded.",
        5, 4, 0, 1, 0
    });
    
    // Legendary Lord
    RegisterDice({
        "lord_legendary", "Lord", UnitArchetype::Soldier, DiceRarity::Legendary,
        10, 4, "Royal Guard", "Noble commander protecting allies.",
        15, 3, 3, 0, 0
    });
}

// ===========================================================================
// ROGUE DICE
// ===========================================================================

void DiceDatabase::RegisterRogueDice() {
    // Common Rogue
    RegisterDice({
        "rogue_common", "Rogue", UnitArchetype::Rogue, DiceRarity::Common,
        2, 2, "", "Quick and deadly striker.",
        0, 0, 0, 0, 0
    });
    
    // Uncommon Rogue
    RegisterDice({
        "rogue_uncommon", "Rogue", UnitArchetype::Rogue, DiceRarity::Uncommon,
        3, 2, "", "Skilled rogue with poisoned blades.",
        2, 2, 0, 0, 0
    });
    
    // Rare Assassin
    RegisterDice({
        "assassin_rare", "Assassin", UnitArchetype::Rogue, DiceRarity::Rare,
        5, 3, "Mark Target", "Master of critical strikes.",
        3, 4, 0, 1, 0
    });
    
    // Epic Ninja
    RegisterDice({
        "ninja_epic", "Ninja", UnitArchetype::Rogue, DiceRarity::Epic,
        7, 3, "Shadow Clone", "Elusive ninja with ranged attacks.",
        5, 3, 1, 1, 1
    });
    
    // Legendary Phantom
    RegisterDice({
        "phantom_legendary", "Phantom", UnitArchetype::Rogue, DiceRarity::Legendary,
        10, 4, "Death Mark", "Silent killer, guaranteed crits on marked.",
        5, 6, 1, 2, 0
    });
}

// ===========================================================================
// HEALER DICE
// ===========================================================================

void DiceDatabase::RegisterHealerDice() {
    // Common Cleric
    RegisterDice({
        "cleric_common", "Cleric", UnitArchetype::Healer, DiceRarity::Common,
        2, 2, "", "Supportive healer for your team.",
        0, 0, 0, 0, 0
    });
    
    // Uncommon Cleric
    RegisterDice({
        "cleric_uncommon", "Cleric", UnitArchetype::Healer, DiceRarity::Uncommon,
        3, 2, "", "Devoted cleric with stronger healing.",
        4, 1, 1, 0, 0
    });
    
    // Rare Priest
    RegisterDice({
        "priest_rare", "Priest", UnitArchetype::Healer, DiceRarity::Rare,
        5, 3, "Sanctuary", "Holy priest with area healing.",
        6, 2, 1, 0, 1
    });
    
    // Epic Paladin
    RegisterDice({
        "paladin_epic", "Paladin", UnitArchetype::Healer, DiceRarity::Epic,
        7, 3, "Divine Favor", "Holy warrior, heals and fights.",
        10, 3, 2, 0, 0
    });
    
    // Legendary High Priest
    RegisterDice({
        "highpriest_legendary", "High Priest", UnitArchetype::Healer, DiceRarity::Legendary,
        10, 4, "Resurrection", "Can revive fallen allies!",
        12, 3, 2, 0, 2
    });
}

// ===========================================================================
// TANK DICE
// ===========================================================================

void DiceDatabase::RegisterTankDice() {
    // Common Guard
    RegisterDice({
        "guard_common", "Guard", UnitArchetype::Tank, DiceRarity::Common,
        2, 2, "", "Sturdy defender with high HP.",
        0, 0, 0, 0, 0
    });
    
    // Uncommon Guard
    RegisterDice({
        "guard_uncommon", "Guard", UnitArchetype::Tank, DiceRarity::Uncommon,
        3, 2, "", "Veteran guard with heavy armor.",
        8, 0, 2, 0, 0
    });
    
    // Rare Sentinel
    RegisterDice({
        "sentinel_rare", "Sentinel", UnitArchetype::Tank, DiceRarity::Rare,
        5, 3, "Fortify", "Immovable sentinel, reduces damage.",
        12, 0, 3, 0, 0
    });
    
    // Epic Champion
    RegisterDice({
        "champion_epic", "Champion", UnitArchetype::Tank, DiceRarity::Epic,
        7, 3, "Rally", "Champion that buffs nearby allies.",
        10, 2, 2, 1, 0
    });
    
    // Legendary Fortress
    RegisterDice({
        "fortress_legendary", "Fortress", UnitArchetype::Tank, DiceRarity::Legendary,
        10, 4, "Unbreakable", "Living fortress, halves all damage!",
        20, 1, 4, 0, 0
    });
}

// ===========================================================================
// ARCHER DICE
// ===========================================================================

void DiceDatabase::RegisterArcherDice() {
    // Common Scout
    RegisterDice({
        "scout_common", "Scout", UnitArchetype::Archer, DiceRarity::Common,
        2, 2, "", "Fast ranged attacker.",
        0, 0, 0, 0, 0
    });
    
    // Uncommon Scout
    RegisterDice({
        "scout_uncommon", "Scout", UnitArchetype::Archer, DiceRarity::Uncommon,
        3, 2, "", "Trained scout with better accuracy.",
        2, 2, 0, 0, 0
    });
    
    // Rare Sniper
    RegisterDice({
        "sniper_rare", "Sniper", UnitArchetype::Archer, DiceRarity::Rare,
        5, 3, "Headshot", "Long-range sniper, bonus vs full HP.",
        0, 4, 0, 0, 1
    });
    
    // Epic Ranger
    RegisterDice({
        "ranger_epic", "Ranger", UnitArchetype::Archer, DiceRarity::Epic,
        7, 3, "Multishot", "Mobile archer with AoE attacks.",
        8, 2, 1, 1, 0
    });
    
    // Legendary Deadeye
    RegisterDice({
        "deadeye_legendary", "Deadeye", UnitArchetype::Archer, DiceRarity::Legendary,
        10, 4, "Perfect Shot", "Master marksman, ignores defense!",
        0, 8, 0, 0, 2
    });
}

// ===========================================================================
// SPECIAL DICE (Unique/Event dice)
// ===========================================================================

void DiceDatabase::RegisterSpecialDice() {
    // Hybrid dice - uncommon
    RegisterDice({
        "battlemage_uncommon", "Mage", UnitArchetype::Mage, DiceRarity::Uncommon,
        4, 2, "Arcane Blade", "Mage trained in close combat.",
        5, 2, 2, 0, -1  // -1 range = melee mage
    });
    
    RegisterDice({
        "shieldbearer_uncommon", "Guard", UnitArchetype::Tank, DiceRarity::Uncommon,
        4, 2, "Protect", "Guard that can shield adjacent allies.",
        5, 0, 3, 0, 0
    });
    
    // Wild dice - rare
    RegisterDice({
        "wildcard_rare", "Rogue", UnitArchetype::Rogue, DiceRarity::Rare,
        6, 3, "Chaos", "Random stat bonuses each face!",
        5, 3, 2, 1, 0
    });
}

} // namespace DDD
