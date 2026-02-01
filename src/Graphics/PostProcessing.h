#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <cmath>

namespace DDD {

/**
 * Post-processing effect types
 */
enum class PostEffectType {
    None,
    Bloom,
    Vignette,
    ChromaticAberration,
    ColorGrading,
    Scanlines,
    CRT,
    Pixelate,
    Blur,
    Sharpen,
    Grayscale,
    Sepia,
    Invert,
    Posterize,
    FilmGrain,
    ScreenWarp,
    Glitch,
    HeatDistortion
};

/**
 * Color grading preset
 */
struct ColorGrade {
    std::string name;
    float brightness = 0.0f;     // -1 to 1
    float contrast = 1.0f;       // 0 to 2
    float saturation = 1.0f;     // 0 to 2
    float gamma = 1.0f;          // 0.1 to 3
    SDL_Color tint = {255, 255, 255, 255};
    SDL_Color shadows = {0, 0, 0, 255};
    SDL_Color highlights = {255, 255, 255, 255};
};

/**
 * Individual effect configuration
 */
struct PostEffectConfig {
    PostEffectType type;
    bool enabled = true;
    float intensity = 1.0f;
    
    // Effect-specific parameters
    float param1 = 0.0f;
    float param2 = 0.0f;
    float param3 = 0.0f;
    float param4 = 0.0f;
    SDL_Color color1 = {255, 255, 255, 255};
    SDL_Color color2 = {0, 0, 0, 255};
};

/**
 * Effect preset (combination of effects)
 */
struct EffectPreset {
    std::string id;
    std::string name;
    std::vector<PostEffectConfig> effects;
};

/**
 * Timed effect for transitions
 */
struct TimedEffect {
    PostEffectType type;
    float duration;
    float elapsed;
    float startIntensity;
    float endIntensity;
    bool active;
};

/**
 * PostProcessingSystem - screen-wide visual effects
 */
class PostProcessingSystem {
public:
    static PostProcessingSystem& Instance();
    
    void Initialize(SDL_Renderer* renderer, int width, int height);
    void Shutdown();
    void Update(float deltaTime);
    
    // Main processing
    void BeginCapture();
    void EndCapture();
    void Apply();
    
    // Effect management
    void EnableEffect(PostEffectType type);
    void DisableEffect(PostEffectType type);
    void SetEffectIntensity(PostEffectType type, float intensity);
    void SetEffectParams(PostEffectType type, float p1, float p2 = 0, float p3 = 0, float p4 = 0);
    bool IsEffectEnabled(PostEffectType type) const;
    float GetEffectIntensity(PostEffectType type) const;
    
    // Presets
    void RegisterPreset(const EffectPreset& preset);
    void ApplyPreset(const std::string& presetId);
    void ClearAllEffects();
    
    // Color grading
    void SetColorGrade(const ColorGrade& grade);
    void SetBrightness(float brightness);
    void SetContrast(float contrast);
    void SetSaturation(float saturation);
    void SetGamma(float gamma);
    void SetTint(const SDL_Color& tint);
    
    // Timed effects
    void FlashEffect(PostEffectType type, float duration, float intensity = 1.0f);
    void FadeEffect(PostEffectType type, float duration, float startIntensity, float endIntensity);
    void PulseEffect(PostEffectType type, float frequency, float minIntensity, float maxIntensity);
    
    // Quick effects
    void FlashWhite(float duration = 0.1f);
    void FlashRed(float duration = 0.1f);
    void FadeToBlack(float duration = 1.0f);
    void FadeFromBlack(float duration = 1.0f);
    void DamageFlash();
    void HealFlash();
    void LevelUpFlash();
    
    // Queries
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    
private:
    PostProcessingSystem() = default;
    
    void RegisterDefaultPresets();
    
    // Effect processors
    void ApplyBloom(SDL_Surface* surface);
    void ApplyVignette(SDL_Surface* surface);
    void ApplyChromaticAberration(SDL_Surface* surface);
    void ApplyColorGrading(SDL_Surface* surface);
    void ApplyScanlines(SDL_Surface* surface);
    void ApplyCRT(SDL_Surface* surface);
    void ApplyPixelate(SDL_Surface* surface);
    void ApplyBlur(SDL_Surface* surface);
    void ApplySharpen(SDL_Surface* surface);
    void ApplyGrayscale(SDL_Surface* surface);
    void ApplySepia(SDL_Surface* surface);
    void ApplyInvert(SDL_Surface* surface);
    void ApplyPosterize(SDL_Surface* surface);
    void ApplyFilmGrain(SDL_Surface* surface);
    void ApplyScreenWarp(SDL_Surface* surface);
    void ApplyGlitch(SDL_Surface* surface);
    void ApplyHeatDistortion(SDL_Surface* surface);
    
    // Helpers
    Uint32 GetPixel(SDL_Surface* surface, int x, int y) const;
    void SetPixel(SDL_Surface* surface, int x, int y, Uint32 pixel);
    SDL_Color Uint32ToColor(Uint32 pixel, SDL_PixelFormat* format) const;
    Uint32 ColorToUint32(const SDL_Color& color, SDL_PixelFormat* format) const;
    float Clamp(float value, float min, float max) const;
    int ClampInt(int value, int min, int max) const;
    
    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* m_captureTexture = nullptr;
    SDL_Texture* m_processedTexture = nullptr;
    int m_width = 0;
    int m_height = 0;
    
    // Active effects
    std::unordered_map<PostEffectType, PostEffectConfig> m_activeEffects;
    std::vector<TimedEffect> m_timedEffects;
    
    // Color grading
    ColorGrade m_colorGrade;
    
    // Presets
    std::unordered_map<std::string, EffectPreset> m_presets;
    
    // Pulse effects
    struct PulseData {
        PostEffectType type;
        float frequency;
        float minIntensity;
        float maxIntensity;
        float phase;
        bool active;
    };
    std::vector<PulseData> m_pulseEffects;
    
    // Flash overlay
    bool m_flashActive = false;
    SDL_Color m_flashColor = {255, 255, 255, 255};
    float m_flashDuration = 0.0f;
    float m_flashTimer = 0.0f;
    
    // Fade overlay
    bool m_fadeActive = false;
    float m_fadeAlpha = 0.0f;
    float m_fadeTarget = 0.0f;
    float m_fadeDuration = 0.0f;
    SDL_Color m_fadeColor = {0, 0, 0, 255};
    
    // Animation time
    float m_time = 0.0f;
    
    bool m_initialized = false;
    bool m_capturing = false;
};

// ===========================================================================
// PRESET BUILDERS
// ===========================================================================

namespace PostPresets {
    EffectPreset CreateRetroPreset();
    EffectPreset CreateCinematicPreset();
    EffectPreset CreateNightmarePreset();
    EffectPreset CreateDreamPreset();
    EffectPreset CreatePoisonedPreset();
    EffectPreset CreateFrozenPreset();
    EffectPreset CreateBurningPreset();
    EffectPreset CreateBossPreset();
}

} // namespace DDD
