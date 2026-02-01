#pragma once

#include "Unit.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace DDD {

/**
 * Unit archetype (determines shape/color)
 */
enum class UnitArchetype {
    Mage,       // Circle - blue shades
    Soldier,    // Square - red shades
    Rogue,      // Diamond - green shades
    Healer,     // Cross - white/gold shades
    Tank,       // Hexagon - gray shades
    Archer      // Triangle - purple shades
};

/**
 * Unit class definition
 */
struct UnitClassDef {
    std::string className;
    UnitArchetype archetype;
    int tier;                           // 1, 2, or 3
    
    // Base stats
    int hp;
    int atk;
    int def;
    int mov;
    int rng;
    
    // Territory pattern [N, E, S, W]
    std::array<int, 4> unfurlPattern;
    
    // Promotion options (for tier 1 and 2)
    std::vector<std::string> promotionOptions;
    
    // Special ability name (if any)
    std::string abilityName;
    
    // Exp to promote (0 for tier 3)
    int expToPromote;
    
    // Description for tooltips
    std::string description;
};

/**
 * UnitDatabase - central repository of all unit definitions
 */
class UnitDatabase {
public:
    static UnitDatabase& Instance();
    
    /**
     * Initialize with all unit definitions
     */
    void Initialize();
    
    /**
     * Get class definition
     */
    const UnitClassDef* GetClassDef(const std::string& className) const;
    
    /**
     * Get all classes of a tier
     */
    std::vector<std::string> GetClassesByTier(int tier) const;
    
    /**
     * Get all classes of an archetype
     */
    std::vector<std::string> GetClassesByArchetype(UnitArchetype archetype) const;
    
    /**
     * Create a unit from a class definition
     */
    std::shared_ptr<Unit> CreateUnit(const std::string& className) const;
    
    /**
     * Get archetype color
     */
    static SDL_Color GetArchetypeColor(UnitArchetype archetype);
    static SDL_Color GetArchetypeColorLight(UnitArchetype archetype);
    
private:
    UnitDatabase() = default;
    
    void RegisterClass(const UnitClassDef& def);
    
    // Tier 1 classes (base)
    void RegisterTier1Classes();
    
    // Tier 2 classes (first promotion)
    void RegisterTier2Classes();
    
    // Tier 3 classes (final promotion)
    void RegisterTier3Classes();
    
    std::unordered_map<std::string, UnitClassDef> m_classes;
    bool m_initialized = false;
};

} // namespace DDD
