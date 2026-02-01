/**
 * Bitmap Font Renderer for Dungeon Dice Duelists
 * Renders text using generated bitmap font sheets
 */

#pragma once

#include <SDL.h>
#include <string>
#include <unordered_map>
#include <memory>

namespace DDD {

struct CharacterInfo {
    int x, y;           // Position in font sheet
    int width, height;  // Character dimensions
    int index;          // Character index
};

class BitmapFont {
public:
    BitmapFont() = default;
    ~BitmapFont();

    /**
     * Load bitmap font from image and metadata
     * @param renderer SDL renderer
     * @param imagePath Path to font sheet PNG
     * @param metadataPath Path to font metadata JSON
     * @return true if successful
     */
    bool Load(SDL_Renderer* renderer, const std::string& imagePath, const std::string& metadataPath);

    /**
     * Render text at specified position
     * @param renderer SDL renderer
     * @param text Text to render
     * @param x X position
     * @param y Y position
     * @param color Text color (optional, default white)
     */
    void RenderText(SDL_Renderer* renderer, const std::string& text, int x, int y, SDL_Color color = {255, 255, 255, 255});

    /**
     * Measure text dimensions
     * @param text Text to measure
     * @param width Output width
     * @param height Output height
     */
    void MeasureText(const std::string& text, int& width, int& height) const;

    /**
     * Get character width
     */
    int GetCharWidth() const { return m_charWidth; }

    /**
     * Get character height
     */
    int GetCharHeight() const { return m_charHeight; }

    /**
     * Check if font is loaded
     */
    bool IsLoaded() const { return m_texture != nullptr; }

private:
    SDL_Texture* m_texture = nullptr;
    std::unordered_map<char, CharacterInfo> m_charMap;
    int m_charWidth = 16;
    int m_charHeight = 16;
    int m_charsPerRow = 16;
};

/**
 * Font Manager - Manages multiple bitmap fonts
 */
class BitmapFontManager {
public:
    static BitmapFontManager& Instance();

    /**
     * Initialize font manager
     * @param renderer SDL renderer
     * @return true if successful
     */
    bool Initialize(SDL_Renderer* renderer);

    /**
     * Get font by name
     * @param fontName Font name ("small", "medium", "large", "title")
     * @return Pointer to font or nullptr
     */
    BitmapFont* GetFont(const std::string& fontName);

    /**
     * Render text with specified font
     * @param renderer SDL renderer
     * @param fontName Font to use
     * @param text Text to render
     * @param x X position
     * @param y Y position
     * @param color Text color
     */
    void RenderText(SDL_Renderer* renderer, const std::string& fontName, const std::string& text, int x, int y, SDL_Color color = {255, 255, 255, 255});

    /**
     * Cleanup
     */
    void Shutdown();

private:
    BitmapFontManager() = default;
    SDL_Renderer* m_renderer = nullptr;
    std::unordered_map<std::string, std::unique_ptr<BitmapFont>> m_fonts;
};

} // namespace DDD
