#include "TerrainSystem.h"
#include "Unit.h"
#include "../Graphics/Renderer.h"
#include "../Utils/Logger.h"
#include "../Utils/Random.h"

#include <algorithm>
#include <cmath>
#include <queue>

namespace DDD {

TerrainSystem& TerrainSystem::Instance() {
    static TerrainSystem instance;
    return instance;
}

void TerrainSystem::Initialize() {
    if (m_initialized) return;
    
    RegisterAllTerrain();
    RegisterMapTemplates();
    
    m_initialized = true;
    Logger::Info("TerrainSystem initialized with {} terrain types", m_terrainDefs.size());
}

void TerrainSystem::RegisterTerrain(const TerrainDef& def) {
    m_terrainDefs[def.type] = def;
}

void TerrainSystem::RegisterMapTemplate(const MapTemplate& tmpl) {
    m_mapTemplates[tmpl.id] = tmpl;
}

const TerrainDef& TerrainSystem::GetTerrainDef(TerrainType type) const {
    auto it = m_terrainDefs.find(type);
    if (it != m_terrainDefs.end()) {
        return it->second;
    }
    return m_defaultTerrain;
}

int TerrainSystem::GetMovementCost(TerrainType terrain, const Unit& unit) const {
    const TerrainDef& def = GetTerrainDef(terrain);
    
    // Flying units ignore most terrain costs
    // (would need Unit::IsFlying() method)
    
    return 1 + def.movementCost;
}

bool TerrainSystem::CanEnter(TerrainType terrain, const Unit& unit) const {
    const TerrainDef& def = GetTerrainDef(terrain);
    
    if (!def.isPassable) return false;
    
    // Flying-only terrain (would need Unit::IsFlying())
    // if (def.flyingOnly && !unit.IsFlying()) return false;
    
    return true;
}

int TerrainSystem::GetDefenseModifier(TerrainType terrain) const {
    return GetTerrainDef(terrain).defenseBonus;
}

int TerrainSystem::GetAttackModifier(TerrainType terrain) const {
    return GetTerrainDef(terrain).attackBonus;
}

int TerrainSystem::GetEvasionBonus(TerrainType terrain) const {
    return GetTerrainDef(terrain).evasionBonus;
}

void TerrainSystem::ApplyTurnStartEffects(Unit& unit, TerrainType terrain) {
    const TerrainDef& def = GetTerrainDef(terrain);
    
    // Damage effects (lava, etc)
    if (def.damagePerTurn > 0) {
        unit.TakeDamage(def.damagePerTurn);
        Logger::Info("{} takes {} damage from {}!", 
                     unit.GetClassName(), def.damagePerTurn, def.name);
    }
    
    // Healing effects
    if (def.healPerTurn > 0) {
        unit.Heal(def.healPerTurn);
        Logger::Info("{} heals {} from {}!", 
                     unit.GetClassName(), def.healPerTurn, def.name);
    }
}

void TerrainSystem::ApplyEntryEffects(Unit& unit, TerrainType terrain) {
    const TerrainDef& def = GetTerrainDef(terrain);
    
    // Trap damage
    if (terrain == TerrainType::Trap) {
        unit.TakeDamage(10);
        unit.ApplyStun(1);
        Logger::Info("{} triggered a trap!", unit.GetClassName());
    }
    
    // Ice sliding would be handled by movement system
}

bool TerrainSystem::BlocksLineOfSight(TerrainType terrain) const {
    return GetTerrainDef(terrain).blocksLineOfSight;
}

const MapTemplate* TerrainSystem::GetMapTemplate(const std::string& id) const {
    auto it = m_mapTemplates.find(id);
    return it != m_mapTemplates.end() ? &it->second : nullptr;
}

std::vector<std::vector<TerrainType>> TerrainSystem::GenerateMap(int width, int height, int waveNumber) {
    // Initialize with normal terrain
    std::vector<std::vector<TerrainType>> map(height, std::vector<TerrainType>(width, TerrainType::Normal));
    
    // Select appropriate template based on wave
    std::string templateId;
    if (waveNumber <= 3) {
        templateId = "forest_clearing";
    } else if (waveNumber <= 6) {
        templateId = "river_crossing";
    } else if (waveNumber <= 9) {
        templateId = "mountain_pass";
    } else if (waveNumber % 5 == 0) {
        templateId = "boss_arena";
    } else {
        templateId = "volcanic_field";
    }
    
    const MapTemplate* tmpl = GetMapTemplate(templateId);
    if (!tmpl) {
        // Fallback to basic map
        PlaceForestCluster(map);
        return map;
    }
    
    // Place terrain based on template
    if (tmpl->hasRiver) {
        PlaceRiver(map);
    }
    
    if (tmpl->hasMountains) {
        PlaceMountainRange(map);
    }
    
    if (tmpl->hasLava) {
        PlaceLavaPool(map);
    }
    
    // Place random terrain clusters
    int numObstacles = Random::Range(tmpl->minObstacles, tmpl->maxObstacles);
    for (int i = 0; i < numObstacles; ++i) {
        // Pick random terrain from weights
        int totalWeight = 0;
        for (const auto& [type, weight] : tmpl->terrainWeights) {
            totalWeight += weight;
        }
        
        int roll = Random::Range(0, totalWeight - 1);
        TerrainType chosen = TerrainType::Forest;
        
        int cumulative = 0;
        for (const auto& [type, weight] : tmpl->terrainWeights) {
            cumulative += weight;
            if (roll < cumulative) {
                chosen = type;
                break;
            }
        }
        
        int clusterSize = Random::Range(1, 3);
        PlaceTerrainCluster(map, chosen, clusterSize);
    }
    
    // Add some healing tiles
    if (Random::Range(0, 100) < 30) {
        int x = Random::Range(1, width - 2);
        int y = Random::Range(1, height - 2);
        map[y][x] = TerrainType::Healing;
    }
    
    // Ensure there's a path from spawn areas
    EnsurePathExists(map);
    
    // Mark spawn points (corners)
    map[0][0] = TerrainType::SpawnPoint;
    map[0][width-1] = TerrainType::SpawnPoint;
    map[height-1][0] = TerrainType::SpawnPoint;
    map[height-1][width-1] = TerrainType::SpawnPoint;
    
    return map;
}

void TerrainSystem::PlaceRiver(std::vector<std::vector<TerrainType>>& map) {
    int height = static_cast<int>(map.size());
    int width = static_cast<int>(map[0].size());
    
    // Vertical or horizontal river
    bool vertical = Random::Range(0, 1) == 0;
    
    if (vertical) {
        int riverX = width / 2 + Random::Range(-1, 1);
        for (int y = 0; y < height; ++y) {
            if (riverX >= 0 && riverX < width) {
                map[y][riverX] = TerrainType::Water;
                // Add some width variation
                if (Random::Range(0, 100) < 40 && riverX > 0) {
                    map[y][riverX - 1] = TerrainType::Water;
                }
            }
            // Meander
            riverX += Random::Range(-1, 1);
        }
        
        // Add bridges
        int bridgeY = height / 3;
        if (map[bridgeY][width/2].type == TerrainType::Water) {
            map[bridgeY][width/2] = TerrainType::Bridge;
        }
        bridgeY = 2 * height / 3;
        if (bridgeY < height && map[bridgeY][width/2].type == TerrainType::Water) {
            map[bridgeY][width/2] = TerrainType::Bridge;
        }
    } else {
        int riverY = height / 2 + Random::Range(-1, 1);
        for (int x = 0; x < width; ++x) {
            if (riverY >= 0 && riverY < height) {
                map[riverY][x] = TerrainType::Water;
            }
            riverY += Random::Range(-1, 1);
        }
        
        // Add bridges
        int bridgeX = width / 3;
        if (map[height/2][bridgeX].type == TerrainType::Water) {
            map[height/2][bridgeX] = TerrainType::Bridge;
        }
    }
}

void TerrainSystem::PlaceMountainRange(std::vector<std::vector<TerrainType>>& map) {
    int height = static_cast<int>(map.size());
    int width = static_cast<int>(map[0].size());
    
    // Place mountains along one edge
    int edge = Random::Range(0, 3);
    int numMountains = Random::Range(3, 6);
    
    for (int i = 0; i < numMountains; ++i) {
        int x, y;
        switch (edge) {
            case 0: // Top
                x = Random::Range(1, width - 2);
                y = Random::Range(0, 2);
                break;
            case 1: // Right
                x = Random::Range(width - 3, width - 1);
                y = Random::Range(1, height - 2);
                break;
            case 2: // Bottom
                x = Random::Range(1, width - 2);
                y = Random::Range(height - 3, height - 1);
                break;
            default: // Left
                x = Random::Range(0, 2);
                y = Random::Range(1, height - 2);
                break;
        }
        
        if (x >= 0 && x < width && y >= 0 && y < height) {
            map[y][x] = TerrainType::Mountain;
        }
    }
}

void TerrainSystem::PlaceLavaPool(std::vector<std::vector<TerrainType>>& map) {
    int height = static_cast<int>(map.size());
    int width = static_cast<int>(map[0].size());
    
    // Place lava pool in center area
    int centerX = width / 2 + Random::Range(-2, 2);
    int centerY = height / 2 + Random::Range(-2, 2);
    int poolSize = Random::Range(2, 4);
    
    for (int dy = -poolSize/2; dy <= poolSize/2; ++dy) {
        for (int dx = -poolSize/2; dx <= poolSize/2; ++dx) {
            int x = centerX + dx;
            int y = centerY + dy;
            if (x >= 1 && x < width - 1 && y >= 1 && y < height - 1) {
                if (std::abs(dx) + std::abs(dy) <= poolSize/2 + 1) {
                    map[y][x] = TerrainType::Lava;
                }
            }
        }
    }
}

void TerrainSystem::PlaceForestCluster(std::vector<std::vector<TerrainType>>& map) {
    PlaceTerrainCluster(map, TerrainType::Forest, Random::Range(2, 5));
}

void TerrainSystem::PlaceTerrainCluster(std::vector<std::vector<TerrainType>>& map, TerrainType type, int size) {
    int height = static_cast<int>(map.size());
    int width = static_cast<int>(map[0].size());
    
    int startX = Random::Range(1, width - 2);
    int startY = Random::Range(1, height - 2);
    
    for (int i = 0; i < size; ++i) {
        int x = startX + Random::Range(-1, 1);
        int y = startY + Random::Range(-1, 1);
        
        if (x >= 1 && x < width - 1 && y >= 1 && y < height - 1) {
            if (map[y][x] == TerrainType::Normal) {
                map[y][x] = type;
            }
        }
        
        startX = x;
        startY = y;
    }
}

void TerrainSystem::EnsurePathExists(std::vector<std::vector<TerrainType>>& map) {
    int height = static_cast<int>(map.size());
    int width = static_cast<int>(map[0].size());
    
    // Simple check: make sure corners can reach center
    // Clear any blocking terrain in middle row/column if needed
    
    int midY = height / 2;
    int midX = width / 2;
    
    // Ensure horizontal path
    for (int x = 0; x < width; ++x) {
        if (!GetTerrainDef(map[midY][x]).isPassable) {
            map[midY][x] = TerrainType::Normal;
        }
    }
    
    // Ensure vertical path
    for (int y = 0; y < height; ++y) {
        if (!GetTerrainDef(map[y][midX]).isPassable) {
            map[y][midX] = TerrainType::Normal;
        }
    }
}

void TerrainSystem::RenderTerrain(Renderer& renderer, TerrainType terrain, int x, int y, int cellSize) {
    const TerrainDef& def = GetTerrainDef(terrain);
    
    // Base color fill
    renderer.FillRect(x, y, cellSize, cellSize, def.baseColor);
    
    // Accent patterns based on terrain type
    switch (terrain) {
        case TerrainType::Forest:
            // Tree-like pattern
            for (int i = 0; i < 3; ++i) {
                int tx = x + 8 + (i * 15);
                int ty = y + 10;
                renderer.FillRect(tx, ty, 8, 12, def.accentColor);
                renderer.FillRect(tx + 2, ty + 12, 4, 8, {80, 50, 30, 255});
            }
            break;
            
        case TerrainType::Water:
            // Wave pattern
            for (int i = 0; i < 4; ++i) {
                int wy = y + 10 + (i * 10);
                renderer.DrawLine(x + 5, wy, x + cellSize - 5, wy + 3, def.accentColor);
            }
            break;
            
        case TerrainType::Mountain:
            // Triangle peaks
            renderer.FillRect(x + cellSize/4, y + cellSize/2, cellSize/2, cellSize/2, def.accentColor);
            renderer.FillRect(x + cellSize/3, y + cellSize/4, cellSize/3, cellSize/4, {220, 220, 230, 255});
            break;
            
        case TerrainType::Lava:
            // Glowing spots
            for (int i = 0; i < 5; ++i) {
                int lx = x + Random::Range(5, cellSize - 10);
                int ly = y + Random::Range(5, cellSize - 10);
                renderer.FillRect(lx, ly, 6, 6, def.accentColor);
            }
            break;
            
        case TerrainType::Swamp:
            // Murky pools
            for (int i = 0; i < 3; ++i) {
                int sx = x + 10 + (i * 12);
                int sy = y + 15 + (i % 2) * 10;
                renderer.FillRect(sx, sy, 10, 6, def.accentColor);
            }
            break;
            
        case TerrainType::Healing:
            // Glowing cross
            renderer.FillRect(x + cellSize/2 - 3, y + 10, 6, cellSize - 20, def.accentColor);
            renderer.FillRect(x + 10, y + cellSize/2 - 3, cellSize - 20, 6, def.accentColor);
            break;
            
        case TerrainType::HighGround:
            // Elevated platform look
            renderer.DrawRect(x + 4, y + 4, cellSize - 8, cellSize - 8, def.accentColor);
            renderer.DrawRect(x + 6, y + 6, cellSize - 12, cellSize - 12, def.accentColor);
            break;
            
        case TerrainType::Ice:
            // Crystalline pattern
            renderer.DrawLine(x + 10, y + 10, x + cellSize - 10, y + cellSize - 10, def.accentColor);
            renderer.DrawLine(x + cellSize - 10, y + 10, x + 10, y + cellSize - 10, def.accentColor);
            break;
            
        case TerrainType::Trap:
            // Danger marks
            renderer.DrawRect(x + 8, y + 8, cellSize - 16, cellSize - 16, def.accentColor);
            break;
            
        default:
            break;
    }
    
    // Border
    renderer.DrawRect(x, y, cellSize, cellSize, {40, 40, 50, 255});
}

void TerrainSystem::RenderTerrainOverlay(Renderer& renderer, TerrainType terrain, int x, int y, int cellSize, float alpha) {
    const TerrainDef& def = GetTerrainDef(terrain);
    
    SDL_Color overlayColor = def.baseColor;
    overlayColor.a = static_cast<uint8_t>(alpha * 100);
    
    renderer.FillRect(x, y, cellSize, cellSize, overlayColor);
}

// ===========================================================================
// TERRAIN DEFINITIONS
// ===========================================================================

void TerrainSystem::RegisterAllTerrain() {
    // Default terrain (used as fallback)
    m_defaultTerrain = {
        TerrainType::Normal, "Ground", "Standard terrain",
        0, true, false,
        0, 0, 0,
        0, 0, false, false, "",
        {60, 60, 70, 255}, {70, 70, 80, 255}, '.'
    };
    
    RegisterTerrain(m_defaultTerrain);
    
    // Normal terrain
    RegisterTerrain({
        TerrainType::Normal, "Ground", "Standard terrain with no special effects",
        0, true, false,
        0, 0, 0,
        0, 0, false, false, "",
        {60, 60, 70, 255}, {70, 70, 80, 255}, '.'
    });
    
    // Forest - defensive cover
    RegisterTerrain({
        TerrainType::Forest, "Forest", "Trees provide cover (+2 DEF, +10% evasion) but slow movement",
        1, true, false,
        2, 0, 10,
        0, 0, false, false, "",
        {40, 70, 40, 255}, {30, 90, 30, 255}, 'T'
    });
    
    // Water - impassable
    RegisterTerrain({
        TerrainType::Water, "Water", "Deep water, impassable for most units",
        0, false, true,
        0, 0, 0,
        0, 0, false, false, "",
        {40, 80, 150, 255}, {60, 100, 180, 255}, '~'
    });
    
    // Mountain - blocks everything
    RegisterTerrain({
        TerrainType::Mountain, "Mountain", "Impassable rocky terrain that blocks line of sight",
        0, false, false,
        0, 0, 0,
        0, 0, true, false, "",
        {100, 90, 80, 255}, {140, 130, 120, 255}, '^'
    });
    
    // Swamp - slows and weakens
    RegisterTerrain({
        TerrainType::Swamp, "Swamp", "Murky terrain that slows movement and reduces defense (-1 DEF)",
        2, true, false,
        -1, 0, 0,
        0, 0, false, false, "",
        {50, 60, 40, 255}, {40, 70, 50, 255}, '%'
    });
    
    // Lava - damages each turn
    RegisterTerrain({
        TerrainType::Lava, "Lava", "Molten rock that damages units (8 damage per turn)",
        0, true, false,
        0, 0, 0,
        8, 0, false, false, "",
        {200, 80, 40, 255}, {255, 150, 50, 255}, '#'
    });
    
    // Ice - slippery
    RegisterTerrain({
        TerrainType::Ice, "Ice", "Slippery surface, units may slide when moving",
        0, true, false,
        0, 0, -5,
        0, 0, false, false, "",
        {180, 200, 230, 255}, {220, 240, 255, 255}, '='
    });
    
    // Sand - slows movement
    RegisterTerrain({
        TerrainType::Sand, "Sand", "Loose sand that slows movement",
        1, true, false,
        0, 0, 0,
        0, 0, false, false, "",
        {180, 160, 120, 255}, {200, 180, 140, 255}, ','
    });
    
    // Ruins - light cover
    RegisterTerrain({
        TerrainType::Ruins, "Ruins", "Crumbling structures provide light cover (+1 DEF)",
        0, true, false,
        1, 0, 5,
        0, 0, false, false, "",
        {80, 80, 90, 255}, {100, 100, 110, 255}, 'R'
    });
    
    // Void - impassable pit
    RegisterTerrain({
        TerrainType::Void, "Void", "A bottomless pit, completely impassable",
        0, false, false,
        0, 0, 0,
        0, 0, false, false, "",
        {20, 20, 30, 255}, {10, 10, 20, 255}, 'O'
    });
    
    // Healing - restores HP
    RegisterTerrain({
        TerrainType::Healing, "Healing Spring", "Magical waters that restore health (5 HP per turn)",
        0, true, false,
        0, 0, 0,
        0, 5, false, false, "",
        {60, 100, 80, 255}, {100, 200, 150, 255}, '+'
    });
    
    // Trap - damages and stuns
    RegisterTerrain({
        TerrainType::Trap, "Trap", "Hidden trap that damages and stuns units entering",
        0, true, false,
        0, 0, 0,
        0, 0, false, true, "Stun",
        {70, 60, 60, 255}, {150, 50, 50, 255}, '!'
    });
    
    // Spawn point
    RegisterTerrain({
        TerrainType::SpawnPoint, "Spawn Point", "Where units can be summoned",
        0, true, false,
        0, 0, 0,
        0, 0, false, false, "",
        {80, 80, 100, 255}, {120, 120, 160, 255}, 'S'
    });
    
    // High ground - combat advantage
    RegisterTerrain({
        TerrainType::HighGround, "High Ground", "Elevated position (+2 ATK, +1 DEF)",
        0, true, false,
        1, 2, 5,
        0, 0, false, false, "",
        {90, 85, 80, 255}, {130, 120, 110, 255}, 'H'
    });
    
    // Bridge - crosses water
    RegisterTerrain({
        TerrainType::Bridge, "Bridge", "Wooden bridge crossing water",
        0, true, false,
        0, 0, 0,
        0, 0, false, false, "",
        {100, 70, 40, 255}, {140, 100, 60, 255}, 'B'
    });
}

// ===========================================================================
// MAP TEMPLATES
// ===========================================================================

void TerrainSystem::RegisterMapTemplates() {
    // Forest clearing (early waves)
    RegisterMapTemplate({
        "forest_clearing", "Forest Clearing",
        8, 8,
        {{TerrainType::Forest, 5}, {TerrainType::Ruins, 2}},
        false, false, false,
        3, 6
    });
    
    // River crossing (mid waves)
    RegisterMapTemplate({
        "river_crossing", "River Crossing",
        8, 8,
        {{TerrainType::Forest, 3}, {TerrainType::Sand, 2}},
        true, false, false,
        2, 4
    });
    
    // Mountain pass (later waves)
    RegisterMapTemplate({
        "mountain_pass", "Mountain Pass",
        8, 8,
        {{TerrainType::HighGround, 3}, {TerrainType::Ruins, 2}},
        false, true, false,
        2, 5
    });
    
    // Volcanic field (late waves)
    RegisterMapTemplate({
        "volcanic_field", "Volcanic Field",
        8, 8,
        {{TerrainType::Lava, 3}, {TerrainType::HighGround, 2}, {TerrainType::Ruins, 1}},
        false, false, true,
        3, 6
    });
    
    // Boss arena
    RegisterMapTemplate({
        "boss_arena", "Boss Arena",
        10, 10,
        {{TerrainType::HighGround, 4}, {TerrainType::Ruins, 2}},
        false, false, false,
        2, 4
    });
    
    // Swamp maze
    RegisterMapTemplate({
        "swamp_maze", "Swamp Maze",
        8, 8,
        {{TerrainType::Swamp, 5}, {TerrainType::Water, 2}},
        true, false, false,
        4, 8
    });
    
    // Frozen wastes
    RegisterMapTemplate({
        "frozen_wastes", "Frozen Wastes",
        8, 8,
        {{TerrainType::Ice, 4}, {TerrainType::Mountain, 2}},
        false, true, false,
        3, 6
    });
}

} // namespace DDD
