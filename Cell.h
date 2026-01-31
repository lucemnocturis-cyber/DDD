#pragma once

#include <memory>

namespace DDD {

// Forward declarations
class Unit;
enum class Owner;

/**
 * Cell - represents a single cell on the tactical grid
 */
struct Cell {
    int x = 0;
    int y = 0;
    Owner owner = static_cast<Owner>(0);  // None
    std::shared_ptr<Unit> occupant = nullptr;
    
    // Terrain flags (for future expansion)
    bool isWalkable = true;
    bool blocksLineOfSight = false;
    
    // Visual state
    bool isHighlighted = false;
    bool isSelected = false;
    bool isValidMove = false;
    bool isValidAttack = false;
};

} // namespace DDD
