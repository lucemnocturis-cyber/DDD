#include "Dice.h"
#include "Unit.h"
#include "../Utils/Random.h"
#include "../Utils/Logger.h"

namespace DDD {

Dice::Dice() {
    m_faces.resize(6);  // Standard 6-sided die
}

Dice::~Dice() = default;

void Dice::Initialize(const std::string& className, int tier, int cost,
                      const std::vector<DiceFace>& faces,
                      const std::vector<std::string>& promotionOptions) {
    m_className = className;
    m_tier = tier;
    m_cost = cost;
    m_faces = faces;
    m_promotionOptions = promotionOptions;
    
    // Ensure we have exactly 6 faces
    if (m_faces.size() < 6) {
        m_faces.resize(6, m_faces.empty() ? DiceFace{} : m_faces[0]);
    }
    
    Roll();  // Initial roll
}

void Dice::Roll() {
    m_currentFaceIndex = Random::GetInt(0, 5);
    Logger::Info("{} rolled face {} (HP:{} ATK:{} DEF:{} MOVE:{} RANGE:{})",
                 m_className, m_currentFaceIndex + 1,
                 m_faces[m_currentFaceIndex].hp,
                 m_faces[m_currentFaceIndex].atk,
                 m_faces[m_currentFaceIndex].def,
                 m_faces[m_currentFaceIndex].move,
                 m_faces[m_currentFaceIndex].range);
}

std::shared_ptr<Unit> Dice::CreateUnit() const {
    auto unit = std::make_shared<Unit>();
    
    const DiceFace& face = GetCurrentFace();
    
    unit->SetClassName(m_className);
    unit->SetTier(m_tier);
    unit->SetExpToPromote(100 + m_tier * 50);  // Scale exp requirement with tier
    unit->SetPromotionOptions(m_promotionOptions);
    unit->SetUnfurlPattern(face.unfurl);
    
    UnitStats stats;
    stats.hp = face.hp;
    stats.maxHp = face.hp;
    stats.atk = face.atk;
    stats.def = face.def;
    stats.mov = face.move;  // DiceFace uses 'move', UnitStats uses 'mov'
    stats.rng = face.range; // DiceFace uses 'range', UnitStats uses 'rng'
    unit->SetStats(stats);
    
    // TODO: Add abilities based on face.abilities
    
    return unit;
}

// ============================================================================
// DiceFactory - Static class data (from the HTML prototype)
// ============================================================================

namespace ClassData {

// Base classes (Tier 0)
const DiceFace MAGE_FACES[6] = {
    {15, 8, 2, 2, 4, {2, 2, 2, 2}, {}},
    {12, 10, 1, 3, 4, {3, 1, 3, 1}, {}},
    {18, 6, 3, 2, 3, {1, 3, 1, 3}, {}},
    {10, 12, 1, 3, 5, {4, 0, 4, 0}, {}},
    {14, 9, 2, 2, 4, {2, 3, 2, 1}, {}},
    {16, 7, 3, 2, 3, {3, 2, 1, 2}, {}}
};

const DiceFace SOLDIER_FACES[6] = {
    {25, 6, 5, 3, 1, {2, 2, 2, 2}, {}},
    {22, 7, 4, 3, 1, {3, 2, 2, 2}, {}},
    {28, 5, 6, 2, 1, {2, 3, 2, 2}, {}},
    {24, 6, 5, 3, 1, {2, 2, 3, 2}, {}},
    {26, 5, 5, 3, 1, {2, 2, 2, 3}, {}},
    {30, 4, 7, 2, 1, {3, 3, 2, 2}, {}}
};

const DiceFace ROGUE_FACES[6] = {
    {18, 10, 2, 4, 1, {1, 2, 1, 2}, {}},
    {16, 11, 1, 5, 1, {1, 1, 2, 2}, {}},
    {20, 9, 3, 4, 1, {2, 2, 2, 2}, {}},
    {17, 10, 2, 5, 1, {1, 2, 1, 3}, {}},
    {19, 10, 2, 4, 1, {1, 2, 2, 2}, {}},
    {15, 12, 1, 5, 1, {1, 1, 1, 2}, {}}
};

// Tier 1 - Mage promotions
const DiceFace ELEMENTALIST_FACES[6] = {
    {20, 10, 3, 2, 5, {3, 2, 2, 2}, {"Fireball"}},
    {18, 11, 2, 3, 5, {3, 2, 3, 2}, {"Ice Shard"}},
    {22, 9, 4, 2, 4, {2, 3, 2, 3}, {"Lightning"}},
    {19, 11, 3, 3, 5, {4, 1, 4, 1}, {"Fireball"}},
    {21, 10, 3, 2, 5, {3, 3, 2, 2}, {"Ice Shard"}},
    {17, 12, 2, 3, 5, {3, 2, 2, 3}, {"Lightning"}}
};

// Tier 1 - Soldier promotions
const DiceFace KNIGHT_FACES[6] = {
    {32, 8, 7, 2, 1, {3, 3, 3, 3}, {"Shield Bash"}},
    {30, 9, 6, 2, 1, {3, 3, 3, 2}, {"Taunt"}},
    {35, 7, 8, 2, 1, {4, 3, 3, 3}, {"Defensive Stance"}},
    {31, 8, 7, 2, 1, {3, 4, 3, 3}, {"Shield Bash"}},
    {33, 7, 7, 2, 1, {3, 3, 4, 3}, {"Taunt"}},
    {28, 10, 6, 3, 1, {3, 3, 3, 4}, {"Defensive Stance"}}
};

// Tier 1 - Rogue promotions
const DiceFace ASSASSIN_FACES[6] = {
    {22, 14, 2, 5, 1, {1, 2, 1, 2}, {"Backstab"}},
    {20, 15, 1, 5, 1, {1, 1, 2, 2}, {"Poison"}},
    {24, 13, 3, 4, 1, {2, 2, 2, 2}, {"Backstab"}},
    {21, 14, 2, 5, 1, {1, 2, 1, 3}, {"Poison"}},
    {23, 13, 2, 5, 1, {1, 2, 2, 2}, {"Backstab"}},
    {19, 16, 1, 5, 1, {1, 1, 1, 2}, {"Poison"}}
};

} // namespace ClassData

std::shared_ptr<Dice> DiceFactory::CreateDice(const std::string& className) {
    auto dice = std::make_shared<Dice>();
    std::vector<DiceFace> faces(6);
    std::vector<std::string> promotions;
    int tier = 0;
    int cost = 1;
    
    // Base classes (Tier 0)
    if (className == "Mage") {
        for (int i = 0; i < 6; ++i) faces[i] = ClassData::MAGE_FACES[i];
        promotions = {"Elementalist", "Warlock", "Sage"};
        tier = 0; cost = 1;
    }
    else if (className == "Soldier") {
        for (int i = 0; i < 6; ++i) faces[i] = ClassData::SOLDIER_FACES[i];
        promotions = {"Knight", "Paladin", "Berserker"};
        tier = 0; cost = 1;
    }
    else if (className == "Rogue") {
        for (int i = 0; i < 6; ++i) faces[i] = ClassData::ROGUE_FACES[i];
        promotions = {"Assassin", "Ranger", "Thief"};
        tier = 0; cost = 1;
    }
    // Tier 1 - Mage tree
    else if (className == "Elementalist") {
        for (int i = 0; i < 6; ++i) faces[i] = ClassData::ELEMENTALIST_FACES[i];
        promotions = {"Pyromancer", "Cryomancer", "Storm Caller"};
        tier = 1; cost = 2;
    }
    // Tier 1 - Soldier tree
    else if (className == "Knight") {
        for (int i = 0; i < 6; ++i) faces[i] = ClassData::KNIGHT_FACES[i];
        promotions = {"Champion", "Warden", "Dragoon"};
        tier = 1; cost = 2;
    }
    // Tier 1 - Rogue tree
    else if (className == "Assassin") {
        for (int i = 0; i < 6; ++i) faces[i] = ClassData::ASSASSIN_FACES[i];
        promotions = {"Nightblade", "Shadowdancer", "Executioner"};
        tier = 1; cost = 2;
    }
    else {
        Logger::Error("Unknown class: {}", className);
        return nullptr;
    }
    
    dice->Initialize(className, tier, cost, faces, promotions);
    return dice;
}

std::vector<std::string> DiceFactory::GetBaseClasses() {
    return {"Mage", "Soldier", "Rogue"};
}

std::vector<std::string> DiceFactory::GetPromotionOptions(const std::string& className) {
    if (className == "Mage") return {"Elementalist", "Warlock", "Sage"};
    if (className == "Soldier") return {"Knight", "Paladin", "Berserker"};
    if (className == "Rogue") return {"Assassin", "Ranger", "Thief"};
    
    // Add more promotion paths as needed
    return {};
}

} // namespace DDD
