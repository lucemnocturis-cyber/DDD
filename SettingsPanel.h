#pragma once

#include <memory>
#include <functional>
#include <string>

namespace DDD {

class Game;
class Renderer;
class Button;

/**
 * Slider UI component for settings
 */
class Slider {
public:
    Slider(const std::string& label, int x, int y, int width, float minVal, float maxVal);
    
    void Update(float deltaTime, int mouseX, int mouseY, bool mouseDown);
    void Render(Renderer& renderer);
    
    void SetValue(float value);
    float GetValue() const { return m_value; }
    
    void SetOnChange(std::function<void(float)> callback) { m_onChange = callback; }
    
private:
    std::string m_label;
    int m_x, m_y;
    int m_width;
    int m_height = 30;
    int m_trackHeight = 8;
    int m_handleWidth = 16;
    
    float m_minVal, m_maxVal;
    float m_value = 0.5f;
    
    bool m_dragging = false;
    bool m_hovered = false;
    
    std::function<void(float)> m_onChange;
};

/**
 * SettingsPanel - displays game settings
 */
class SettingsPanel {
public:
    SettingsPanel(Game& game);
    ~SettingsPanel();
    
    void Initialize();
    
    void Update(float deltaTime, int mouseX, int mouseY, bool mouseDown);
    void Render(Renderer& renderer);
    
    void Show();
    void Hide();
    bool IsVisible() const { return m_visible; }
    
    void SetOnClose(std::function<void()> callback) { m_onClose = callback; }
    
    // Settings accessors
    float GetMusicVolume() const { return m_musicVolume; }
    float GetSFXVolume() const { return m_sfxVolume; }
    bool IsFullscreen() const { return m_fullscreen; }
    bool IsParticlesEnabled() const { return m_particlesEnabled; }
    bool IsScreenShakeEnabled() const { return m_screenShakeEnabled; }
    
    // Apply settings
    void ApplySettings();
    void LoadSettings();
    void SaveSettings();
    
private:
    void CreateUI();
    
    Game& m_game;
    bool m_visible = false;
    
    // Panel dimensions
    int m_panelX = 340;
    int m_panelY = 180;
    int m_panelWidth = 600;
    int m_panelHeight = 600;
    
    // Settings values
    float m_musicVolume = 0.5f;
    float m_sfxVolume = 0.8f;
    bool m_fullscreen = false;
    bool m_particlesEnabled = true;
    bool m_screenShakeEnabled = true;
    
    // UI elements
    std::unique_ptr<Slider> m_musicSlider;
    std::unique_ptr<Slider> m_sfxSlider;
    std::unique_ptr<Button> m_particlesToggle;
    std::unique_ptr<Button> m_shakeToggle;
    std::unique_ptr<Button> m_applyButton;
    std::unique_ptr<Button> m_backButton;
    
    std::function<void()> m_onClose;
    
    // Animation
    float m_fadeIn = 0.0f;
};

} // namespace DDD
