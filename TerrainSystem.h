#pragma once

#include "../Utils/Math.h"
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace DDD {

class Unit;
class Board;
class Renderer;

/**
 * Terrain types available in the game
 */
enum class TerrainType {
    Normal,         // Standard terrain, no modifiers
    Forest,         // Defensive cover, slows movement
    Water,          // Impassable for most, some can cross
    Mountain,       // Impassable, blocks line of sight
    Swamp,          // Slows movement, reduces defense
    Lava,           // Damages units that enter
    Ice,            // Slippery, units slide
    Sand,           // Slows movement
    Ruins,          // Light cover, normal movement
    Void,           // Impassable pit
    Healing,        // Heals units standing on it
    Trap,           // Damages and stuns
    SpawnPoint,     // Where units can be summoned
    HighGround,     // Attack bonus, defense bonus
    Bridge          // Crossable water
};

/**
 * Terrain definition with all properties
 */
struct TerrainDef {
    TerrainType type;
    std::string name;
    std::string description;
    
    // Movement
    int movementCost;       // Extra movement points needed (0 = normal)
    bool isPassable;        // Can units walk on it?
    bool flyingOnly;        // Only flying units can cross
    
    // Combat modifiers
    int defenseBonus;       // Added to unit's defense
    int attackBonus;        // Added to unit's attack
    int evasionBonus;       // % chance to dodge attacks
    
    // Effects
    int damagePerTurn;      // Damage taken each turn
    int healPerTurn;        // Healing received each turn
    bool blocksLineOfSight; // Blocks ranged attacks through
    bool appliesStatus;     // Does it apply a status effect?
    std::string statusEffect; // Status effect name
    
    // Visuals
    SDL_Color baseColor;
    SDL_Color accentColor;
    char symbol;            // For debugging/minimaps
};

/**
 * Map template for procedural generation
 */
struct MapTemplate {
    std::string id;
    std::string name;
    int width;
    int height;
    
    // Terrain distribution weights
    std::vector<std::pair<TerrainType, int>> terrainWeights;
    
    // Special features
    bool hasRiver;
    bool hasMountains;
    bool hasLava;
    int minObstacles;
    int maxObstacles;
};

/**
 * TerrainSystem - manages terrain types and effects
 */
class TerrainSystem {
public:
    static TerrainSystem& Instance();
    
    void Initialize();
    
    /**
     * Get terrain definition
     */
    const TerrainDef& GetTerrainDef(TerrainType type) const;
    
    /**
     * Calculate movement cost for a unit at position
     */
    int GetMovementCost(TerrainType terrain, const Unit& unit) const;
    
    /**
     * Check if unit can enter terrain
     */
    bool CanEnter(TerrainType terrain, const Unit& unit) const;
    
    /**
     * Get defense modifier
     */
    int GetDefenseModifier(TerrainType terrain) const;
    
    /**
     * Get attack modifier
     */
    int GetAttackModifier(TerrainType terrain) const;
    
    /**
     * Get evasion bonus (% chance to completely dodge)
     */
    int GetEvasionBonus(TerrainType terrain) const;
    
    /**
     * Apply terrain effects at start of turn
     */
    void ApplyTurnStartEffects(Unit& unit, TerrainType terrain);
    
    /**
     * Apply terrain effects when entering
     */
    void ApplyEntryEffects(Unit& unit, TerrainType terrain);
    
    /**
     * Check if terrain blocks line of sight
     */
    bool BlocksLineOfSight(TerrainType terrain) const;
    
    /**
     * Generate terrain for a map
     */
    std::vector<std::vector<TerrainType>> GenerateMap(int width, int height, int waveNumber);
    
    /**
     * Get map template
     */
    const MapTemplate* GetMapTemplate(const std::string& id) const;
    
    /**
     * Render terrain at position
     */
    void RenderTerrain(Renderer& renderer, TerrainType terrain, int x, int y, int cellSize);
    
    /**
     * Render terrain overlay effects (for highlighting)
     */
    void RenderTerrainOverlay(Renderer& renderer, TerrainType terrain, int x, int y, int cellSize, float alpha);
    
private:
    TerrainSystem() = default;
    
    void RegisterTerrain(const TerrainDef& def);
    void RegisterMapTemplate(const MapTemplate& tmpl);
    void RegisterAllTerrain();
    void RegisterMapTemplates();
    
    // Map generation helpers
    void PlaceRiver(std::vector<std::vector<TerrainType>>& map);
    void PlaceMountainRange(std::vector<std::vector<TerrainType>>& map);
    void PlaceLavaPool(std::vector<std::vector<TerrainType>>& map);
    void PlaceForestCluster(std::vector<std::vector<TerrainType>>& map);
    void PlaceTerrainCluster(std::vector<std::vector<TerrainType>>& map, TerrainType type, int size);
    void EnsurePathExists(std::vector<std::vector<TerrainType>>& map);
    
    std::unordered_map<TerrainType, TerrainDef> m_terrainDefs;
    std::unordered_map<std::string, MapTemplate> m_mapTemplates;
    TerrainDef m_defaultTerrain;
    bool m_initialized = false;
};

} // namespace DDD
