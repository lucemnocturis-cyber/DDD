#include "Board.h"
#include "Dice.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/TextRenderer.h"
#include "../Utils/Logger.h"

#include <algorithm>
#include <queue>

namespace DDD {

Board::Board() = default;
Board::~Board() = default;

bool Board::Initialize() {
    Reset();
    
    // Create unit renderer
    m_unitRenderer = std::make_unique<UnitRenderer>();
    
    Logger::Info("Board initialized: {}x{}", WIDTH, HEIGHT);
    return true;
}

void Board::Reset() {
    // Initialize all cells
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            m_cells[y][x] = Cell{x, y, Owner::None};
        }
    }
    
    // Set starting zones
    m_cells[PLAYER_START_ZONE.y][PLAYER_START_ZONE.x].owner = Owner::Player;
    m_cells[ENEMY_START_ZONE.y][ENEMY_START_ZONE.x].owner = Owner::Enemy;
    
    // Clear units
    m_units.clear();
    
    Logger::Info("Board reset");
}

void Board::Update(float deltaTime) {
    // Update unit animations
    for (auto& unit : m_units) {
        unit->Update(deltaTime);
    }
    
    // Update unit renderer animations
    if (m_unitRenderer) {
        m_unitRenderer->Update(deltaTime);
    }
}

void Board::Render(Renderer& renderer) {
    // Render board background (sand arena)
    SDL_Color sandColor = {193, 154, 107, 255};  // #c19a6b
    
    // Calculate board rendering area
    int boardPixelWidth = WIDTH * CELL_SIZE;
    int boardPixelHeight = HEIGHT * CELL_SIZE;
    
    // Draw arena floor
    renderer.FillRect(m_offsetX, m_offsetY, boardPixelWidth, boardPixelHeight, sandColor);
    
    // Draw cells with territory colors
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            const Cell& cell = m_cells[y][x];
            int screenX = m_offsetX + x * CELL_SIZE;
            int screenY = m_offsetY + y * CELL_SIZE;
            
            // Draw territory overlay
            SDL_Color overlayColor;
            switch (cell.owner) {
                case Owner::Player:
                    overlayColor = {52, 152, 219, 64};  // Blue with alpha
                    renderer.FillRect(screenX, screenY, CELL_SIZE, CELL_SIZE, overlayColor);
                    break;
                case Owner::Enemy:
                    overlayColor = {231, 76, 60, 64};  // Red with alpha
                    renderer.FillRect(screenX, screenY, CELL_SIZE, CELL_SIZE, overlayColor);
                    break;
                default:
                    break;
            }
            
            // Draw grid lines
            SDL_Color gridColor = {90, 74, 58, 128};  // Brown grid lines
            renderer.DrawRect(screenX, screenY, CELL_SIZE, CELL_SIZE, gridColor);
        }
    }
    
    // Draw stone wall border
    SDL_Color wallColor = {90, 74, 58, 255};
    int borderWidth = 15;
    
    // Top wall
    renderer.FillRect(m_offsetX - borderWidth, m_offsetY - borderWidth,
                      boardPixelWidth + borderWidth * 2, borderWidth, wallColor);
    // Bottom wall
    renderer.FillRect(m_offsetX - borderWidth, m_offsetY + boardPixelHeight,
                      boardPixelWidth + borderWidth * 2, borderWidth, wallColor);
    // Left wall
    renderer.FillRect(m_offsetX - borderWidth, m_offsetY,
                      borderWidth, boardPixelHeight, wallColor);
    // Right wall
    renderer.FillRect(m_offsetX + boardPixelWidth, m_offsetY,
                      borderWidth, boardPixelHeight, wallColor);
    
    // Render units using UnitRenderer
    for (const auto& unit : m_units) {
        int screenX = m_offsetX + unit->GetPosition().x * CELL_SIZE;
        int screenY = m_offsetY + unit->GetPosition().y * CELL_SIZE;
        
        if (m_unitRenderer) {
            m_unitRenderer->RenderUnit(renderer, *unit, screenX, screenY, CELL_SIZE);
        } else {
            // Fallback to unit's own render
            unit->Render(renderer, screenX, screenY, CELL_SIZE);
        }
    }
}

