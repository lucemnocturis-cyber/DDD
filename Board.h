#pragma once

#include "Cell.h"
#include "Unit.h"
#include "../Utils/Math.h"
#include "../Graphics/UnitRenderer.h"

#include <array>
#include <vector>
#include <memory>
#include <functional>

namespace DDD {

// Forward declarations
class Renderer;
class Dice;

/**
 * Territory owner types
 */
enum class Owner {
    None,
    Player,
    Enemy
};

/**
 * Board - the tactical grid battlefield
 */
class Board {
public:
    // Grid dimensions (from the prototype)
    static constexpr int WIDTH = 13;
    static constexpr int HEIGHT = 19;
    
    // Cell size in pixels
    static constexpr int CELL_SIZE = 48;
    
    Board();
    ~Board();
    
    /**
     * Initialize the board
     */
    bool Initialize();
    
    /**
     * Reset the board for a new game
     */
    void Reset();
    
    /**
     * Update board state
     */
    void Update(float deltaTime);
    
    /**
     * Render the board
     */
    void Render(Renderer& renderer);
    
    /**
     * Get cell at position
     */
    Cell& GetCell(int x, int y);
    const Cell& GetCell(int x, int y) const;
    
    /**
     * Check if position is valid
     */
    bool IsValidPosition(int x, int y) const;
    
    /**
     * Check if a dice can be placed at position
     */
    bool IsValidPlacement(const Dice& dice, int x, int y, Owner owner) const;
    
    /**
     * Place a dice on the board
     */
    bool PlaceDice(std::shared_ptr<Dice> dice, int x, int y, Owner owner);
    
    /**
     * Get cells affected by unfurl pattern
     */
    std::vector<Position> GetUnfurlCells(int x, int y, const std::array<int, 4>& pattern) const;
    
    /**
     * Convert territory ownership
     */
    void ConvertTerritory(const std::vector<Position>& cells, Owner newOwner);
    
    /**
     * Get all units on the board
     */
    const std::vector<std::shared_ptr<Unit>>& GetUnits() const { return m_units; }
    
    /**
     * Get units by owner
     */
    std::vector<std::shared_ptr<Unit>> GetUnitsByOwner(Owner owner) const;
    
    /**
     * Get unit at position
     */
    std::shared_ptr<Unit> GetUnitAt(int x, int y) const;
    
    /**
     * Remove unit from board
     */
    void RemoveUnit(const std::shared_ptr<Unit>& unit);
    
    /**
     * Get valid movement cells for a unit
     */
    std::vector<Position> GetValidMoveCells(const Unit& unit) const;
    
    /**
     * Get valid attack targets for a unit
     */
    std::vector<Position> GetValidAttackTargets(const Unit& unit) const;
    
    /**
     * Alias for GetValidAttackTargets (used by SelectionManager)
     */
    std::vector<Position> GetValidAttackCells(const Unit& unit) const { return GetValidAttackTargets(unit); }
    
    /**
     * Move a unit to a new position
     */
    void MoveUnit(std::shared_ptr<Unit> unit, int x, int y);
    
    /**
     * Calculate Manhattan distance between two positions
     */
    int GetDistance(const Position& a, const Position& b) const;
    
    /**
     * Check line of sight between two positions
     */
    bool HasLineOfSight(const Position& from, const Position& to) const;
    
    /**
     * Convert screen coordinates to grid position
     */
    Position ScreenToGrid(int screenX, int screenY) const;
    
    /**
     * Convert grid position to screen coordinates
     */
    Position GridToScreen(int gridX, int gridY) const;
    
    /**
     * Render selection highlights (hovered cell, valid moves, etc.)
     */
    void RenderSelectionHighlights(Renderer& renderer, 
                                   const Position& hoveredCell,
                                   bool isHovering,
                                   const std::vector<Position>& validMoves,
                                   const std::vector<Position>& validAttacks,
                                   const std::vector<Position>& validPlacements);
    
    /**
     * Render unfurl preview for dice placement
     */
    void RenderUnfurlPreview(Renderer& renderer, const Position& center, 
                             const std::array<int, 4>& unfurl, bool isValid);
    
    /**
     * Get screen position for a cell
     */
    void GetCellScreenPosition(int gridX, int gridY, int& screenX, int& screenY) const;
    
    /**
     * Set/get board offset (for camera/UI layout)
     */
    void SetOffset(int x, int y) { m_offsetX = x; m_offsetY = y; }
    void GetOffset(int& x, int& y) const { x = m_offsetX; y = m_offsetY; }
    int GetOffsetX() const { return m_offsetX; }
    int GetOffsetY() const { return m_offsetY; }
    
    /**
     * Get/set cell size
     */
    int GetCellSize() const { return m_cellSize; }
    void SetCellSize(int size) { m_cellSize = size; }
    
    // Starting zones
    static constexpr Position PLAYER_START_ZONE = {6, 16};
    static constexpr Position ENEMY_START_ZONE = {6, 2};
    
private:
    // Pathfinding helpers
    bool IsWalkable(int x, int y) const;
    std::vector<Position> GetNeighbors(const Position& pos) const;
    
    // Grid storage
    std::array<std::array<Cell, WIDTH>, HEIGHT> m_cells;
    
    // Units on the board
    std::vector<std::shared_ptr<Unit>> m_units;
    
    // Unit renderer for visual display
    std::unique_ptr<UnitRenderer> m_unitRenderer;
    
    // Board position offset (for rendering)
    int m_offsetX = 0;
    int m_offsetY = 0;
    int m_cellSize = CELL_SIZE;
};

} // namespace DDD
