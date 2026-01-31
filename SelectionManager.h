#pragma once

#include "../Utils/Math.h"
#include <memory>
#include <vector>
#include <functional>

namespace DDD {

class Board;
class Unit;
class Dice;

/**
 * Selection modes for different game states
 */
enum class SelectionMode {
    None,           // No selection active
    Idle,           // Default - can select units or cells
    UnitSelected,   // A unit is selected, showing move/attack options
    PlacingDice,    // Placing a dice on the board
    SelectingTarget,// Selecting a target for an ability
    SelectingMove   // Selecting where to move
};

/**
 * SelectionManager - handles all mouse-based selection in the game
 */
class SelectionManager {
public:
    SelectionManager();
    ~SelectionManager();
    
    /**
     * Set the board reference
     */
    void SetBoard(Board* board) { m_board = board; }
    
    /**
     * Update selection based on mouse position
     */
    void Update(int mouseX, int mouseY, int boardOffsetX, int boardOffsetY, int cellSize);
    
    /**
     * Handle left click
     */
    void OnLeftClick();
    
    /**
     * Handle right click (cancel)
     */
    void OnRightClick();
    
    /**
     * Get current hovered cell (grid coordinates)
     */
    Position GetHoveredCell() const { return m_hoveredCell; }
    
    /**
     * Check if mouse is over the board
     */
    bool IsMouseOverBoard() const { return m_isMouseOverBoard; }
    
    /**
     * Get currently selected unit
     */
    std::shared_ptr<Unit> GetSelectedUnit() const { return m_selectedUnit; }
    
    /**
     * Get currently selected dice (for placement)
     */
    std::shared_ptr<Dice> GetSelectedDice() const { return m_selectedDice; }
    
    /**
     * Set selected dice for placement
     */
    void SetSelectedDice(std::shared_ptr<Dice> dice);
    
    /**
     * Get current selection mode
     */
    SelectionMode GetMode() const { return m_mode; }
    
    /**
     * Set selection mode
     */
    void SetMode(SelectionMode mode) { m_mode = mode; }
    
    /**
     * Clear all selections
     */
    void ClearSelection();
    
    /**
     * Get valid move cells for selected unit
     */
    const std::vector<Position>& GetValidMoveCells() const { return m_validMoveCells; }
    
    /**
     * Get valid attack cells for selected unit
     */
    const std::vector<Position>& GetValidAttackCells() const { return m_validAttackCells; }
    
    /**
     * Get valid placement cells for selected dice
     */
    const std::vector<Position>& GetValidPlacementCells() const { return m_validPlacementCells; }
    
    // Callbacks
    using UnitSelectedCallback = std::function<void(std::shared_ptr<Unit>)>;
    using CellSelectedCallback = std::function<void(Position)>;
    using UnitMovedCallback = std::function<void(std::shared_ptr<Unit>, Position)>;
    using UnitAttackedCallback = std::function<void(std::shared_ptr<Unit>, std::shared_ptr<Unit>)>;
    using DicePlacedCallback = std::function<void(std::shared_ptr<Dice>, Position)>;
    
    void SetOnUnitSelected(UnitSelectedCallback cb) { m_onUnitSelected = cb; }
    void SetOnCellSelected(CellSelectedCallback cb) { m_onCellSelected = cb; }
    void SetOnUnitMoved(UnitMovedCallback cb) { m_onUnitMoved = cb; }
    void SetOnUnitAttacked(UnitAttackedCallback cb) { m_onUnitAttacked = cb; }
    void SetOnDicePlaced(DicePlacedCallback cb) { m_onDicePlaced = cb; }
    void SetOnSelectionCleared(std::function<void()> cb) { m_onSelectionCleared = cb; }
    
private:
    void UpdateValidCells();
    void SelectUnit(std::shared_ptr<Unit> unit);
    void TryMoveUnit(const Position& target);
    void TryAttackUnit(const Position& target);
    void TryPlaceDice(const Position& target);
    
    Board* m_board = nullptr;
    
    // Mouse state
    Position m_hoveredCell;
    bool m_isMouseOverBoard = false;
    
    // Selection state
    SelectionMode m_mode = SelectionMode::Idle;
    std::shared_ptr<Unit> m_selectedUnit;
    std::shared_ptr<Dice> m_selectedDice;
    
    // Valid cells for actions
    std::vector<Position> m_validMoveCells;
    std::vector<Position> m_validAttackCells;
    std::vector<Position> m_validPlacementCells;
    
    // Callbacks
    UnitSelectedCallback m_onUnitSelected;
    CellSelectedCallback m_onCellSelected;
    UnitMovedCallback m_onUnitMoved;
    UnitAttackedCallback m_onUnitAttacked;
    DicePlacedCallback m_onDicePlaced;
    std::function<void()> m_onSelectionCleared;
};

} // namespace DDD
