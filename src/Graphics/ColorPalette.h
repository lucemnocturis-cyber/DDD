#pragma once

#include <SDL2/SDL.h>

namespace DDD {

/**
 * ColorPalette - centralized color definitions for visual consistency
 */
namespace Colors {
    // Background colors
    constexpr SDL_Color Background      = {26, 26, 46, 255};
    constexpr SDL_Color BackgroundLight = {35, 35, 60, 255};
    constexpr SDL_Color BackgroundDark  = {18, 18, 32, 255};
    
    // Panel colors
    constexpr SDL_Color PanelBg         = {30, 30, 45, 255};
    constexpr SDL_Color PanelBorder     = {60, 60, 90, 255};
    constexpr SDL_Color PanelHighlight  = {80, 80, 120, 255};
    
    // Primary colors (blue theme)
    constexpr SDL_Color Primary         = {70, 130, 200, 255};
    constexpr SDL_Color PrimaryLight    = {100, 160, 230, 255};
    constexpr SDL_Color PrimaryDark     = {50, 100, 160, 255};
    
    // Accent colors (gold/yellow)
    constexpr SDL_Color Accent          = {255, 200, 80, 255};
    constexpr SDL_Color AccentLight     = {255, 220, 120, 255};
    constexpr SDL_Color AccentDark      = {200, 160, 60, 255};
    
    // Player territory (blue)
    constexpr SDL_Color PlayerTerritory      = {40, 60, 120, 255};
    constexpr SDL_Color PlayerTerritoryLight = {50, 80, 150, 255};
    constexpr SDL_Color PlayerBorder         = {80, 120, 200, 255};
    
    // Enemy territory (red)
    constexpr SDL_Color EnemyTerritory       = {100, 40, 50, 255};
    constexpr SDL_Color EnemyTerritoryLight  = {130, 50, 60, 255};
    constexpr SDL_Color EnemyBorder          = {180, 80, 100, 255};
    
    // Neutral territory
    constexpr SDL_Color NeutralTerritory     = {45, 45, 55, 255};
    constexpr SDL_Color NeutralBorder        = {70, 70, 85, 255};
    
    // Unit class colors
    constexpr SDL_Color MageColor       = {100, 120, 220, 255};
    constexpr SDL_Color MageColorLight  = {130, 150, 255, 255};
    constexpr SDL_Color SoldierColor    = {200, 100, 100, 255};
    constexpr SDL_Color SoldierColorLight = {230, 130, 130, 255};
    constexpr SDL_Color RogueColor      = {100, 180, 120, 255};
    constexpr SDL_Color RogueColorLight = {130, 210, 150, 255};
    
    // Health bar colors
    constexpr SDL_Color HealthFull      = {80, 200, 80, 255};
    constexpr SDL_Color HealthMedium    = {200, 180, 60, 255};
    constexpr SDL_Color HealthLow       = {200, 80, 80, 255};
    constexpr SDL_Color HealthBg        = {40, 40, 50, 255};
    
    // Text colors
    constexpr SDL_Color TextPrimary     = {240, 240, 250, 255};
    constexpr SDL_Color TextSecondary   = {180, 180, 200, 255};
    constexpr SDL_Color TextMuted       = {120, 120, 140, 255};
    constexpr SDL_Color TextHighlight   = {255, 230, 150, 255};
    constexpr SDL_Color TextDanger      = {255, 100, 100, 255};
    constexpr SDL_Color TextSuccess     = {100, 255, 150, 255};
    
    // Button colors
    constexpr SDL_Color ButtonPrimary   = {60, 100, 180, 255};
    constexpr SDL_Color ButtonHover     = {80, 130, 210, 255};
    constexpr SDL_Color ButtonPressed   = {50, 80, 140, 255};
    constexpr SDL_Color ButtonDisabled  = {50, 50, 60, 255};
    
    constexpr SDL_Color ButtonDanger    = {180, 60, 60, 255};
    constexpr SDL_Color ButtonDangerHover = {210, 80, 80, 255};
    
    constexpr SDL_Color ButtonSuccess   = {60, 140, 80, 255};
    constexpr SDL_Color ButtonSuccessHover = {80, 170, 100, 255};
    
    // Selection/highlight colors
    constexpr SDL_Color SelectionValid  = {100, 200, 255, 180};
    constexpr SDL_Color SelectionAttack = {255, 100, 100, 180};
    constexpr SDL_Color SelectionHover  = {255, 255, 255, 100};
    
    // Damage number colors
    constexpr SDL_Color DamageNormal    = {255, 255, 255, 255};
    constexpr SDL_Color DamageCritical  = {255, 200, 50, 255};
    constexpr SDL_Color DamageHeal      = {100, 255, 150, 255};
    
    // Dice colors
    constexpr SDL_Color DiceAvailable   = {70, 100, 160, 255};
    constexpr SDL_Color DiceSelected    = {100, 150, 220, 255};
    constexpr SDL_Color DiceCooldown    = {50, 50, 60, 255};
    
    // Status effect colors
    constexpr SDL_Color StatusBuff      = {100, 200, 255, 255};
    constexpr SDL_Color StatusDebuff    = {255, 100, 100, 255};
    constexpr SDL_Color StatusNeutral   = {200, 200, 200, 255};
    
    // Turn indicator colors
    constexpr SDL_Color TurnPlayer      = {80, 150, 255, 255};
    constexpr SDL_Color TurnEnemy       = {255, 100, 100, 255};
    
    // Wave/level colors
    constexpr SDL_Color WaveNormal      = {255, 200, 80, 255};
    constexpr SDL_Color WaveBoss        = {200, 50, 50, 255};
}

/**
 * Helper functions
 */
inline SDL_Color WithAlpha(const SDL_Color& color, uint8_t alpha) {
    return {color.r, color.g, color.b, alpha};
}

inline SDL_Color Lerp(const SDL_Color& a, const SDL_Color& b, float t) {
    return {
        static_cast<uint8_t>(a.r + (b.r - a.r) * t),
        static_cast<uint8_t>(a.g + (b.g - a.g) * t),
        static_cast<uint8_t>(a.b + (b.b - a.b) * t),
        static_cast<uint8_t>(a.a + (b.a - a.a) * t)
    };
}

inline SDL_Color Brighten(const SDL_Color& color, float amount) {
    return {
        static_cast<uint8_t>(std::min(255, static_cast<int>(color.r * (1.0f + amount)))),
        static_cast<uint8_t>(std::min(255, static_cast<int>(color.g * (1.0f + amount)))),
        static_cast<uint8_t>(std::min(255, static_cast<int>(color.b * (1.0f + amount)))),
        color.a
    };
}

inline SDL_Color Darken(const SDL_Color& color, float amount) {
    return {
        static_cast<uint8_t>(static_cast<int>(color.r * (1.0f - amount))),
        static_cast<uint8_t>(static_cast<int>(color.g * (1.0f - amount))),
        static_cast<uint8_t>(static_cast<int>(color.b * (1.0f - amount))),
        color.a
    };
}

} // namespace DDD
