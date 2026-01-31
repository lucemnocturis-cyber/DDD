#include "LocalizationSystem.h"
#include "../Utils/Logger.h"

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <fstream>
#include <regex>

namespace DDD {

LocalizationSystem& LocalizationSystem::Instance() {
    static LocalizationSystem instance;
    return instance;
}

void LocalizationSystem::Initialize() {
    if (m_initialized) return;
    
    RegisterLanguages();
    LoadStringTables();
    
    // Try auto-detection
    UseAutoDetectedLanguage();
    
    m_initialized = true;
    Logger::Info("LocalizationSystem initialized, language: {}", GetLanguageCode());
}

void LocalizationSystem::Shutdown() {
    m_stringTables.clear();
    m_languageInfo.clear();
    m_missingStrings.clear();
    m_initialized = false;
}

void LocalizationSystem::SetLanguage(Language language) {
    if (m_currentLanguage == language) return;
    
    m_currentLanguage = language;
    
    Logger::Info("Language changed to: {}", GetLanguageInfo(language).englishName);
    
    if (m_languageCallback) {
        m_languageCallback(language);
    }
}

std::string LocalizationSystem::GetLanguageCode() const {
    auto it = m_languageInfo.find(m_currentLanguage);
    return it != m_languageInfo.end() ? it->second.code : "en";
}

LanguageInfo LocalizationSystem::GetLanguageInfo(Language language) const {
    auto it = m_languageInfo.find(language);
    if (it != m_languageInfo.end()) {
        return it->second;
    }
    
    LanguageInfo empty;
    empty.id = language;
    empty.code = "en";
    return empty;
}

std::vector<LanguageInfo> LocalizationSystem::GetSupportedLanguages() const {
    std::vector<LanguageInfo> result;
    for (const auto& [id, info] : m_languageInfo) {
        result.push_back(info);
    }
    return result;
}

Language LocalizationSystem::DetectSystemLanguage() const {
    // In production, would use platform APIs
    // Windows: GetUserDefaultUILanguage()
    // macOS: [[NSLocale preferredLanguages] firstObject]
    // Linux: getenv("LANG")
    
    return Language::English;
}

Language LocalizationSystem::DetectSteamLanguage() const {
    // In production: SteamApps()->GetCurrentGameLanguage()
    // Returns strings like "english", "spanish", "japanese", etc.
    
    return Language::English;
}

void LocalizationSystem::UseAutoDetectedLanguage() {
    // Prefer Steam language if available
    Language detected = DetectSteamLanguage();
    
    // Fall back to system language
    if (detected == Language::English) {
        detected = DetectSystemLanguage();
    }
    
    SetLanguage(detected);
}

std::string LocalizationSystem::Get(const std::string& key) const {
    return Get("common", key);
}

std::string LocalizationSystem::Get(const std::string& category, const std::string& key) const {
    // Try current language
    auto catIt = m_stringTables.find(category);
    if (catIt != m_stringTables.end()) {
        auto langIt = catIt->second.find(m_currentLanguage);
        if (langIt != catIt->second.end()) {
            auto strIt = langIt->second.strings.find(key);
            if (strIt != langIt->second.strings.end()) {
                return strIt->second.text;
            }
        }
    }
    
    // Try fallback
    return GetFallbackString(category + "." + key);
}

std::string LocalizationSystem::GetPlural(const std::string& key, int count) const {
    auto catIt = m_stringTables.find("common");
    if (catIt == m_stringTables.end()) return GetFallbackString(key);
    
    auto langIt = catIt->second.find(m_currentLanguage);
    if (langIt == catIt->second.end()) return GetFallbackString(key);
    
    auto strIt = langIt->second.strings.find(key);
    if (strIt == langIt->second.strings.end()) return GetFallbackString(key);
    
    const LocalizedString& ls = strIt->second;
    int form = GetPluralForm(count);
    
    switch (form) {
        case 0: return ls.pluralOne.empty() ? ls.text : ls.pluralOne;
        case 1: return ls.pluralFew.empty() ? ls.text : ls.pluralFew;
        case 2: return ls.pluralMany.empty() ? ls.text : ls.pluralMany;
        default: return ls.text;
    }
}

std::string LocalizationSystem::Format(const std::string& key, const FormatContext& context) const {
    std::string text = Get(key);
    return SubstituteVariables(text, context);
}

std::string LocalizationSystem::GetUI(const std::string& key) const {
    return Get("ui", key);
}

std::string LocalizationSystem::GetGameplay(const std::string& key) const {
    return Get("gameplay", key);
}

std::string LocalizationSystem::GetStory(const std::string& key) const {
    return Get("story", key);
}

std::string LocalizationSystem::GetTooltip(const std::string& key) const {
    return Get("tooltips", key);
}

std::string LocalizationSystem::GetAchievement(const std::string& key) const {
    return Get("achievements", key);
}

std::string LocalizationSystem::FormatNumber(int number) const {
    std::string str = std::to_string(number);
    
    // Add thousand separators based on locale
    std::string separator = ",";
    if (m_currentLanguage == Language::German || m_currentLanguage == Language::French ||
        m_currentLanguage == Language::Spanish || m_currentLanguage == Language::Italian) {
        separator = ".";
    }
    
    int n = static_cast<int>(str.length()) - 3;
    while (n > 0) {
        str.insert(n, separator);
        n -= 3;
    }
    
    return str;
}

std::string LocalizationSystem::FormatNumber(float number, int decimals) const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(decimals) << number;
    std::string str = ss.str();
    
