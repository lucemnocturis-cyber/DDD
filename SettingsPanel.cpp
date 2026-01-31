#include "SettingsPanel.h"
#include "Button.h"
#include "../Core/Game.h"
#include "../Core/Engine.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/TextRenderer.h"
#include "../Audio/SoundManager.h"
#include "../Audio/MusicManager.h"
#include "../Utils/Logger.h"

#include <fstream>
#include <algorithm>

namespace DDD {

// ============================================================================
// Slider Implementation
// ============================================================================

Slider::Slider(const std::string& label, int x, int y, int width, float minVal, float maxVal)
    : m_label(label)
    , m_x(x)
    , m_y(y)
    , m_width(width)
    , m_minVal(minVal)
    , m_maxVal(maxVal)
    , m_value((minVal + maxVal) / 2.0f)
{
}

void Slider::Update(float deltaTime, int mouseX, int mouseY, bool mouseDown) {
    // Track area
    int trackX = m_x;
    int trackY = m_y + m_height - m_trackHeight - 5;
    int trackW = m_width;
    int trackH = m_trackHeight;
    
    // Handle position
    float normalizedValue = (m_value - m_minVal) / (m_maxVal - m_minVal);
    int handleX = trackX + static_cast<int>(normalizedValue * (trackW - m_handleWidth));
    int handleY = trackY - 4;
    int handleH = m_trackHeight + 8;
    
    // Check if mouse is over handle
    bool overHandle = mouseX >= handleX && mouseX < handleX + m_handleWidth &&
                      mouseY >= handleY && mouseY < handleY + handleH;
    
    // Check if mouse is over track
    bool overTrack = mouseX >= trackX && mouseX < trackX + trackW &&
                     mouseY >= trackY - 5 && mouseY < trackY + trackH + 5;
    
    m_hovered = overHandle || overTrack;
    
    // Handle dragging
    if (mouseDown && (overHandle || overTrack || m_dragging)) {
        m_dragging = true;
        
        // Calculate new value from mouse position
        float newNormalized = static_cast<float>(mouseX - trackX - m_handleWidth/2) / 
                              static_cast<float>(trackW - m_handleWidth);
        newNormalized = std::clamp(newNormalized, 0.0f, 1.0f);
        float newValue = m_minVal + newNormalized * (m_maxVal - m_minVal);
        
        if (newValue != m_value) {
            m_value = newValue;
            if (m_onChange) {
                m_onChange(m_value);
            }
        }
    } else {
        m_dragging = false;
    }
}

void Slider::Render(Renderer& renderer) {
    // Label
    renderer.GetTextRenderer()->RenderText(m_label, m_x, m_y, FontSize::Medium, {220, 220, 220, 255});
    
    // Value display
    int percentage = static_cast<int>(((m_value - m_minVal) / (m_maxVal - m_minVal)) * 100);
    std::string valueStr = std::to_string(percentage) + "%";
    renderer.GetTextRenderer()->RenderText(valueStr, m_x + m_width - 50, m_y, FontSize::Medium, {180, 180, 180, 255});
    
    // Track background
    int trackX = m_x;
    int trackY = m_y + m_height - m_trackHeight - 5;
    int trackW = m_width;
    int trackH = m_trackHeight;
    
    renderer.FillRect(trackX, trackY, trackW, trackH, {40, 40, 50, 255});
    renderer.DrawRect(trackX, trackY, trackW, trackH, {80, 80, 90, 255});
    
    // Filled portion
    float normalizedValue = (m_value - m_minVal) / (m_maxVal - m_minVal);
    int filledWidth = static_cast<int>(normalizedValue * trackW);
    
    SDL_Color fillColor = m_hovered || m_dragging ? 
        SDL_Color{100, 180, 255, 255} : SDL_Color{70, 130, 200, 255};
    renderer.FillRect(trackX, trackY, filledWidth, trackH, fillColor);
    
    // Handle
    int handleX = trackX + static_cast<int>(normalizedValue * (trackW - m_handleWidth));
    int handleY = trackY - 4;
    int handleH = m_trackHeight + 8;
    
    SDL_Color handleColor = m_dragging ? 
        SDL_Color{255, 255, 255, 255} : 
        (m_hovered ? SDL_Color{220, 220, 230, 255} : SDL_Color{180, 180, 190, 255});
    
    renderer.FillRect(handleX, handleY, m_handleWidth, handleH, handleColor);
    renderer.DrawRect(handleX, handleY, m_handleWidth, handleH, {100, 100, 110, 255});
}

void Slider::SetValue(float value) {
    m_value = std::clamp(value, m_minVal, m_maxVal);
}

// ============================================================================
// SettingsPanel Implementation
// ============================================================================

SettingsPanel::SettingsPanel(Game& game)
    : m_game(game)
{
}

SettingsPanel::~SettingsPanel() = default;

void SettingsPanel::Initialize() {
    LoadSettings();
    CreateUI();
}

void SettingsPanel::CreateUI() {
    int contentX = m_panelX + 50;
    int contentY = m_panelY + 80;
    int sliderWidth = m_panelWidth - 100;
    int rowHeight = 70;
    
    // Music volume slider
    m_musicSlider = std::make_unique<Slider>("Music Volume", contentX, contentY, sliderWidth, 0.0f, 1.0f);
    m_musicSlider->SetValue(m_musicVolume);
    m_musicSlider->SetOnChange([this](float value) {
        m_musicVolume = value;
        if (g_musicManager) {
            g_musicManager->SetVolume(value);
        }
    });
    
    // SFX volume slider
    m_sfxSlider = std::make_unique<Slider>("Sound Effects", contentX, contentY + rowHeight, sliderWidth, 0.0f, 1.0f);
    m_sfxSlider->SetValue(m_sfxVolume);
    m_sfxSlider->SetOnChange([this](float value) {
        m_sfxVolume = value;
        if (g_soundManager) {
            g_soundManager->SetVolume(value);
            // Play test sound
            g_soundManager->Play(SoundID::ButtonClick);
        }
    });
    
    // Particles toggle
    int toggleY = contentY + rowHeight * 2 + 20;
    m_particlesToggle = std::make_unique<Button>(
        m_particlesEnabled ? "Particles: ON" : "Particles: OFF",
        contentX + sliderWidth / 2, toggleY, 200, 45
    );
    m_particlesToggle->SetCentered(true);
    m_particlesToggle->SetStyle(m_particlesEnabled ? ButtonStyle::Primary : ButtonStyle::Secondary);
    m_particlesToggle->SetCallback([this]() {
        m_particlesEnabled = !m_particlesEnabled;
        m_particlesToggle->SetText(m_particlesEnabled ? "Particles: ON" : "Particles: OFF");
        m_particlesToggle->SetStyle(m_particlesEnabled ? ButtonStyle::Primary : ButtonStyle::Secondary);
    });
    
    // Screen shake toggle
    m_shakeToggle = std::make_unique<Button>(
        m_screenShakeEnabled ? "Screen Shake: ON" : "Screen Shake: OFF",
        contentX + sliderWidth / 2, toggleY + 60, 200, 45
    );
    m_shakeToggle->SetCentered(true);
    m_shakeToggle->SetStyle(m_screenShakeEnabled ? ButtonStyle::Primary : ButtonStyle::Secondary);
    m_shakeToggle->SetCallback([this]() {
        m_screenShakeEnabled = !m_screenShakeEnabled;
        m_shakeToggle->SetText(m_screenShakeEnabled ? "Screen Shake: ON" : "Screen Shake: OFF");
        m_shakeToggle->SetStyle(m_screenShakeEnabled ? ButtonStyle::Primary : ButtonStyle::Secondary);
    });
    
    // Back button
    m_backButton = std::make_unique<Button>("BACK", m_panelX + m_panelWidth / 2, m_panelY + m_panelHeight - 60, 200, 50);
    m_backButton->SetCentered(true);
    m_backButton->SetStyle(ButtonStyle::Secondary);
    m_backButton->SetCallback([this]() {
        SaveSettings();
        ApplySettings();
        Hide();
        if (m_onClose) m_onClose();
    });
}

void SettingsPanel::Update(float deltaTime, int mouseX, int mouseY, bool mouseDown) {
    if (!m_visible) return;
    
    // Fade in
    if (m_fadeIn < 1.0f) {
        m_fadeIn += deltaTime * 4.0f;
        if (m_fadeIn > 1.0f) m_fadeIn = 1.0f;
    }
    
    // Update sliders
    if (m_musicSlider) m_musicSlider->Update(deltaTime, mouseX, mouseY, mouseDown);
    if (m_sfxSlider) m_sfxSlider->Update(deltaTime, mouseX, mouseY, mouseDown);
    
    // Update buttons
    if (m_particlesToggle) m_particlesToggle->Update(deltaTime, mouseX, mouseY, mouseDown);
    if (m_shakeToggle) m_shakeToggle->Update(deltaTime, mouseX, mouseY, mouseDown);
    if (m_backButton) m_backButton->Update(deltaTime, mouseX, mouseY, mouseDown);
}

void SettingsPanel::Render(Renderer& renderer) {
    if (!m_visible) return;
    
    uint8_t alpha = static_cast<uint8_t>(m_fadeIn * 255);
    
    // Dim background
    renderer.FillRect(0, 0, renderer.GetWidth(), renderer.GetHeight(), {0, 0, 0, static_cast<uint8_t>(alpha * 0.7f)});
    
    // Panel background
    renderer.FillRect(m_panelX, m_panelY, m_panelWidth, m_panelHeight, {30, 30, 40, alpha});
    renderer.DrawRect(m_panelX, m_panelY, m_panelWidth, m_panelHeight, {100, 100, 120, alpha});
    
    // Inner border
    renderer.DrawRect(m_panelX + 4, m_panelY + 4, m_panelWidth - 8, m_panelHeight - 8, {60, 60, 80, alpha});
    
    // Title
    renderer.GetTextRenderer()->RenderText("SETTINGS", m_panelX + m_panelWidth / 2 - 60, m_panelY + 25, 
                                           FontSize::XLarge, {255, 220, 100, alpha});
    
    // Decorative line under title
    renderer.FillRect(m_panelX + 50, m_panelY + 60, m_panelWidth - 100, 2, {100, 100, 120, alpha});
    
    // Audio section header
    renderer.GetTextRenderer()->RenderText("Audio", m_panelX + 50, m_panelY + 75, 
                                           FontSize::Large, {180, 180, 200, alpha});
    
    // Sliders
    if (m_musicSlider) m_musicSlider->Render(renderer);
    if (m_sfxSlider) m_sfxSlider->Render(renderer);
    
    // Visual section header
    int visualY = m_panelY + 230;
    renderer.FillRect(m_panelX + 50, visualY, m_panelWidth - 100, 2, {100, 100, 120, alpha});
    renderer.GetTextRenderer()->RenderText("Visual Effects", m_panelX + 50, visualY + 10, 
                                           FontSize::Large, {180, 180, 200, alpha});
    
    // Toggle buttons
    if (m_particlesToggle) m_particlesToggle->Render(renderer);
    if (m_shakeToggle) m_shakeToggle->Render(renderer);
    
    // Controls section
    int controlsY = m_panelY + 400;
    renderer.FillRect(m_panelX + 50, controlsY, m_panelWidth - 100, 2, {100, 100, 120, alpha});
    renderer.GetTextRenderer()->RenderText("Controls", m_panelX + 50, controlsY + 10, 
                                           FontSize::Large, {180, 180, 200, alpha});
    
    // Control hints
    int hintY = controlsY + 45;
    SDL_Color hintColor = {150, 150, 160, alpha};
    renderer.GetTextRenderer()->RenderText("Left Click - Select / Move / Attack", m_panelX + 70, hintY, 
                                           FontSize::Small, hintColor);
    renderer.GetTextRenderer()->RenderText("Right Click - Cancel Selection", m_panelX + 70, hintY + 22, 
                                           FontSize::Small, hintColor);
    renderer.GetTextRenderer()->RenderText("Escape - Pause Game", m_panelX + 70, hintY + 44, 
                                           FontSize::Small, hintColor);
    renderer.GetTextRenderer()->RenderText("Space - End Turn", m_panelX + 70, hintY + 66, 
                                           FontSize::Small, hintColor);
    
    // Back button
    if (m_backButton) m_backButton->Render(renderer);
}

void SettingsPanel::Show() {
    m_visible = true;
    m_fadeIn = 0.0f;
    
    // Sync UI with current settings
    if (m_musicSlider) m_musicSlider->SetValue(m_musicVolume);
    if (m_sfxSlider) m_sfxSlider->SetValue(m_sfxVolume);
    
    Logger::Debug("Settings panel shown");
}

void SettingsPanel::Hide() {
    m_visible = false;
    Logger::Debug("Settings panel hidden");
}

void SettingsPanel::ApplySettings() {
    // Apply audio settings
    if (g_musicManager) {
        g_musicManager->SetVolume(m_musicVolume);
    }
    if (g_soundManager) {
        g_soundManager->SetVolume(m_sfxVolume);
    }
    
    // Visual settings would be applied through Game's particle/effects systems
    // For now, store them for the game to query
    
    Logger::Info("Settings applied - Music: {}%, SFX: {}%, Particles: {}, Shake: {}",
                 static_cast<int>(m_musicVolume * 100),
                 static_cast<int>(m_sfxVolume * 100),
                 m_particlesEnabled ? "ON" : "OFF",
                 m_screenShakeEnabled ? "ON" : "OFF");
}

void SettingsPanel::LoadSettings() {
    std::ifstream file("settings.cfg");
    if (!file.is_open()) {
        Logger::Debug("No settings file found, using defaults");
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find('=');
        if (pos == std::string::npos) continue;
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        if (key == "music_volume") {
            m_musicVolume = std::stof(value);
        } else if (key == "sfx_volume") {
            m_sfxVolume = std::stof(value);
        } else if (key == "particles") {
            m_particlesEnabled = (value == "1");
        } else if (key == "screen_shake") {
            m_screenShakeEnabled = (value == "1");
        }
    }
    
    // Clamp values
    m_musicVolume = std::clamp(m_musicVolume, 0.0f, 1.0f);
    m_sfxVolume = std::clamp(m_sfxVolume, 0.0f, 1.0f);
    
    Logger::Info("Settings loaded from file");
}

void SettingsPanel::SaveSettings() {
    std::ofstream file("settings.cfg");
    if (!file.is_open()) {
        Logger::Warning("Failed to save settings file");
        return;
    }
    
    file << "music_volume=" << m_musicVolume << "\n";
    file << "sfx_volume=" << m_sfxVolume << "\n";
    file << "particles=" << (m_particlesEnabled ? "1" : "0") << "\n";
    file << "screen_shake=" << (m_screenShakeEnabled ? "1" : "0") << "\n";
    
    Logger::Info("Settings saved to file");
}

} // namespace DDD