Cell& Board::GetCell(int x, int y) {
    return m_cells[y][x];
}

const Cell& Board::GetCell(int x, int y) const {
    return m_cells[y][x];
}

bool Board::IsValidPosition(int x, int y) const {
    return x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT;
}

bool Board::IsValidPlacement(const Dice& dice, int x, int y, Owner owner) const {
    // Check if target cell is valid
    if (!IsValidPosition(x, y)) return false;
    
    // Check if cell is occupied
    if (GetUnitAt(x, y)) return false;
    
    // Get unfurl pattern
    const auto& unfurl = dice.GetCurrentFace().unfurl;
    int north = unfurl[0], east = unfurl[1], south = unfurl[2], west = unfurl[3];
    
    // Check if unfurl fits within bounds
    if (y - north < 0 || y + south >= HEIGHT ||
        x - west < 0 || x + east >= WIDTH) {
        return false;
    }
    
    // Check if all unfurl cells are clear
    auto unfurlCells = GetUnfurlCells(x, y, unfurl);
    for (const auto& pos : unfurlCells) {
        if (GetUnitAt(pos.x, pos.y)) return false;
    }
    
    // First placement: must be adjacent to start zone or existing territory
    auto ownerUnits = GetUnitsByOwner(owner);
    if (ownerUnits.empty()) {
        // First unit - must be near start zone
        Position startZone = (owner == Owner::Player) ? PLAYER_START_ZONE : ENEMY_START_ZONE;
        int distance = GetDistance({x, y}, startZone);
        return distance <= 3;
    }
    
    // Check if placement connects to existing territory
    for (const auto& pos : unfurlCells) {
        auto neighbors = GetNeighbors(pos);
        for (const auto& neighbor : neighbors) {
            if (m_cells[neighbor.y][neighbor.x].owner == owner) {
                return true;
            }
        }
    }
    
    return false;
}

bool Board::PlaceDice(std::shared_ptr<Dice> dice, int x, int y, Owner owner) {
    if (!IsValidPlacement(*dice, x, y, owner)) {
        return false;
    }
    
    // Create unit from dice
    auto unit = dice->CreateUnit();
    unit->SetPosition({x, y});
    unit->SetOwner(owner);
    
    // Place unit
    m_cells[y][x].occupant = unit;
    m_units.push_back(unit);
    
    // Apply unfurl
    auto unfurlCells = GetUnfurlCells(x, y, dice->GetCurrentFace().unfurl);
    ConvertTerritory(unfurlCells, owner);
    
    Logger::Info("Placed {} at ({}, {})", dice->GetClassName(), x, y);
    return true;
}

std::vector<Position> Board::GetUnfurlCells(int x, int y, const std::array<int, 4>& pattern) const {
    std::vector<Position> cells;
    cells.push_back({x, y});  // Center cell
    
    int north = pattern[0], east = pattern[1], south = pattern[2], west = pattern[3];
    
    // North
    for (int i = 1; i <= north && IsValidPosition(x, y - i); ++i) {
        cells.push_back({x, y - i});
    }
    // East
    for (int i = 1; i <= east && IsValidPosition(x + i, y); ++i) {
        cells.push_back({x + i, y});
    }
    // South
    for (int i = 1; i <= south && IsValidPosition(x, y + i); ++i) {
        cells.push_back({x, y + i});
    }
    // West
    for (int i = 1; i <= west && IsValidPosition(x - i, y); ++i) {
        cells.push_back({x - i, y});
    }
    
    return cells;
}

