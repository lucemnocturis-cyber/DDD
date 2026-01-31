#include "PostProcessing.h"
#include "../Utils/Logger.h"
#include "../Utils/Random.h"

#include <algorithm>
#include <cstring>

namespace DDD {

PostProcessingSystem& PostProcessingSystem::Instance() {
    static PostProcessingSystem instance;
    return instance;
}

void PostProcessingSystem::Initialize(SDL_Renderer* renderer, int width, int height) {
    if (m_initialized) return;
    
    m_renderer = renderer;
    m_width = width;
    m_height = height;
    
    // Create capture texture
    m_captureTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                          SDL_TEXTUREACCESS_TARGET, width, height);
    m_processedTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                            SDL_TEXTUREACCESS_STREAMING, width, height);
    
    // Default color grade
    m_colorGrade.name = "Default";
    m_colorGrade.brightness = 0.0f;
    m_colorGrade.contrast = 1.0f;
    m_colorGrade.saturation = 1.0f;
    m_colorGrade.gamma = 1.0f;
    
    RegisterDefaultPresets();
    
    m_initialized = true;
    Logger::Info("PostProcessingSystem initialized ({}x{})", width, height);
}

void PostProcessingSystem::Shutdown() {
    if (m_captureTexture) {
        SDL_DestroyTexture(m_captureTexture);
        m_captureTexture = nullptr;
    }
    if (m_processedTexture) {
        SDL_DestroyTexture(m_processedTexture);
        m_processedTexture = nullptr;
    }
    
    m_activeEffects.clear();
    m_timedEffects.clear();
    m_presets.clear();
    m_initialized = false;
}

void PostProcessingSystem::Update(float deltaTime) {
    m_time += deltaTime;
    
    // Update timed effects
    for (auto& effect : m_timedEffects) {
        if (!effect.active) continue;
        
        effect.elapsed += deltaTime;
        float t = effect.elapsed / effect.duration;
        
        if (t >= 1.0f) {
            effect.active = false;
            if (effect.endIntensity <= 0.0f) {
                DisableEffect(effect.type);
            } else {
                SetEffectIntensity(effect.type, effect.endIntensity);
            }
        } else {
            float intensity = effect.startIntensity + (effect.endIntensity - effect.startIntensity) * t;
            SetEffectIntensity(effect.type, intensity);
        }
    }
    
    // Remove inactive timed effects
    m_timedEffects.erase(
        std::remove_if(m_timedEffects.begin(), m_timedEffects.end(),
            [](const TimedEffect& e) { return !e.active; }),
        m_timedEffects.end()
    );
    
    // Update pulse effects
    for (auto& pulse : m_pulseEffects) {
        if (!pulse.active) continue;
        
        pulse.phase += deltaTime * pulse.frequency * 2.0f * 3.14159f;
        float t = (std::sin(pulse.phase) + 1.0f) * 0.5f;
        float intensity = pulse.minIntensity + (pulse.maxIntensity - pulse.minIntensity) * t;
        SetEffectIntensity(pulse.type, intensity);
    }
    
    // Update flash
    if (m_flashActive) {
        m_flashTimer += deltaTime;
        if (m_flashTimer >= m_flashDuration) {
            m_flashActive = false;
        }
    }
    
    // Update fade
    if (m_fadeActive) {
        float fadeSpeed = 1.0f / m_fadeDuration;
        if (m_fadeAlpha < m_fadeTarget) {
            m_fadeAlpha = std::min(m_fadeAlpha + fadeSpeed * deltaTime, m_fadeTarget);
        } else if (m_fadeAlpha > m_fadeTarget) {
            m_fadeAlpha = std::max(m_fadeAlpha - fadeSpeed * deltaTime, m_fadeTarget);
        }
        
        if (std::abs(m_fadeAlpha - m_fadeTarget) < 0.01f) {
            m_fadeAlpha = m_fadeTarget;
            if (m_fadeTarget == 0.0f) {
                m_fadeActive = false;
            }
        }
    }
}

void PostProcessingSystem::BeginCapture() {
    if (!m_initialized || m_capturing) return;
    
    SDL_SetRenderTarget(m_renderer, m_captureTexture);
    m_capturing = true;
}

void PostProcessingSystem::EndCapture() {
    if (!m_capturing) return;
    
    SDL_SetRenderTarget(m_renderer, nullptr);
    m_capturing = false;
}

