#include "HUD.h"
#include "../Core/Game.h"
#include "../Gameplay/TurnManager.h"
#include "../Graphics/TextRenderer.h"

#include <algorithm>
#include <cmath>

namespace DDD {

HUD::HUD(Game& game)
    : m_game(game)
    , m_goldCounter(0)
    , m_scoreCounter(0)
{
}

void HUD::Update(float deltaTime) {
    m_pulseTime += deltaTime;
    
    // Update animated counters
    m_goldCounter.Update(deltaTime);
    m_scoreCounter.Update(deltaTime);
    
    // Update pop scales (decay back to 1.0)
    m_goldPopScale = 1.0f + (m_goldPopScale - 1.0f) * std::pow(0.1f, deltaTime);
    m_scorePopScale = 1.0f + (m_scorePopScale - 1.0f) * std::pow(0.1f, deltaTime);
    
    // Clamp to prevent floating point drift
    if (std::abs(m_goldPopScale - 1.0f) < 0.01f) m_goldPopScale = 1.0f;
    if (std::abs(m_scorePopScale - 1.0f) < 0.01f) m_scorePopScale = 1.0f;
    
    // Update floating numbers
    for (auto& num : m_floatingNumbers) {
        num.lifetime -= deltaTime;
        num.y -= static_cast<int>(50.0f * deltaTime);  // Float upward
    }
    
    // Remove expired numbers
    m_floatingNumbers.erase(
        std::remove_if(m_floatingNumbers.begin(), m_floatingNumbers.end(),
            [](const FloatingNumber& n) { return n.lifetime <= 0; }),
        m_floatingNumbers.end()
    );
    
    // Update message timer
    if (m_messageTimer > 0) {
        m_messageTimer -= deltaTime;
    }
}

void HUD::SetGold(int gold) {
    if (gold != m_goldCounter.GetActualValue()) {
        int diff = gold - m_goldCounter.GetActualValue();
        m_goldCounter.SetValue(gold, 0.4f);
        
        // Pop effect on increase
        if (diff > 0) {
            m_goldPopScale = 1.3f;
        }
    }
}

void HUD::SetScore(int score) {
    if (score != m_scoreCounter.GetActualValue()) {
        int diff = score - m_scoreCounter.GetActualValue();
        m_scoreCounter.SetValue(score, 0.5f);
        
        // Pop effect on increase
        if (diff > 0) {
            m_scorePopScale = 1.2f;
        }
    }
}

void HUD::Render(Renderer& renderer) {
    RenderTopBar(renderer);
    RenderTurnIndicator(renderer);
    RenderFloatingNumbers(renderer);
    RenderMessage(renderer);
}

void HUD::RenderTopBar(Renderer& renderer) {
    int screenWidth = renderer.GetWidth();
    
    // Background bar
    SDL_Color barBg = {20, 20, 40, 220};
    renderer.FillRect(0, 0, screenWidth, 50, barBg);
    
    // Bottom border
    SDL_Color borderColor = Colors::Gold;
    renderer.FillRect(0, 48, screenWidth, 2, borderColor);
    
    // Wave number (left side)
    std::string waveText = "WAVE " + std::to_string(m_game.GetCurrentWave());
    renderer.DrawTextWithOutline(waveText, 20, 12, Colors::White, Colors::Black, 
                                  FontSize::Large, TextAlign::Left);
    
    // Gold (center-left) with animated counter
    int goldDisplay = m_goldCounter.GetDisplayValue();
    std::string goldText = std::to_string(goldDisplay);
    
    // Gold icon (simple coin representation)
    renderer.FillRect(200, 14, 22, 22, Colors::Gold);
    renderer.FillRect(202, 16, 18, 18, Colors::Orange);
    
    // Gold text with pop effect
    SDL_Color goldColor = Colors::Gold;
    if (m_goldPopScale > 1.05f) {
        // Flash brighter when popping
        goldColor = {255, 255, 200, 255};
    }
    
    // Simulate scale by using larger font when popping
    FontSize goldFontSize = m_goldPopScale > 1.1f ? FontSize::XLarge : FontSize::Large;
    int goldY = m_goldPopScale > 1.1f ? 8 : 12;
    
    renderer.DrawTextWithOutline(goldText, 230, goldY, goldColor, Colors::Black,
                                  goldFontSize, TextAlign::Left);
    
    // Score (right side) with animated counter
    int scoreDisplay = m_scoreCounter.GetDisplayValue();
    std::string scoreText = "SCORE: " + std::to_string(scoreDisplay);
    
    SDL_Color scoreColor = Colors::White;
    if (m_scorePopScale > 1.05f) {
        scoreColor = {255, 255, 200, 255};
    }
    
    FontSize scoreFontSize = m_scorePopScale > 1.1f ? FontSize::XLarge : FontSize::Large;
    int scoreY = m_scorePopScale > 1.1f ? 8 : 12;
    
    renderer.DrawTextWithOutline(scoreText, screenWidth - 20, scoreY, scoreColor, Colors::Black,
                                  scoreFontSize, TextAlign::Right);
}

void HUD::RenderTurnIndicator(Renderer& renderer) {
    auto* turnManager = m_game.GetTurnManager();
    if (!turnManager) return;
    
    int screenWidth = renderer.GetWidth();
    bool isPlayerTurn = turnManager->IsPlayerTurn();
    
    // Turn indicator box
    int boxWidth = 200;
    int boxHeight = 40;
    int boxX = (screenWidth - boxWidth) / 2;
    int boxY = 60;
    
    SDL_Color boxColor = isPlayerTurn ? Colors::PlayerColor : Colors::EnemyColor;
    SDL_Color bgColor = {boxColor.r, boxColor.g, boxColor.b, 200};
    
    // Pulsing effect
    float pulse = (std::sin(m_pulseTime * 4.0f) + 1.0f) * 0.5f;
    bgColor.a = static_cast<uint8_t>(150 + pulse * 50);
    
    // Background
    renderer.FillRect(boxX, boxY, boxWidth, boxHeight, bgColor);
    
    // Border
    renderer.DrawRect(boxX, boxY, boxWidth, boxHeight, Colors::White);
    
    // Text
    std::string turnText = isPlayerTurn ? "YOUR TURN" : "ENEMY TURN";
    renderer.DrawTextWithOutline(turnText, screenWidth / 2, boxY + 8, 
                                  Colors::White, Colors::Black,
                                  FontSize::Large, TextAlign::Center);
}

void HUD::RenderFloatingNumbers(Renderer& renderer) {
    for (const auto& num : m_floatingNumbers) {
        // Calculate alpha based on remaining lifetime
        float alpha = std::min(1.0f, num.lifetime / num.maxLifetime * 2.0f);
        
        // Choose color: green for heal, red for damage, orange for critical
        SDL_Color color;
        if (num.isHeal) {
            color = Colors::Green;
        } else if (num.isCritical) {
            color = Colors::Orange;  // Critical hits are orange/gold
        } else {
            color = Colors::Red;
        }
        color.a = static_cast<uint8_t>(255 * alpha);
        
        // Build text
        std::string text;
        if (num.isHeal) {
            text = "+" + std::to_string(std::abs(num.value));
        } else {
            text = "-" + std::to_string(std::abs(num.value));
            if (num.isCritical) {
                text += "!";  // Add exclamation for crits
            }
        }
        
        // Scale based on damage amount and critical
        FontSize size = FontSize::Medium;
        if (num.isCritical) {
            size = FontSize::Title;  // Critical hits are always big
        } else if (std::abs(num.value) >= 20) {
            size = FontSize::Large;
        } else if (std::abs(num.value) >= 50) {
            size = FontSize::Title;
        }
        
        // Shake effect for critical hits
        int shakeX = 0, shakeY = 0;
        if (num.isCritical && num.lifetime > num.maxLifetime * 0.7f) {
            shakeX = (rand() % 5) - 2;
            shakeY = (rand() % 5) - 2;
        }
        
        renderer.DrawTextWithOutline(text, num.x + shakeX, num.y + shakeY, 
                                      color, Colors::Black, size, TextAlign::Center);
    }
}

void HUD::RenderMessage(Renderer& renderer) {
    if (m_messageTimer <= 0 || m_message.empty()) return;
    
    int screenWidth = renderer.GetWidth();
    int screenHeight = renderer.GetHeight();
    
    // Fade out effect
    float alpha = std::min(1.0f, m_messageTimer);
    
    // Background
    SDL_Color bgColor = {0, 0, 0, static_cast<uint8_t>(180 * alpha)};
    int boxWidth = 400;
    int boxHeight = 60;
    int boxX = (screenWidth - boxWidth) / 2;
    int boxY = screenHeight / 2 - boxHeight / 2;
    
    renderer.FillRect(boxX, boxY, boxWidth, boxHeight, bgColor);
    
    // Border
    SDL_Color borderColor = Colors::Gold;
    borderColor.a = static_cast<uint8_t>(255 * alpha);
    renderer.DrawRect(boxX, boxY, boxWidth, boxHeight, borderColor);
    
    // Text
    SDL_Color textColor = Colors::White;
    textColor.a = static_cast<uint8_t>(255 * alpha);
    renderer.DrawTextWithOutline(m_message, screenWidth / 2, boxY + 15,
                                  textColor, Colors::Black,
                                  FontSize::Large, TextAlign::Center);
}

void HUD::ShowDamageNumber(int x, int y, int damage, bool isCritical) {
    FloatingNumber num;
    num.x = x;
    num.y = y;
    num.value = damage;
    num.isHeal = false;
    num.isCritical = isCritical;
    num.lifetime = isCritical ? 2.0f : 1.5f;  // Critical hits stay longer
    num.maxLifetime = num.lifetime;
    
    m_floatingNumbers.push_back(num);
}

void HUD::ShowHealNumber(int x, int y, int amount) {
    FloatingNumber num;
    num.x = x;
    num.y = y;
    num.value = amount;
    num.isHeal = true;
    num.isCritical = false;
    num.lifetime = 1.5f;
    num.maxLifetime = 1.5f;
    
    m_floatingNumbers.push_back(num);
}

void HUD::ShowMessage(const std::string& message, float duration) {
    m_message = message;
    m_messageTimer = duration;
}

} // namespace DDD
