#pragma once

#include "Button.h"
#include <memory>
#include <string>
#include <functional>

namespace DDD {

class Game;
class Renderer;

/**
 * GameOverScreen - displays victory or defeat with final stats
 */
class GameOverScreen {
public:
    explicit GameOverScreen(Game& game);
    ~GameOverScreen() = default;
    
    /**
     * Initialize the screen
     */
    void Initialize();
    
    /**
     * Set victory or defeat state
     */
    void SetVictory(bool victory);
    
    /**
     * Update animations
     */
    void Update(float deltaTime, int mouseX, int mouseY, bool mouseDown);
    
    /**
     * Render the screen
     */
    void Render(Renderer& renderer);
    
    /**
     * Show/hide the screen
     */
    void SetVisible(bool visible);
    bool IsVisible() const { return m_visible; }
    
    // Callbacks
    void SetOnMainMenu(std::function<void()> cb) { m_onMainMenu = cb; }
    void SetOnPlayAgain(std::function<void()> cb) { m_onPlayAgain = cb; }
    
private:
    void RenderBackground(Renderer& renderer);
    void RenderTitle(Renderer& renderer);
    void RenderStats(Renderer& renderer);
    void RenderButtons(Renderer& renderer);
    
    Game& m_game;
    bool m_visible = false;
    bool m_victory = false;
    
    // Animation
    float m_time = 0.0f;
    float m_fadeIn = 0.0f;
    
    // Buttons
    std::unique_ptr<Button> m_mainMenuButton;
    std::unique_ptr<Button> m_playAgainButton;
    
    // Callbacks
    std::function<void()> m_onMainMenu;
    std::function<void()> m_onPlayAgain;
};

} // namespace DDD