void PostProcessingSystem::Apply() {
    if (!m_initialized || m_activeEffects.empty() && !m_flashActive && !m_fadeActive) {
        // No effects, just render capture directly
        SDL_RenderCopy(m_renderer, m_captureTexture, nullptr, nullptr);
        return;
    }
    
    // For CPU-based processing, we need to read pixels
    // This is simplified - real implementation would use shaders
    
    // Read capture texture to surface
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, m_width, m_height, 32, SDL_PIXELFORMAT_RGBA8888);
    if (!surface) {
        SDL_RenderCopy(m_renderer, m_captureTexture, nullptr, nullptr);
        return;
    }
    
    SDL_RenderReadPixels(m_renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch);
    SDL_SetRenderTarget(m_renderer, m_captureTexture);
    SDL_RenderReadPixels(m_renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch);
    SDL_SetRenderTarget(m_renderer, nullptr);
    
    // Apply effects in order
    SDL_LockSurface(surface);
    
    // Always apply color grading first
    ApplyColorGrading(surface);
    
    for (const auto& [type, config] : m_activeEffects) {
        if (!config.enabled || config.intensity <= 0.0f) continue;
        
        switch (type) {
            case PostEffectType::Vignette: ApplyVignette(surface); break;
            case PostEffectType::Scanlines: ApplyScanlines(surface); break;
            case PostEffectType::Grayscale: ApplyGrayscale(surface); break;
            case PostEffectType::Sepia: ApplySepia(surface); break;
            case PostEffectType::Invert: ApplyInvert(surface); break;
            case PostEffectType::Posterize: ApplyPosterize(surface); break;
            case PostEffectType::FilmGrain: ApplyFilmGrain(surface); break;
            case PostEffectType::Pixelate: ApplyPixelate(surface); break;
            default: break;
        }
    }
    
    SDL_UnlockSurface(surface);
    
    // Update processed texture
    SDL_UpdateTexture(m_processedTexture, nullptr, surface->pixels, surface->pitch);
    SDL_FreeSurface(surface);
    
    // Render processed result
    SDL_RenderCopy(m_renderer, m_processedTexture, nullptr, nullptr);
    
    // Apply overlays
    if (m_flashActive) {
        float t = m_flashTimer / m_flashDuration;
        Uint8 alpha = static_cast<Uint8>((1.0f - t) * 255);
        SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(m_renderer, m_flashColor.r, m_flashColor.g, m_flashColor.b, alpha);
        SDL_Rect fullScreen = {0, 0, m_width, m_height};
        SDL_RenderFillRect(m_renderer, &fullScreen);
    }
    
    if (m_fadeActive && m_fadeAlpha > 0.0f) {
        Uint8 alpha = static_cast<Uint8>(m_fadeAlpha * 255);
        SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(m_renderer, m_fadeColor.r, m_fadeColor.g, m_fadeColor.b, alpha);
        SDL_Rect fullScreen = {0, 0, m_width, m_height};
        SDL_RenderFillRect(m_renderer, &fullScreen);
    }
}

void PostProcessingSystem::EnableEffect(PostEffectType type) {
    if (m_activeEffects.find(type) == m_activeEffects.end()) {
        PostEffectConfig config;
        config.type = type;
        config.enabled = true;
        config.intensity = 1.0f;
        m_activeEffects[type] = config;
    } else {
        m_activeEffects[type].enabled = true;
    }
}

void PostProcessingSystem::DisableEffect(PostEffectType type) {
    auto it = m_activeEffects.find(type);
    if (it != m_activeEffects.end()) {
        it->second.enabled = false;
    }
}

void PostProcessingSystem::SetEffectIntensity(PostEffectType type, float intensity) {
    if (m_activeEffects.find(type) == m_activeEffects.end()) {
        EnableEffect(type);
    }
    m_activeEffects[type].intensity = Clamp(intensity, 0.0f, 2.0f);
}

void PostProcessingSystem::SetEffectParams(PostEffectType type, float p1, float p2, float p3, float p4) {
    if (m_activeEffects.find(type) == m_activeEffects.end()) {
        EnableEffect(type);
    }
    auto& config = m_activeEffects[type];
    config.param1 = p1;
    config.param2 = p2;
    config.param3 = p3;
    config.param4 = p4;
}

bool PostProcessingSystem::IsEffectEnabled(PostEffectType type) const {
    auto it = m_activeEffects.find(type);
    return it != m_activeEffects.end() && it->second.enabled;
}

