#pragma once

#include "../Graphics/TextRenderer.h"
#include "../Utils/Math.h"
#include <SDL2/SDL.h>
#include <string>
#include <functional>

namespace DDD {

class Renderer;

/**
 * Button states
 */
enum class ButtonState {
    Normal,
    Hovered,
    Pressed,
    Disabled
};

/**
 * Button styles for different UI contexts
 */
enum class ButtonStyle {
    Primary,    // Main action buttons (gold)
    Secondary,  // Alternative actions (blue)
    Danger,     // Destructive actions (red)
    Ghost       // Text-only, minimal
};

/**
 * Interactive UI Button with hover/press states and callbacks
 */
class Button {
public:
    Button();
    Button(const std::string& text, int x, int y, int width, int height);
    virtual ~Button() = default;
    
    /**
     * Update button state based on mouse
     */
    void Update(float deltaTime, int mouseX, int mouseY, bool mouseDown);
    
    /**
     * Render the button
     */
    void Render(Renderer& renderer);
    
    /**
     * Check if point is inside button
     */
    bool Contains(int x, int y) const;
    
    /**
     * Handle click (call when mouse released over button)
     */
    void OnClick();
    
    // Setters
    void SetText(const std::string& text) { m_text = text; }
    void SetPosition(int x, int y) { m_x = x; m_y = y; }
    void SetSize(int width, int height) { m_width = width; m_height = height; }
    void SetStyle(ButtonStyle style) { m_style = style; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    void SetVisible(bool visible) { m_visible = visible; }
    void SetFontSize(FontSize size) { m_fontSize = size; }
    void SetCallback(std::function<void()> callback) { m_onClick = callback; }
    void SetCentered(bool centered) { m_centered = centered; }
    
    // Getters
    const std::string& GetText() const { return m_text; }
    int GetX() const { return m_x; }
    int GetY() const { return m_y; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    ButtonState GetState() const { return m_state; }
    bool IsEnabled() const { return m_enabled; }
    bool IsVisible() const { return m_visible; }
    bool IsHovered() const { return m_state == ButtonState::Hovered; }
    bool IsPressed() const { return m_state == ButtonState::Pressed; }
    
protected:
    void UpdateColors();
    
    std::string m_text;
    int m_x = 0;
    int m_y = 0;
    int m_width = 200;
    int m_height = 50;
    
    ButtonState m_state = ButtonState::Normal;
    ButtonStyle m_style = ButtonStyle::Primary;
    FontSize m_fontSize = FontSize::Large;
    
    bool m_enabled = true;
    bool m_visible = true;
    bool m_centered = false;  // If true, x/y is center position
    bool m_wasPressed = false;
    
    // Colors based on state
    SDL_Color m_bgColor;
    SDL_Color m_borderColor;
    SDL_Color m_textColor;
    
    // Animation
    float m_hoverScale = 1.0f;
    float m_pressOffset = 0.0f;
    
    // Callback
    std::function<void()> m_onClick;
};

} // namespace DDD
