#pragma once

#include "DiceCard.h"
#include "Button.h"
#include <vector>
#include <memory>
#include <functional>

namespace DDD {

class Renderer;
class Dice;
class Game;

/**
 * DicePanel - UI panel showing player's dice hand
 * Allows selecting dice for placement and re-rolling
 */
class DicePanel {
public:
    explicit DicePanel(Game& game);
    ~DicePanel() = default;
    
    /**
     * Initialize the panel
     */
    void Initialize();
    
    /**
     * Update panel state
     */
    void Update(float deltaTime, int mouseX, int mouseY, bool mouseDown);
    
    /**
     * Render the panel
     */
    void Render(Renderer& renderer);
    
    /**
     * Set the dice to display
     */
    void SetDice(const std::vector<std::shared_ptr<Dice>>& dice);
    
    /**
     * Add a dice to the hand
     */
    void AddDice(std::shared_ptr<Dice> dice);
    
    /**
     * Remove a dice from the hand
     */
    void RemoveDice(std::shared_ptr<Dice> dice);
    
    /**
     * Clear all dice
     */
    void ClearDice();
    
    /**
     * Get selected dice
     */
    std::shared_ptr<Dice> GetSelectedDice() const { return m_selectedDice; }
    
    /**
     * Clear selection
     */
    void ClearSelection();
    
    /**
     * Roll all dice (randomize faces)
     */
    void RollAllDice();
    
    /**
     * Reset roll count for new wave
     */
    void ResetRolls();
    
    // Panel positioning
    void SetPosition(int x, int y) { m_x = x; m_y = y; UpdateLayout(); }
    void SetVisible(bool visible) { m_visible = visible; }
    bool IsVisible() const { return m_visible; }
    
    // Callbacks
    void SetOnDiceSelected(std::function<void(std::shared_ptr<Dice>)> cb) { m_onDiceSelected = cb; }
    void SetOnRollClicked(std::function<void()> cb) { m_onRollClicked = cb; }
    
    // Panel dimensions
    static constexpr int PANEL_WIDTH = 170;
    static constexpr int MAX_DICE = 5;
    
private:
    void UpdateLayout();
    void OnDiceCardClicked(std::shared_ptr<Dice> dice);
    void RenderPanelBackground(Renderer& renderer);
    void RenderTitle(Renderer& renderer);
    void RenderGoldDisplay(Renderer& renderer);
    
    Game& m_game;
    
    int m_x = 0;
    int m_y = 0;
    bool m_visible = true;
    
    // Dice cards
    std::vector<std::unique_ptr<DiceCard>> m_diceCards;
    std::shared_ptr<Dice> m_selectedDice;
    
    // Roll button
    std::unique_ptr<Button> m_rollButton;
    int m_rollsRemaining = 3;
    
    // Callbacks
    std::function<void(std::shared_ptr<Dice>)> m_onDiceSelected;
    std::function<void()> m_onRollClicked;
    
    // Animation
    float m_time = 0.0f;
};

} // namespace DDD
