#include "Button.h"
#include "../Graphics/Renderer.h"
#include "../Audio/SoundManager.h"

#include <algorithm>
#include <cmath>

namespace DDD {

Button::Button() {
    UpdateColors();
}

Button::Button(const std::string& text, int x, int y, int width, int height)
    : m_text(text)
    , m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
{
    UpdateColors();
}

void Button::Update(float deltaTime, int mouseX, int mouseY, bool mouseDown) {
    if (!m_visible) return;
    
    if (!m_enabled) {
        m_state = ButtonState::Disabled;
        UpdateColors();
        return;
    }
    
    bool wasHovered = (m_state == ButtonState::Hovered || m_state == ButtonState::Pressed);
    bool isOver = Contains(mouseX, mouseY);
    
    // Determine new state
    if (isOver) {
        // Play hover sound on first hover
        if (!wasHovered) {
            PlaySound(SoundID::ButtonHover);
        }
        
        if (mouseDown) {
            m_state = ButtonState::Pressed;
        } else {
            // Check if this was a click (mouse released while over button)
            if (m_wasPressed && m_state == ButtonState::Pressed) {
                OnClick();
            }
            m_state = ButtonState::Hovered;
        }
    } else {
        m_state = ButtonState::Normal;
    }
    
    m_wasPressed = mouseDown;
    
    // Animate hover scale
    float targetScale = (m_state == ButtonState::Hovered) ? 1.05f : 1.0f;
    m_hoverScale += (targetScale - m_hoverScale) * deltaTime * 10.0f;
    
    // Animate press offset
    float targetOffset = (m_state == ButtonState::Pressed) ? 3.0f : 0.0f;
    m_pressOffset += (targetOffset - m_pressOffset) * deltaTime * 20.0f;
    
    UpdateColors();
}

void Button::Render(Renderer& renderer) {
    if (!m_visible) return;
    
    // Calculate actual position (centered if needed)
    int drawX = m_centered ? m_x - m_width / 2 : m_x;
    int drawY = m_centered ? m_y - m_height / 2 : m_y;
    
    // Apply press offset
    drawY += static_cast<int>(m_pressOffset);
    
    // Draw shadow (offset down and right)
    if (m_state != ButtonState::Pressed) {
        SDL_Color shadowColor = {0, 0, 0, 100};
        renderer.FillRect(drawX + 4, drawY + 4, m_width, m_height, shadowColor);
    }
    
    // Draw button background
    renderer.FillRect(drawX, drawY, m_width, m_height, m_bgColor);
    
    // Draw border (multiple layers for thickness)
    renderer.DrawRect(drawX, drawY, m_width, m_height, m_borderColor);
    renderer.DrawRect(drawX + 1, drawY + 1, m_width - 2, m_height - 2, m_borderColor);
    
    // Draw inner highlight (top-left lighter)
    if (m_state != ButtonState::Pressed && m_state != ButtonState::Disabled) {
        SDL_Color highlight = {255, 255, 255, 40};
        renderer.FillRect(drawX + 2, drawY + 2, m_width - 4, 2, highlight);
        renderer.FillRect(drawX + 2, drawY + 2, 2, m_height - 4, highlight);
    }
    
    // Draw text centered in button
    int textX = drawX + m_width / 2;
    int textY = drawY + m_height / 2 - static_cast<int>(m_fontSize) / 2;
    
    // Text shadow
    SDL_Color textShadow = {0, 0, 0, 180};
    renderer.DrawText(m_text, textX + 2, textY + 2, textShadow, m_fontSize, TextAlign::Center);
    
    // Main text
    renderer.DrawText(m_text, textX, textY, m_textColor, m_fontSize, TextAlign::Center);
}

bool Button::Contains(int x, int y) const {
    int drawX = m_centered ? m_x - m_width / 2 : m_x;
    int drawY = m_centered ? m_y - m_height / 2 : m_y;
    
    return x >= drawX && x < drawX + m_width &&
           y >= drawY && y < drawY + m_height;
}

void Button::OnClick() {
    if (m_onClick && m_enabled) {
        PlaySound(SoundID::ButtonClick);
        m_onClick();
    }
}

void Button::UpdateColors() {
    // Base colors by style
    SDL_Color baseBg, baseBorder, baseText;
    
    switch (m_style) {
        case ButtonStyle::Primary:
            baseBg = {180, 140, 50, 255};      // Gold
            baseBorder = {255, 215, 0, 255};   // Bright gold
            baseText = {255, 255, 255, 255};   // White
            break;
            
        case ButtonStyle::Secondary:
            baseBg = {40, 100, 160, 255};      // Blue
            baseBorder = {52, 152, 219, 255};  // Bright blue
            baseText = {255, 255, 255, 255};
            break;
            
        case ButtonStyle::Danger:
            baseBg = {160, 50, 50, 255};       // Red
            baseBorder = {231, 76, 60, 255};   // Bright red
            baseText = {255, 255, 255, 255};
            break;
            
        case ButtonStyle::Ghost:
            baseBg = {0, 0, 0, 50};            // Near transparent
            baseBorder = {255, 255, 255, 100}; // Light border
            baseText = {255, 255, 255, 255};
            break;
    }
    
    // Modify based on state
    switch (m_state) {
        case ButtonState::Normal:
            m_bgColor = baseBg;
            m_borderColor = baseBorder;
            m_textColor = baseText;
            break;
            
        case ButtonState::Hovered:
            // Brighten
            m_bgColor = {
                static_cast<uint8_t>(std::min(255, baseBg.r + 30)),
                static_cast<uint8_t>(std::min(255, baseBg.g + 30)),
                static_cast<uint8_t>(std::min(255, baseBg.b + 30)),
                baseBg.a
            };
            m_borderColor = {255, 255, 255, 255};  // White border on hover
            m_textColor = baseText;
            break;
            
        case ButtonState::Pressed:
            // Darken
            m_bgColor = {
                static_cast<uint8_t>(std::max(0, baseBg.r - 30)),
                static_cast<uint8_t>(std::max(0, baseBg.g - 30)),
                static_cast<uint8_t>(std::max(0, baseBg.b - 30)),
                baseBg.a
            };
            m_borderColor = baseBorder;
            m_textColor = baseText;
            break;
            
        case ButtonState::Disabled:
            m_bgColor = {80, 80, 80, 200};
            m_borderColor = {100, 100, 100, 200};
            m_textColor = {150, 150, 150, 200};
            break;
    }
}

} // namespace DDD
