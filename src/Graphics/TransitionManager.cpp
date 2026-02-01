#include "TransitionManager.h"
#include "Renderer.h"
#include "../Utils/Logger.h"
#include "../Utils/Tween.h"

#include <cmath>
#include <algorithm>

namespace DDD {

TransitionManager::TransitionManager() = default;

void TransitionManager::StartTransition(TransitionType type, float duration,
                                         std::function<void()> onMidpoint) {
    if (m_state != TransitionState::Idle) {
        Logger::Warning("Transition already in progress");
        return;
    }
    
    m_type = type;
    m_duration = duration;
    m_timer = 0.0f;
    m_onMidpoint = onMidpoint;
    m_midpointCalled = false;
    m_state = TransitionState::TransitioningOut;
    
    // Set color based on type
    switch (type) {
        case TransitionType::FadeWhite:
            m_transitionColor = {255, 255, 255, 255};
            break;
        default:
            m_transitionColor = {0, 0, 0, 255};
            break;
    }
    
    Logger::Debug("Started transition, duration: {}s", duration);
}

void TransitionManager::FadeTransition(std::function<void()> onMidpoint, float duration) {
    StartTransition(TransitionType::Fade, duration, onMidpoint);
}

void TransitionManager::Update(float deltaTime) {
    if (m_state == TransitionState::Idle) return;
    
    m_timer += deltaTime;
    float halfDuration = m_duration / 2.0f;
    
    switch (m_state) {
        case TransitionState::TransitioningOut:
            if (m_timer >= halfDuration) {
                m_state = TransitionState::Holding;
                m_timer = 0.0f;
            }
            break;
            
        case TransitionState::Holding:
            // Call midpoint callback
            if (!m_midpointCalled && m_onMidpoint) {
                m_onMidpoint();
                m_midpointCalled = true;
            }
            
            if (m_timer >= m_holdDuration) {
                m_state = TransitionState::TransitioningIn;
                m_timer = 0.0f;
            }
            break;
            
        case TransitionState::TransitioningIn:
            if (m_timer >= halfDuration) {
                m_state = TransitionState::Complete;
                m_timer = 0.0f;
            }
            break;
            
        case TransitionState::Complete:
            // Reset after completion
            m_state = TransitionState::Idle;
            m_type = TransitionType::None;
            break;
            
        case TransitionState::Idle:
            break;
    }
}

float TransitionManager::GetProgress() const {
    float halfDuration = m_duration / 2.0f;
    if (halfDuration <= 0) return 0.0f;
    
    switch (m_state) {
        case TransitionState::TransitioningOut:
            return m_timer / halfDuration * 0.5f;  // 0.0 to 0.5
            
        case TransitionState::Holding:
            return 0.5f;
            
        case TransitionState::TransitioningIn:
            return 0.5f + (m_timer / halfDuration * 0.5f);  // 0.5 to 1.0
            
        case TransitionState::Complete:
            return 1.0f;
            
        default:
            return 0.0f;
    }
}

void TransitionManager::ForceComplete() {
    if (m_state != TransitionState::Idle && !m_midpointCalled && m_onMidpoint) {
        m_onMidpoint();
        m_midpointCalled = true;
    }
    m_state = TransitionState::Idle;
    m_type = TransitionType::None;
}

void TransitionManager::Render(Renderer& renderer) {
    if (m_state == TransitionState::Idle) return;
    
    switch (m_type) {
        case TransitionType::Fade:
            RenderFade(renderer, false);
            break;
            
        case TransitionType::FadeWhite:
            RenderFade(renderer, true);
            break;
            
        case TransitionType::SlideLeft:
            RenderSlide(renderer, false);
            break;
            
        case TransitionType::SlideUp:
            RenderSlide(renderer, true);
            break;
            
        case TransitionType::Diamond:
            RenderDiamond(renderer);
            break;
            
        case TransitionType::Pixelate:
            RenderPixelate(renderer);
            break;
            
        case TransitionType::None:
            break;
    }
}

void TransitionManager::RenderFade(Renderer& renderer, bool white) {
    float halfDuration = m_duration / 2.0f;
    float alpha = 0.0f;
    
    switch (m_state) {
        case TransitionState::TransitioningOut:
            // Fade in (alpha increases)
            alpha = Easing::QuadIn(m_timer / halfDuration);
            break;
            
        case TransitionState::Holding:
            alpha = 1.0f;
            break;
            
        case TransitionState::TransitioningIn:
            // Fade out (alpha decreases)
            alpha = 1.0f - Easing::QuadOut(m_timer / halfDuration);
            break;
            
        default:
            return;
    }
    
    SDL_Color color = white ? SDL_Color{255, 255, 255, 255} : SDL_Color{0, 0, 0, 255};
    color.a = static_cast<uint8_t>(alpha * 255);
    
    renderer.FillRect(0, 0, renderer.GetWidth(), renderer.GetHeight(), color);
}

void TransitionManager::RenderSlide(Renderer& renderer, bool vertical) {
    float halfDuration = m_duration / 2.0f;
    int screenW = renderer.GetWidth();
    int screenH = renderer.GetHeight();
    float progress = 0.0f;
    
    switch (m_state) {
        case TransitionState::TransitioningOut:
            progress = Easing::QuadIn(m_timer / halfDuration);
            break;
            
        case TransitionState::Holding:
            progress = 1.0f;
            break;
            
        case TransitionState::TransitioningIn:
            progress = 1.0f - Easing::QuadOut(m_timer / halfDuration);
            break;
            
        default:
            return;
    }
    
    if (vertical) {
        // Slide from bottom
        int coverHeight = static_cast<int>(screenH * progress);
        renderer.FillRect(0, screenH - coverHeight, screenW, coverHeight, m_transitionColor);
    } else {
        // Slide from right
        int coverWidth = static_cast<int>(screenW * progress);
        renderer.FillRect(screenW - coverWidth, 0, coverWidth, screenH, m_transitionColor);
    }
}

void TransitionManager::RenderDiamond(Renderer& renderer) {
    float halfDuration = m_duration / 2.0f;
    int screenW = renderer.GetWidth();
    int screenH = renderer.GetHeight();
    int centerX = screenW / 2;
    int centerY = screenH / 2;
    float maxRadius = std::sqrt(static_cast<float>(centerX * centerX + centerY * centerY));
    
    float progress = 0.0f;
    
    switch (m_state) {
        case TransitionState::TransitioningOut:
            // Diamond grows from center
            progress = Easing::QuadIn(m_timer / halfDuration);
            break;
            
        case TransitionState::Holding:
            progress = 1.0f;
            break;
            
        case TransitionState::TransitioningIn:
            // Diamond shrinks to center
            progress = 1.0f - Easing::QuadOut(m_timer / halfDuration);
            break;
            
        default:
            return;
    }
    
    // When transitioning out, we cover screen with growing diamond
    // When transitioning in, we reveal screen by shrinking diamond
    
    float currentRadius = maxRadius * progress;
    
    // Draw the black overlay, leaving diamond-shaped hole
    // Simplified: draw black rectangles around a diamond area
    
    // For simplicity, just use a circular approximation with rectangles
    // covering corners
    if (m_state == TransitionState::TransitioningOut || m_state == TransitionState::Holding) {
        // Cover everything except diamond
        for (int y = 0; y < screenH; y++) {
            for (int x = 0; x < screenW; x += 4) {
                // Manhattan distance from center (diamond shape)
                int dx = std::abs(x - centerX);
                int dy = std::abs(y - centerY);
                float dist = static_cast<float>(dx + dy);
                
                if (dist > currentRadius * 0.7f) {
                    renderer.FillRect(x, y, 4, 1, m_transitionColor);
                }
            }
        }
    } else {
        // During transition in, cover the diamond area
        for (int y = 0; y < screenH; y++) {
            for (int x = 0; x < screenW; x += 4) {
                int dx = std::abs(x - centerX);
                int dy = std::abs(y - centerY);
                float dist = static_cast<float>(dx + dy);
                
                if (dist < currentRadius * 0.7f) {
                    renderer.FillRect(x, y, 4, 1, m_transitionColor);
                }
            }
        }
    }
}

void TransitionManager::RenderPixelate(Renderer& renderer) {
    float halfDuration = m_duration / 2.0f;
    int screenW = renderer.GetWidth();
    int screenH = renderer.GetHeight();
    
    float progress = 0.0f;
    
    switch (m_state) {
        case TransitionState::TransitioningOut:
            progress = Easing::QuadIn(m_timer / halfDuration);
            break;
            
        case TransitionState::Holding:
            progress = 1.0f;
            break;
            
        case TransitionState::TransitioningIn:
            progress = 1.0f - Easing::QuadOut(m_timer / halfDuration);
            break;
            
        default:
            return;
    }
    
    // Draw random pixels that increase in coverage
    int coverage = static_cast<int>(progress * 100);
    uint8_t alpha = static_cast<uint8_t>(progress * 255);
    
    SDL_Color color = m_transitionColor;
    color.a = alpha;
    
    // Draw a grid of blocks with increasing size
    int blockSize = std::max(2, static_cast<int>(progress * 20));
    
    for (int y = 0; y < screenH; y += blockSize) {
        for (int x = 0; x < screenW; x += blockSize) {
            // Random chance to draw based on coverage
            if ((x + y) % (101 - coverage) < blockSize) {
                renderer.FillRect(x, y, blockSize, blockSize, color);
            }
        }
    }
}

} // namespace DDD
