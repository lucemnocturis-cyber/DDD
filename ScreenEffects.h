#pragma once

#include <SDL2/SDL.h>
#include <functional>

namespace DDD {

class Renderer;

/**
 * ScreenEffects - manages screen shake, flash, and hit pause effects
 */
class ScreenEffects {
public:
    ScreenEffects();
    ~ScreenEffects() = default;
    
    /**
     * Update all effects
     * Returns the actual deltaTime to use (accounting for hit pause)
     */
    float Update(float deltaTime);
    
    /**
     * Apply screen offset for shake
     * Call before rendering game world
     */
    void ApplyShakeOffset(int& offsetX, int& offsetY);
    
    /**
     * Render flash overlay
     * Call after rendering everything else
     */
    void RenderFlash(Renderer& renderer);
    
    // ========== SCREEN SHAKE ==========
    
    /**
     * Trigger screen shake
     * @param intensity Maximum pixel offset
     * @param duration Duration in seconds
     * @param decayRate How fast intensity decreases (1.0 = linear)
     */
    void Shake(float intensity, float duration, float decayRate = 1.0f);
    
    /**
     * Small shake for light hits
     */
    void ShakeLight() { Shake(3.0f, 0.15f, 1.5f); }
    
    /**
     * Medium shake for normal damage
     */
    void ShakeMedium() { Shake(6.0f, 0.2f, 1.2f); }
    
    /**
     * Heavy shake for critical hits
     */
    void ShakeHeavy() { Shake(10.0f, 0.3f, 1.0f); }
    
    /**
     * Massive shake for deaths/big events
     */
    void ShakeMassive() { Shake(15.0f, 0.4f, 0.8f); }
    
    /**
     * Stop all shaking
     */
    void StopShake();
    
    // ========== HIT PAUSE ==========
    
    /**
     * Trigger hit pause (freeze frame)
     * @param duration Duration in seconds (usually 0.05-0.15)
     */
    void HitPause(float duration);
    
    /**
     * Quick pause for light hits
     */
    void HitPauseLight() { HitPause(0.03f); }
    
    /**
     * Standard pause for hits
     */
    void HitPauseMedium() { HitPause(0.06f); }
    
    /**
     * Long pause for critical/kill
     */
    void HitPauseHeavy() { HitPause(0.1f); }
    
    /**
     * Check if game is paused
     */
    bool IsPaused() const { return m_hitPauseTimer > 0; }
    
    // ========== SCREEN FLASH ==========
    
    /**
     * Trigger screen flash
     * @param color Flash color
     * @param duration Duration in seconds
     * @param startAlpha Starting alpha (0-255)
     */
    void Flash(SDL_Color color, float duration, uint8_t startAlpha = 150);
    
    /**
     * White flash for hits
     */
    void FlashWhite() { Flash({255, 255, 255, 255}, 0.1f, 100); }
    
    /**
     * Red flash for damage taken
     */
    void FlashRed() { Flash({255, 50, 50, 255}, 0.15f, 80); }
    
    /**
     * Gold flash for critical
     */
    void FlashGold() { Flash({255, 200, 50, 255}, 0.12f, 120); }
    
    /**
     * Green flash for heal
     */
    void FlashGreen() { Flash({100, 255, 100, 255}, 0.2f, 60); }
    
    // ========== COMBINED EFFECTS ==========
    
    /**
     * Light hit effect (small shake + quick pause)
     */
    void OnLightHit();
    
    /**
     * Normal hit effect
     */
    void OnHit();
    
    /**
     * Critical hit effect
     */
    void OnCriticalHit();
    
    /**
     * Death effect (big shake + pause + flash)
     */
    void OnDeath();
    
    /**
     * Player damaged effect
     */
    void OnPlayerDamaged();
    
private:
    // Screen shake
    float m_shakeIntensity = 0.0f;
    float m_shakeDuration = 0.0f;
    float m_shakeTimer = 0.0f;
    float m_shakeDecayRate = 1.0f;
    int m_shakeOffsetX = 0;
    int m_shakeOffsetY = 0;
    
    // Hit pause
    float m_hitPauseTimer = 0.0f;
    
    // Screen flash
    SDL_Color m_flashColor = {255, 255, 255, 0};
    float m_flashDuration = 0.0f;
    float m_flashTimer = 0.0f;
    uint8_t m_flashStartAlpha = 0;
};

} // namespace DDD
