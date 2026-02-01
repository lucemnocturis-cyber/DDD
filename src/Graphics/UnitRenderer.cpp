#include "UnitRenderer.h"
#include "Renderer.h"
#include "TextRenderer.h"
#include "../Gameplay/Unit.h"
#include "../Utils/Logger.h"

#include <cmath>
#include <algorithm>

namespace DDD {

UnitRenderer::UnitRenderer() = default;
UnitRenderer::~UnitRenderer() = default;

void UnitRenderer::Initialize(SDL_Renderer* sdlRenderer) {
    m_sdlRenderer = sdlRenderer;
    Logger::Info("UnitRenderer initialized");
}

void UnitRenderer::Update(float deltaTime) {
    m_globalTime += deltaTime;
    
    // Update animation timers
    for (auto& [unit, timer] : m_unitAnimTimers) {
        timer += deltaTime;
    }
}

void UnitRenderer::SetUnitState(const Unit* unit, UnitVisualState state) {
    m_unitStates[unit] = state;
    m_unitAnimTimers[unit] = 0.0f;
}

void UnitRenderer::RenderUnit(Renderer& renderer, const Unit& unit, int x, int y, int cellSize) {
    // For now, use fallback rendering (colored shapes)
    // When sprites are added, this will check for loaded textures first
    RenderUnitFallback(renderer, unit, x, y, cellSize);
    RenderHealthBar(renderer, unit, x, y, cellSize);
    RenderStatusIndicators(renderer, unit, x, y, cellSize);
}

void UnitRenderer::RenderUnitFallback(Renderer& renderer, const Unit& unit, int x, int y, int cellSize) {
    SDL_Color classColor = GetClassColor(unit.GetClassName());
    bool isEnemy = (unit.GetOwner() == Owner::Enemy);
    
    // Get animation state
    auto stateIt = m_unitStates.find(&unit);
    UnitVisualState state = (stateIt != m_unitStates.end()) ? stateIt->second : UnitVisualState::Idle;
    
    float animTimer = 0.0f;
    auto timerIt = m_unitAnimTimers.find(&unit);
    if (timerIt != m_unitAnimTimers.end()) {
        animTimer = timerIt->second;
    }
    
    // Unit dimensions
    int padding = 4;
    int unitSize = cellSize - padding * 2;
    int unitX = x + padding;
    int unitY = y + padding;
    
    // Idle bobbing animation
    if (state == UnitVisualState::Idle && !unit.IsDead()) {
        float bob = std::sin(m_globalTime * 3.0f + reinterpret_cast<std::uintptr_t>(&unit) * 0.1f) * 2.0f;
        unitY += static_cast<int>(bob);
    }
    
    // Shake on damage
    if (state == UnitVisualState::TakingDamage && animTimer < 0.3f) {
        int shake = static_cast<int>((std::rand() % 5) - 2);
        unitX += shake;
    }
    
    // Fade on death
    uint8_t alpha = 255;
    if (state == UnitVisualState::Dying) {
        alpha = static_cast<uint8_t>(std::max(0.0f, 1.0f - animTimer * 2.0f) * 255);
    }
    
    // Draw shadow
    SDL_Color shadowColor = {0, 0, 0, static_cast<uint8_t>(alpha / 3)};
    renderer.FillRect(unitX + 2, unitY + unitSize - 4, unitSize, 6, shadowColor);
    
    // Draw unit body based on class archetype
    char icon = GetClassIcon(unit.GetClassName());
    
    // Background shape (circle for mages, diamond for rogues, square for soldiers)
    SDL_Color bodyColor = {classColor.r, classColor.g, classColor.b, alpha};
    SDL_Color darkColor = {
        static_cast<uint8_t>(classColor.r * 0.6f),
        static_cast<uint8_t>(classColor.g * 0.6f),
        static_cast<uint8_t>(classColor.b * 0.6f),
        alpha
    };
    
    // Draw body
    int centerX = unitX + unitSize / 2;
    int centerY = unitY + unitSize / 2;
    int radius = unitSize / 2 - 2;
    
    if (icon == 'M' || icon == 'E' || icon == 'W' || icon == 'S') {
        // Mage types - circular
        renderer.FillCircle(centerX, centerY, radius, bodyColor);
        renderer.DrawCircle(centerX, centerY, radius, darkColor);
    } else if (icon == 'R' || icon == 'A' || icon == 'T' || icon == 'N') {
        // Rogue types - diamond
        int halfSize = radius;
        SDL_Point points[4] = {
            {centerX, centerY - halfSize},
            {centerX + halfSize, centerY},
            {centerX, centerY + halfSize},
            {centerX - halfSize, centerY}
        };
        // Fill diamond (approximate with rect rotated - simplified)
        for (int i = 0; i < halfSize; i++) {
            int w = (halfSize - i) * 2;
            renderer.FillRect(centerX - (halfSize - i), centerY - i, w, 1, bodyColor);
            renderer.FillRect(centerX - (halfSize - i), centerY + i, w, 1, bodyColor);
        }
        // Border
        renderer.DrawLine(points[0].x, points[0].y, points[1].x, points[1].y, darkColor);
        renderer.DrawLine(points[1].x, points[1].y, points[2].x, points[2].y, darkColor);
        renderer.DrawLine(points[2].x, points[2].y, points[3].x, points[3].y, darkColor);
        renderer.DrawLine(points[3].x, points[3].y, points[0].x, points[0].y, darkColor);
    } else {
        // Soldier types - square with rounded feeling
        renderer.FillRect(unitX + 2, unitY + 2, unitSize - 4, unitSize - 4, bodyColor);
        renderer.DrawRect(unitX + 2, unitY + 2, unitSize - 4, unitSize - 4, darkColor);
    }
    
    // Draw class icon in center
    SDL_Color textColor = {255, 255, 255, alpha};
    std::string iconStr(1, icon);
    renderer.DrawTextWithOutline(iconStr, centerX, centerY - 8, textColor, 
                                  {0, 0, 0, alpha}, FontSize::Medium, TextAlign::Center);
    
    // Draw enemy/ally indicator border
    SDL_Color ownerColor = isEnemy ? SDL_Color{200, 50, 50, alpha} : SDL_Color{50, 150, 200, alpha};
    int borderThickness = 2;
    for (int i = 0; i < borderThickness; i++) {
        renderer.DrawRect(unitX + i, unitY + i, unitSize - i * 2, unitSize - i * 2, ownerColor);
    }
    
    // Draw tier indicator (small dots)
    int tier = unit.GetTier();
    if (tier > 0) {
        SDL_Color tierColor = {255, 215, 0, alpha};  // Gold
        for (int i = 0; i < tier && i < 3; i++) {
            int dotX = unitX + 4 + i * 6;
            int dotY = unitY + unitSize - 6;
            renderer.FillRect(dotX, dotY, 4, 4, tierColor);
        }
    }
}

void UnitRenderer::RenderHealthBar(Renderer& renderer, const Unit& unit, int x, int y, int cellSize) {
    if (unit.IsDead()) return;
    
    int barWidth = cellSize - 8;
    int barHeight = 4;
    int barX = x + 4;
    int barY = y + cellSize - 6;
    
    // Background
    SDL_Color bgColor = {40, 40, 40, 200};
    renderer.FillRect(barX, barY, barWidth, barHeight, bgColor);
    
    // Health fill
    float healthPercent = static_cast<float>(unit.GetStats().hp) / unit.GetMaxHP();
    int fillWidth = static_cast<int>(barWidth * healthPercent);
    
    SDL_Color healthColor;
    if (healthPercent > 0.6f) {
        healthColor = {80, 200, 80, 255};  // Green
    } else if (healthPercent > 0.3f) {
        healthColor = {220, 180, 50, 255};  // Yellow/Orange
    } else {
        healthColor = {200, 60, 60, 255};  // Red
    }
    
    renderer.FillRect(barX, barY, fillWidth, barHeight, healthColor);
    
    // Border
    SDL_Color borderColor = {60, 60, 60, 255};
    renderer.DrawRect(barX, barY, barWidth, barHeight, borderColor);
}

void UnitRenderer::RenderStatusIndicators(Renderer& renderer, const Unit& unit, int x, int y, int cellSize) {
    // Small icons in corner for status effects
    int iconX = x + cellSize - 10;
    int iconY = y + 2;
    int iconSize = 8;
    int iconSpacing = 10;
    
    // Moved indicator
    if (unit.HasMoved()) {
        SDL_Color movedColor = {100, 100, 150, 200};
        renderer.FillRect(iconX, iconY, iconSize, iconSize, movedColor);
        iconY += iconSpacing;
    }
    
    // Attacked indicator
    if (unit.HasAttacked()) {
        SDL_Color attackedColor = {150, 100, 100, 200};
        renderer.FillRect(iconX, iconY, iconSize, iconSize, attackedColor);
        iconY += iconSpacing;
    }
    
    // "Can act" pulsing indicator
    if (!unit.HasMoved() || !unit.HasAttacked()) {
        float pulse = (std::sin(m_globalTime * 4.0f) + 1.0f) * 0.5f;
        uint8_t alpha = static_cast<uint8_t>(100 + pulse * 100);
        SDL_Color canActColor = {50, 200, 50, alpha};
        renderer.FillRect(x + 2, y + 2, 6, 6, canActColor);
    }
}

SDL_Color UnitRenderer::GetClassColor(const std::string& className) {
    // Mage tree - Blue/Purple
    if (className == "Mage") return {80, 100, 200, 255};
    if (className == "Wizard") return {100, 80, 220, 255};
    if (className == "Warlock") return {140, 60, 180, 255};
    if (className == "Archmage") return {120, 100, 255, 255};
    if (className == "Lich") return {100, 60, 160, 255};
    
    // Soldier tree - Red/Brown
    if (className == "Soldier") return {200, 80, 80, 255};
    if (className == "Knight") return {180, 100, 80, 255};
    if (className == "Berserker") return {220, 60, 60, 255};
    if (className == "Lord") return {200, 160, 100, 255};
    if (className == "Warlord") return {180, 40, 40, 255};
    
    // Rogue tree - Green/Teal
    if (className == "Rogue") return {80, 180, 100, 255};
    if (className == "Assassin") return {50, 140, 90, 255};
    if (className == "Ninja") return {60, 120, 140, 255};
    if (className == "Phantom") return {40, 100, 80, 255};
    if (className == "Shadow") return {50, 80, 100, 255};
    
    // Healer tree - Gold/White
    if (className == "Cleric") return {220, 200, 100, 255};
    if (className == "Priest") return {240, 220, 120, 255};
    if (className == "Paladin") return {200, 180, 80, 255};
    if (className == "High Priest") return {255, 240, 150, 255};
    if (className == "Holy Knight") return {220, 200, 60, 255};
    
    // Tank tree - Gray/Steel
    if (className == "Guard") return {120, 120, 140, 255};
    if (className == "Sentinel") return {100, 100, 130, 255};
    if (className == "Champion") return {140, 140, 160, 255};
    if (className == "Fortress") return {90, 90, 120, 255};
    if (className == "Titan") return {160, 160, 180, 255};
    
    // Archer tree - Purple
    if (className == "Scout") return {160, 80, 200, 255};
    if (className == "Sniper") return {140, 60, 180, 255};
    if (className == "Ranger") return {180, 100, 180, 255};
    if (className == "Deadeye") return {120, 40, 160, 255};
    if (className == "Warden") return {160, 120, 160, 255};
    
    // Default
    return {150, 150, 150, 255};
}

char UnitRenderer::GetClassIcon(const std::string& className) {
    // Mage tree
    if (className == "Mage") return 'M';
    if (className == "Wizard") return 'W';
    if (className == "Warlock") return 'L';
    if (className == "Archmage") return 'A';
    if (className == "Lich") return 'I';
    
    // Soldier tree
    if (className == "Soldier") return 'S';
    if (className == "Knight") return 'K';
    if (className == "Berserker") return 'B';
    if (className == "Lord") return 'D';
    if (className == "Warlord") return 'W';
    
    // Rogue tree
    if (className == "Rogue") return 'R';
    if (className == "Assassin") return 'A';
    if (className == "Ninja") return 'N';
    if (className == "Phantom") return 'P';
    if (className == "Shadow") return 'H';
    
    // Healer tree
    if (className == "Cleric") return 'C';
    if (className == "Priest") return 'P';
    if (className == "Paladin") return 'L';
    if (className == "High Priest") return 'H';
    if (className == "Holy Knight") return 'Y';
    
    // Tank tree
    if (className == "Guard") return 'G';
    if (className == "Sentinel") return 'E';
    if (className == "Champion") return 'C';
    if (className == "Fortress") return 'F';
    if (className == "Titan") return 'T';
    
    // Archer tree
    if (className == "Scout") return 'O';
    if (className == "Sniper") return 'I';
    if (className == "Ranger") return 'R';
    if (className == "Deadeye") return 'D';
    if (className == "Warden") return 'W';
    
    return '?';
}

} // namespace DDD
