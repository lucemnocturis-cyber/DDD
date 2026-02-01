#include "TextRenderer.h"
#include "../Utils/Logger.h"

#include <algorithm>

namespace DDD {

TextRenderer::TextRenderer() = default;

TextRenderer::~TextRenderer() {
    Shutdown();
}

bool TextRenderer::Initialize(SDL_Renderer* renderer, const std::string& fontPath) {
    if (m_initialized) {
        Logger::Warning("TextRenderer already initialized");
        return true;
    }
    
    m_renderer = renderer;
    m_fontPath = fontPath;
    
    // Pre-load common font sizes
    std::vector<FontSize> sizes = {
        FontSize::Small,
        FontSize::Medium,
        FontSize::Large,
        FontSize::Title,
        FontSize::Huge
    };
    
    for (FontSize size : sizes) {
        TTF_Font* font = TTF_OpenFont(fontPath.c_str(), static_cast<int>(size));
        if (!font) {
            Logger::Error("Failed to load font '{}' at size {}: {}", 
                         fontPath, static_cast<int>(size), TTF_GetError());
            // Try to continue with other sizes
            continue;
        }
        m_fonts[static_cast<int>(size)] = font;
    }
    
    if (m_fonts.empty()) {
        Logger::Error("Failed to load any font sizes");
        return false;
    }
    
    m_initialized = true;
    Logger::Info("TextRenderer initialized with font: {}", fontPath);
    return true;
}

void TextRenderer::Shutdown() {
    for (auto& [size, font] : m_fonts) {
        if (font) {
            TTF_CloseFont(font);
        }
    }
    m_fonts.clear();
    m_initialized = false;
}

TTF_Font* TextRenderer::GetFont(FontSize size) {
    int sizeInt = static_cast<int>(size);
    
    // Check if already loaded
    auto it = m_fonts.find(sizeInt);
    if (it != m_fonts.end()) {
        return it->second;
    }
    
    // Try to load on demand
    TTF_Font* font = TTF_OpenFont(m_fontPath.c_str(), sizeInt);
    if (font) {
        m_fonts[sizeInt] = font;
        return font;
    }
    
    // Fall back to closest available size
    int closestSize = 0;
    int closestDiff = INT_MAX;
    for (auto& [s, f] : m_fonts) {
        int diff = std::abs(s - sizeInt);
        if (diff < closestDiff) {
            closestDiff = diff;
            closestSize = s;
        }
    }
    
    if (closestSize > 0) {
        return m_fonts[closestSize];
    }
    
    return nullptr;
}

SDL_Texture* TextRenderer::CreateTextTexture(const std::string& text, TTF_Font* font, SDL_Color color) {
    if (!font || text.empty()) return nullptr;
    
    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surface) {
        Logger::Error("Failed to render text surface: {}", TTF_GetError());
        return nullptr;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_FreeSurface(surface);
    
    if (!texture) {
        Logger::Error("Failed to create text texture: {}", SDL_GetError());
    }
    
    return texture;
}

int TextRenderer::CalculateAlignedX(int x, int textWidth, TextAlign align) {
    switch (align) {
        case TextAlign::Center:
            return x - textWidth / 2;
        case TextAlign::Right:
            return x - textWidth;
        case TextAlign::Left:
        default:
            return x;
    }
}

void TextRenderer::RenderText(const std::string& text, int x, int y, SDL_Color color,
                               FontSize size, TextAlign align) {
    if (!m_initialized || text.empty()) return;
    
    TTF_Font* font = GetFont(size);
    if (!font) return;
    
    SDL_Texture* texture = CreateTextTexture(text, font, color);
    if (!texture) return;
    
    int width, height;
    SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
    
    int alignedX = CalculateAlignedX(x, width, align);
    SDL_Rect destRect = {alignedX, y, width, height};
    
    SDL_RenderCopy(m_renderer, texture, nullptr, &destRect);
    SDL_DestroyTexture(texture);
}

void TextRenderer::RenderTextWithShadow(const std::string& text, int x, int y,
                                         SDL_Color color, SDL_Color shadowColor,
                                         FontSize size, TextAlign align, int shadowOffset) {
    if (!m_initialized || text.empty()) return;
    
    // Render shadow first (offset down-right)
    RenderText(text, x + shadowOffset, y + shadowOffset, shadowColor, size, align);
    
    // Render main text on top
    RenderText(text, x, y, color, size, align);
}

void TextRenderer::RenderTextWithOutline(const std::string& text, int x, int y,
                                          SDL_Color color, SDL_Color outlineColor,
                                          FontSize size, TextAlign align, int outlineWidth) {
    if (!m_initialized || text.empty()) return;
    
    // Render outline in 8 directions
    for (int dx = -outlineWidth; dx <= outlineWidth; dx++) {
        for (int dy = -outlineWidth; dy <= outlineWidth; dy++) {
            if (dx == 0 && dy == 0) continue;
            RenderText(text, x + dx, y + dy, outlineColor, size, align);
        }
    }
    
    // Render main text on top
    RenderText(text, x, y, color, size, align);
}

void TextRenderer::GetTextSize(const std::string& text, FontSize size, int& width, int& height) {
    width = 0;
    height = 0;
    
    if (!m_initialized || text.empty()) return;
    
    TTF_Font* font = GetFont(size);
    if (!font) return;
    
    TTF_SizeText(font, text.c_str(), &width, &height);
}

} // namespace DDD
