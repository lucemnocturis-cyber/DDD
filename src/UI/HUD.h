#pragma once

#include "../Graphics/Renderer.h"
#include "../Utils/Tween.h"
#include <string>

namespace DDD {

class Game;

/**
 * HUD - Heads Up Display showing game status
 */
class HUD {
public:
    explicit HUD(Game& game);
    ~HUD() = default;
    
    /**
     * Update HUD (for animations)
     */
    void Update(float deltaTime);
    
    /**
     * Render the HUD
     */
    void Render(Renderer& renderer);
    
    /**
     * Show floating damage number
     */
    void ShowDamageNumber(int x, int y, int damage, bool isCritical = false);
    
    /**
     * Show floating heal number
     */
    void ShowHealNumber(int x, int y, int amount);
    
    /**
     * Show status message
     */
    void ShowMessage(const std::string& message, float duration = 2.0f);
    
    /**
     * Update gold display (triggers animation)
     */
    void SetGold(int gold);
    
    /**
     * Update score display (triggers animation)
     */
    void SetScore(int score);
    
private:
    void RenderTopBar(Renderer& renderer);
    void RenderTurnIndicator(Renderer& renderer);
    void RenderFloatingNumbers(Renderer& renderer);
    void RenderMessage(Renderer& renderer);
    
    Game& m_game;
    
    // Animated counters
    AnimatedCounter m_goldCounter;
    AnimatedCounter m_scoreCounter;
    
    // Floating damage numbers
    struct FloatingNumber {
        int x, y;
        int value;
        bool isHeal;
        bool isCritical;
        float lifetime;
        float maxLifetime;
    };
    std::vector<FloatingNumber> m_floatingNumbers;
    
    // Status message
    std::string m_message;
    float m_messageTimer = 0.0f;
    
    // Animation
    float m_pulseTime = 0.0f;
    
    // Counter pop effect
    float m_goldPopScale = 1.0f;
    float m_scorePopScale = 1.0f;
};

} // namespace DDD
