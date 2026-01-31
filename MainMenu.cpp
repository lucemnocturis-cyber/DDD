#include "MainMenu.h"
#include "../Core/Game.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/TextRenderer.h"

#include <cmath>

namespace DDD {

MainMenu::MainMenu(Game& game)
    : m_game(game)
{
    CreateButtons();
}

void MainMenu::CreateButtons() {
    int centerX = m_screenWidth / 2;
    int buttonY = 380;
    int buttonSpacing = 70;
    int buttonWidth = 280;
    int buttonHeight = 55;
    
    // New Game button
    m_newGameButton = std::make_unique<Button>("NEW GAME", centerX, buttonY, buttonWidth, buttonHeight);
    m_newGameButton->SetCentered(true);
    m_newGameButton->SetStyle(ButtonStyle::Primary);
    m_newGameButton->SetCallback([this]() {
        if (m_onNewGame) m_onNewGame();
    });
    
    // Settings button
    m_settingsButton = std::make_unique<Button>("SETTINGS", centerX, buttonY + buttonSpacing, buttonWidth, buttonHeight);
    m_settingsButton->SetCentered(true);
    m_settingsButton->SetStyle(ButtonStyle::Secondary);
    m_settingsButton->SetCallback([this]() {
        if (m_onSettings) m_onSettings();
    });
    
    // Quit button
    m_quitButton = std::make_unique<Button>("QUIT", centerX, buttonY + buttonSpacing * 2, buttonWidth, buttonHeight);
    m_quitButton->SetCentered(true);
    m_quitButton->SetStyle(ButtonStyle::Danger);
    m_quitButton->SetCallback([this]() {
        if (m_onQuit) m_onQuit();
    });
}

void MainMenu::Update(float deltaTime, int mouseX, int mouseY, bool mouseDown) {
    if (!m_visible) return;
    
    m_time += deltaTime;
    
    // Fade in effect
    if (m_fadeIn < 1.0f) {
        m_fadeIn += deltaTime * 2.0f;
        if (m_fadeIn > 1.0f) m_fadeIn = 1.0f;
    }
    
    // Title bobbing animation
    m_titleBob = std::sin(m_time * 2.0f) * 5.0f;
    
    // Update buttons
    m_newGameButton->Update(deltaTime, mouseX, mouseY, mouseDown);
    m_settingsButton->Update(deltaTime, mouseX, mouseY, mouseDown);
    m_quitButton->Update(deltaTime, mouseX, mouseY, mouseDown);
}

void MainMenu::Render(Renderer& renderer) {
    if (!m_visible) return;
    
    m_screenWidth = renderer.GetWidth();
    m_screenHeight = renderer.GetHeight();
    
    // Update button positions if screen size changed
    int centerX = m_screenWidth / 2;
    int buttonY = m_screenHeight / 2 - 20;
    int buttonSpacing = 70;
    
    m_newGameButton->SetPosition(centerX, buttonY);
    m_settingsButton->SetPosition(centerX, buttonY + buttonSpacing);
    m_quitButton->SetPosition(centerX, buttonY + buttonSpacing * 2);
    
    RenderBackground(renderer);
    RenderTitle(renderer);
    RenderSubtitle(renderer);
    
    // Render buttons
    m_newGameButton->Render(renderer);
    m_settingsButton->Render(renderer);
    m_quitButton->Render(renderer);
    
    RenderVersion(renderer);
}

void MainMenu::RenderBackground(Renderer& renderer) {
    // Dark gradient background
    int height = renderer.GetHeight();
    int width = renderer.GetWidth();
    
    // Draw vertical gradient (dark blue to darker blue)
    for (int y = 0; y < height; y += 4) {
        float t = static_cast<float>(y) / height;
        uint8_t r = static_cast<uint8_t>(15 + t * 10);
        uint8_t g = static_cast<uint8_t>(15 + t * 15);
        uint8_t b = static_cast<uint8_t>(35 + t * 20);
        SDL_Color color = {r, g, b, 255};
        renderer.FillRect(0, y, width, 4, color);
    }
    
    // Draw decorative grid pattern (subtle)
    SDL_Color gridColor = {255, 255, 255, 10};
    int gridSize = 64;
    for (int x = 0; x < width; x += gridSize) {
        renderer.FillRect(x, 0, 1, height, gridColor);
    }
    for (int y = 0; y < height; y += gridSize) {
        renderer.FillRect(0, y, width, 1, gridColor);
    }
    
    // Draw animated diagonal lines
    SDL_Color lineColor = {255, 215, 0, 15};  // Gold, very subtle
    float offset = std::fmod(m_time * 30.0f, 100.0f);
    for (int i = -height; i < width + height; i += 100) {
        int x1 = static_cast<int>(i + offset);
        renderer.FillRect(x1, 0, 2, height, lineColor);
    }
    
    // Vignette effect (darker corners)
    int vignetteSize = 200;
    for (int i = 0; i < vignetteSize; i++) {
        uint8_t alpha = static_cast<uint8_t>((vignetteSize - i) * 0.3f);
        SDL_Color vColor = {0, 0, 0, alpha};
        
        // Top
        renderer.FillRect(0, i, width, 1, vColor);
        // Bottom
        renderer.FillRect(0, height - i - 1, width, 1, vColor);
        // Left
        renderer.FillRect(i, 0, 1, height, vColor);
        // Right
        renderer.FillRect(width - i - 1, 0, 1, height, vColor);
    }
}

void MainMenu::RenderTitle(Renderer& renderer) {
    int centerX = m_screenWidth / 2;
    int titleY = 100 + static_cast<int>(m_titleBob);
    
    // Apply fade in
    uint8_t alpha = static_cast<uint8_t>(255 * m_fadeIn);
    
    // Draw title shadow
    SDL_Color shadowColor = {0, 0, 0, static_cast<uint8_t>(alpha * 0.7f)};
    renderer.DrawText("DUNGEON DICE", centerX + 4, titleY + 4, shadowColor, FontSize::Huge, TextAlign::Center);
    renderer.DrawText("DUELISTS", centerX + 4, titleY + 60 + 4, shadowColor, FontSize::Huge, TextAlign::Center);
    
    // Draw title with gold color
    SDL_Color goldColor = {255, 215, 0, alpha};
    renderer.DrawText("DUNGEON DICE", centerX, titleY, goldColor, FontSize::Huge, TextAlign::Center);
    
    // Second line with slight color variation
    SDL_Color brightGold = {255, 230, 100, alpha};
    renderer.DrawText("DUELISTS", centerX, titleY + 60, brightGold, FontSize::Huge, TextAlign::Center);
    
    // Draw decorative line under title
    int lineY = titleY + 130;
    int lineWidth = 350;
    int lineX = centerX - lineWidth / 2;
    
    SDL_Color lineColor = {255, 215, 0, static_cast<uint8_t>(alpha * 0.6f)};
    renderer.FillRect(lineX, lineY, lineWidth, 3, lineColor);
    
    // Diamond decorations at line ends
    renderer.FillRect(lineX - 8, lineY - 4, 10, 10, lineColor);
    renderer.FillRect(lineX + lineWidth - 2, lineY - 4, 10, 10, lineColor);
}

void MainMenu::RenderSubtitle(Renderer& renderer) {
    int centerX = m_screenWidth / 2;
    int subtitleY = 260;
    
    uint8_t alpha = static_cast<uint8_t>(200 * m_fadeIn);
    SDL_Color subtitleColor = {180, 180, 200, alpha};
    
    renderer.DrawText("Tactical Roguelike Combat", centerX, subtitleY, subtitleColor, FontSize::Medium, TextAlign::Center);
    
    // Pulsing "Press any key" or hint text
    float pulse = (std::sin(m_time * 3.0f) + 1.0f) * 0.5f;
    uint8_t hintAlpha = static_cast<uint8_t>((100 + pulse * 100) * m_fadeIn);
    SDL_Color hintColor = {150, 150, 180, hintAlpha};
    
    renderer.DrawText("Roll the dice, conquer the arena", centerX, subtitleY + 30, hintColor, FontSize::Small, TextAlign::Center);
}

void MainMenu::RenderVersion(Renderer& renderer) {
    // Version info in bottom right
    SDL_Color versionColor = {100, 100, 120, 180};
    renderer.DrawText("v0.1.0 Alpha", m_screenWidth - 10, m_screenHeight - 25, 
                      versionColor, FontSize::Small, TextAlign::Right);
    
    // Credits/copyright in bottom left
    renderer.DrawText("(C) 2026 Dungeon Dice Duelists", 10, m_screenHeight - 25,
                      versionColor, FontSize::Small, TextAlign::Left);
}

void MainMenu::OnClick(int mouseX, int mouseY) {
    // Clicks are handled by button Update() method via callbacks
    // This method can be used for additional click handling if needed
}

} // namespace DDD
