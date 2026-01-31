#include "GameOverScreen.h"
#include "../Core/Game.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/TextRenderer.h"
#include "../Utils/Logger.h"

#include <cmath>

namespace DDD {

GameOverScreen::GameOverScreen(Game& game) : m_game(game) {}

void GameOverScreen::Initialize() {
    int screenWidth = 1280;
    int screenHeight = 720;
    int buttonWidth = 200;
    int buttonHeight = 50;
    int buttonSpacing = 20;
    int centerX = screenWidth / 2;
    int buttonsY = screenHeight - 150;
    
    // Main menu button
    m_mainMenuButton = std::make_unique<Button>("MAIN MENU", 
        centerX - buttonWidth - buttonSpacing / 2, buttonsY, 
        buttonWidth, buttonHeight);
    m_mainMenuButton->SetCallback([this]() {
        if (m_onMainMenu) m_onMainMenu();
    });
    m_mainMenuButton->SetColors(
        {60, 60, 80, 255},    // Normal
        {80, 80, 100, 255},   // Hover
        {40, 40, 60, 255}     // Pressed
    );
    
    // Play again button
    m_playAgainButton = std::make_unique<Button>("PLAY AGAIN",
        centerX + buttonSpacing / 2, buttonsY,
        buttonWidth, buttonHeight);
    m_playAgainButton->SetCallback([this]() {
        if (m_onPlayAgain) m_onPlayAgain();
    });
    m_playAgainButton->SetColors(
        {80, 120, 80, 255},   // Normal (green)
        {100, 150, 100, 255}, // Hover
        {60, 100, 60, 255}    // Pressed
    );
    
    Logger::Info("GameOverScreen initialized");
}

void GameOverScreen::SetVictory(bool victory) {
    m_victory = victory;
    m_fadeIn = 0.0f;
    m_time = 0.0f;
}

void GameOverScreen::SetVisible(bool visible) {
    m_visible = visible;
    if (visible) {
        m_fadeIn = 0.0f;
        m_time = 0.0f;
    }
}

void GameOverScreen::Update(float deltaTime, int mouseX, int mouseY, bool mouseDown) {
    if (!m_visible) return;
    
    m_time += deltaTime;
    
    // Fade in animation
    if (m_fadeIn < 1.0f) {
        m_fadeIn += deltaTime * 2.0f;  // 0.5 second fade
        if (m_fadeIn > 1.0f) m_fadeIn = 1.0f;
    }
    
    // Update buttons
    if (m_mainMenuButton) {
        m_mainMenuButton->Update(deltaTime, mouseX, mouseY, mouseDown);
    }
    if (m_playAgainButton) {
        m_playAgainButton->Update(deltaTime, mouseX, mouseY, mouseDown);
    }
}

void GameOverScreen::Render(Renderer& renderer) {
    if (!m_visible) return;
    
    RenderBackground(renderer);
    RenderTitle(renderer);
    RenderStats(renderer);
    RenderButtons(renderer);
}

void GameOverScreen::RenderBackground(Renderer& renderer) {
    int screenWidth = renderer.GetWidth();
    int screenHeight = renderer.GetHeight();
    
    // Dark overlay with fade
    uint8_t alpha = static_cast<uint8_t>(200 * m_fadeIn);
    SDL_Color bgColor = {15, 15, 25, alpha};
    renderer.FillRect(0, 0, screenWidth, screenHeight, bgColor);
    
    // Central panel
    int panelWidth = 500;
    int panelHeight = 450;
    int panelX = (screenWidth - panelWidth) / 2;
    int panelY = (screenHeight - panelHeight) / 2 - 30;
    
    // Panel background
    SDL_Color panelBg = {25, 25, 45, static_cast<uint8_t>(240 * m_fadeIn)};
    renderer.FillRect(panelX, panelY, panelWidth, panelHeight, panelBg);
    
    // Panel border
    SDL_Color borderColor = m_victory ? SDL_Color{200, 180, 50, 255} : SDL_Color{180, 50, 50, 255};
    borderColor.a = static_cast<uint8_t>(255 * m_fadeIn);
    renderer.DrawRect(panelX, panelY, panelWidth, panelHeight, borderColor);
    renderer.DrawRect(panelX + 2, panelY + 2, panelWidth - 4, panelHeight - 4, borderColor);
    
    // Accent bar at top
    SDL_Color accentColor = m_victory ? SDL_Color{255, 215, 0, 255} : SDL_Color{200, 60, 60, 255};
    accentColor.a = static_cast<uint8_t>(255 * m_fadeIn);
    renderer.FillRect(panelX, panelY, panelWidth, 6, accentColor);
}

void GameOverScreen::RenderTitle(Renderer& renderer) {
    int screenWidth = renderer.GetWidth();
    int centerX = screenWidth / 2;
    int titleY = 150;
    
    // Pulsing effect for title
    float pulse = (std::sin(m_time * 3.0f) + 1.0f) * 0.5f;
    
    SDL_Color titleColor;
    if (m_victory) {
        titleColor = {
            static_cast<uint8_t>(200 + pulse * 55),
            static_cast<uint8_t>(180 + pulse * 35),
            static_cast<uint8_t>(50),
            static_cast<uint8_t>(255 * m_fadeIn)
        };
    } else {
        titleColor = {
            static_cast<uint8_t>(180 + pulse * 50),
            static_cast<uint8_t>(50),
            static_cast<uint8_t>(50),
            static_cast<uint8_t>(255 * m_fadeIn)
        };
    }
    
    std::string title = m_victory ? "VICTORY!" : "DEFEAT";
    SDL_Color outlineColor = {0, 0, 0, static_cast<uint8_t>(255 * m_fadeIn)};
    
    renderer.DrawTextWithOutline(title, centerX, titleY, titleColor, outlineColor,
                                  FontSize::Title, TextAlign::Center);
    
    // Subtitle
    std::string subtitle = m_victory ? 
        "You have conquered the dungeon!" : 
        "Your army has fallen...";
    
    SDL_Color subColor = {180, 180, 200, static_cast<uint8_t>(200 * m_fadeIn)};
    renderer.DrawText(subtitle, centerX, titleY + 60, subColor, FontSize::Medium, TextAlign::Center);
}

void GameOverScreen::RenderStats(Renderer& renderer) {
    int screenWidth = renderer.GetWidth();
    int centerX = screenWidth / 2;
    int statsY = 280;
    int lineHeight = 40;
    
    SDL_Color labelColor = {150, 150, 170, static_cast<uint8_t>(255 * m_fadeIn)};
    SDL_Color valueColor = {255, 255, 255, static_cast<uint8_t>(255 * m_fadeIn)};
    SDL_Color goldColor = {255, 215, 0, static_cast<uint8_t>(255 * m_fadeIn)};
    
    // Final Score
    renderer.DrawText("FINAL SCORE", centerX, statsY, labelColor, FontSize::Medium, TextAlign::Center);
    
    std::string scoreStr = std::to_string(m_game.GetScore());
    // Add comma separators for large numbers
    int insertPos = static_cast<int>(scoreStr.length()) - 3;
    while (insertPos > 0) {
        scoreStr.insert(insertPos, ",");
        insertPos -= 3;
    }
    
    renderer.DrawTextWithOutline(scoreStr, centerX, statsY + 30, goldColor, 
                                  {0, 0, 0, static_cast<uint8_t>(255 * m_fadeIn)},
                                  FontSize::Title, TextAlign::Center);
    
    // Wave reached
    int waveY = statsY + 100;
    renderer.DrawText("WAVE REACHED", centerX, waveY, labelColor, FontSize::Medium, TextAlign::Center);
    
    int wave = m_game.GetCurrentWave();
    if (!m_victory) wave = std::max(1, wave);  // Show wave where player died
    std::string waveStr = std::to_string(wave) + " / " + std::to_string(Game::MAX_WAVES);
    renderer.DrawText(waveStr, centerX, waveY + 30, valueColor, FontSize::Large, TextAlign::Center);
    
    // Gold earned
    int goldY = statsY + 170;
    renderer.DrawText("GOLD EARNED", centerX, goldY, labelColor, FontSize::Medium, TextAlign::Center);
    renderer.DrawText(std::to_string(m_game.GetPlayerGold()), centerX, goldY + 30, goldColor, 
                      FontSize::Large, TextAlign::Center);
}

void GameOverScreen::RenderButtons(Renderer& renderer) {
    if (m_mainMenuButton) {
        m_mainMenuButton->Render(renderer);
    }
    if (m_playAgainButton) {
        m_playAgainButton->Render(renderer);
    }
}

} // namespace DDD
