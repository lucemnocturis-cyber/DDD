/**
 * Bitmap Font Renderer Implementation
 */

#include "BitmapFont.h"
#include "../Utils/Logger.h"
#include <SDL_image.h>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace DDD {

BitmapFont::~BitmapFont() {
    if (m_texture) {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }
}

bool BitmapFont::Load(SDL_Renderer* renderer, const std::string& imagePath, const std::string& metadataPath) {
    // Load font texture
    SDL_Surface* surface = IMG_Load(imagePath.c_str());
    if (!surface) {
        Logger::Error("Failed to load bitmap font image: {}", imagePath);
        return false;
    }

    m_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!m_texture) {
        Logger::Error("Failed to create texture from bitmap font");
        return false;
    }

    // Set blend mode for transparency
    SDL_SetTextureBlendMode(m_texture, SDL_BLENDMODE_BLEND);

    // Load metadata
    std::ifstream file(metadataPath);
    if (!file.is_open()) {
        Logger::Error("Failed to open bitmap font metadata: {}", metadataPath);
        return false;
    }

    try {
        json data = json::parse(file);
        
        m_charWidth = data["char_width"];
        m_charHeight = data["char_height"];
        m_charsPerRow = data["chars_per_row"];

        // Load character map
        for (auto& [charStr, info] : data["char_map"].items()) {
            if (charStr.length() == 1) {
                char c = charStr[0];
                CharacterInfo charInfo;
                charInfo.x = info["x"];
                charInfo.y = info["y"];
                charInfo.width = info["width"];
                charInfo.height = info["height"];
                charInfo.index = info["index"];
                m_charMap[c] = charInfo;
            }
        }

        Logger::Info("Loaded bitmap font with {} characters ({}x{})", m_charMap.size(), m_charWidth, m_charHeight);
        return true;

    } catch (const std::exception& e) {
        Logger::Error("Failed to parse bitmap font metadata: {}", e.what());
        return false;
    }
}

void BitmapFont::RenderText(SDL_Renderer* renderer, const std::string& text, int x, int y, SDL_Color color) {
    if (!m_texture) return;

    // Set texture color modulation
    SDL_SetTextureColorMod(m_texture, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(m_texture, color.a);

    int currentX = x;
    int currentY = y;

    for (char c : text) {
        // Handle newlines
        if (c == '\n') {
            currentX = x;
            currentY += m_charHeight;
            continue;
        }

        // Get character info
        auto it = m_charMap.find(c);
        if (it == m_charMap.end()) {
            // Use space for unknown characters
            currentX += m_charWidth;
            continue;
        }

        const CharacterInfo& charInfo = it->second;

        // Source rectangle (from font sheet)
        SDL_Rect srcRect = {
            charInfo.x,
            charInfo.y,
            charInfo.width,
            charInfo.height
        };

        // Destination rectangle
        SDL_Rect dstRect = {
            currentX,
            currentY,
            charInfo.width,
            charInfo.height
        };

        SDL_RenderCopy(renderer, m_texture, &srcRect, &dstRect);

        currentX += charInfo.width;
    }
}

void BitmapFont::MeasureText(const std::string& text, int& width, int& height) const {
    width = 0;
    height = m_charHeight;

    int currentLineWidth = 0;
    int maxWidth = 0;
    int lineCount = 1;

    for (char c : text) {
        if (c == '\n') {
            maxWidth = std::max(maxWidth, currentLineWidth);
            currentLineWidth = 0;
            lineCount++;
        } else {
            auto it = m_charMap.find(c);
            if (it != m_charMap.end()) {
                currentLineWidth += it->second.width;
            } else {
                currentLineWidth += m_charWidth; // Space width
            }
        }
    }

    maxWidth = std::max(maxWidth, currentLineWidth);
    width = maxWidth;
    height = m_charHeight * lineCount;
}

// ========== BitmapFontManager ==========

BitmapFontManager& BitmapFontManager::Instance() {
    static BitmapFontManager instance;
    return instance;
}

bool BitmapFontManager::Initialize(SDL_Renderer* renderer) {
    m_renderer = renderer;

    // Load all bitmap fonts
    const std::vector<std::pair<std::string, std::string>> fonts = {
        {"small", "bitmap_font_small"},
        {"medium", "bitmap_font_medium"},
        {"large", "bitmap_font_large"},
        {"title", "bitmap_font_title"}
    };

    bool allLoaded = true;

    for (const auto& [name, filename] : fonts) {
        auto font = std::make_unique<BitmapFont>();
        
        std::string imagePath = "assets/fonts/" + filename + ".png";
        std::string metadataPath = "assets/fonts/" + filename + ".json";

        if (font->Load(renderer, imagePath, metadataPath)) {
            m_fonts[name] = std::move(font);
            Logger::Info("Loaded bitmap font: {}", name);
        } else {
            Logger::Warn("Failed to load bitmap font: {}", name);
            allLoaded = false;
        }
    }

    // Set up aliases for compatibility
    if (m_fonts.count("medium")) {
        m_fonts["game_font"] = std::make_unique<BitmapFont>();
        m_fonts["game_font"]->Load(renderer, "assets/fonts/bitmap_font_medium.png", "assets/fonts/bitmap_font_medium.json");
    }

    if (m_fonts.count("title")) {
        m_fonts["title_font"] = std::make_unique<BitmapFont>();
        m_fonts["title_font"]->Load(renderer, "assets/fonts/bitmap_font_title.png", "assets/fonts/bitmap_font_title.json");
    }

    return allLoaded;
}

BitmapFont* BitmapFontManager::GetFont(const std::string& fontName) {
    auto it = m_fonts.find(fontName);
    if (it != m_fonts.end()) {
        return it->second.get();
    }
    
    // Return default font (medium) if specific font not found
    auto defaultIt = m_fonts.find("medium");
    if (defaultIt != m_fonts.end()) {
        return defaultIt->second.get();
    }

    return nullptr;
}

void BitmapFontManager::RenderText(SDL_Renderer* renderer, const std::string& fontName, const std::string& text, int x, int y, SDL_Color color) {
    BitmapFont* font = GetFont(fontName);
    if (font) {
        font->RenderText(renderer, text, x, y, color);
    }
}

void BitmapFontManager::Shutdown() {
    m_fonts.clear();
    m_renderer = nullptr;
}

} // namespace DDD
