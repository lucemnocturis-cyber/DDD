#include "DiceCard.h"
#include "../Graphics/Renderer.h"
#include "../Gameplay/Dice.h"
#include "../Gameplay/DiceDatabase.h"

#include <algorithm>
#include <cmath>

namespace DDD {

DiceCard::DiceCard() = default;

DiceCard::DiceCard(std::shared_ptr<Dice> dice)
    : m_dice(dice)
{
}

void DiceCard::Update(float deltaTime, int mouseX, int mouseY, bool mouseDown) {
    if (!m_visible || !m_dice) return;
    
    bool wasHovered = m_hovered;
    m_hovered = m_enabled && Contains(mouseX, mouseY);
    
    // Handle click
    if (m_hovered && !mouseDown && m_wasPressed) {
        if (m_onClick) {
            m_onClick(m_dice);
        }
    }
    m_wasPressed = mouseDown && m_hovered;
    
    // Animate hover offset
    float targetOffset = m_hovered ? -8.0f : 0.0f;
    m_hoverOffset += (targetOffset - m_hoverOffset) * deltaTime * 15.0f;
    
    // Animate selection pulse
    if (m_selected) {
        m_selectPulse += deltaTime * 4.0f;
    } else {
        m_selectPulse = 0.0f;
    }
    
    // Rarity glow animation
    m_rarityGlow += deltaTime * 2.0f;
}

void DiceCard::Render(Renderer& renderer) {
    if (!m_visible || !m_dice) return;
    
    RenderRarityBorder(renderer);
    RenderBackground(renderer);
    RenderClassName(renderer);
    RenderStats(renderer);
    RenderUnfurlPattern(renderer);
    RenderCost(renderer);
    
    if (m_selected) {
        RenderSelectionHighlight(renderer);
    }
}

bool DiceCard::Contains(int x, int y) const {
    int drawY = m_y + static_cast<int>(m_hoverOffset);
    return x >= m_x && x < m_x + m_width &&
           y >= drawY && y < drawY + m_height;
}

void DiceCard::RenderBackground(Renderer& renderer) {
    int drawY = m_y + static_cast<int>(m_hoverOffset);
    
    // Shadow
    if (!m_wasPressed) {
        SDL_Color shadowColor = {0, 0, 0, 100};
        renderer.FillRect(m_x + 4, drawY + 4, m_width, m_height, shadowColor);
    }
    
    // Card background
    SDL_Color bgColor = m_enabled ? SDL_Color{40, 40, 55, 255} : SDL_Color{30, 30, 35, 255};
    if (m_hovered && m_enabled) {
        bgColor = {50, 50, 70, 255};
    }
    renderer.FillRect(m_x, drawY, m_width, m_height, bgColor);
    
    // Border
    SDL_Color borderColor = GetClassColor();
    if (!m_enabled) {
        borderColor = {80, 80, 80, 255};
    }
    renderer.DrawRect(m_x, drawY, m_width, m_height, borderColor);
    renderer.DrawRect(m_x + 1, drawY + 1, m_width - 2, m_height - 2, borderColor);
    
    // Top color bar (class indicator)
    renderer.FillRect(m_x + 2, drawY + 2, m_width - 4, 6, borderColor);
    
    // Inner gradient effect
    SDL_Color highlight = {255, 255, 255, 15};
    renderer.FillRect(m_x + 2, drawY + 10, m_width - 4, 2, highlight);
}

void DiceCard::RenderClassName(Renderer& renderer) {
    if (!m_dice) return;
    
    int drawY = m_y + static_cast<int>(m_hoverOffset);
    
    // Class name
    SDL_Color textColor = m_enabled ? Colors::White : Colors::Gray;
    std::string className = m_dice->GetClassName();
    
    // Truncate if too long
    if (className.length() > 12) {
        className = className.substr(0, 10) + "..";
    }
    
    renderer.DrawTextWithShadow(className, m_x + m_width / 2, drawY + 18, 
                                 textColor, FontSize::Medium, TextAlign::Center);
}

void DiceCard::RenderStats(Renderer& renderer) {
    if (!m_dice) return;
    
    int drawY = m_y + static_cast<int>(m_hoverOffset);
    const auto& face = m_dice->GetCurrentFace();
    
    int statsY = drawY + 40;
    int leftX = m_x + 10;
    int rightX = m_x + m_width / 2 + 5;
    
    SDL_Color statColor = m_enabled ? SDL_Color{200, 200, 220, 255} : Colors::Gray;
    SDL_Color valueColor = m_enabled ? Colors::White : Colors::Gray;
    
    // HP
    renderer.DrawText("HP", leftX, statsY, Colors::Green, FontSize::Small, TextAlign::Left);
    renderer.DrawText(std::to_string(face.stats.hp), leftX + 25, statsY, valueColor, FontSize::Small, TextAlign::Left);
    
    // ATK
    renderer.DrawText("ATK", rightX, statsY, Colors::Red, FontSize::Small, TextAlign::Left);
    renderer.DrawText(std::to_string(face.stats.atk), rightX + 30, statsY, valueColor, FontSize::Small, TextAlign::Left);
    
    // DEF
    statsY += 16;
    renderer.DrawText("DEF", leftX, statsY, Colors::Blue, FontSize::Small, TextAlign::Left);
    renderer.DrawText(std::to_string(face.stats.def), leftX + 30, statsY, valueColor, FontSize::Small, TextAlign::Left);
    
    // MOV
    renderer.DrawText("MOV", rightX, statsY, Colors::Cyan, FontSize::Small, TextAlign::Left);
    renderer.DrawText(std::to_string(face.stats.mov), rightX + 30, statsY, valueColor, FontSize::Small, TextAlign::Left);
    
    // RNG
    statsY += 16;
    renderer.DrawText("RNG", leftX, statsY, Colors::Orange, FontSize::Small, TextAlign::Left);
    renderer.DrawText(std::to_string(face.stats.rng), leftX + 30, statsY, valueColor, FontSize::Small, TextAlign::Left);
}

void DiceCard::RenderUnfurlPattern(Renderer& renderer) {
    if (!m_dice) return;
    
    int drawY = m_y + static_cast<int>(m_hoverOffset);
    const auto& face = m_dice->GetCurrentFace();
    const auto& unfurl = face.unfurl;
    
    // Unfurl pattern display area
    int patternX = m_x + m_width / 2;
    int patternY = drawY + 115;
    int cellSize = 12;
    int maxExtent = std::max({unfurl[0], unfurl[1], unfurl[2], unfurl[3], 1});
    
    // Label
    SDL_Color labelColor = {150, 150, 170, 255};
    renderer.DrawText("Unfurl", m_x + m_width / 2, patternY - 15, labelColor, FontSize::Small, TextAlign::Center);
    
    // Draw unfurl pattern (cross shape)
    SDL_Color cellColor = m_enabled ? GetClassColor() : SDL_Color{80, 80, 80, 255};
    SDL_Color emptyColor = {60, 60, 70, 255};
    
    // Center cell (unit spawns here)
    renderer.FillRect(patternX - cellSize/2, patternY - cellSize/2, cellSize, cellSize, Colors::Gold);
    renderer.DrawRect(patternX - cellSize/2, patternY - cellSize/2, cellSize, cellSize, Colors::White);
    
    // North cells
    for (int i = 1; i <= unfurl[0]; i++) {
        int cy = patternY - i * cellSize - cellSize/2;
        renderer.FillRect(patternX - cellSize/2, cy, cellSize, cellSize, cellColor);
        renderer.DrawRect(patternX - cellSize/2, cy, cellSize, cellSize, Colors::White);
    }
    
    // East cells
    for (int i = 1; i <= unfurl[1]; i++) {
        int cx = patternX + i * cellSize - cellSize/2;
        renderer.FillRect(cx, patternY - cellSize/2, cellSize, cellSize, cellColor);
        renderer.DrawRect(cx, patternY - cellSize/2, cellSize, cellSize, Colors::White);
    }
    
    // South cells
    for (int i = 1; i <= unfurl[2]; i++) {
        int cy = patternY + i * cellSize - cellSize/2;
        renderer.FillRect(patternX - cellSize/2, cy, cellSize, cellSize, cellColor);
        renderer.DrawRect(patternX - cellSize/2, cy, cellSize, cellSize, Colors::White);
    }
    
    // West cells
    for (int i = 1; i <= unfurl[3]; i++) {
        int cx = patternX - i * cellSize - cellSize/2;
        renderer.FillRect(cx, patternY - cellSize/2, cellSize, cellSize, cellColor);
        renderer.DrawRect(cx, patternY - cellSize/2, cellSize, cellSize, Colors::White);
    }
}

void DiceCard::RenderCost(Renderer& renderer) {
    if (!m_dice) return;
    
    int drawY = m_y + static_cast<int>(m_hoverOffset);
    int cost = m_dice->GetCost();
    
    // Cost badge in bottom right
    int badgeX = m_x + m_width - 30;
    int badgeY = drawY + m_height - 25;
    
    // Badge background
    SDL_Color badgeBg = {180, 140, 50, 255};
    renderer.FillRect(badgeX, badgeY, 25, 20, badgeBg);
    renderer.DrawRect(badgeX, badgeY, 25, 20, Colors::Gold);
    
    // Cost number
    renderer.DrawText(std::to_string(cost), badgeX + 12, badgeY + 2, Colors::White, FontSize::Medium, TextAlign::Center);
}

void DiceCard::RenderSelectionHighlight(Renderer& renderer) {
    int drawY = m_y + static_cast<int>(m_hoverOffset);
    
    // Pulsing border
    float pulse = (std::sin(m_selectPulse) + 1.0f) * 0.5f;
    uint8_t alpha = static_cast<uint8_t>(150 + pulse * 105);
    
    SDL_Color highlightColor = {255, 215, 0, alpha};
    
    // Draw multiple layers for glow effect
    for (int i = 0; i < 3; i++) {
        renderer.DrawRect(m_x - i - 1, drawY - i - 1, 
                         m_width + (i + 1) * 2, m_height + (i + 1) * 2, 
                         highlightColor);
    }
    
    // Corner diamonds
    int cornerSize = 8;
    SDL_Color cornerColor = Colors::Gold;
    
    // Top-left
    renderer.FillRect(m_x - 4, drawY - 4, cornerSize, cornerSize, cornerColor);
    // Top-right
    renderer.FillRect(m_x + m_width - 4, drawY - 4, cornerSize, cornerSize, cornerColor);
    // Bottom-left
    renderer.FillRect(m_x - 4, drawY + m_height - 4, cornerSize, cornerSize, cornerColor);
    // Bottom-right
    renderer.FillRect(m_x + m_width - 4, drawY + m_height - 4, cornerSize, cornerSize, cornerColor);
}

SDL_Color DiceCard::GetClassColor() const {
    if (!m_dice) return {128, 128, 128, 255};
    
    std::string className = m_dice->GetClassName();
    
    // Mage line (blue/purple)
    if (className == "Mage" || className == "Wizard" || className == "Sorcerer" ||
        className == "Warlock" || className == "Archmage" || className == "Lich") {
        return {100, 100, 220, 255};
    }
    
    // Soldier line (red/brown)
    if (className == "Soldier" || className == "Knight" || className == "Berserker" ||
        className == "Lord" || className == "Warlord") {
        return {200, 80, 80, 255};
    }
    
    // Rogue line (green/teal)
    if (className == "Rogue" || className == "Assassin" || className == "Ninja" ||
        className == "Phantom" || className == "Shadow") {
        return {80, 180, 100, 255};
    }
    
    // Archer line (purple)
    if (className == "Scout" || className == "Ranger" || className == "Sniper" ||
        className == "Deadeye" || className == "Warden") {
        return {160, 80, 200, 255};
    }
    
    // Healer line (gold/white)
    if (className == "Cleric" || className == "Priest" || className == "Paladin" ||
        className == "High Priest" || className == "Holy Knight") {
        return {220, 200, 100, 255};
    }
    
    // Tank line (gray/steel)
    if (className == "Guard" || className == "Sentinel" || className == "Champion" ||
        className == "Fortress" || className == "Titan") {
        return {120, 120, 150, 255};
    }
    
    return {150, 150, 150, 255};  // Default gray
}

void DiceCard::RenderRarityBorder(Renderer& renderer) {
    if (m_diceId.empty()) return;
    
    auto& db = DiceDatabase::Instance();
    const DiceDef* def = db.GetDiceDef(m_diceId);
    if (!def || def->rarity == DiceRarity::Common) return;
    
    int drawY = m_y + static_cast<int>(m_hoverOffset);
    SDL_Color rarityColor = DiceDatabase::GetRarityColor(def->rarity);
    
    // Pulsing glow for rare+ dice
    float pulse = (std::sin(m_rarityGlow) + 1.0f) * 0.5f;
    int glowSize = 3 + static_cast<int>(pulse * 2);
    
    // Outer glow
    for (int i = glowSize; i > 0; --i) {
        uint8_t alpha = static_cast<uint8_t>((glowSize - i + 1) * 30);
        SDL_Color glowColor = {rarityColor.r, rarityColor.g, rarityColor.b, alpha};
        renderer.DrawRect(m_x - i, drawY - i, m_width + i * 2, m_height + i * 2, glowColor);
    }
    
    // Solid border
    renderer.DrawRect(m_x - 1, drawY - 1, m_width + 2, m_height + 2, rarityColor);
    renderer.DrawRect(m_x - 2, drawY - 2, m_width + 4, m_height + 4, rarityColor);
}

} // namespace DDD
