#pragma once

#include "Button.h"
#include <vector>
#include <memory>
#include <functional>

namespace DDD {

class Renderer;
class Game;

/**
 * MainMenu - Title screen with game logo and menu buttons
 */
class MainMenu {
public:
    explicit MainMenu(Game& game);
    ~MainMenu() = default;
    
    /**
     * Update menu (animations, button states)
     */
    void Update(float deltaTime, int mouseX, int mouseY, bool mouseDown);
    
    /**
     * Render the menu
     */
    void Render(Renderer& renderer);
    
    /**
     * Handle click events
     */
    void OnClick(int mouseX, int mouseY);
    
    /**
     * Set visibility
     */
    void SetVisible(bool visible) { m_visible = visible; }
    bool IsVisible() const { return m_visible; }
    
    // Callbacks for menu actions
    void SetOnNewGame(std::function<void()> callback) { m_onNewGame = callback; }
    void SetOnSettings(std::function<void()> callback) { m_onSettings = callback; }
    void SetOnQuit(std::function<void()> callback) { m_onQuit = callback; }
    
private:
    void CreateButtons();
    void RenderBackground(Renderer& renderer);
    void RenderTitle(Renderer& renderer);
    void RenderSubtitle(Renderer& renderer);
    void RenderVersion(Renderer& renderer);
    
    Game& m_game;
    bool m_visible = true;
    
    // Buttons
    std::unique_ptr<Button> m_newGameButton;
    std::unique_ptr<Button> m_settingsButton;
    std::unique_ptr<Button> m_quitButton;
    
    // Animation
    float m_time = 0.0f;
    float m_titleBob = 0.0f;
    float m_fadeIn = 0.0f;
    
    // Callbacks
    std::function<void()> m_onNewGame;
    std::function<void()> m_onSettings;
    std::function<void()> m_onQuit;
    
    // Screen dimensions (for centering)
    int m_screenWidth = 1280;
    int m_screenHeight = 960;
};

} // namespace DDD
