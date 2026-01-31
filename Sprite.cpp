#include "Sprite.h"
#include "../Utils/Logger.h"

namespace DDD {

Sprite::Sprite() = default;
Sprite::~Sprite() = default;

void Sprite::SetTexture(SDL_Texture* texture) {
    m_texture = texture;
    
    // Get texture dimensions if not set
    if (texture && m_width == 32 && m_height == 32) {
        int w, h;
        SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
        m_srcRect = {0, 0, w, h};
        m_width = w;
        m_height = h;
    }
}

void Sprite::SetSourceRect(int x, int y, int w, int h) {
    m_srcRect = {x, y, w, h};
    m_width = w;
    m_height = h;
}

void Sprite::AddAnimation(const std::string& name, const Animation& anim) {
    m_animations[name] = anim;
}

void Sprite::PlayAnimation(const std::string& name, bool restart) {
    auto it = m_animations.find(name);
    if (it == m_animations.end()) {
        Logger::Warning("Animation '{}' not found", name);
        return;
    }
    
    if (m_currentAnimation == name && !restart && m_isPlaying) {
        return;  // Already playing this animation
    }
    
    m_currentAnimation = name;
    m_currentFrame = 0;
    m_frameTimer = 0.0f;
    m_isPlaying = true;
}

void Sprite::StopAnimation() {
    m_isPlaying = false;
}

void Sprite::Update(float deltaTime) {
    if (!m_isPlaying || m_currentAnimation.empty()) {
        return;
    }
    
    auto it = m_animations.find(m_currentAnimation);
    if (it == m_animations.end() || it->second.frames.empty()) {
        return;
    }
    
    const Animation& anim = it->second;
    m_frameTimer += deltaTime;
    
    // Advance frames
    while (m_frameTimer >= anim.frames[m_currentFrame].duration) {
        m_frameTimer -= anim.frames[m_currentFrame].duration;
        m_currentFrame++;
        
        if (m_currentFrame >= static_cast<int>(anim.frames.size())) {
            if (anim.loop) {
                m_currentFrame = 0;
            } else {
                m_currentFrame = static_cast<int>(anim.frames.size()) - 1;
                m_isPlaying = false;
                break;
            }
        }
    }
}

SDL_Rect Sprite::GetCurrentFrameRect() const {
    if (m_currentAnimation.empty()) {
        return m_srcRect;
    }
    
    auto it = m_animations.find(m_currentAnimation);
    if (it == m_animations.end() || it->second.frames.empty()) {
        return m_srcRect;
    }
    
    return it->second.frames[m_currentFrame].srcRect;
}

} // namespace DDD