void Board::ConvertTerritory(const std::vector<Position>& cells, Owner newOwner) {
    for (const auto& pos : cells) {
        if (IsValidPosition(pos.x, pos.y)) {
            Owner oldOwner = m_cells[pos.y][pos.x].owner;
            m_cells[pos.y][pos.x].owner = newOwner;
            
            // Deal damage to enemy units standing on converted territory
            if (oldOwner != Owner::None && oldOwner != newOwner) {
                auto unit = GetUnitAt(pos.x, pos.y);
                if (unit && unit->GetOwner() != newOwner) {
                    unit->TakeDamage(1);  // Territory damage
                    Logger::Info("Territory damage to {} at ({}, {})", 
                                 unit->GetClassName(), pos.x, pos.y);
                }
            }
        }
    }
}

std::vector<std::shared_ptr<Unit>> Board::GetUnitsByOwner(Owner owner) const {
    std::vector<std::shared_ptr<Unit>> result;
    for (const auto& unit : m_units) {
        if (unit->GetOwner() == owner) {
            result.push_back(unit);
        }
    }
    return result;
}

std::shared_ptr<Unit> Board::GetUnitAt(int x, int y) const {
    if (!IsValidPosition(x, y)) return nullptr;
    
    for (const auto& unit : m_units) {
        if (unit->GetPosition().x == x && unit->GetPosition().y == y) {
            return unit;
        }
    }
    return nullptr;
}

void Board::RemoveUnit(const std::shared_ptr<Unit>& unit) {
    auto it = std::find(m_units.begin(), m_units.end(), unit);
    if (it != m_units.end()) {
        Position pos = unit->GetPosition();
        m_cells[pos.y][pos.x].occupant = nullptr;
        m_units.erase(it);
        Logger::Info("Removed unit {} from ({}, {})", unit->GetClassName(), pos.x, pos.y);
    }
}

std::vector<Position> Board::GetValidMoveCells(const Unit& unit) const {
    std::vector<Position> validCells;
    const Position& start = unit.GetPosition();
    int moveRange = unit.GetStats().mov;
    
    // BFS to find all reachable cells
    std::queue<std::pair<Position, int>> queue;
    std::vector<std::vector<bool>> visited(HEIGHT, std::vector<bool>(WIDTH, false));
    
    queue.push({start, 0});
    visited[start.y][start.x] = true;
    
    while (!queue.empty()) {
        auto [pos, dist] = queue.front();
        queue.pop();
        
        if (dist > 0 && !GetUnitAt(pos.x, pos.y)) {
            validCells.push_back(pos);
        }
        
        if (dist < moveRange) {
            for (const auto& neighbor : GetNeighbors(pos)) {
                if (!visited[neighbor.y][neighbor.x] && IsWalkable(neighbor.x, neighbor.y)) {
                    visited[neighbor.y][neighbor.x] = true;
                    queue.push({neighbor, dist + 1});
                }
            }
        }
    }
    
    return validCells;
}

std::vector<Position> Board::GetValidAttackTargets(const Unit& unit) const {
    std::vector<Position> targets;
    const Position& pos = unit.GetPosition();
    int range = unit.GetStats().rng;
    Owner owner = unit.GetOwner();
    
    // Check all cells within range
    for (int dy = -range; dy <= range; ++dy) {
        for (int dx = -range; dx <= range; ++dx) {
            int nx = pos.x + dx;
            int ny = pos.y + dy;
            
            if (!IsValidPosition(nx, ny)) continue;
            if (GetDistance(pos, {nx, ny}) > range) continue;
            
            auto target = GetUnitAt(nx, ny);
            if (target && target->GetOwner() != owner) {
                // For ranged attacks, check line of sight
                if (range > 1 && !HasLineOfSight(pos, {nx, ny})) continue;
                targets.push_back({nx, ny});
            }
        }
    }
    
    return targets;
}

