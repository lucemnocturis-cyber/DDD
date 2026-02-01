#include "SelectionManager.h"
#include "Board.h"
#include "Unit.h"
#include "Dice.h"
#include "../Utils/Logger.h"

namespace DDD {

SelectionManager::SelectionManager() = default;
SelectionManager::~SelectionManager() = default;

void SelectionManager::Update(int mouseX, int mouseY, int boardOffsetX, int boardOffsetY, int cellSize) {
    if (!m_board) return;
    
    // Convert screen coordinates to grid coordinates
    int gridX = (mouseX - boardOffsetX) / cellSize;
    int gridY = (mouseY - boardOffsetY) / cellSize;
    
    // Check if mouse is over the board
    bool wasOverBoard = m_isMouseOverBoard;
    m_isMouseOverBoard = m_board->IsValidPosition(gridX, gridY) &&
                         mouseX >= boardOffsetX && 
                         mouseY >= boardOffsetY &&
                         mouseX < boardOffsetX + Board::WIDTH * cellSize &&
                         mouseY < boardOffsetY + Board::HEIGHT * cellSize;
    
    if (m_isMouseOverBoard) {
        Position newHovered = {gridX, gridY};
        if (newHovered != m_hoveredCell) {
            m_hoveredCell = newHovered;
        }
    }
}

void SelectionManager::OnLeftClick() {
    if (!m_isMouseOverBoard || !m_board) return;
    
    switch (m_mode) {
        case SelectionMode::Idle: {
            // Check if clicking on a unit
            auto unit = m_board->GetUnitAt(m_hoveredCell.x, m_hoveredCell.y);
            if (unit && unit->GetOwner() == Owner::Player) {
                SelectUnit(unit);
            } else if (m_onCellSelected) {
                m_onCellSelected(m_hoveredCell);
            }
            break;
        }
        
        case SelectionMode::UnitSelected: {
            // Check if clicking on valid move cell
            bool isValidMove = false;
            for (const auto& cell : m_validMoveCells) {
                if (cell == m_hoveredCell) {
                    isValidMove = true;
                    break;
                }
            }
            
            if (isValidMove) {
                TryMoveUnit(m_hoveredCell);
                break;
            }
            
            // Check if clicking on valid attack cell
            bool isValidAttack = false;
            for (const auto& cell : m_validAttackCells) {
                if (cell == m_hoveredCell) {
                    isValidAttack = true;
                    break;
                }
            }
            
            if (isValidAttack) {
                TryAttackUnit(m_hoveredCell);
                break;
            }
            
            // Check if clicking on another player unit
            auto unit = m_board->GetUnitAt(m_hoveredCell.x, m_hoveredCell.y);
            if (unit && unit->GetOwner() == Owner::Player && unit != m_selectedUnit) {
                SelectUnit(unit);
            } else {
                // Click on invalid cell - deselect
                ClearSelection();
            }
            break;
        }
        
        case SelectionMode::PlacingDice: {
            TryPlaceDice(m_hoveredCell);
            break;
        }
        
        case SelectionMode::SelectingTarget:
        case SelectionMode::SelectingMove:
            // Handle ability targeting (future implementation)
            if (m_onCellSelected) {
                m_onCellSelected(m_hoveredCell);
            }
            break;
            
        default:
            break;
    }
}

void SelectionManager::OnRightClick() {
    // Right-click always cancels current selection
    ClearSelection();
    Logger::Debug("Selection cancelled");
}

void SelectionManager::SetSelectedDice(std::shared_ptr<Dice> dice) {
    ClearSelection();
    m_selectedDice = dice;
    
    if (dice) {
        m_mode = SelectionMode::PlacingDice;
        UpdateValidCells();
        Logger::Info("Selected dice: {} for placement", dice->GetClassName());
    }
}

void SelectionManager::ClearSelection() {
    m_selectedUnit = nullptr;
    m_selectedDice = nullptr;
    m_mode = SelectionMode::Idle;
    m_validMoveCells.clear();
    m_validAttackCells.clear();
    m_validPlacementCells.clear();
    
    if (m_onSelectionCleared) {
        m_onSelectionCleared();
    }
}

void SelectionManager::UpdateValidCells() {
    m_validMoveCells.clear();
    m_validAttackCells.clear();
    m_validPlacementCells.clear();
    
    if (!m_board) return;
    
    if (m_selectedUnit && m_mode == SelectionMode::UnitSelected) {
        // Get valid move cells
        if (!m_selectedUnit->HasMoved()) {
            m_validMoveCells = m_board->GetValidMoveCells(*m_selectedUnit);
        }
        
        // Get valid attack cells
        if (!m_selectedUnit->HasAttacked()) {
            m_validAttackCells = m_board->GetValidAttackCells(*m_selectedUnit);
        }
    }
    
    if (m_selectedDice && m_mode == SelectionMode::PlacingDice) {
        // Get valid placement cells for the dice
        // For now, use the player's territory and starting area
        for (int y = 0; y < Board::HEIGHT; ++y) {
            for (int x = 0; x < Board::WIDTH; ++x) {
                if (m_board->IsValidPlacement(*m_selectedDice, x, y, Owner::Player)) {
                    m_validPlacementCells.push_back({x, y});
                }
            }
        }
    }
}

void SelectionManager::SelectUnit(std::shared_ptr<Unit> unit) {
    m_selectedUnit = unit;
    m_selectedDice = nullptr;
    m_mode = SelectionMode::UnitSelected;
    UpdateValidCells();
    
    if (m_onUnitSelected) {
        m_onUnitSelected(unit);
    }
    
    Logger::Info("Selected unit: {} at ({},{})", 
                 unit->GetClassName(), unit->GetPosition().x, unit->GetPosition().y);
}

void SelectionManager::TryMoveUnit(const Position& target) {
    if (!m_selectedUnit || !m_board) return;
    
    // Verify it's a valid move
    bool valid = false;
    for (const auto& cell : m_validMoveCells) {
        if (cell == target) {
            valid = true;
            break;
        }
    }
    
    if (!valid) {
        Logger::Warning("Invalid move target");
        return;
    }
    
    // Move the unit
    Position oldPos = m_selectedUnit->GetPosition();
    m_board->MoveUnit(m_selectedUnit, target.x, target.y);
    m_selectedUnit->SetHasMoved(true);
    
    // Update valid cells (can still attack after moving)
    UpdateValidCells();
    
    if (m_onUnitMoved) {
        m_onUnitMoved(m_selectedUnit, target);
    }
    
    Logger::Info("{} moved from ({},{}) to ({},{})",
                 m_selectedUnit->GetClassName(), oldPos.x, oldPos.y, target.x, target.y);
    
    // If unit can't do anything else, deselect
    if (m_validMoveCells.empty() && m_validAttackCells.empty()) {
        ClearSelection();
    }
}

void SelectionManager::TryAttackUnit(const Position& target) {
    if (!m_selectedUnit || !m_board) return;
    
    auto targetUnit = m_board->GetUnitAt(target.x, target.y);
    if (!targetUnit) {
        Logger::Warning("No unit at attack target");
        return;
    }
    
    if (targetUnit->GetOwner() == m_selectedUnit->GetOwner()) {
        Logger::Warning("Cannot attack friendly unit");
        return;
    }
    
    // Perform attack
    int damage = std::max(1, m_selectedUnit->GetStats().atk - targetUnit->GetStats().def);
    targetUnit->TakeDamage(damage);
    m_selectedUnit->SetHasAttacked(true);
    
    if (m_onUnitAttacked) {
        m_onUnitAttacked(m_selectedUnit, targetUnit);
    }
    
    Logger::Info("{} attacks {} for {} damage",
                 m_selectedUnit->GetClassName(), targetUnit->GetClassName(), damage);
    
    // Update valid cells
    UpdateValidCells();
    
    // If unit can't do anything else, deselect
    if (m_validMoveCells.empty() && m_validAttackCells.empty()) {
        ClearSelection();
    }
}

void SelectionManager::TryPlaceDice(const Position& target) {
    if (!m_selectedDice || !m_board) return;
    
    // Verify it's a valid placement
    bool valid = false;
    for (const auto& cell : m_validPlacementCells) {
        if (cell == target) {
            valid = true;
            break;
        }
    }
    
    if (!valid) {
        Logger::Warning("Invalid dice placement");
        return;
    }
    
    if (m_onDicePlaced) {
        m_onDicePlaced(m_selectedDice, target);
    }
    
    Logger::Info("Placed {} at ({},{})", 
                 m_selectedDice->GetClassName(), target.x, target.y);
    
    ClearSelection();
}

} // namespace DDD
