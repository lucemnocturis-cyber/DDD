#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace DDD {

/**
 * Color blindness modes
 */
enum class ColorBlindMode {
    None,
    Protanopia,     // Red-blind
    Deuteranopia,   // Green-blind
    Tritanopia,     // Blue-blind
    Achromatopsia,  // Complete color blindness
    Custom          // User-defined palette
};

/**
 * Text size presets
 */
enum class TextSizePreset {
    Small,          // 80% of default
    Normal,         // 100%
    Large,          // 125%
    ExtraLarge,     // 150%
    Huge            // 200%
};

/**
 * Screen reader support level
 */
enum class ScreenReaderSupport {
    None,
    Basic,          // Menu navigation only
    Full            // Full game narration
};

/**
 * Input assistance level
 */
enum class InputAssistance {
    None,
    Low,            // Extended timers
    Medium,         // Auto-targeting hints
    High            // Full auto-assist options
};

/**
 * Visual accessibility settings
 */
struct VisualSettings {
    // Color blindness
    ColorBlindMode colorBlindMode = ColorBlindMode::None;
    float colorBlindIntensity = 1.0f;   // 0-1, strength of filter
    
    // High contrast
    bool highContrastMode = false;
    float contrastLevel = 1.0f;         // 1.0 = normal, 2.0 = high
    
    // Text
    TextSizePreset textSize = TextSizePreset::Normal;
    float textSizeMultiplier = 1.0f;
    bool boldText = false;
    std::string fontOverride = "";      // Custom font path
    
    // UI scaling
    float uiScale = 1.0f;               // 0.75 - 2.0
    bool largeButtons = false;
    bool extraPadding = false;
    
    // Visual effects
    bool reduceMotion = false;          // Disable non-essential animations
    bool reduceFlashing = false;        // Prevent rapid flashing
    float flashReductionLevel = 0.5f;
    bool disableScreenShake = false;
    bool disableParticles = false;
    
    // Indicators
    bool showSoundIndicators = false;   // Visual cues for sounds
    bool showDamageNumbers = true;
    bool showHealthBars = true;
    bool showRangeIndicators = true;
    bool showTurnOrderBar = true;
    
    // Color customization
    uint32_t playerColor = 0x4488FF;
    uint32_t enemyColor = 0xFF4444;
    uint32_t allyColor = 0x44FF44;
    uint32_t neutralColor = 0xFFFF44;
    uint32_t dangerColor = 0xFF0000;
    uint32_t safeColor = 0x00FF00;
};

/**
 * Audio accessibility settings
 */
struct AudioSettings {
    // Screen reader
    ScreenReaderSupport screenReader = ScreenReaderSupport::None;
    float screenReaderSpeed = 1.0f;     // 0.5 - 2.0
    float screenReaderVolume = 1.0f;
    std::string screenReaderVoice = "default";
    
    // Audio cues
    bool audioDescriptions = false;     // Describe visual events
    bool enhancedAudioCues = false;     // Extra audio feedback
    bool spatialAudioCues = false;      // Directional sounds for events
    
    // Subtitles
    bool subtitlesEnabled = true;
    bool closedCaptions = false;        // Include sound descriptions [EXPLOSION]
    float subtitleSize = 1.0f;
    bool subtitleBackground = true;
    uint32_t subtitleColor = 0xFFFFFF;
    uint32_t subtitleBgColor = 0x000000;
    float subtitleBgOpacity = 0.7f;
    
    // Speaker identification
    bool showSpeakerNames = true;
    bool colorCodeSpeakers = true;
    
    // Mono audio
    bool monoAudio = false;             // Combine stereo to mono
    float audioBalance = 0.5f;          // 0=left, 1=right
};

/**
 * Motor/Input accessibility settings
 */
struct MotorSettings {
    // Input assistance
    InputAssistance assistanceLevel = InputAssistance::None;
    
    // Timing
    bool disableTimeLimits = false;
    float timeLimitMultiplier = 1.0f;   // Extend time limits
    bool pauseOnFocusLoss = true;
    
    // Controls
    bool holdToConfirm = false;         // Hold instead of click
    float holdDuration = 0.5f;
    bool stickyKeys = false;            // Toggle instead of hold
    float doubleClickTime = 0.4f;
    float longPressTime = 0.8f;
    
    // Auto features
    bool autoTargeting = false;         // Assist with target selection
    bool autoMove = false;              // Click to auto-path
    bool autoEndTurn = false;           // End turn when no actions
    float autoEndTurnDelay = 2.0f;
    
    // Simplified controls
    bool oneButtonMode = false;         // Simplified single-button gameplay
    bool gestureSimplification = false;
    
    // Mouse/cursor
    float cursorSize = 1.0f;
    bool cursorHighlight = false;
    uint32_t cursorColor = 0xFFFFFF;
    float clickTolerance = 1.0f;        // Larger click targets
    
    // Gamepad
    bool gamepadRumble = true;
    float rumbleIntensity = 1.0f;
    bool swapConfirmCancel = false;
};

/**
 * Cognitive accessibility settings
 */
struct CognitiveSettings {
    // Difficulty
    bool simplifiedUI = false;          // Reduce UI complexity
    bool tutorialHints = true;          // Always show hints
    bool extendedTutorial = false;
    
    // Information
    bool showDetailedTooltips = true;
    float tooltipDelay = 0.5f;
    bool showObjectiveReminders = true;
    float reminderFrequency = 60.0f;    // Seconds between reminders
    