float PostProcessingSystem::GetEffectIntensity(PostEffectType type) const {
    auto it = m_activeEffects.find(type);
    return it != m_activeEffects.end() ? it->second.intensity : 0.0f;
}

void PostProcessingSystem::RegisterPreset(const EffectPreset& preset) {
    m_presets[preset.id] = preset;
}

void PostProcessingSystem::ApplyPreset(const std::string& presetId) {
    auto it = m_presets.find(presetId);
    if (it == m_presets.end()) {
        Logger::Warning("Post-processing preset not found: {}", presetId);
        return;
    }
    
    ClearAllEffects();
    
    for (const auto& effect : it->second.effects) {
        m_activeEffects[effect.type] = effect;
    }
    
    Logger::Info("Applied post-processing preset: {}", it->second.name);
}

void PostProcessingSystem::ClearAllEffects() {
    m_activeEffects.clear();
    m_timedEffects.clear();
    m_pulseEffects.clear();
}

void PostProcessingSystem::SetColorGrade(const ColorGrade& grade) {
    m_colorGrade = grade;
}

void PostProcessingSystem::SetBrightness(float brightness) {
    m_colorGrade.brightness = Clamp(brightness, -1.0f, 1.0f);
}

void PostProcessingSystem::SetContrast(float contrast) {
    m_colorGrade.contrast = Clamp(contrast, 0.0f, 2.0f);
}

void PostProcessingSystem::SetSaturation(float saturation) {
    m_colorGrade.saturation = Clamp(saturation, 0.0f, 2.0f);
}

void PostProcessingSystem::SetGamma(float gamma) {
    m_colorGrade.gamma = Clamp(gamma, 0.1f, 3.0f);
}

void PostProcessingSystem::SetTint(const SDL_Color& tint) {
    m_colorGrade.tint = tint;
}

void PostProcessingSystem::FlashEffect(PostEffectType type, float duration, float intensity) {
    EnableEffect(type);
    SetEffectIntensity(type, intensity);
    
    TimedEffect timed;
    timed.type = type;
    timed.duration = duration;
    timed.elapsed = 0.0f;
    timed.startIntensity = intensity;
    timed.endIntensity = 0.0f;
    timed.active = true;
    
    m_timedEffects.push_back(timed);
}

void PostProcessingSystem::FadeEffect(PostEffectType type, float duration, float startIntensity, float endIntensity) {
    EnableEffect(type);
    SetEffectIntensity(type, startIntensity);
    
    TimedEffect timed;
    timed.type = type;
    timed.duration = duration;
    timed.elapsed = 0.0f;
    timed.startIntensity = startIntensity;
    timed.endIntensity = endIntensity;
    timed.active = true;
    
    m_timedEffects.push_back(timed);
}

void PostProcessingSystem::PulseEffect(PostEffectType type, float frequency, float minIntensity, float maxIntensity) {
    EnableEffect(type);
    
    PulseData pulse;
    pulse.type = type;
    pulse.frequency = frequency;
    pulse.minIntensity = minIntensity;
    pulse.maxIntensity = maxIntensity;
    pulse.phase = 0.0f;
    pulse.active = true;
    
    m_pulseEffects.push_back(pulse);
}

void PostProcessingSystem::FlashWhite(float duration) {
    m_flashActive = true;
    m_flashColor = {255, 255, 255, 255};
    m_flashDuration = duration;
    m_flashTimer = 0.0f;
}

void PostProcessingSystem::FlashRed(float duration) {
    m_flashActive = true;
    m_flashColor = {255, 50, 50, 255};
    m_flashDuration = duration;
    m_flashTimer = 0.0f;
}

void PostProcessingSystem::FadeToBlack(float duration) {
    m_fadeActive = true;
    m_fadeColor = {0, 0, 0, 255};
    m_fadeTarget = 1.0f;
    m_fadeDuration = duration;
}

void PostProcessingSystem::FadeFromBlack(float duration) {
    m_fadeActive = true;
    m_fadeColor = {0, 0, 0, 255};
    m_fadeAlpha = 1.0f;
    m_fadeTarget = 0.0f;
    m_fadeDuration = duration;
}

void PostProcessingSystem::DamageFlash() {
    FlashRed(0.15f);
    FlashEffect(PostEffectType::Vignette, 0.3f, 0.8f);
}