    // Swap decimal separator for European locales
    if (m_currentLanguage == Language::German || m_currentLanguage == Language::French ||
        m_currentLanguage == Language::Spanish || m_currentLanguage == Language::Italian) {
        size_t pos = str.find('.');
        if (pos != std::string::npos) {
            str[pos] = ',';
        }
    }
    
    return str;
}

std::string LocalizationSystem::FormatCurrency(int amount) const {
    std::string numStr = FormatNumber(amount);
    
    // Currency symbol and position varies by locale
    switch (m_currentLanguage) {
        case Language::Japanese:
            return numStr + "G";
        case Language::Korean:
            return numStr + " 골드";
        case Language::ChineseSimplified:
        case Language::ChineseTraditional:
            return numStr + "金";
        default:
            return numStr + " Gold";
    }
}

std::string LocalizationSystem::FormatTime(float seconds) const {
    int totalSeconds = static_cast<int>(seconds);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int secs = totalSeconds % 60;
    
    std::ostringstream ss;
    if (hours > 0) {
        ss << hours << ":" << std::setfill('0') << std::setw(2) << minutes 
           << ":" << std::setw(2) << secs;
    } else {
        ss << minutes << ":" << std::setfill('0') << std::setw(2) << secs;
    }
    
    return ss.str();
}

std::string LocalizationSystem::FormatDate(uint64_t timestamp) const {
    std::time_t time = static_cast<std::time_t>(timestamp);
    std::tm* tm = std::localtime(&time);
    
    char buffer[64];
    
    // Date format varies by locale
    switch (m_currentLanguage) {
        case Language::Japanese:
        case Language::Korean:
        case Language::ChineseSimplified:
        case Language::ChineseTraditional:
            std::strftime(buffer, sizeof(buffer), "%Y/%m/%d", tm);
            break;
        case Language::German:
        case Language::French:
        case Language::Spanish:
        case Language::Italian:
        case Language::Portuguese:
        case Language::Russian:
        case Language::Polish:
            std::strftime(buffer, sizeof(buffer), "%d/%m/%Y", tm);
            break;
        default:
            std::strftime(buffer, sizeof(buffer), "%m/%d/%Y", tm);
            break;
    }
    
    return std::string(buffer);
}

std::string LocalizationSystem::FormatPercent(float value) const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(0) << (value * 100) << "%";
    return ss.str();
}

