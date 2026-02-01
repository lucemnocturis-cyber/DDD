#include "UnitInfoPanel.h"
#include "../Graphics/Renderer.h"
#include "../Gameplay/Unit.h"

#include <algorithm>
#include <cmath>

namespace DDD {

UnitInfoPanel::UnitInfoPanel() = default;

void UnitInfoPanel::Update(float deltaTime) {
    m_time += deltaTime;
    
    // Slide in animation
    float targetSlide = m_unit ? 1.0f : 0.0f;
    m_slideIn += (targetSlide - m_slideIn) * deltaTime * 10.0f;
}

void UnitInfoPanel::Render(Renderer& renderer) {
    if (!m_visible || m_slideIn < 0.01f) return;
    
    // Apply slide animation (slide in from right)
    int slideOffset = static_cast<int>((1.0f - m_slideIn) * PANEL_WIDTH);
    int drawX = m_x + slideOffset;
    
    // Temporarily offset for rendering
    int originalX = m_x;
    m_x = drawX;
    
    RenderBackground(renderer);
    
    if (m_unit) {
        RenderUnitName(renderer);
        RenderHealthBar(renderer);
        RenderStats(renderer);
        RenderStatusIcons(renderer);
        RenderAbilityInfo(renderer);
    }
    
    m_x = originalX;
}

void UnitInfoPanel::SetUnit(std::shared_ptr<Unit> unit) {
    m_unit = unit;
}

void UnitInfoPanel::ClearUnit() {
    m_unit = nullptr;
}

void UnitInfoPanel::RenderBackground(Renderer& renderer) {
    // Panel background
    SDL_Color bgColor = {25, 25, 40, 240};
    renderer.FillRect(m_x, m_y, PANEL_WIDTH, PANEL_HEIGHT, bgColor);
    
    // Border with class color
    SDL_Color borderColor = m_unit ? GetClassColor() : SDL_Color{80, 80, 100, 255};
    renderer.DrawRect(m_x, m_y, PANEL_WIDTH, PANEL_HEIGHT, borderColor);
    renderer.DrawRect(m_x + 1, m_y + 1, PANEL_WIDTH - 2, PANEL_HEIGHT - 2, borderColor);
    
    // Top accent bar
    renderer.FillRect(m_x + 2, m_y + 2, PANEL_WIDTH - 4, 4, borderColor);
}

void UnitInfoPanel::RenderUnitName(Renderer& renderer) {
    if (!m_unit) return;
    
    // Unit name
    std::string name = m_unit->GetClassName();
    SDL_Color nameColor = GetClassColor();
    renderer.DrawTextWithShadow(name, m_x + PANEL_WIDTH / 2, m_y + 15, 
                                 nameColor, FontSize::Large, TextAlign::Center);
    
    // Level indicator
    std::string levelText = "Lv." + std::to_string(m_unit->GetLevel());
    renderer.DrawText(levelText, m_x + PANEL_WIDTH - 10, m_y + 12, 
                      Colors::Gray, FontSize::Small, TextAlign::Right);
    
    // Owner indicator
    std::string ownerText = (m_unit->GetOwner() == Owner::Player) ? "ALLY" : "ENEMY";
    SDL_Color ownerColor = (m_unit->GetOwner() == Owner::Player) ? Colors::Blue : Colors::Red;
    renderer.DrawText(ownerText, m_x + 10, m_y + 12, ownerColor, FontSize::Small, TextAlign::Left);
}

void UnitInfoPanel::RenderHealthBar(Renderer& renderer) {
    if (!m_unit) return;
    
    int barX = m_x + 10;
    int barY = m_y + 45;
    int barWidth = PANEL_WIDTH - 20;
    int barHeight = 16;
    
    // Background
    SDL_Color barBg = {40, 40, 50, 255};
    renderer.FillRect(barX, barY, barWidth, barHeight, barBg);
    
    // Health fill
    const auto& stats = m_unit->GetStats();
    float healthPercent = static_cast<float>(stats.hp) / m_unit->GetMaxHP();
    healthPercent = std::max(0.0f, std::min(1.0f, healthPercent));
    
    int fillWidth = static_cast<int>(barWidth * healthPercent);
    
    // Color based on health percentage
    SDL_Color healthColor;
    if (healthPercent > 0.6f) {
        healthColor = Colors::Green;
    } else if (healthPercent > 0.3f) {
        healthColor = Colors::Orange;
    } else {
        healthColor = Colors::Red;
    }
    
    renderer.FillRect(barX, barY, fillWidth, barHeight, healthColor);
    
    // Border
    renderer.DrawRect(barX, barY, barWidth, barHeight, Colors::White);
    
    // HP text
    std::string hpText = std::to_string(stats.hp) + "/" + std::to_string(m_unit->GetMaxHP());
    renderer.DrawTextWithShadow(hpText, barX + barWidth / 2, barY + 1, 
                                 Colors::White, FontSize::Small, TextAlign::Center);
}

void UnitInfoPanel::RenderStats(Renderer& renderer) {
    if (!m_unit) return;
    
    const auto& stats = m_unit->GetStats();
    int startY = m_y + 70;
    int leftX = m_x + 15;
    int rightX = m_x + PANEL_WIDTH / 2 + 10;
    int lineHeight = 20;
    
    // ATK
    renderer.DrawText("ATK", leftX, startY, Colors::Red, FontSize::Small, TextAlign::Left);
    renderer.DrawText(std::to_string(stats.atk), leftX + 40, startY, Colors::White, FontSize::Small, TextAlign::Left);
    
    // DEF
    renderer.DrawText("DEF", rightX, startY, Colors::Blue, FontSize::Small, TextAlign::Left);
    renderer.DrawText(std::to_string(stats.def), rightX + 40, startY, Colors::White, FontSize::Small, TextAlign::Left);
    
    // MOV
    startY += lineHeight;
    renderer.DrawText("MOV", leftX, startY, Colors::Cyan, FontSize::Small, TextAlign::Left);
    renderer.DrawText(std::to_string(stats.mov), leftX + 40, startY, Colors::White, FontSize::Small, TextAlign::Left);
    
    // RNG
    renderer.DrawText("RNG", rightX, startY, Colors::Orange, FontSize::Small, TextAlign::Left);
    renderer.DrawText(std::to_string(stats.rng), rightX + 40, startY, Colors::White, FontSize::Small, TextAlign::Left);
    
    // EXP bar
    startY += lineHeight + 5;
    int expBarWidth = PANEL_WIDTH - 30;
    int expBarHeight = 8;
    
    renderer.DrawText("EXP", leftX, startY, Colors::ExpYellow, FontSize::Small, TextAlign::Left);
    
    int expBarX = leftX + 35;
    renderer.FillRect(expBarX, startY + 2, expBarWidth - 35, expBarHeight, {40, 40, 50, 255});
    
    float expPercent = static_cast<float>(m_unit->GetExperience()) / m_unit->GetExpToNextLevel();
    int expFill = static_cast<int>((expBarWidth - 35) * expPercent);
    renderer.FillRect(expBarX, startY + 2, expFill, expBarHeight, Colors::ExpYellow);
    renderer.DrawRect(expBarX, startY + 2, expBarWidth - 35, expBarHeight, Colors::Gray);
}

void UnitInfoPanel::RenderStatusIcons(Renderer& renderer) {
    if (!m_unit) return;
    
    int iconY = m_y + 135;
    int iconX = m_x + 15;
    int iconSize = 20;
    int iconSpacing = 25;
    
    // "Moved" indicator
    if (m_unit->HasMoved()) {
        SDL_Color movedColor = {100, 100, 100, 200};
        renderer.FillRect(iconX, iconY, iconSize, iconSize, movedColor);
        renderer.DrawText("M", iconX + iconSize/2, iconY + 2, Colors::White, FontSize::Small, TextAlign::Center);
        iconX += iconSpacing;
    }
    
    // "Attacked" indicator
    if (m_unit->HasAttacked()) {
        SDL_Color attackedColor = {150, 80, 80, 200};
        renderer.FillRect(iconX, iconY, iconSize, iconSize, attackedColor);
        renderer.DrawText("A", iconX + iconSize/2, iconY + 2, Colors::White, FontSize::Small, TextAlign::Center);
        iconX += iconSpacing;
    }
    
    // "Can act" indicator
    if (!m_unit->HasMoved() || !m_unit->HasAttacked()) {
        float pulse = (std::sin(m_time * 4.0f) + 1.0f) * 0.5f;
        uint8_t alpha = static_cast<uint8_t>(150 + pulse * 105);
        SDL_Color canActColor = {50, 200, 100, alpha};
        renderer.FillRect(iconX, iconY, iconSize, iconSize, canActColor);
        renderer.DrawText("!", iconX + iconSize/2, iconY + 2, Colors::White, FontSize::Small, TextAlign::Center);
    }
}

void UnitInfoPanel::RenderAbilityInfo(Renderer& renderer) {
    if (!m_unit) return;
    
    // Ability name (if any)
    std::string abilityName = m_unit->GetAbilityName();
    if (!abilityName.empty()) {
        int abilityY = m_y + 165;
        
        // Divider line
        SDL_Color lineColor = {60, 60, 80, 255};
        renderer.FillRect(m_x + 10, abilityY - 5, PANEL_WIDTH - 20, 1, lineColor);
        
        // Ability label and name
        renderer.DrawText("Ability:", m_x + 15, abilityY, Colors::Gray, FontSize::Small, TextAlign::Left);
        renderer.DrawText(abilityName, m_x + 70, abilityY, Colors::Purple, FontSize::Small, TextAlign::Left);
    }
}

SDL_Color UnitInfoPanel::GetClassColor() const {
    if (!m_unit) return {150, 150, 150, 255};
    
    std::string className = m_unit->GetClassName();
    
    // Match DiceCard colors
    if (className == "Mage" || className == "Wizard" || className == "Sorcerer") {
        return {100, 100, 220, 255};
    }
    if (className == "Soldier" || className == "Knight" || className == "Paladin") {
        return {200, 80, 80, 255};
    }
    if (className == "Rogue" || className == "Assassin" || className == "Ninja") {
        return {80, 180, 100, 255};
    }
    if (className == "Archer" || className == "Ranger" || className == "Sniper") {
        return {220, 150, 50, 255};
    }
    if (className == "Cleric" || className == "Priest" || className == "Bishop") {
        return {220, 200, 100, 255};
    }
    
    return {150, 150, 150, 255};
}

} // namespace DDD