void PostProcessingSystem::HealFlash() {
    m_flashActive = true;
    m_flashColor = {100, 255, 100, 255};
    m_flashDuration = 0.2f;
    m_flashTimer = 0.0f;
}

void PostProcessingSystem::LevelUpFlash() {
    m_flashActive = true;
    m_flashColor = {255, 220, 100, 255};
    m_flashDuration = 0.3f;
    m_flashTimer = 0.0f;
}

// ===========================================================================
// EFFECT IMPLEMENTATIONS
// ===========================================================================

void PostProcessingSystem::ApplyColorGrading(SDL_Surface* surface) {
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);
    int pixelCount = m_width * m_height;
    
    for (int i = 0; i < pixelCount; ++i) {
        SDL_Color c = Uint32ToColor(pixels[i], surface->format);
        
        // Apply brightness
        float r = c.r / 255.0f + m_colorGrade.brightness;
        float g = c.g / 255.0f + m_colorGrade.brightness;
        float b = c.b / 255.0f + m_colorGrade.brightness;
        
        // Apply contrast
        r = (r - 0.5f) * m_colorGrade.contrast + 0.5f;
        g = (g - 0.5f) * m_colorGrade.contrast + 0.5f;
        b = (b - 0.5f) * m_colorGrade.contrast + 0.5f;
        
        // Apply saturation
        float gray = 0.299f * r + 0.587f * g + 0.114f * b;
        r = gray + (r - gray) * m_colorGrade.saturation;
        g = gray + (g - gray) * m_colorGrade.saturation;
        b = gray + (b - gray) * m_colorGrade.saturation;
        
        // Apply gamma
        r = std::pow(Clamp(r, 0.0f, 1.0f), 1.0f / m_colorGrade.gamma);
        g = std::pow(Clamp(g, 0.0f, 1.0f), 1.0f / m_colorGrade.gamma);
        b = std::pow(Clamp(b, 0.0f, 1.0f), 1.0f / m_colorGrade.gamma);
        
        // Apply tint
        r *= m_colorGrade.tint.r / 255.0f;
        g *= m_colorGrade.tint.g / 255.0f;
        b *= m_colorGrade.tint.b / 255.0f;
        
        c.r = static_cast<Uint8>(Clamp(r, 0.0f, 1.0f) * 255);
        c.g = static_cast<Uint8>(Clamp(g, 0.0f, 1.0f) * 255);
        c.b = static_cast<Uint8>(Clamp(b, 0.0f, 1.0f) * 255);
        
        pixels[i] = ColorToUint32(c, surface->format);
    }
}

void PostProcessingSystem::ApplyVignette(SDL_Surface* surface) {
    auto it = m_activeEffects.find(PostEffectType::Vignette);
    float intensity = (it != m_activeEffects.end()) ? it->second.intensity : 0.5f;
    
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);
    float centerX = m_width * 0.5f;
    float centerY = m_height * 0.5f;
    float maxDist = std::sqrt(centerX * centerX + centerY * centerY);
    
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            float dx = x - centerX;
            float dy = y - centerY;
            float dist = std::sqrt(dx * dx + dy * dy) / maxDist;
            
            float vignette = 1.0f - dist * dist * intensity;
            vignette = Clamp(vignette, 0.0f, 1.0f);
            
            int idx = y * m_width + x;
            SDL_Color c = Uint32ToColor(pixels[idx], surface->format);
            
            c.r = static_cast<Uint8>(c.r * vignette);
            c.g = static_cast<Uint8>(c.g * vignette);
            c.b = static_cast<Uint8>(c.b * vignette);
            
            pixels[idx] = ColorToUint32(c, surface->format);
        }
    }
}

void PostProcessingSystem::ApplyScanlines(SDL_Surface* surface) {
    auto it = m_activeEffects.find(PostEffectType::Scanlines);
    float intensity = (it != m_activeEffects.end()) ? it->second.intensity : 0.3f;
    int spacing = (it != m_activeEffects.end()) ? static_cast<int>(it->second.param1) : 2;
    if (spacing < 1) spacing = 2;
    
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);
    
    for (int y = 0; y < m_height; y += spacing) {
        for (int x = 0; x < m_width; ++x) {
            int idx = y * m_width + x;
            SDL_Color c = Uint32ToColor(pixels[idx], surface->format);
            
            float darken = 1.0f - intensity;
            c.r = static_cast<Uint8>(c.r * darken);
            c.g = static_cast<Uint8>(c.g * darken);
            c.b = static_cast<Uint8>(c.b * darken);
            
            pixels[idx] = ColorToUint32(c, surface->format);
        }
    }
}

