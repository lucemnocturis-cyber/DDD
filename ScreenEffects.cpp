#include "ScreenEffects.h"
#include "Renderer.h"
#include "../Utils/Random.h"
#include "../Utils/Logger.h"

#include <cmath>
#include <algorithm>

namespace DDD {

ScreenEffects::ScreenEffects() = default;

float ScreenEffects::Update(float deltaTime) {
    // Handle hit pause - return 0 deltaTime while paused
    if (m_hitPauseTimer > 0) {
        m_hitPauseTimer -= deltaTime;
        if (m_hitPauseTimer <= 0) {
            m_hitPauseTimer = 0;
        }
        return 0.0f;  // Game is frozen
    }
    
    // Update screen shake
    if (m_shakeTimer > 0) {
        m_shakeTimer -= deltaTime;
        
        // Calculate current intensity with decay
        float progress = 1.0f - (m_shakeTimer / m_shakeDuration);
        float decay = std::pow(1.0f - progress, m_shakeDecayRate);
        float currentIntensity = m_shakeIntensity * decay;
        
        // Random offset
        m_shakeOffsetX = static_cast<int>((Random::GetFloat(-1.0f, 1.0f)) * currentIntensity);
        m_shakeOffsetY = static_cast<int>((Random::GetFloat(-1.0f, 1.0f)) * currentIntensity);
        
        if (m_shakeTimer <= 0) {
            m_shakeTimer = 0;
            m_shakeOffsetX = 0;
            m_shakeOffsetY = 0;
        }
    }
    
    // Update flash
    if (m_flashTimer > 0) {
        m_flashTimer -= deltaTime;
        if (m_flashTimer <= 0) {
            m_flashTimer = 0;
            m_flashColor.a = 0;
        }
    }
    
    return deltaTime;  // Normal time passage
}

void ScreenEffects::ApplyShakeOffset(int& offsetX, int& offsetY) {
    offsetX += m_shakeOffsetX;
    offsetY += m_shakeOffsetY;
}

void ScreenEffects::RenderFlash(Renderer& renderer) {
    if (m_flashTimer <= 0 || m_flashDuration <= 0) return;
    
    // Calculate alpha based on time remaining
    float progress = m_flashTimer / m_flashDuration;
    uint8_t alpha = static_cast<uint8_t>(m_flashStartAlpha * progress);
    
    SDL_Color color = m_flashColor;
    color.a = alpha;
    
    // Draw fullscreen overlay
    renderer.FillRect(0, 0, renderer.GetWidth(), renderer.GetHeight(), color);
}

void ScreenEffects::Shake(float intensity, float duration, float decayRate) {
    // Only override if new shake is stronger or current is nearly done
    if (intensity > m_shakeIntensity * 0.5f || m_shakeTimer < 0.05f) {
        m_shakeIntensity = intensity;
        m_shakeDuration = duration;
        m_shakeTimer = duration;
        m_shakeDecayRate = decayRate;
    }
}

void ScreenEffects::StopShake() {
    m_shakeTimer = 0;
    m_shakeOffsetX = 0;
    m_shakeOffsetY = 0;
}

void ScreenEffects::HitPause(float duration) {
    // Stack hit pauses up to a limit
    m_hitPauseTimer = std::min(m_hitPauseTimer + duration, 0.2f);
}

void ScreenEffects::Flash(SDL_Color color, float duration, uint8_t startAlpha) {
    m_flashColor = color;
    m_flashDuration = duration;
    m_flashTimer = duration;
    m_flashStartAlpha = startAlpha;
}

// ============================================================================
// Combined Effects
// ============================================================================

void ScreenEffects::OnLightHit() {
    ShakeLight();
    HitPauseLight();
}

void ScreenEffects::OnHit() {
    ShakeMedium();
    HitPauseMedium();
    FlashWhite();
}

void ScreenEffects::OnCriticalHit() {
    ShakeHeavy();
    HitPauseHeavy();
    FlashGold();
    Logger::Debug("Critical hit effect triggered");
}

void ScreenEffects::OnDeath() {
    ShakeMassive();
    HitPause(0.15f);
    Flash({255, 255, 255, 255}, 0.2f, 180);
    Logger::Debug("Death effect triggered");
}

void ScreenEffects::OnPlayerDamaged() {
    ShakeMedium();
    HitPauseMedium();
    FlashRed();
}

} // namespace DDD
