#pragma once

#include "../Graphics/TextRenderer.h"
#include <SDL2/SDL.h>
#include <string>
#include <memory>
#include <functional>
#include <array>

namespace DDD {

class Renderer;
class Dice;

/**
 * DiceCard - Visual representation of a dice in the UI
 * Shows class name, stats, cost, and unfurl pattern
 */
class DiceCard {
public:
    DiceCard();
    explicit DiceCard(std::shared_ptr<Dice> dice);
    ~DiceCard() = default;
    
    /**
     * Update card state (animations, hover)
     */
    void Update(float deltaTime, int mouseX, int mouseY, bool mouseDown);
    
    /**
     * Render the card
     */
    void Render(Renderer& renderer);
    
    /**
     * Check if point is inside card
     */
    bool Contains(int x, int y) const;
    
    // Setters
    void SetDice(std::shared_ptr<Dice> dice) { m_dice = dice; }
    void SetPosition(int x, int y) { m_x = x; m_y = y; }
    void SetSize(int width, int height) { m_width = width; m_height = height; }
    void SetSelected(bool selected) { m_selected = selected; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    void SetVisible(bool visible) { m_visible = visible; }
    void SetCallback(std::function<void(std::shared_ptr<Dice>)> callback) { m_onClick = callback; }
    void SetDiceId(const std::string& id) { m_diceId = id; }
    
    // Getters
    std::shared_ptr<Dice> GetDice() const { return m_dice; }
    const std::string& GetDiceId() const { return m_diceId; }
    int GetX() const { return m_x; }
    int GetY() const { return m_y; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    bool IsSelected() const { return m_selected; }
    bool IsEnabled() const { return m_enabled; }
    bool IsVisible() const { return m_visible; }
    bool IsHovered() const { return m_hovered; }
    
    // Card dimensions
    static constexpr int DEFAULT_WIDTH = 140;
    static constexpr int DEFAULT_HEIGHT = 180;
    
private:
    void RenderBackground(Renderer& renderer);
    void RenderClassName(Renderer& renderer);
    void RenderStats(Renderer& renderer);
    void RenderUnfurlPattern(Renderer& renderer);
    void RenderCost(Renderer& renderer);
    void RenderSelectionHighlight(Renderer& renderer);
    void RenderRarityBorder(Renderer& renderer);
    
    // Get class color based on class type
    SDL_Color GetClassColor() const;
    
    std::shared_ptr<Dice> m_dice;
    std::string m_diceId;  // ID for looking up rarity/effects
    
    int m_x = 0;
    int m_y = 0;
    int m_width = DEFAULT_WIDTH;
    int m_height = DEFAULT_HEIGHT;
    
    bool m_visible = true;
    bool m_enabled = true;
    bool m_selected = false;
    bool m_hovered = false;
    bool m_wasPressed = false;
    
    // Animation
    float m_hoverOffset = 0.0f;
    float m_selectPulse = 0.0f;
    float m_rarityGlow = 0.0f;
    
    std::function<void(std::shared_ptr<Dice>)> m_onClick;
};

} // namespace DDD