void PostProcessingSystem::ApplyGrayscale(SDL_Surface* surface) {
    auto it = m_activeEffects.find(PostEffectType::Grayscale);
    float intensity = (it != m_activeEffects.end()) ? it->second.intensity : 1.0f;
    
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);
    int pixelCount = m_width * m_height;
    
    for (int i = 0; i < pixelCount; ++i) {
        SDL_Color c = Uint32ToColor(pixels[i], surface->format);
        
        Uint8 gray = static_cast<Uint8>(0.299f * c.r + 0.587f * c.g + 0.114f * c.b);
        
        c.r = static_cast<Uint8>(c.r + (gray - c.r) * intensity);
        c.g = static_cast<Uint8>(c.g + (gray - c.g) * intensity);
        c.b = static_cast<Uint8>(c.b + (gray - c.b) * intensity);
        
        pixels[i] = ColorToUint32(c, surface->format);
    }
}

void PostProcessingSystem::ApplySepia(SDL_Surface* surface) {
    auto it = m_activeEffects.find(PostEffectType::Sepia);
    float intensity = (it != m_activeEffects.end()) ? it->second.intensity : 1.0f;
    
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);
    int pixelCount = m_width * m_height;
    
    for (int i = 0; i < pixelCount; ++i) {
        SDL_Color c = Uint32ToColor(pixels[i], surface->format);
        
        float sr = c.r * 0.393f + c.g * 0.769f + c.b * 0.189f;
        float sg = c.r * 0.349f + c.g * 0.686f + c.b * 0.168f;
        float sb = c.r * 0.272f + c.g * 0.534f + c.b * 0.131f;
        
        c.r = static_cast<Uint8>(Clamp(c.r + (sr - c.r) * intensity, 0.0f, 255.0f));
        c.g = static_cast<Uint8>(Clamp(c.g + (sg - c.g) * intensity, 0.0f, 255.0f));
        c.b = static_cast<Uint8>(Clamp(c.b + (sb - c.b) * intensity, 0.0f, 255.0f));
        
        pixels[i] = ColorToUint32(c, surface->format);
    }
}

void PostProcessingSystem::ApplyInvert(SDL_Surface* surface) {
    auto it = m_activeEffects.find(PostEffectType::Invert);
    float intensity = (it != m_activeEffects.end()) ? it->second.intensity : 1.0f;
    
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);
    int pixelCount = m_width * m_height;
    
    for (int i = 0; i < pixelCount; ++i) {
        SDL_Color c = Uint32ToColor(pixels[i], surface->format);
        
        Uint8 ir = 255 - c.r;
        Uint8 ig = 255 - c.g;
        Uint8 ib = 255 - c.b;
        
        c.r = static_cast<Uint8>(c.r + (ir - c.r) * intensity);
        c.g = static_cast<Uint8>(c.g + (ig - c.g) * intensity);
        c.b = static_cast<Uint8>(c.b + (ib - c.b) * intensity);
        
        pixels[i] = ColorToUint32(c, surface->format);
    }
}

void PostProcessingSystem::ApplyPosterize(SDL_Surface* surface) {
    auto it = m_activeEffects.find(PostEffectType::Posterize);
    int levels = (it != m_activeEffects.end()) ? static_cast<int>(it->second.param1) : 4;
    if (levels < 2) levels = 4;
    
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);
    int pixelCount = m_width * m_height;
    float step = 255.0f / (levels - 1);
    
    for (int i = 0; i < pixelCount; ++i) {
        SDL_Color c = Uint32ToColor(pixels[i], surface->format);
        
        c.r = static_cast<Uint8>(std::round(c.r / step) * step);
        c.g = static_cast<Uint8>(std::round(c.g / step) * step);
        c.b = static_cast<Uint8>(std::round(c.b / step) * step);
        
        pixels[i] = ColorToUint32(c, surface->format);
    }
}

