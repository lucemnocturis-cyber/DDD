#include "Renderer.h"
#include "../Utils/Logger.h"

#include <SDL2/SDL.h>

namespace DDD {

Renderer::Renderer(SDL_Renderer* sdlRenderer, int width, int height)
    : m_renderer(sdlRenderer)
    , m_width(width)
    , m_height(height)
{
}

Renderer::~Renderer() = default;

bool Renderer::Initialize() {
    // Set blend mode for alpha support
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    
    // Initialize text renderer
    // Try multiple font paths
    std::vector<std::string> fontPaths = {
        "assets/fonts/PressStart2P.ttf",
        "assets/fonts/pixel.ttf",
        "assets/fonts/default.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",  // Linux fallback
        "C:/Windows/Fonts/arial.ttf"  // Windows fallback
    };
    
    bool fontLoaded = false;
    for (const auto& path : fontPaths) {
        if (m_textRenderer.Initialize(m_renderer, path)) {
            fontLoaded = true;
            break;
        }
    }
    
    if (!fontLoaded) {
        Logger::Warning("Could not load any fonts - text will not display");
    }
    
    Logger::Info("Renderer initialized: {}x{}", m_width, m_height);
    return true;
}

void Renderer::OnWindowResize(int width, int height) {
    m_width = width;
    m_height = height;
}

void Renderer::Clear() {
    SDL_SetRenderDrawColor(m_renderer, 26, 26, 46, 255);  // Dark blue background
    SDL_RenderClear(m_renderer);
}

void Renderer::Present() {
    SDL_RenderPresent(m_renderer);
}

void Renderer::SetDrawColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    SDL_SetRenderDrawColor(m_renderer, r, g, b, a);
}

void Renderer::DrawRect(int x, int y, int w, int h, SDL_Color color) {
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderDrawRect(m_renderer, &rect);
}

void Renderer::FillRect(int x, int y, int w, int h, SDL_Color color) {
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(m_renderer, &rect);
}

void Renderer::DrawLine(int x1, int y1, int x2, int y2, SDL_Color color) {
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(m_renderer, x1, y1, x2, y2);
}

void Renderer::DrawCircle(int centerX, int centerY, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    
    int x = radius - 1;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (radius << 1);
    
    while (x >= y) {
        SDL_RenderDrawPoint(m_renderer, centerX + x, centerY + y);
        SDL_RenderDrawPoint(m_renderer, centerX + y, centerY + x);
        SDL_RenderDrawPoint(m_renderer, centerX - y, centerY + x);
        SDL_RenderDrawPoint(m_renderer, centerX - x, centerY + y);
        SDL_RenderDrawPoint(m_renderer, centerX - x, centerY - y);
        SDL_RenderDrawPoint(m_renderer, centerX - y, centerY - x);
        SDL_RenderDrawPoint(m_renderer, centerX + y, centerY - x);
        SDL_RenderDrawPoint(m_renderer, centerX + x, centerY - y);
        
        if (err <= 0) {
            y++;
            err += dy;
            dy += 2;
        }
        if (err > 0) {
            x--;
            dx += 2;
            err += dx - (radius << 1);
        }
    }
}

void Renderer::FillCircle(int centerX, int centerY, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                SDL_RenderDrawPoint(m_renderer, centerX + x, centerY + y);
            }
        }
    }
}

void Renderer::DrawTexture(SDL_Texture* texture, int x, int y) {
    int w, h;
    SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
    SDL_Rect destRect = {x, y, w, h};
    SDL_RenderCopy(m_renderer, texture, nullptr, &destRect);
}

void Renderer::DrawTexture(SDL_Texture* texture, const Rect& destRect) {
    SDL_Rect dest = {destRect.x, destRect.y, destRect.width, destRect.height};
    SDL_RenderCopy(m_renderer, texture, nullptr, &dest);
}

void Renderer::DrawTexture(SDL_Texture* texture, const Rect& srcRect, const Rect& destRect) {
    SDL_Rect src = {srcRect.x, srcRect.y, srcRect.width, srcRect.height};
    SDL_Rect dest = {destRect.x, destRect.y, destRect.width, destRect.height};
    SDL_RenderCopy(m_renderer, texture, &src, &dest);
}

void Renderer::DrawTextureColored(SDL_Texture* texture, const Rect& destRect, SDL_Color color) {
    SDL_SetTextureColorMod(texture, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(texture, color.a);
    DrawTexture(texture, destRect);
    SDL_SetTextureColorMod(texture, 255, 255, 255);
    SDL_SetTextureAlphaMod(texture, 255);
}

void Renderer::DrawText(const std::string& text, int x, int y, SDL_Color color,
                        FontSize size, TextAlign align) {
    if (m_textRenderer.IsInitialized()) {
        m_textRenderer.RenderText(text, x, y, color, size, align);
    }
}

void Renderer::DrawTextWithShadow(const std::string& text, int x, int y, SDL_Color color,
                                   FontSize size, TextAlign align) {
    if (m_textRenderer.IsInitialized()) {
        m_textRenderer.RenderTextWithShadow(text, x, y, color, Colors::ShadowColor, size, align);
    }
}

void Renderer::DrawTextWithOutline(const std::string& text, int x, int y,
                                    SDL_Color color, SDL_Color outlineColor,
                                    FontSize size, TextAlign align) {
    if (m_textRenderer.IsInitialized()) {
        m_textRenderer.RenderTextWithOutline(text, x, y, color, outlineColor, size, align);
    }
}

void Renderer::GetTextSize(const std::string& text, FontSize size, int& width, int& height) {
    m_textRenderer.GetTextSize(text, size, width, height);
}

} // namespace DDD
