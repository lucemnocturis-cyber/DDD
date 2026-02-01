#pragma once

#include <SDL2/SDL.h>
#include "../Utils/Math.h"
#include "TextRenderer.h"

#include <string>
#include <memory>

namespace DDD {

// Forward declarations
class Sprite;
class ResourceManager;

/**
 * Renderer - handles all drawing operations
 */
class Renderer {
public:
    Renderer(SDL_Renderer* sdlRenderer, int width, int height);
    ~Renderer();
    
    /**
     * Initialize renderer
     */
    bool Initialize();
    
    /**
     * Handle window resize
     */
    void OnWindowResize(int width, int height);
    
    /**
     * Clear the screen
     */
    void Clear();
    
    /**
     * Present the frame
     */
    void Present();
    
    /**
     * Set draw color
     */
    void SetDrawColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    
    /**
     * Draw rectangle outline
     */
    void DrawRect(int x, int y, int w, int h, SDL_Color color);
    
    /**
     * Draw filled rectangle
     */
    void FillRect(int x, int y, int w, int h, SDL_Color color);
    
    /**
     * Draw line
     */
    void DrawLine(int x1, int y1, int x2, int y2, SDL_Color color);
    
    /**
     * Draw circle outline
     */
    void DrawCircle(int x, int y, int radius, SDL_Color color);
    
    /**
     * Draw filled circle
     */
    void FillCircle(int x, int y, int radius, SDL_Color color);
    
    /**
     * Draw texture
     */
    void DrawTexture(SDL_Texture* texture, int x, int y);
    void DrawTexture(SDL_Texture* texture, const Rect& destRect);
    void DrawTexture(SDL_Texture* texture, const Rect& srcRect, const Rect& destRect);
    
    /**
     * Draw texture with color modulation
     */
    void DrawTextureColored(SDL_Texture* texture, const Rect& destRect, SDL_Color color);
    
    /**
     * Draw text (requires font)
     */
    void DrawText(const std::string& text, int x, int y, SDL_Color color, 
                  FontSize size = FontSize::Medium, TextAlign align = TextAlign::Left);
    
    /**
     * Draw text with shadow
     */
    void DrawTextWithShadow(const std::string& text, int x, int y, SDL_Color color,
                            FontSize size = FontSize::Medium, TextAlign align = TextAlign::Left);
    
    /**
     * Draw text with outline
     */
    void DrawTextWithOutline(const std::string& text, int x, int y, 
                             SDL_Color color, SDL_Color outlineColor,
                             FontSize size = FontSize::Medium, TextAlign align = TextAlign::Left);
    
    /**
     * Get text dimensions
     */
    void GetTextSize(const std::string& text, FontSize size, int& width, int& height);
    
    /**
     * Get TextRenderer for advanced text operations
     */
    TextRenderer& GetTextRenderer() { return m_textRenderer; }
    
    /**
     * Get SDL renderer
     */
    SDL_Renderer* GetSDLRenderer() { return m_renderer; }
    
    /**
     * Get screen dimensions
     */
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    
private:
    SDL_Renderer* m_renderer;
    int m_width;
    int m_height;
    TextRenderer m_textRenderer;
};

} // namespace DDD