void PostProcessingSystem::ApplyFilmGrain(SDL_Surface* surface) {
    auto it = m_activeEffects.find(PostEffectType::FilmGrain);
    float intensity = (it != m_activeEffects.end()) ? it->second.intensity : 0.1f;
    
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);
    int pixelCount = m_width * m_height;
    
    for (int i = 0; i < pixelCount; ++i) {
        SDL_Color c = Uint32ToColor(pixels[i], surface->format);
        
        float noise = (Random::Range(0.0f, 1.0f) - 0.5f) * 2.0f * intensity * 255.0f;
        
        c.r = static_cast<Uint8>(ClampInt(c.r + static_cast<int>(noise), 0, 255));
        c.g = static_cast<Uint8>(ClampInt(c.g + static_cast<int>(noise), 0, 255));
        c.b = static_cast<Uint8>(ClampInt(c.b + static_cast<int>(noise), 0, 255));
        
        pixels[i] = ColorToUint32(c, surface->format);
    }
}

void PostProcessingSystem::ApplyPixelate(SDL_Surface* surface) {
    auto it = m_activeEffects.find(PostEffectType::Pixelate);
    int pixelSize = (it != m_activeEffects.end()) ? static_cast<int>(it->second.param1) : 4;
    if (pixelSize < 2) pixelSize = 4;
    
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);
    
    for (int y = 0; y < m_height; y += pixelSize) {
        for (int x = 0; x < m_width; x += pixelSize) {
            // Sample center of block
            int sampleX = std::min(x + pixelSize / 2, m_width - 1);
            int sampleY = std::min(y + pixelSize / 2, m_height - 1);
            Uint32 sampleColor = pixels[sampleY * m_width + sampleX];
            
            // Fill block
            for (int by = 0; by < pixelSize && y + by < m_height; ++by) {
                for (int bx = 0; bx < pixelSize && x + bx < m_width; ++bx) {
                    pixels[(y + by) * m_width + (x + bx)] = sampleColor;
                }
            }
        }
    }
}

void PostProcessingSystem::ApplyBloom(SDL_Surface* surface) {
    // Simplified bloom - just brighten bright pixels
    auto it = m_activeEffects.find(PostEffectType::Bloom);
    float intensity = (it != m_activeEffects.end()) ? it->second.intensity : 0.5f;
    float threshold = 0.7f;
    
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);
    int pixelCount = m_width * m_height;
    
    for (int i = 0; i < pixelCount; ++i) {
        SDL_Color c = Uint32ToColor(pixels[i], surface->format);
        
        float brightness = (c.r + c.g + c.b) / (3.0f * 255.0f);
        if (brightness > threshold) {
            float bloom = (brightness - threshold) * intensity;
            c.r = static_cast<Uint8>(std::min(255.0f, c.r + bloom * 255));
            c.g = static_cast<Uint8>(std::min(255.0f, c.g + bloom * 255));
            c.b = static_cast<Uint8>(std::min(255.0f, c.b + bloom * 255));
            pixels[i] = ColorToUint32(c, surface->format);
        }
    }
}

void PostProcessingSystem::ApplyChromaticAberration(SDL_Surface* surface) {
    // Would need proper implementation with offset sampling
}

void PostProcessingSystem::ApplyCRT(SDL_Surface* surface) {
    ApplyScanlines(surface);
    ApplyVignette(surface);
}

void PostProcessingSystem::ApplyBlur(SDL_Surface* surface) {
    // Simple box blur would go here
}

void PostProcessingSystem::ApplySharpen(SDL_Surface* surface) {
    // Convolution kernel would go here
}

void PostProcessingSystem::ApplyScreenWarp(SDL_Surface* surface) {
    // Barrel distortion would go here
}

void PostProcessingSystem::ApplyGlitch(SDL_Surface* surface) {
    // Random horizontal shifts would go here
}

void PostProcessingSystem::ApplyHeatDistortion(SDL_Surface* surface) {
    // Sine wave distortion would go here
}

// ===========================================================================
// HELPERS
// ===========================================================================

Uint32 PostProcessingSystem::GetPixel(SDL_Surface* surface, int x, int y) const {
    if (x < 0 || x >= surface->w || y < 0 || y >= surface->h) return 0;
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);
    return pixels[y * surface->w + x];
}

void PostProcessingSystem::SetPixel(SDL_Surface* surface, int x, int y, Uint32 pixel) {
    if (x < 0 || x >= surface->w || y < 0 || y >= surface->h) return;
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);
    pixels[y * surface->w + x] = pixel;
}

SDL_Color PostProcessingSystem::Uint32ToColor(Uint32 pixel, SDL_PixelFormat* format) const {
    SDL_Color color;
    SDL_GetRGBA(pixel, format, &color.r, &color.g, &color.b, &color.a);
    return color;
}

