#include "Config.h"
#include "../Utils/Logger.h"

#include <fstream>

namespace DDD {

Config Config::Load(const std::string& path) {
    Config config;
    
    try {
        std::ifstream file(path);
        if (file.is_open()) {
            nlohmann::json j;
            file >> j;
            config = j.get<Config>();
            Logger::Info("Config loaded from: {}", path);
        } else {
            Logger::Warning("Config file not found, using defaults: {}", path);
            config = GetDefault();
        }
    } catch (const std::exception& e) {
        Logger::Error("Failed to load config: {}", e.what());
        config = GetDefault();
    }
    
    return config;
}

void Config::Save(const std::string& path) const {
    try {
        std::ofstream file(path);
        if (file.is_open()) {
            nlohmann::json j = *this;
            file << j.dump(4);  // Pretty print with 4-space indent
            Logger::Info("Config saved to: {}", path);
        } else {
            Logger::Error("Failed to open config file for writing: {}", path);
        }
    } catch (const std::exception& e) {
        Logger::Error("Failed to save config: {}", e.what());
    }
}

Config Config::GetDefault() {
    return Config{};
}

void to_json(nlohmann::json& j, const Config& c) {
    j = nlohmann::json{
        {"window", {
            {"width", c.windowWidth},
            {"height", c.windowHeight},
            {"fullscreen", c.fullscreen},
            {"vsync", c.vsync}
        }},
        {"audio", {
            {"masterVolume", c.masterVolume},
            {"musicVolume", c.musicVolume},
            {"sfxVolume", c.sfxVolume}
        }},
        {"gameplay", {
            {"animationSpeed", c.animationSpeed},
            {"showDamageNumbers", c.showDamageNumbers},
            {"showGridOverlay", c.showGridOverlay}
        }},
        {"controls", {
            {"invertMouseY", c.invertMouseY},
            {"mouseSensitivity", c.mouseSensitivity}
        }}
    };
}

void from_json(const nlohmann::json& j, Config& c) {
    // Window
    if (j.contains("window")) {
        const auto& w = j["window"];
        c.windowWidth = w.value("width", 1280);
        c.windowHeight = w.value("height", 960);
        c.fullscreen = w.value("fullscreen", false);
        c.vsync = w.value("vsync", true);
    }
    
    // Audio
    if (j.contains("audio")) {
        const auto& a = j["audio"];
        c.masterVolume = a.value("masterVolume", 1.0f);
        c.musicVolume = a.value("musicVolume", 0.7f);
        c.sfxVolume = a.value("sfxVolume", 1.0f);
    }
    
    // Gameplay
    if (j.contains("gameplay")) {
        const auto& g = j["gameplay"];
        c.animationSpeed = g.value("animationSpeed", 1.0f);
        c.showDamageNumbers = g.value("showDamageNumbers", true);
        c.showGridOverlay = g.value("showGridOverlay", true);
    }
    
    // Controls
    if (j.contains("controls")) {
        const auto& ctrl = j["controls"];
        c.invertMouseY = ctrl.value("invertMouseY", false);
        c.mouseSensitivity = ctrl.value("mouseSensitivity", 1.0f);
    }
}

} // namespace DDD