    // Navigation
    bool showWaypoints = true;
    bool highlightInteractables = true;
    bool highlightEnemies = true;
    
    // Reading
    bool dyslexiaFont = false;          // Use dyslexia-friendly font
    float lineSpacing = 1.2f;
    bool numberedLists = true;
    
    // Memory aids
    bool showActionHistory = true;
    int historyLength = 10;
    bool showTurnSummary = true;
    bool confirmActions = false;        // Confirm before executing
};

/**
 * Accessibility preset
 */
struct AccessibilityPreset {
    std::string id;
    std::string name;
    std::string description;
    VisualSettings visual;
    AudioSettings audio;
    MotorSettings motor;
    CognitiveSettings cognitive;
};

/**
 * Screen reader text queue entry
 */
struct ScreenReaderEntry {
    std::string text;
    int priority;           // Higher = more important
    bool interruptible;
    float delay;
};

/**
 * AccessibilitySystem - comprehensive accessibility features
 */
class AccessibilitySystem {
public:
    static AccessibilitySystem& Instance();
    
    void Initialize();
    void Shutdown();
    void Update(float deltaTime);
    
    // Settings access
    VisualSettings& GetVisualSettings() { return m_visual; }
    AudioSettings& GetAudioSettings() { return m_audio; }
    MotorSettings& GetMotorSettings() { return m_motor; }
    CognitiveSettings& GetCognitiveSettings() { return m_cognitive; }
    
    const VisualSettings& GetVisualSettings() const { return m_visual; }
    const AudioSettings& GetAudioSettings() const { return m_audio; }
    const MotorSettings& GetMotorSettings() const { return m_motor; }
    const CognitiveSettings& GetCognitiveSettings() const { return m_cognitive; }
    
    // Apply settings
    void ApplySettings();
    void ResetToDefaults();
    
    // Presets
    void ApplyPreset(const std::string& presetId);
    std::vector<AccessibilityPreset> GetPresets() const;
    void SaveCustomPreset(const std::string& name);
    
    // Color blindness
    uint32_t AdjustColor(uint32_t color) const;
    void SetColorBlindMode(ColorBlindMode mode);
    std::vector<std::pair<std::string, ColorBlindMode>> GetColorBlindModes() const;
    
    // Text scaling
    float GetTextScale() const;
    float GetUIScale() const;
    void SetTextSize(TextSizePreset preset);
    
    // Screen reader
    void Speak(const std::string& text, int priority = 0, bool interrupt = false);
    void SpeakUI(const std::string& elementType, const std::string& label, const std::string& state = "");
    void StopSpeaking();
    bool IsSpeaking() const { return m_isSpeaking; }
    void SetScreenReaderEnabled(bool enabled);
    
    // Audio descriptions
    void DescribeEvent(const std::string& event, const std::string& details = "");
    void DescribeUnit(const std::string& unitName, int hp, int maxHp, const std::string& status);
    void DescribeBoardState(int playerUnits, int enemyUnits, int wave);
    
    // Subtitles
    void ShowSubtitle(const std::string& speaker, const std::string& text, float duration);
    void ShowCaption(const std::string& soundDescription, float duration);
    void ClearSubtitles();
    
    // Motion reduction
    bool ShouldReduceMotion() const { return m_visual.reduceMotion; }
    bool ShouldReduceFlashing() const { return m_visual.reduceFlashing; }
    float GetFlashIntensity(float original) const;
    
    // Input assistance
    bool IsAutoTargetingEnabled() const { return m_motor.autoTargeting; }
    float GetTimeLimitMultiplier() const { return m_motor.timeLimitMultiplier; }
    bool ShouldConfirmAction() const { return m_cognitive.confirmActions; }
    
    // High contrast
    bool IsHighContrastEnabled() const { return m_visual.highContrastMode; }
    uint32_t GetHighContrastColor(uint32_t original, bool isBackground) const;
    
    // Remapping helpers
    float GetClickTolerance() const { return m_motor.clickTolerance; }
    float GetCursorSize() const { return m_motor.cursorSize; }
    
    // Save/Load
    void SaveSettings(const std::string& filepath);
    void LoadSettings(const std::string& filepath);
    
    // Callbacks
    using SettingsChangedCallback = std::function<void()>;
    void SetSettingsChangedCallback(SettingsChangedCallback callback) { m_settingsCallback = callback; }
    
private:
    AccessibilitySystem() = default;
    
    void RegisterPresets();
    void ProcessScreenReaderQueue();
    uint32_t ApplyColorBlindFilter(uint32_t color, ColorBlindMode mode) const;
    
    // Settings
    VisualSettings m_visual;
    AudioSettings m_audio;
    MotorSettings m_motor;
    CognitiveSettings m_cognitive;
    
    // Presets
    std::vector<AccessibilityPreset> m_presets;
    
    // Screen reader
    std::vector<ScreenReaderEntry> m_speakQueue;
    bool m_isSpeaking = false;
    float m_speakTimer = 0.0f;
    
    // Subtitles
    struct SubtitleEntry {
        std::string speaker;
        std::string text;
        float remainingTime;
        bool isCaption;
    };
    std::vector<SubtitleEntry> m_subtitles;
    
    // Callback
    SettingsChangedCallback m_settingsCallback;
    
    bool m_initialized = false;
};

} // namespace DDD
