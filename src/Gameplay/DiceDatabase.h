#pragma once

#include "Dice.h"
#include "UnitDatabase.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace DDD {

/**
 * Dice rarity levels
 */
enum class DiceRarity {
    Common,      // Basic dice, easily obtained
    Uncommon,    // Slightly better stats
    Rare,        // Good stats, special abilities
    Epic,        // Excellent stats, powerful abilities
    Legendary    // Best stats, unique abilities
};

/**
 * Dice definition for the database
 */
struct DiceDef {
    std::string id;                     // Unique identifier
    std::string className;              // Unit class it summons
    UnitArchetype archetype;            // For visual styling
    DiceRarity rarity;                  // Rarity level
    int cost;                           // Gold cost to place
    int cooldown;                       // Turns before reuse
    std::string specialEffect;          // Special effect name (empty if none)
    std::string description;            // Tooltip description
    
    // Stat modifiers (applied on top of base class stats)
    int hpBonus = 0;
    int atkBonus = 0;
    int defBonus = 0;
    int movBonus = 0;
    int rngBonus = 0;
};

/**
 * DiceDatabase - central repository of all dice definitions
 */
class DiceDatabase {
public:
    static DiceDatabase& Instance();
    
    void Initialize();
    
    /**
     * Get dice definition by ID
     */
    const DiceDef* GetDiceDef(const std::string& id) const;
    
    /**
     * Get all dice of a rarity
     */
    std::vector<std::string> GetDiceByRarity(DiceRarity rarity) const;
    
    /**
     * Get all dice of an archetype
     */
    std::vector<std::string> GetDiceByArchetype(UnitArchetype archetype) const;
    
    /**
     * Get starter dice IDs
     */
    std::vector<std::string> GetStarterDice() const;
    
    /**
     * Create a dice instance from definition
     */
    std::shared_ptr<Dice> CreateDice(const std::string& id) const;
    
    /**
     * Get random dice from shop pool
     */
    std::vector<std::string> GetShopDice(int waveNumber, int count) const;
    
    /**
     * Get rarity color
     */
    static SDL_Color GetRarityColor(DiceRarity rarity);
    static std::string GetRarityName(DiceRarity rarity);
    
private:
    DiceDatabase() = default;
    
    void RegisterDice(const DiceDef& def);
    void RegisterMageDice();
    void RegisterSoldierDice();
    void RegisterRogueDice();
    void RegisterHealerDice();
    void RegisterTankDice();
    void RegisterArcherDice();
    void RegisterSpecialDice();
    
    std::unordered_map<std::string, DiceDef> m_dice;
    bool m_initialized = false;
};

} // namespace DDD