bool LocalizationSystem::HasString(const std::string& key) const {
    return HasString("common", key);
}

bool LocalizationSystem::HasString(const std::string& category, const std::string& key) const {
    auto catIt = m_stringTables.find(category);
    if (catIt == m_stringTables.end()) return false;
    
    auto langIt = catIt->second.find(m_currentLanguage);
    if (langIt == catIt->second.end()) return false;
    
    return langIt->second.strings.find(key) != langIt->second.strings.end();
}

std::vector<std::string> LocalizationSystem::GetMissingStrings() const {
    return m_missingStrings;
}

bool LocalizationSystem::LoadLanguageFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        Logger::Error("Failed to load language file: {}", filepath);
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    // Determine category from filename
    std::string category = "common";
    size_t lastSlash = filepath.find_last_of("/\\");
    size_t lastDot = filepath.find_last_of('.');
    if (lastSlash != std::string::npos && lastDot != std::string::npos) {
        category = filepath.substr(lastSlash + 1, lastDot - lastSlash - 1);
    }
    
    return LoadLanguageFromJSON(buffer.str(), category);
}

bool LocalizationSystem::LoadLanguageFromJSON(const std::string& json, const std::string& category) {
    // In production, would use proper JSON parser
    // For now, create placeholder strings
    
    Logger::Info("Loading localization for category: {}", category);
    return true;
}

void LocalizationSystem::ReloadCurrentLanguage() {
    // Clear and reload string tables for current language
    LoadStringTables();
}

TextDirection LocalizationSystem::GetTextDirection() const {
    auto info = GetLanguageInfo(m_currentLanguage);
    return info.direction;
}

bool LocalizationSystem::IsRightToLeft() const {
    return GetTextDirection() == TextDirection::RightToLeft;
}

std::string LocalizationSystem::GetFontForLanguage() const {
    auto info = GetLanguageInfo(m_currentLanguage);
    return info.fontOverride;
}

float LocalizationSystem::GetTextScaleFactor() const {
    auto info = GetLanguageInfo(m_currentLanguage);
    return info.textScaleFactor;
}

bool LocalizationSystem::RequiresIME() const {
    auto info = GetLanguageInfo(m_currentLanguage);
    return info.requiresIME;
}

void LocalizationSystem::SetMissingStringBehavior(MissingStringBehavior behavior) {
    m_missingBehavior = behavior;
}

void LocalizationSystem::SetFallbackLanguage(Language language) {
    m_fallbackLanguage = language;
}

void LocalizationSystem::DumpMissingStrings(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) return;
    
    file << "Missing Strings Report\n";
    file << "Language: " << GetLanguageCode() << "\n";
    file << "Count: " << m_missingCount << "\n\n";
    
    for (const auto& key : m_missingStrings) {
        file << key << "\n";
    }
    
    Logger::Info("Missing strings dumped to: {}", filepath);
}

void LocalizationSystem::ValidateAllStrings() const {
    int totalStrings = 0;
    int missingTranslations = 0;
    
    for (const auto& [category, languages] : m_stringTables) {
        auto englishIt = languages.find(Language::English);
        if (englishIt == languages.end()) continue;
        
        for (const auto& [key, value] : englishIt->second.strings) {
            totalStrings++;
            
            // Check if string exists in current language
            auto currentIt = languages.find(m_currentLanguage);
            if (currentIt == languages.end() ||
                currentIt->second.strings.find(key) == currentIt->second.strings.end()) {
                missingTranslations++;
            }
        }
    }
    
    Logger::Info("Localization validation: {}/{} strings translated",
                 totalStrings - missingTranslations, totalStrings);
}

// ===========================================================================
// PRIVATE METHODS
// ===========================================================================

