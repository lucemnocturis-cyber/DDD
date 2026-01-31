#pragma once

#include "../Graphics/TextRenderer.h"
#include "../Utils/Tween.h"
#include <SDL2/SDL.h>
#include <string>

namespace DDD {

class Renderer;

/**
 * TurnBanner - animated banner that slides in to announce turns/events
 */
class TurnBanner {
public:
    TurnBanner();
    ~TurnBanner() = default;
    
    /**
     * Show the banner with text
     */
    void Show(const std::string& text, SDL_Color color, float displayDuration = 1.5f);
    
    /**
     * Show "Player Turn" banner
     */
    void ShowPlayerTurn();
    
    /**
     * Show "Enemy Turn" banner
     */
    void ShowEnemyTurn();
    
    /**
     * Show wave announcement
     */
    void ShowWaveStart(int waveNum, bool isBossWave = false);
    
    /**
     * Show custom message
     */
    void ShowMessage(const std::string& text, SDL_Color color);
    
    /**
     * Update animation
     */
    void Update(float deltaTime);
    
    /**
     * Render the banner
     */
    void Render(Renderer& renderer, int screenWidth, int screenHeight);
    
    /**
     * Check if banner is active
     */
    bool IsActive() const { return m_active; }
    
    /**
     * Set Y position (center of banner)
     */
    void SetY(int y) { m_baseY = y; }
    
private:
    enum class State {
        Inactive,
        SlidingIn,
        Displaying,
        SlidingOut
    };
    
    std::string m_text;
    std::string m_subText;
    SDL_Color m_color = {255, 255, 255, 255};
    SDL_Color m_bgColor = {0, 0, 0, 180};
    
    State m_state = State::Inactive;
    bool m_active = false;
    
    // Animation
    Tween m_slideTween;
    Tween m_alphaTween;
    float m_displayTimer = 0.0f;
    float m_displayDuration = 1.5f;
    
    float m_slideOffset = 0.0f;  // X offset for slide animation
    float m_alpha = 0.0f;
    
    // Position
    int m_baseY = 200;  // Center Y position
    int m_bannerHeight = 80;
    
    // Decorations
    bool m_showDecorations = true;
    float m_decorPulse = 0.0f;
};

} // namespace DDD
