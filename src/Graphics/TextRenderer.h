#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <unordered_map>
#include <memory>

namespace DDD {

/**
 * Text alignment options
 */
enum class TextAlign {
    Left,
    Center,
    Right
};

/**
 * Font size presets
 */
enum class FontSize {
    Small = 12,
    Medium = 16,
    Large = 24,
    Title = 32,
    Huge = 48
};

/**
 * TextRenderer - handles all text rendering with SDL_ttf
 */
class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();
    
    /**
     * Initialize with SDL renderer
     */
    bool Initialize(SDL_Renderer* renderer, const std::string& fontPath);
    
    /**
     * Shutdown and free resources
     */
    void Shutdown();
    
    /**
     * Render text at position
     */
    void RenderText(const std::string& text, int x, int y, SDL_Color color, 
                    FontSize size = FontSize::Medium, TextAlign align = TextAlign::Left);
    
    /**
     * Render text with shadow for better visibility
     */
    void RenderTextWithShadow(const std::string& text, int x, int y, 
                               SDL_Color color, SDL_Color shadowColor,
                               FontSize size = FontSize::Medium, 
                               TextAlign align = TextAlign::Left,
                               int shadowOffset = 2);
    
    /**
     * Render text with outline (stroke effect)
     */
    void RenderTextWithOutline(const std::string& text, int x, int y,
                                SDL_Color color, SDL_Color outlineColor,
                                FontSize size = FontSize::Medium,
                                TextAlign align = TextAlign::Left,
                                int outlineWidth = 1);
    
    /**
     * Get the dimensions of rendered text
     */
    void GetTextSize(const std::string& text, FontSize size, int& width, int& height);
    
    /**
     * Check if initialized
     */
    bool IsInitialized() const { return m_initialized; }
    
private:
    /**
     * Get or load font at specific size
     */
    TTF_Font* GetFont(FontSize size);
    
    /**
     * Create texture from text
     */
    SDL_Texture* CreateTextTexture(const std::string& text, TTF_Font* font, SDL_Color color);
    
    /**
     * Calculate X position based on alignment
     */
    int CalculateAlignedX(int x, int textWidth, TextAlign align);
    
    SDL_Renderer* m_renderer = nullptr;
    std::string m_fontPath;
    std::unordered_map<int, TTF_Font*> m_fonts;  // Size -> Font
    bool m_initialized = false;
};

/**
 * Predefined colors for UI consistency
 */
namespace Colors {
    const SDL_Color White = {255, 255, 255, 255};
    const SDL_Color Black = {0, 0, 0, 255};
    const SDL_Color Gold = {255, 215, 0, 255};
    const SDL_Color Red = {231, 76, 60, 255};
    const SDL_Color Blue = {52, 152, 219, 255};
    const SDL_Color Green = {46, 204, 113, 255};
    const SDL_Color Gray = {149, 165, 166, 255};
    const SDL_Color DarkGray = {52, 73, 94, 255};
    const SDL_Color Purple = {155, 89, 182, 255};
    const SDL_Color Orange = {243, 156, 18, 255};
    const SDL_Color Cyan = {26, 188, 156, 255};
    
    // UI specific
    const SDL_Color PlayerColor = {52, 152, 219, 255};
    const SDL_Color EnemyColor = {231, 76, 60, 255};
    const SDL_Color HealthGreen = {46, 204, 113, 255};
    const SDL_Color HealthRed = {231, 76, 60, 255};
    const SDL_Color ManaBlue = {52, 152, 219, 255};
    const SDL_Color ExpYellow = {241, 196, 15, 255};
    const SDL_Color ShadowColor = {0, 0, 0, 180};
}

} // namespace DDD