Uint32 PostProcessingSystem::ColorToUint32(const SDL_Color& color, SDL_PixelFormat* format) const {
    return SDL_MapRGBA(format, color.r, color.g, color.b, color.a);
}

float PostProcessingSystem::Clamp(float value, float min, float max) const {
    return std::max(min, std::min(max, value));
}

int PostProcessingSystem::ClampInt(int value, int min, int max) const {
    return std::max(min, std::min(max, value));
}

// ===========================================================================
// PRESET REGISTRATION
// ===========================================================================

void PostProcessingSystem::RegisterDefaultPresets() {
    // Retro/CRT preset
    EffectPreset retro;
    retro.id = "retro";
    retro.name = "Retro CRT";
    retro.effects = {
        {PostEffectType::Scanlines, true, 0.3f, 2.0f},
        {PostEffectType::Vignette, true, 0.5f},
        {PostEffectType::FilmGrain, true, 0.05f}
    };
    RegisterPreset(retro);
    
    // Cinematic preset
    EffectPreset cinematic;
    cinematic.id = "cinematic";
    cinematic.name = "Cinematic";
    cinematic.effects = {
        {PostEffectType::Vignette, true, 0.4f}
    };
    RegisterPreset(cinematic);
    
    // Nightmare preset
    EffectPreset nightmare;
    nightmare.id = "nightmare";
    nightmare.name = "Nightmare";
    nightmare.effects = {
        {PostEffectType::Vignette, true, 0.8f},
        {PostEffectType::FilmGrain, true, 0.15f}
    };
    RegisterPreset(nightmare);
    
    // Poisoned preset
    EffectPreset poisoned;
    poisoned.id = "poisoned";
    poisoned.name = "Poisoned";
    poisoned.effects = {
        {PostEffectType::Vignette, true, 0.6f}
    };
    RegisterPreset(poisoned);
    
    // Frozen preset
    EffectPreset frozen;
    frozen.id = "frozen";
    frozen.name = "Frozen";
    frozen.effects = {
        {PostEffectType::Vignette, true, 0.4f}
    };
    RegisterPreset(frozen);
    
    // Boss encounter preset
    EffectPreset boss;
    boss.id = "boss";
    boss.name = "Boss Encounter";
    boss.effects = {
        {PostEffectType::Vignette, true, 0.7f}
    };
    RegisterPreset(boss);
}

// ===========================================================================
// PRESET BUILDERS
// ===========================================================================

namespace PostPresets {

EffectPreset CreateRetroPreset() {
    return {
        "retro", "Retro",
        {
            {PostEffectType::Scanlines, true, 0.4f, 2.0f},
            {PostEffectType::Vignette, true, 0.5f},
            {PostEffectType::Pixelate, true, 1.0f, 2.0f}
        },
    };
}

EffectPreset CreateCinematicPreset() {
    return {
        "cinematic", "Cinematic",
        {
            {PostEffectType::Vignette, true, 0.4f}
        },
    };
}

EffectPreset CreateNightmarePreset() {
    return {
        "nightmare", "Nightmare",
        {
            {PostEffectType::Vignette, true, 0.8f},
            {PostEffectType::Grayscale, true, 0.5f},
            {PostEffectType::FilmGrain, true, 0.2f}
        },
    };
}

EffectPreset CreateDreamPreset() {
    return {
        "dream", "Dream",
        {
            {PostEffectType::Bloom, true, 0.5f},
            {PostEffectType::Vignette, true, 0.3f}
        },
    };
}

EffectPreset CreatePoisonedPreset() {
    return {
        "poisoned", "Poisoned",
        {
            {PostEffectType::Vignette, true, 0.6f}
        },
    };
}

EffectPreset CreateFrozenPreset() {
    return {
        "frozen", "Frozen",
        {
            {PostEffectType::Vignette, true, 0.5f}
        },
    };
}

EffectPreset CreateBurningPreset() {
    return {
        "burning", "Burning",
        {
            {PostEffectType::Vignette, true, 0.7f},
            {PostEffectType::FilmGrain, true, 0.1f}
        },
    };
}

EffectPreset CreateBossPreset() {
    return {
        "boss", "Boss",
        {
            {PostEffectType::Vignette, true, 0.7f}
        },
    };
}

} // namespace PostPresets

} // namespace DDD
