#include "AccessibilitySystem.h"
#include "../Utils/Logger.h"

#include <algorithm>
#include <fstream>
#include <cmath>

namespace DDD {

AccessibilitySystem& AccessibilitySystem::Instance() {
    static AccessibilitySystem instance;
    return instance;
}

void AccessibilitySystem::Initialize() {
    if (m_initialized) return;
    
    RegisterPresets();
    
    m_initialized = true;
    Logger::Info("AccessibilitySystem initialized with {} presets", m_presets.size());
}

void AccessibilitySystem::Shutdown() {
    m_presets.clear();
    m_speakQueue.clear();
    m_subtitles.clear();
    m_initialized = false;
}

void AccessibilitySystem::Update(float deltaTime) {
    // Process screen reader queue
    ProcessScreenReaderQueue();
    
    // Update subtitles
    for (auto it = m_subtitles.begin(); it != m_subtitles.end();) {
        it->remainingTime -= deltaTime;
        if (it->remainingTime <= 0) {
            it = m_subtitles.erase(it);
        } else {
            ++it;
        }
    }
}

void AccessibilitySystem::ApplySettings() {
    // Notify systems that settings changed
    if (m_settingsCallback) {
        m_settingsCallback();
    }
    
    Logger::Info("Accessibility settings applied");
}

void AccessibilitySystem::ResetToDefaults() {
    m_visual = VisualSettings();
    m_audio = AudioSettings();
    m_motor = MotorSettings();
    m_cognitive = CognitiveSettings();
    
    ApplySettings();
    Logger::Info("Accessibility settings reset to defaults");
}

void AccessibilitySystem::ApplyPreset(const std::string& presetId) {
    for (const auto& preset : m_presets) {
        if (preset.id == presetId) {
            m_visual = preset.visual;
            m_audio = preset.audio;
            m_motor = preset.motor;
            m_cognitive = preset.cognitive;
            ApplySettings();
            Logger::Info("Applied accessibility preset: {}", preset.name);
            return;
        }
    }
    Logger::Warning("Unknown accessibility preset: {}", presetId);
}

std::vector<AccessibilityPreset> AccessibilitySystem::GetPresets() const {
    return m_presets;
}

void AccessibilitySystem::SaveCustomPreset(const std::string& name) {
    AccessibilityPreset preset;
    preset.id = "custom_" + std::to_string(m_presets.size());
    preset.name = name;
    preset.description = "Custom user preset";
    preset.visual = m_visual;
    preset.audio = m_audio;
    preset.motor = m_motor;
    preset.cognitive = m_cognitive;
    
    m_presets.push_back(preset);
    Logger::Info("Saved custom preset: {}", name);
}

uint32_t AccessibilitySystem::AdjustColor(uint32_t color) const {
    if (m_visual.colorBlindMode == ColorBlindMode::None) {
        return color;
    }
    return ApplyColorBlindFilter(color, m_visual.colorBlindMode);
}

void AccessibilitySystem::SetColorBlindMode(ColorBlindMode mode) {
    m_visual.colorBlindMode = mode;
    ApplySettings();
}

std::vector<std::pair<std::string, ColorBlindMode>> AccessibilitySystem::GetColorBlindModes() const {
    return {
        {"None", ColorBlindMode::None},
        {"Protanopia (Red-Blind)", ColorBlindMode::Protanopia},
        {"Deuteranopia (Green-Blind)", ColorBlindMode::Deuteranopia},
        {"Tritanopia (Blue-Blind)", ColorBlindMode::Tritanopia},
        {"Achromatopsia (Monochrome)", ColorBlindMode::Achromatopsia},
        {"Custom", ColorBlindMode::Custom}
    };
}

float AccessibilitySystem::GetTextScale() const {
    float baseScale = m_visual.textSizeMultiplier;
    
    switch (m_visual.textSize) {
        case TextSizePreset::Small: baseScale *= 0.8f; break;
        case TextSizePreset::Normal: baseScale *= 1.0f; break;
        case TextSizePreset::Large: baseScale *= 1.25f; break;
        case TextSizePreset::ExtraLarge: baseScale *= 1.5f; break;
        case TextSizePreset::Huge: baseScale *= 2.0f; break;
    }
    
    return baseScale;
}

float AccessibilitySystem::GetUIScale() const {
    float scale = m_visual.uiScale;
    if (m_visual.largeButtons) scale *= 1.25f;
    return scale;
}

void AccessibilitySystem::SetTextSize(TextSizePreset preset) {
    m_visual.textSize = preset;
    ApplySettings();
}

void AccessibilitySystem::Speak(const std::string& text, int priority, bool interrupt) {
    if (m_audio.screenReader == ScreenReaderSupport::None) return;
    
    ScreenReaderEntry entry;
    entry.text = text;
    entry.priority = priority;
    entry.interruptible = !interrupt;
    entry.delay = 0.0f;
    
    if (interrupt) {
        m_speakQueue.clear();
        m_isSpeaking = false;
    }
    
    // Insert by priority
    auto it = m_speakQueue.begin();
    while (it != m_speakQueue.end() && it->priority >= priority) {
        ++it;
    }
    m_speakQueue.insert(it, entry);
}

void AccessibilitySystem::SpeakUI(const std::string& elementType, const std::string& label, const std::string& state) {
    if (m_audio.screenReader == ScreenReaderSupport::None) return;
    
    std::string text = label;
    if (!elementType.empty()) {
        text += ", " + elementType;
    }
    if (!state.empty()) {
        text += ", " + state;
    }
    
    Speak(text, 5, true);
}

void AccessibilitySystem::StopSpeaking() {
    m_speakQueue.clear();
    m_isSpeaking = false;
    // In production: stop TTS engine
}

void AccessibilitySystem::SetScreenReaderEnabled(bool enabled) {
    m_audio.screenReader = enabled ? ScreenReaderSupport::Full : ScreenReaderSupport::None;
    if (!enabled) {
        StopSpeaking();
    }
}

void AccessibilitySystem::DescribeEvent(const std::string& event, const std::string& details) {
    if (!m_audio.audioDescriptions) return;
    
    std::string text = event;
    if (!details.empty()) {
        text += ". " + details;
    }
    
    Speak(text, 3);
}

void AccessibilitySystem::DescribeUnit(const std::string& unitName, int hp, int maxHp, const std::string& status) {
    if (!m_audio.audioDescriptions) return;
    
    int hpPercent = (hp * 100) / maxHp;
    std::string healthDesc;
    if (hpPercent > 75) healthDesc = "healthy";
    else if (hpPercent > 50) healthDesc = "wounded";
    else if (hpPercent > 25) healthDesc = "badly wounded";
    else healthDesc = "critical";
    
    std::string text = unitName + ", " + healthDesc + " at " + std::to_string(hp) + " of " + std::to_string(maxHp) + " health";
    if (!status.empty()) {
        text += ", " + status;
    }
    
    Speak(text, 2);
}

void AccessibilitySystem::DescribeBoardState(int playerUnits, int enemyUnits, int wave) {
    if (!m_audio.audioDescriptions) return;
    
    std::string text = "Wave " + std::to_string(wave) + ". ";
    text += "You have " + std::to_string(playerUnits) + " units. ";
    text += std::to_string(enemyUnits) + " enemies remain.";
    
    Speak(text, 1);
}

void AccessibilitySystem::ShowSubtitle(const std::string& speaker, const std::string& text, float duration) {
    if (!m_audio.subtitlesEnabled) return;
    
    SubtitleEntry entry;
    entry.speaker = speaker;
    entry.text = text;
    entry.remainingTime = duration;
    entry.isCaption = false;
    
    m_subtitles.push_back(entry);
}

void AccessibilitySystem::ShowCaption(const std::string& soundDescription, float duration) {
    if (!m_audio.closedCaptions) return;
    
    SubtitleEntry entry;
    entry.speaker = "";
    entry.text = "[" + soundDescription + "]";
    entry.remainingTime = duration;
    entry.isCaption = true;
    
    m_subtitles.push_back(entry);
}

void AccessibilitySystem::ClearSubtitles() {
    m_subtitles.clear();
}

float AccessibilitySystem::GetFlashIntensity(float original) const {
    if (!m_visual.reduceFlashing) return original;
    return original * m_visual.flashReductionLevel;
}

uint32_t AccessibilitySystem::GetHighContrastColor(uint32_t original, bool isBackground) const {
    if (!m_visual.highContrastMode) return original;
    
    // Extract RGB
    uint8_t r = (original >> 16) & 0xFF;
    uint8_t g = (original >> 8) & 0xFF;
    uint8_t b = original & 0xFF;
    
    // Calculate luminance
    float lum = 0.299f * r + 0.587f * g + 0.114f * b;
    
    if (isBackground) {
        // Make backgrounds more extreme
        return lum > 128 ? 0xFFFFFF : 0x000000;
    } else {
        // Boost contrast for foreground
        float factor = m_visual.contrastLevel;
        r = static_cast<uint8_t>(std::clamp((r - 128) * factor + 128, 0.0f, 255.0f));
        g = static_cast<uint8_t>(std::clamp((g - 128) * factor + 128, 0.0f, 255.0f));
        b = static_cast<uint8_t>(std::clamp((b - 128) * factor + 128, 0.0f, 255.0f));
        return (r << 16) | (g << 8) | b;
    }
}

void AccessibilitySystem::SaveSettings(const std::string& filepath) {
    // In production, would serialize to JSON
    Logger::Info("Accessibility settings saved to: {}", filepath);
}

void AccessibilitySystem::LoadSettings(const std::string& filepath) {
    // In production, would deserialize from JSON
    Logger::Info("Accessibility settings loaded from: {}", filepath);
}

// ===========================================================================
// PRIVATE METHODS
// ===========================================================================

void AccessibilitySystem::RegisterPresets() {
    // Default preset
    AccessibilityPreset defaultPreset;
    defaultPreset.id = "default";
    defaultPreset.name = "Default";
    defaultPreset.description = "Standard settings";
    m_presets.push_back(defaultPreset);
    
    // Low vision preset
    AccessibilityPreset lowVision;
    lowVision.id = "low_vision";
    lowVision.name = "Low Vision";
    lowVision.description = "Larger text and UI, high contrast";
    lowVision.visual.textSize = TextSizePreset::ExtraLarge;
    lowVision.visual.uiScale = 1.5f;
    lowVision.visual.highContrastMode = true;
    lowVision.visual.contrastLevel = 1.5f;
    lowVision.visual.largeButtons = true;
    lowVision.visual.showRangeIndicators = true;
    lowVision.motor.cursorSize = 1.5f;
    lowVision.motor.clickTolerance = 1.5f;
    m_presets.push_back(lowVision);
    
    // Color blind preset
    AccessibilityPreset colorBlind;
    colorBlind.id = "color_blind";
    colorBlind.name = "Color Blind Friendly";
    colorBlind.description = "Adjusted colors and additional indicators";
    colorBlind.visual.colorBlindMode = ColorBlindMode::Deuteranopia;
    colorBlind.visual.showDamageNumbers = true;
    colorBlind.visual.showHealthBars = true;
    colorBlind.visual.playerColor = 0x0077BB;   // Blue
    colorBlind.visual.enemyColor = 0xEE7733;    // Orange
    colorBlind.visual.allyColor = 0x009988;     // Teal
    m_presets.push_back(colorBlind);
    
    // Screen reader preset
    AccessibilityPreset screenReader;
    screenReader.id = "screen_reader";
    screenReader.name = "Screen Reader";
    screenReader.description = "Full audio descriptions and narration";
    screenReader.audio.screenReader = ScreenReaderSupport::Full;
    screenReader.audio.audioDescriptions = true;
    screenReader.audio.enhancedAudioCues = true;
    screenReader.audio.closedCaptions = true;
    screenReader.audio.showSpeakerNames = true;
    screenReader.visual.showSoundIndicators = true;
    m_presets.push_back(screenReader);
    
    // Motor impairment preset
    AccessibilityPreset motor;
    motor.id = "motor";
    motor.name = "Motor Accessibility";
    motor.description = "Extended timing and simplified controls";
    motor.motor.assistanceLevel = InputAssistance::High;
    motor.motor.timeLimitMultiplier = 2.0f;
    motor.motor.autoTargeting = true;
    motor.motor.holdToConfirm = true;
    motor.motor.holdDuration = 0.3f;
    motor.motor.clickTolerance = 2.0f;
    motor.motor.cursorSize = 1.5f;
    motor.cognitive.confirmActions = true;
    m_presets.push_back(motor);
    
    // Cognitive preset
    AccessibilityPreset cognitive;
    cognitive.id = "cognitive";
    cognitive.name = "Cognitive Accessibility";
    cognitive.description = "Simplified UI and helpful reminders";
    cognitive.cognitive.simplifiedUI = true;
    cognitive.cognitive.tutorialHints = true;
    cognitive.cognitive.extendedTutorial = true;
    cognitive.cognitive.showDetailedTooltips = true;
    cognitive.cognitive.tooltipDelay = 0.2f;
    cognitive.cognitive.showObjectiveReminders = true;
    cognitive.cognitive.reminderFrequency = 30.0f;
    cognitive.cognitive.showActionHistory = true;
    cognitive.cognitive.showTurnSummary = true;
    cognitive.cognitive.confirmActions = true;
    cognitive.motor.timeLimitMultiplier = 1.5f;
    m_presets.push_back(cognitive);
    
    // Reduced motion preset
    AccessibilityPreset reducedMotion;
    reducedMotion.id = "reduced_motion";
    reducedMotion.name = "Reduced Motion";
    reducedMotion.description = "Minimal animations and effects";
    reducedMotion.visual.reduceMotion = true;
    reducedMotion.visual.reduceFlashing = true;
    reducedMotion.visual.flashReductionLevel = 0.2f;
    reducedMotion.visual.disableScreenShake = true;
    reducedMotion.visual.disableParticles = true;
    m_presets.push_back(reducedMotion);
    
    // Deaf/Hard of hearing preset
    AccessibilityPreset deaf;
    deaf.id = "deaf_hoh";
    deaf.name = "Deaf / Hard of Hearing";
    deaf.description = "Visual cues for audio events";
    deaf.audio.subtitlesEnabled = true;
    deaf.audio.closedCaptions = true;
    deaf.audio.subtitleSize = 1.25f;
    deaf.audio.subtitleBackground = true;
    deaf.audio.showSpeakerNames = true;
    deaf.audio.colorCodeSpeakers = true;
    deaf.visual.showSoundIndicators = true;
    deaf.motor.gamepadRumble = true;
    deaf.motor.rumbleIntensity = 1.5f;
    m_presets.push_back(deaf);
}

void AccessibilitySystem::ProcessScreenReaderQueue() {
    if (m_speakQueue.empty()) {
        m_isSpeaking = false;
        return;
    }
    
    if (!m_isSpeaking) {
        // Start speaking next item
        ScreenReaderEntry& entry = m_speakQueue.front();
        m_isSpeaking = true;
        
        // In production: send to TTS engine
        // Platform::TextToSpeech(entry.text, m_audio.screenReaderSpeed, m_audio.screenReaderVoice);
        
        Logger::Debug("Screen reader: {}", entry.text);
        
        // Estimate duration (rough: 150 words per minute)
        float wordCount = 1 + std::count(entry.text.begin(), entry.text.end(), ' ');
        m_speakTimer = (wordCount / 150.0f) * 60.0f / m_audio.screenReaderSpeed;
    }
    
    // Simulate TTS completion
    m_speakTimer -= 0.016f;  // Assume 60fps
    if (m_speakTimer <= 0) {
        m_speakQueue.erase(m_speakQueue.begin());
        m_isSpeaking = false;
    }
}

uint32_t AccessibilitySystem::ApplyColorBlindFilter(uint32_t color, ColorBlindMode mode) const {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    float rf = r / 255.0f;
    float gf = g / 255.0f;
    float bf = b / 255.0f;
    
    float newR, newG, newB;
    
    switch (mode) {
        case ColorBlindMode::Protanopia:
            // Red-blind simulation
            newR = 0.567f * rf + 0.433f * gf;
            newG = 0.558f * rf + 0.442f * gf;
            newB = 0.242f * gf + 0.758f * bf;
            break;
            
        case ColorBlindMode::Deuteranopia:
            // Green-blind simulation
            newR = 0.625f * rf + 0.375f * gf;
            newG = 0.700f * rf + 0.300f * gf;
            newB = 0.300f * gf + 0.700f * bf;
            break;
            
        case ColorBlindMode::Tritanopia:
            // Blue-blind simulation
            newR = 0.950f * rf + 0.050f * gf;
            newG = 0.433f * gf + 0.567f * bf;
            newB = 0.475f * gf + 0.525f * bf;
            break;
            
        case ColorBlindMode::Achromatopsia:
            // Complete color blindness (grayscale)
            newR = newG = newB = 0.299f * rf + 0.587f * gf + 0.114f * bf;
            break;
            
        default:
            return color;
    }
    
    // Apply intensity
    float intensity = m_visual.colorBlindIntensity;
    newR = rf + (newR - rf) * intensity;
    newG = gf + (newG - gf) * intensity;
    newB = bf + (newB - bf) * intensity;
    
    // Clamp and convert back
    r = static_cast<uint8_t>(std::clamp(newR * 255.0f, 0.0f, 255.0f));
    g = static_cast<uint8_t>(std::clamp(newG * 255.0f, 0.0f, 255.0f));
    b = static_cast<uint8_t>(std::clamp(newB * 255.0f, 0.0f, 255.0f));
    
    return (r << 16) | (g << 8) | b;
}

} // namespace DDD