int Board::GetDistance(const Position& a, const Position& b) const {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

bool Board::HasLineOfSight(const Position& from, const Position& to) const {
    // Simple Bresenham line check for blocking units
    int dx = std::abs(to.x - from.x);
    int dy = std::abs(to.y - from.y);
    int sx = from.x < to.x ? 1 : -1;
    int sy = from.y < to.y ? 1 : -1;
    int err = dx - dy;
    
    int x = from.x, y = from.y;
    
    while (x != to.x || y != to.y) {
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
        
        // Skip start and end positions
        if ((x == to.x && y == to.y) || (x == from.x && y == from.y)) continue;
        
        // Check for blocking unit
        if (GetUnitAt(x, y)) return false;
    }
    
    return true;
}

Position Board::ScreenToGrid(int screenX, int screenY) const {
    return {
        (screenX - m_offsetX) / CELL_SIZE,
        (screenY - m_offsetY) / CELL_SIZE
    };
}

Position Board::GridToScreen(int gridX, int gridY) const {
    return {
        m_offsetX + gridX * CELL_SIZE,
        m_offsetY + gridY * CELL_SIZE
    };
}

bool Board::IsWalkable(int x, int y) const {
    if (!IsValidPosition(x, y)) return false;
    return GetUnitAt(x, y) == nullptr;
}

std::vector<Position> Board::GetNeighbors(const Position& pos) const {
    std::vector<Position> neighbors;
    const int dx[] = {0, 1, 0, -1};
    const int dy[] = {-1, 0, 1, 0};
    
    for (int i = 0; i < 4; ++i) {
        int nx = pos.x + dx[i];
        int ny = pos.y + dy[i];
        if (IsValidPosition(nx, ny)) {
            neighbors.push_back({nx, ny});
        }
    }
    
    return neighbors;
}

void Board::MoveUnit(std::shared_ptr<Unit> unit, int x, int y) {
    if (!unit) return;
    
    // Update cell occupancy
    Position oldPos = unit->GetPosition();
    m_cells[oldPos.y][oldPos.x].occupant = nullptr;
    
    // Move unit
    unit->SetPosition({x, y});
    m_cells[y][x].occupant = unit.get();
}

void Board::RenderSelectionHighlights(Renderer& renderer,
                                       const Position& hoveredCell,
                                       bool isHovering,
                                       const std::vector<Position>& validMoves,
                                       const std::vector<Position>& validAttacks,
                                       const std::vector<Position>& validPlacements) {
    // Draw valid movement cells (blue)
    for (const auto& pos : validMoves) {
        int screenX, screenY;
        GetCellScreenPosition(pos.x, pos.y, screenX, screenY);
        
        SDL_Color moveColor = {52, 152, 219, 100};  // Blue
        renderer.FillRect(screenX + 2, screenY + 2, m_cellSize - 4, m_cellSize - 4, moveColor);
        
        // Border
        SDL_Color moveBorder = {52, 152, 219, 200};
        renderer.DrawRect(screenX + 2, screenY + 2, m_cellSize - 4, m_cellSize - 4, moveBorder);
    }
    
    // Draw valid attack cells (red)
    for (const auto& pos : validAttacks) {
        int screenX, screenY;
        GetCellScreenPosition(pos.x, pos.y, screenX, screenY);
        
        SDL_Color attackColor = {231, 76, 60, 100};  // Red
        renderer.FillRect(screenX + 2, screenY + 2, m_cellSize - 4, m_cellSize - 4, attackColor);
        
        // Border
        SDL_Color attackBorder = {231, 76, 60, 200};
        renderer.DrawRect(screenX + 2, screenY + 2, m_cellSize - 4, m_cellSize - 4, attackBorder);
    }
    
    // Draw valid placement cells (green)
    for (const auto& pos : validPlacements) {
        int screenX, screenY;
        GetCellScreenPosition(pos.x, pos.y, screenX, screenY);
        
        SDL_Color placeColor = {46, 204, 113, 100};  // Green
        renderer.FillRect(screenX + 2, screenY + 2, m_cellSize - 4, m_cellSize - 4, placeColor);
        
        // Border
        SDL_Color placeBorder = {46, 204, 113, 200};
        renderer.DrawRect(screenX + 2, screenY + 2, m_cellSize - 4, m_cellSize - 4, placeBorder);
    }
    
    // Draw hovered cell highlight (yellow/white)
    if (isHovering && IsValidPosition(hoveredCell.x, hoveredCell.y)) {
        int screenX, screenY;
        GetCellScreenPosition(hoveredCell.x, hoveredCell.y, screenX, screenY);
        
        // White border for hover
        SDL_Color hoverColor = {255, 255, 255, 180};
        renderer.DrawRect(screenX, screenY, m_cellSize, m_cellSize, hoverColor);
        renderer.DrawRect(screenX + 1, screenY + 1, m_cellSize - 2, m_cellSize - 2, hoverColor);
        
        // Corner highlights
        int cornerSize = 8;
        SDL_Color cornerColor = {255, 215, 0, 255};  // Gold
        
        // Top-left
        renderer.FillRect(screenX, screenY, cornerSize, 2, cornerColor);
        renderer.FillRect(screenX, screenY, 2, cornerSize, cornerColor);
        
        // Top-right
        renderer.FillRect(screenX + m_cellSize - cornerSize, screenY, cornerSize, 2, cornerColor);
        renderer.FillRect(screenX + m_cellSize - 2, screenY, 2, cornerSize, cornerColor);
        
        // Bottom-left
        renderer.FillRect(screenX, screenY + m_cellSize - 2, cornerSize, 2, cornerColor);
        renderer.FillRect(screenX, screenY + m_cellSize - cornerSize, 2, cornerSize, cornerColor);
        
        // Bottom-right
        renderer.FillRect(screenX + m_cellSize - cornerSize, screenY + m_cellSize - 2, cornerSize, 2, cornerColor);
        renderer.FillRect(screenX + m_cellSize - 2, screenY + m_cellSize - cornerSize, 2, cornerSize, cornerColor);
    }
}

void Board::GetCellScreenPosition(int gridX, int gridY, int& screenX, int& screenY) const {
    screenX = m_offsetX + gridX * m_cellSize;
    screenY = m_offsetY + gridY * m_cellSize;
}

void Board::RenderUnfurlPreview(Renderer& renderer, const Position& center,
                                 const std::array<int, 4>& unfurl, bool isValid) {
    // Get all cells that would be affected
    auto cells = GetUnfurlCells(center.x, center.y, unfurl);
    
    // Choose color based on validity
    SDL_Color fillColor = isValid ? SDL_Color{46, 204, 113, 80} : SDL_Color{231, 76, 60, 80};
    SDL_Color borderColor = isValid ? SDL_Color{46, 204, 113, 200} : SDL_Color{231, 76, 60, 200};
    
    // Draw all unfurl cells
    for (const auto& pos : cells) {
        int screenX, screenY;
        GetCellScreenPosition(pos.x, pos.y, screenX, screenY);
        
        renderer.FillRect(screenX + 1, screenY + 1, m_cellSize - 2, m_cellSize - 2, fillColor);
        renderer.DrawRect(screenX + 1, screenY + 1, m_cellSize - 2, m_cellSize - 2, borderColor);
    }
    
    // Highlight the center cell (where unit spawns) more prominently
    int centerX, centerY;
    GetCellScreenPosition(center.x, center.y, centerX, centerY);
    
    SDL_Color unitColor = isValid ? SDL_Color{255, 215, 0, 150} : SDL_Color{255, 100, 100, 150};
    renderer.FillRect(centerX + 4, centerY + 4, m_cellSize - 8, m_cellSize - 8, unitColor);
    
    // Draw a star or marker in center
    SDL_Color markerColor = Colors::White;
    int cx = centerX + m_cellSize / 2;
    int cy = centerY + m_cellSize / 2;
    renderer.FillRect(cx - 6, cy - 2, 12, 4, markerColor);
    renderer.FillRect(cx - 2, cy - 6, 4, 12, markerColor);
}

} // namespace DDD
