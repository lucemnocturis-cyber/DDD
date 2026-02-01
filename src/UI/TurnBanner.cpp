#include "TurnBanner.h"
#include "../Graphics/Renderer.h"
#include "../Utils/Logger.h"

#include <cmath>

namespace DDD {

TurnBanner::TurnBanner() = default;

void TurnBanner::Show(const std::string& text, SDL_Color color, float displayDuration) {
    m_text = text;
    m_subText = "";
    m_color = color;
    m_displayDuration = displayDuration;
    m_active = true;
    m_state = State::SlidingIn;
    
    // Start slide in from left
    m_slideTween.Start(-800.0f, 0.0f, 0.3f, Easing::BackOut);
    m_alphaTween.Start(0.0f, 1.0f, 0.2f, Easing::QuadOut);
    m_displayTimer = 0.0f;
}

void TurnBanner::ShowPlayerTurn() {
    Show("YOUR TURN", {100, 180, 255, 255}, 1.2f);
    m_subText = "Select a unit or place a dice";
}

void TurnBanner::ShowEnemyTurn() {
    Show("ENEMY TURN", {255, 100, 100, 255}, 1.0f);
    m_subText = "";
}

void TurnBanner::ShowWaveStart(int waveNum, bool isBossWave) {
    if (isBossWave) {
        Show("BOSS WAVE " + std::to_string(waveNum), {255, 50, 50, 255}, 2.0f);
        m_subText = "Defeat the boss!";
        m_bgColor = {60, 20, 20, 200};
    } else {
        Show("WAVE " + std::to_string(waveNum), {255, 215, 0, 255}, 1.5f);
        m_subText = "";
        m_bgColor = {0, 0, 0, 180};
    }
}

void TurnBanner::ShowMessage(const std::string& text, SDL_Color color) {
    Show(text, color, 1.5f);
}

void TurnBanner::Update(float deltaTime) {
    if (!m_active) return;
    
    m_decorPulse += deltaTime * 3.0f;
    
    switch (m_state) {
        case State::SlidingIn:
            m_slideTween.Update(deltaTime);
            m_alphaTween.Update(deltaTime);
            m_slideOffset = m_slideTween.GetValue();
            m_alpha = m_alphaTween.GetValue();
            
            if (!m_slideTween.IsActive()) {
                m_state = State::Displaying;
                m_displayTimer = 0.0f;
            }
            break;
            
        case State::Displaying:
            m_displayTimer += deltaTime;
            if (m_displayTimer >= m_displayDuration) {
                m_state = State::SlidingOut;
                m_slideTween.Start(0.0f, 800.0f, 0.25f, Easing::BackIn);
                m_alphaTween.Start(1.0f, 0.0f, 0.2f, Easing::QuadIn);
            }
            break;
            
        case State::SlidingOut:
            m_slideTween.Update(deltaTime);
            m_alphaTween.Update(deltaTime);
            m_slideOffset = m_slideTween.GetValue();
            m_alpha = m_alphaTween.GetValue();
            
            if (!m_slideTween.IsActive()) {
                m_state = State::Inactive;
                m_active = false;
            }
            break;
            
        case State::Inactive:
            break;
    }
}

void TurnBanner::Render(Renderer& renderer, int screenWidth, int screenHeight) {
    if (!m_active || m_alpha <= 0.01f) return;
    
    int centerX = screenWidth / 2 + static_cast<int>(m_slideOffset);
    int centerY = m_baseY;
    
    int bannerWidth = 400;
    int bannerX = centerX - bannerWidth / 2;
    int bannerY = centerY - m_bannerHeight / 2;
    
    uint8_t alpha = static_cast<uint8_t>(m_alpha * 255);
    
    // Draw background with alpha
    SDL_Color bgColor = m_bgColor;
    bgColor.a = static_cast<uint8_t>(bgColor.a * m_alpha);
    renderer.FillRect(bannerX, bannerY, bannerWidth, m_bannerHeight, bgColor);
    
    // Draw border
    SDL_Color borderColor = m_color;
    borderColor.a = alpha;
    renderer.DrawRect(bannerX, bannerY, bannerWidth, m_bannerHeight, borderColor);
    renderer.DrawRect(bannerX + 1, bannerY + 1, bannerWidth - 2, m_bannerHeight - 2, borderColor);
    
    // Draw decorative lines
    if (m_showDecorations) {
        float pulse = (std::sin(m_decorPulse) + 1.0f) * 0.5f;
        uint8_t decorAlpha = static_cast<uint8_t>((150 + pulse * 100) * m_alpha);
        SDL_Color decorColor = {m_color.r, m_color.g, m_color.b, decorAlpha};
        
        // Left decoration
        renderer.FillRect(bannerX + 10, bannerY + m_bannerHeight / 2 - 2, 30, 4, decorColor);
        renderer.FillRect(bannerX + 10, bannerY + m_bannerHeight / 2 - 8, 4, 16, decorColor);
        
        // Right decoration
        renderer.FillRect(bannerX + bannerWidth - 40, bannerY + m_bannerHeight / 2 - 2, 30, 4, decorColor);
        renderer.FillRect(bannerX + bannerWidth - 14, bannerY + m_bannerHeight / 2 - 8, 4, 16, decorColor);
    }
    
    // Draw main text
    SDL_Color textColor = m_color;
    textColor.a = alpha;
    
    int textY = m_subText.empty() ? centerY - 12 : centerY - 18;
    renderer.DrawTextWithOutline(m_text, centerX, textY, textColor, 
                                  {0, 0, 0, alpha}, FontSize::XLarge, TextAlign::Center);
    
    // Draw sub text
    if (!m_subText.empty()) {
        SDL_Color subColor = {200, 200, 200, alpha};
        renderer.DrawText(m_subText, centerX, centerY + 15, subColor, 
                         FontSize::Small, TextAlign::Center);
    }
}

} // namespace DDD
