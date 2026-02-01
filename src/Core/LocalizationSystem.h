#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace DDD {

/**
 * Supported languages
 */
enum class Language {
    English,
    Spanish,
    French,
    German,
    Italian,
    Portuguese,
    Russian,
    Japanese,
    Korean,
    ChineseSimplified,
    ChineseTraditional,
    Polish,
    Turkish,
    Arabic,
    Thai
};

/**
 * Text direction
 */
enum class TextDirection {
    LeftToRight,
    RightToLeft
};

/**
 * Plural form rules
 */
enum class PluralRule {
    OneOther,           // English, German, etc. (1 vs other)
    OneZeroOther,       // French (0-1 vs other)
    OneFewMany,         // Russian, Polish (1, 2-4, 5+)
    OneTwoFewMany,      // Arabic (1, 2, 3-10, 11+)
    Other               // Japanese, Korean, Chinese (no plural)
};

/**
 * Language info
 */
struct LanguageInfo {
    Language id;
    std::string code;           // ISO 639-1 (e.g., "en", "ja")
    std::string nativeName;     // Name in native script
    std::string englishName;    // Name in English
    TextDirection direction;
    PluralRule pluralRule;
    std::string fontOverride;   // Font for this language (for CJK, etc.)
    bool requiresIME;           // Needs input method editor
    float textScaleFactor;      // Some languages need larger text
};

/**
 * Localized string with metadata
 */
struct LocalizedString {
    std::string key;
    std::string text;
    std::string context;        // Translator note
    std::string pluralOne;      // Singular form
    std::string pluralFew;      // Few form (2-4 in Slavic)
    std::string pluralMany;     // Many form (5+ in Slavic)
    bool verified;              // Has been reviewed
    uint64_t lastModified;
};

/**
 * String table for a category
 */
struct StringTable {
    std::string category;       // e.g., "ui", "gameplay", "story"
    Language language;
    std::unordered_map<std::string, LocalizedString> strings;
};

/**
 * Formatting context for variable substitution
 */
struct FormatContext {
    std::unordered_map<std::string, std::string> stringVars;
    std::unordered_map<std::string, int> intVars;
    std::unordered_map<std::string, float> floatVars;
};

/**
 * Missing string handler
 */
enum class MissingStringBehavior {
    ReturnKey,          // Return the key itself
    ReturnEmpty,        // Return empty string
    ReturnFallback,     // Return English version
    ThrowError          // Log error and return key
};

/**
 * LocalizationSystem - manages translations and localization
 */
class LocalizationSystem {
public:
    static LocalizationSystem& Instance();
    
    void Initialize();
    void Shutdown();
    
    // Language management
    void SetLanguage(Language language);
    Language GetCurrentLanguage() const { return m_currentLanguage; }
    std::string GetLanguageCode() const;
    LanguageInfo GetLanguageInfo(Language language) const;
    std::vector<LanguageInfo> GetSupportedLanguages() const;
    
    // Auto-detection
    Language DetectSystemLanguage() const;
    Language DetectSteamLanguage() const;
    void UseAutoDetectedLanguage();
    
    // String retrieval
    std::string Get(const std::string& key) const;
    std::string Get(const std::string& category, const std::string& key) const;
    std::string GetPlural(const std::string& key, int count) const;
    std::string Format(const std::string& key, const FormatContext& context) const;
    
    // Convenience methods
    std::string GetUI(const std::string& key) const;
    std::string GetGameplay(const std::string& key) const;
    std::string GetStory(const std::string& key) const;
    std::string GetTooltip(const std::string& key) const;
    std::string GetAchievement(const std::string& key) const;
    
    // Format helpers
    std::string FormatNumber(int number) const;
    std::string FormatNumber(float number, int decimals = 2) const;
    std::string FormatCurrency(int amount) const;
    std::string FormatTime(float seconds) const;
    std::string FormatDate(uint64_t timestamp) const;
    std::string FormatPercent(float value) const;
    
    // String checking
    bool HasString(const std::string& key) const;
    bool HasString(const std::string& category, const std::string& key) const;
    int GetMissingStringCount() const { return m_missingCount; }
    std::vector<std::string> GetMissingStrings() const;
    
    // Loading
    bool LoadLanguageFile(const std::string& filepath);
    bool LoadLanguageFromJSON(const std::string& json, const std::string& category);
    void ReloadCurrentLanguage();
    
    // Text direction
    TextDirection GetTextDirection() const;
    bool IsRightToLeft() const;
    
    // Font requirements
    std::string GetFontForLanguage() const;
    float GetTextScaleFactor() const;
    bool RequiresIME() const;
    
    // Settings
    void SetMissingStringBehavior(MissingStringBehavior behavior);
    void SetFallbackLanguage(Language language);
    
    // Callbacks
    using LanguageChangedCallback = std::function<void(Language)>;
    void SetLanguageChangedCallback(LanguageChangedCallback callback) { m_languageCallback = callback; }
    
    // Debug
    void DumpMissingStrings(const std::string& filepath) const;
    void ValidateAllStrings() const;
    
private:
    LocalizationSystem() = default;
    
    void RegisterLanguages();
    void LoadStringTables();
    std::string GetFallbackString(const std::string& key) const;
    int GetPluralForm(int count) const;
    std::string SubstituteVariables(const std::string& text, const FormatContext& context) const;
    std::string ProcessFormatSpecifiers(const std::string& text) const;
    
    // Current state
    Language m_currentLanguage = Language::English;
    Language m_fallbackLanguage = Language::English;
    MissingStringBehavior m_missingBehavior = MissingStringBehavior::ReturnFallback;
    
    // Language data
    std::unordered_map<Language, LanguageInfo> m_languageInfo;
    
    // String tables (category -> language -> strings)
    std::unordered_map<std::string, std::unordered_map<Language, StringTable>> m_stringTables;
    
    // Missing string tracking
    mutable int m_missingCount = 0;
    mutable std::vector<std::string> m_missingStrings;
    
    // Callback
    LanguageChangedCallback m_languageCallback;
    
    bool m_initialized = false;
};

// Convenience macros
#define LOC(key) LocalizationSystem::Instance().Get(key)
#define LOC_UI(key) LocalizationSystem::Instance().GetUI(key)
#define LOC_PLURAL(key, count) LocalizationSystem::Instance().GetPlural(key, count)

} // namespace DDD
