#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace DDD {

/**
 * A single frame of animation
 */
struct SpriteFrame {
    SDL_Rect srcRect;      // Source rectangle in texture
    float duration = 0.1f; // Duration of this frame in seconds
    int offsetX = 0;       // X offset when rendering
    int offsetY = 0;       // Y offset when rendering
};

/**
 * An animation sequence
 */
struct Animation {
    std::string name;
    std::vector<SpriteFrame> frames;
    bool loop = true;
    
    float GetTotalDuration() const {
        float total = 0;
        for (const auto& f : frames) total += f.duration;
        return total;
    }
};

/**
 * Sprite - a renderable image with animation support
 */
class Sprite {
public:
    Sprite();
    ~Sprite();
    
    /**
     * Load sprite from texture
     */
    void SetTexture(SDL_Texture* texture);
    SDL_Texture* GetTexture() const { return m_texture; }
    
    /**
     * Set the source rectangle (for sprite sheets)
     */
    void SetSourceRect(int x, int y, int w, int h);
    const SDL_Rect& GetSourceRect() const { return m_srcRect; }
    
    /**
     * Set sprite dimensions
     */
    void SetSize(int w, int h) { m_width = w; m_height = h; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    
    /**
     * Add an animation
     */
    void AddAnimation(const std::string& name, const Animation& anim);
    
    /**
     * Play an animation
     */
    void PlayAnimation(const std::string& name, bool restart = false);
    
    /**
     * Stop animation
     */
    void StopAnimation();
    
    /**
     * Update animation
     */
    void Update(float deltaTime);
    
    /**
     * Get current frame's source rect
     */
    SDL_Rect GetCurrentFrameRect() const;
    
    /**
     * Check if current animation is playing
     */
    bool IsAnimationPlaying() const { return m_isPlaying; }
    
    /**
     * Get current animation name
     */
    const std::string& GetCurrentAnimation() const { return m_currentAnimation; }
    
    /**
     * Flip sprite horizontally
     */
    void SetFlipH(bool flip) { m_flipH = flip; }
    bool IsFlippedH() const { return m_flipH; }
    
    /**
     * Set tint color
     */
    void SetTint(SDL_Color color) { m_tint = color; }
    SDL_Color GetTint() const { return m_tint; }
    
private:
    SDL_Texture* m_texture = nullptr;
    SDL_Rect m_srcRect = {0, 0, 32, 32};
    int m_width = 32;
    int m_height = 32;
    
    // Animations
    std::unordered_map<std::string, Animation> m_animations;
    std::string m_currentAnimation;
    int m_currentFrame = 0;
    float m_frameTimer = 0.0f;
    bool m_isPlaying = false;
    
    // Visual modifiers
    bool m_flipH = false;
    SDL_Color m_tint = {255, 255, 255, 255};
};

} // namespace DDD
