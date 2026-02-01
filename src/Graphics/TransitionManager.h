#pragma once

#include <SDL2/SDL.h>
#include <functional>
#include <string>

namespace DDD {

class Renderer;

/**
 * Transition types
 */
enum class TransitionType {
    None,
    Fade,           // Simple fade to black and back
    FadeWhite,      // Fade to white
    SlideLeft,      // Slide out left, slide in from right
    SlideUp,        // Slide up
    Diamond,        // Diamond wipe
    Pixelate        // Pixelate transition
};

/**
 * Transition state
 */
enum class TransitionState {
    Idle,
    TransitioningOut,  // Fading/moving out
    Holding,           // Brief hold at peak
    TransitioningIn,   // Fading/moving in
    Complete
};

/**
 * TransitionManager - handles smooth transitions between game states
 */
class TransitionManager {
public:
    TransitionManager();
    ~TransitionManager() = default;
    
    /**
     * Start a transition
     * @param type Transition effect type
     * @param duration Total duration (half out, half in)
     * @param onMidpoint Callback when transition reaches midpoint (for state change)
     */
    void StartTransition(TransitionType type, float duration, 
                         std::function<void()> onMidpoint = nullptr);
    
    /**
     * Quick fade transition
     */
    void FadeTransition(std::function<void()> onMidpoint, float duration = 0.5f);
    
    /**
     * Update transition
     */
    void Update(float deltaTime);
    
    /**
     * Render transition overlay
     * Call AFTER rendering game content
     */
    void Render(Renderer& renderer);
    
    /**
     * Check if transitioning
     */
    bool IsTransitioning() const { return m_state != TransitionState::Idle; }
    
    /**
     * Check if past midpoint (new state should be visible)
     */
    bool IsPastMidpoint() const { 
        return m_state == TransitionState::TransitioningIn || 
               m_state == TransitionState::Complete; 
    }
    
    /**
     * Get transition progress (0.0 to 1.0)
     */
    float GetProgress() const;
    
    /**
     * Force complete transition
     */
    void ForceComplete();
    
private:
    void RenderFade(Renderer& renderer, bool white = false);
    void RenderSlide(Renderer& renderer, bool vertical = false);
    void RenderDiamond(Renderer& renderer);
    void RenderPixelate(Renderer& renderer);
    
    TransitionType m_type = TransitionType::None;
    TransitionState m_state = TransitionState::Idle;
    
    float m_duration = 0.5f;
    float m_timer = 0.0f;
    float m_holdDuration = 0.05f;  // Brief hold at midpoint
    
    std::function<void()> m_onMidpoint;
    bool m_midpointCalled = false;
    
    // For certain effects
    SDL_Color m_transitionColor = {0, 0, 0, 255};
};

} // namespace DDD