void LocalizationSystem::RegisterLanguages() {
    m_languageInfo[Language::English] = {
        Language::English, "en", "English", "English",
        TextDirection::LeftToRight, PluralRule::OneOther, "", false, 1.0f
    };
    
    m_languageInfo[Language::Spanish] = {
        Language::Spanish, "es", "Español", "Spanish",
        TextDirection::LeftToRight, PluralRule::OneOther, "", false, 1.0f
    };
    
    m_languageInfo[Language::French] = {
        Language::French, "fr", "Français", "French",
        TextDirection::LeftToRight, PluralRule::OneZeroOther, "", false, 1.0f
    };
    
    m_languageInfo[Language::German] = {
        Language::German, "de", "Deutsch", "German",
        TextDirection::LeftToRight, PluralRule::OneOther, "", false, 1.0f
    };
    
    m_languageInfo[Language::Italian] = {
        Language::Italian, "it", "Italiano", "Italian",
        TextDirection::LeftToRight, PluralRule::OneOther, "", false, 1.0f
    };
    
    m_languageInfo[Language::Portuguese] = {
        Language::Portuguese, "pt", "Português", "Portuguese",
        TextDirection::LeftToRight, PluralRule::OneOther, "", false, 1.0f
    };
    
    m_languageInfo[Language::Russian] = {
        Language::Russian, "ru", "Русский", "Russian",
        TextDirection::LeftToRight, PluralRule::OneFewMany, "", false, 1.0f
    };
    
    m_languageInfo[Language::Japanese] = {
        Language::Japanese, "ja", "日本語", "Japanese",
        TextDirection::LeftToRight, PluralRule::Other, "NotoSansJP", true, 1.1f
    };
    
    m_languageInfo[Language::Korean] = {
        Language::Korean, "ko", "한국어", "Korean",
        TextDirection::LeftToRight, PluralRule::Other, "NotoSansKR", true, 1.1f
    };
    
    m_languageInfo[Language::ChineseSimplified] = {
        Language::ChineseSimplified, "zh-CN", "简体中文", "Chinese (Simplified)",
        TextDirection::LeftToRight, PluralRule::Other, "NotoSansSC", true, 1.1f
    };
    
    m_languageInfo[Language::ChineseTraditional] = {
        Language::ChineseTraditional, "zh-TW", "繁體中文", "Chinese (Traditional)",
        TextDirection::LeftToRight, PluralRule::Other, "NotoSansTC", true, 1.1f
    };
    
    m_languageInfo[Language::Polish] = {
        Language::Polish, "pl", "Polski", "Polish",
        TextDirection::LeftToRight, PluralRule::OneFewMany, "", false, 1.0f
    };
    
    m_languageInfo[Language::Turkish] = {
        Language::Turkish, "tr", "Türkçe", "Turkish",
        TextDirection::LeftToRight, PluralRule::OneOther, "", false, 1.0f
    };
    
    m_languageInfo[Language::Arabic] = {
        Language::Arabic, "ar", "العربية", "Arabic",
        TextDirection::RightToLeft, PluralRule::OneTwoFewMany, "NotoSansArabic", false, 1.1f
    };
    
    m_languageInfo[Language::Thai] = {
        Language::Thai, "th", "ไทย", "Thai",
        TextDirection::LeftToRight, PluralRule::Other, "NotoSansThai", false, 1.2f
    };
}

