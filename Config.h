#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace DDD {

/**
 * Game configuration settings
 */
struct Config {
    // Window settings
    int windowWidth = 1280;
    int windowHeight = 960;
    bool fullscreen = false;
    bool vsync = true;
    
    // Audio settings
    float masterVolume = 1.0f;
    float musicVolume = 0.7f;
    float sfxVolume = 1.0f;
    
    // Gameplay settings
    float animationSpeed = 1.0f;
    bool showDamageNumbers = true;
    bool showGridOverlay = true;
    
    // Controls
    bool invertMouseY = false;
    float mouseSensitivity = 1.0f;
    
    /**
     * Load config from file
     */
    static Config Load(const std::string& path);
    
    /**
     * Save config to file
     */
    void Save(const std::string& path) const;
    
    /**
     * Get default config
     */
    static Config GetDefault();
};

// JSON serialization
void to_json(nlohmann::json& j, const Config& c);
void from_json(const nlohmann::json& j, Config& c);

} // namespace DDD