void LocalizationSystem::LoadStringTables() {
    // Create default English strings for all categories
    
    // UI Strings
    StringTable uiTable;
    uiTable.category = "ui";
    uiTable.language = Language::English;
    
    uiTable.strings["main_menu"] = {"main_menu", "Main Menu", "", "", "", "", true, 0};
    uiTable.strings["new_game"] = {"new_game", "New Game", "", "", "", "", true, 0};
    uiTable.strings["continue"] = {"continue", "Continue", "", "", "", "", true, 0};
    uiTable.strings["settings"] = {"settings", "Settings", "", "", "", "", true, 0};
    uiTable.strings["quit"] = {"quit", "Quit", "", "", "", "", true, 0};
    uiTable.strings["pause"] = {"pause", "Pause", "", "", "", "", true, 0};
    uiTable.strings["resume"] = {"resume", "Resume", "", "", "", "", true, 0};
    uiTable.strings["restart"] = {"restart", "Restart", "", "", "", "", true, 0};
    uiTable.strings["confirm"] = {"confirm", "Confirm", "", "", "", "", true, 0};
    uiTable.strings["cancel"] = {"cancel", "Cancel", "", "", "", "", true, 0};
    uiTable.strings["yes"] = {"yes", "Yes", "", "", "", "", true, 0};
    uiTable.strings["no"] = {"no", "No", "", "", "", "", true, 0};
    uiTable.strings["back"] = {"back", "Back", "", "", "", "", true, 0};
    uiTable.strings["next"] = {"next", "Next", "", "", "", "", true, 0};
    uiTable.strings["loading"] = {"loading", "Loading...", "", "", "", "", true, 0};
    uiTable.strings["saving"] = {"saving", "Saving...", "", "", "", "", true, 0};
    
    m_stringTables["ui"][Language::English] = uiTable;
    
    // Gameplay Strings
    StringTable gameplayTable;
    gameplayTable.category = "gameplay";
    gameplayTable.language = Language::English;
    
    gameplayTable.strings["wave"] = {"wave", "Wave", "", "", "", "", true, 0};
    gameplayTable.strings["gold"] = {"gold", "Gold", "", "", "", "", true, 0};
    gameplayTable.strings["score"] = {"score", "Score", "", "", "", "", true, 0};
    gameplayTable.strings["health"] = {"health", "Health", "", "", "", "", true, 0};
    gameplayTable.strings["attack"] = {"attack", "Attack", "", "", "", "", true, 0};
    gameplayTable.strings["defense"] = {"defense", "Defense", "", "", "", "", true, 0};
    gameplayTable.strings["speed"] = {"speed", "Speed", "", "", "", "", true, 0};
    gameplayTable.strings["range"] = {"range", "Range", "", "", "", "", true, 0};
    gameplayTable.strings["damage"] = {"damage", "Damage", "", "", "", "", true, 0};
    gameplayTable.strings["critical"] = {"critical", "Critical!", "", "", "", "", true, 0};
    gameplayTable.strings["miss"] = {"miss", "Miss", "", "", "", "", true, 0};
    gameplayTable.strings["victory"] = {"victory", "Victory!", "", "", "", "", true, 0};
    gameplayTable.strings["defeat"] = {"defeat", "Defeat", "", "", "", "", true, 0};
    gameplayTable.strings["your_turn"] = {"your_turn", "Your Turn", "", "", "", "", true, 0};
    gameplayTable.strings["enemy_turn"] = {"enemy_turn", "Enemy Turn", "", "", "", "", true, 0};
    gameplayTable.strings["summon"] = {"summon", "Summon", "", "", "", "", true, 0};
    gameplayTable.strings["move"] = {"move", "Move", "", "", "", "", true, 0};
    gameplayTable.strings["end_turn"] = {"end_turn", "End Turn", "", "", "", "", true, 0};
    
    m_stringTables["gameplay"][Language::English] = gameplayTable;
    
    // Unit names
    StringTable unitsTable;
    unitsTable.category = "units";
    unitsTable.language = Language::English;
    
    unitsTable.strings["mage"] = {"mage", "Mage", "", "", "", "", true, 0};
    unitsTable.strings["soldier"] = {"soldier", "Soldier", "", "", "", "", true, 0};
    unitsTable.strings["rogue"] = {"rogue", "Rogue", "", "", "", "", true, 0};
    unitsTable.strings["healer"] = {"healer", "Healer", "", "", "", "", true, 0};
    unitsTable.strings["tank"] = {"tank", "Tank", "", "", "", "", true, 0};
    unitsTable.strings["archer"] = {"archer", "Archer", "", "", "", "", true, 0};
    
    m_stringTables["units"][Language::English] = unitsTable;
    
    // Achievements
    StringTable achievementsTable;
    achievementsTable.category = "achievements";
    achievementsTable.language = Language::English;
    
    achievementsTable.strings["first_victory"] = {"first_victory", "First Blood", "", "", "", "", true, 0};
    achievementsTable.strings["first_victory_desc"] = {"first_victory_desc", "Win your first game", "", "", "", "", true, 0};
    achievementsTable.strings["boss_slayer"] = {"boss_slayer", "Boss Slayer", "", "", "", "", true, 0};
    achievementsTable.strings["boss_slayer_desc"] = {"boss_slayer_desc", "Defeat all bosses", "", "", "", "", true, 0};
    
    m_stringTables["achievements"][Language::English] = achievementsTable;
    
    Logger::Info("Loaded {} string categories", m_stringTables.size());
}

std::string LocalizationSystem::GetFallbackString(const std::string& key) const {
    m_missingCount++;
    m_missingStrings.push_back(key);
    
    switch (m_missingBehavior) {
        case MissingStringBehavior::ReturnKey:
            return "[" + key + "]";
        case MissingStringBehavior::ReturnEmpty:
            return "";
        case MissingStringBehavior::ReturnFallback:
            // Try English fallback
            for (const auto& [category, languages] : m_stringTables) {
                auto it = languages.find(m_fallbackLanguage);
                if (it != languages.end()) {
                    auto strIt = it->second.strings.find(key);
                    if (strIt != it->second.strings.end()) {
                        return strIt->second.text;
                    }
                }
            }
            return "[" + key + "]";
        case MissingStringBehavior::ThrowError:
            Logger::Warning("Missing localization string: {}", key);
            return "[" + key + "]";
    }
    
    return "[" + key + "]";
}

int LocalizationSystem::GetPluralForm(int count) const {
    auto info = GetLanguageInfo(m_currentLanguage);
    
    switch (info.pluralRule) {
        case PluralRule::OneOther:
            return (count == 1) ? 0 : 1;
            
        case PluralRule::OneZeroOther:
            return (count == 0 || count == 1) ? 0 : 1;
            
        case PluralRule::OneFewMany:
            // Slavic languages (Russian, Polish)
            if (count == 1) return 0;
            if (count >= 2 && count <= 4) return 1;
            return 2;
            
        case PluralRule::OneTwoFewMany:
            // Arabic
            if (count == 1) return 0;
            if (count == 2) return 1;
            if (count >= 3 && count <= 10) return 2;
            return 3;
            
        case PluralRule::Other:
            // No plural (CJK)
            return 0;
    }
    
    return 0;
}

std::string LocalizationSystem::SubstituteVariables(const std::string& text, const FormatContext& context) const {
    std::string result = text;
    
    // Replace {varname} patterns
    std::regex varPattern("\\{([^}]+)\\}");
    std::smatch match;
    std::string::const_iterator searchStart = result.cbegin();
    
    while (std::regex_search(searchStart, result.cend(), match, varPattern)) {
        std::string varName = match[1].str();
        std::string replacement;
        
        // Check string vars
        auto strIt = context.stringVars.find(varName);
        if (strIt != context.stringVars.end()) {
            replacement = strIt->second;
        } else {
            // Check int vars
            auto intIt = context.intVars.find(varName);
            if (intIt != context.intVars.end()) {
                replacement = FormatNumber(intIt->second);
            } else {
                // Check float vars
                auto floatIt = context.floatVars.find(varName);
                if (floatIt != context.floatVars.end()) {
                    replacement = FormatNumber(floatIt->second, 1);
                }
            }
        }
        
        if (!replacement.empty()) {
            size_t pos = match.position() + (searchStart - result.cbegin());
            result.replace(pos, match.length(), replacement);
            searchStart = result.cbegin() + pos + replacement.length();
        } else {
            searchStart = match.suffix().first;
        }
    }
    
    return result;
}

std::string LocalizationSystem::ProcessFormatSpecifiers(const std::string& text) const {
    // Process format specifiers like %d, %s, %f
    // In production, would use proper formatting library
    return text;
}

} // namespace DDD
