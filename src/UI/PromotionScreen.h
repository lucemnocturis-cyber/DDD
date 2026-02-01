#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <string>

namespace DDD {

class Game;
class Renderer;
class Unit;
class Button;

/**
 * Promotion option for display
 */
struct PromotionOption {
    std::string className;
    std::string description;
    int hp, atk, def, mov, rng;
    std::string abilityName;
    bool selected = false;
};

/**
 * PromotionScreen - UI for promoting units to higher tiers
 */
class PromotionScreen {
public:
    PromotionScreen(Game& game);
    ~PromotionScreen();
    
    void Initialize();
    void Update(float deltaTime, int mouseX, int mouseY, bool mouseDown);
    void Render(Renderer& renderer);
    
    void Show();
    void Hide();
    bool IsVisible() const { return m_visible; }
    
    /**
     * Set the unit to promote
     */
    void SetUnit(std::shared_ptr<Unit> unit);
    
    /**
     * Callbacks
     */
    void SetOnPromote(std::function<void(std::shared_ptr<Unit>, const std::string&)> callback) {
        m_onPromote = callback;
    }
    void SetOnSkip(std::function<void()> callback) { m_onSkip = callback; }
    void SetOnClose(std::function<void()> callback) { m_onClose = callback; }
    
private:
    void CreateButtons();
    void LoadPromotionOptions();
    void SelectOption(int index);
    void ConfirmPromotion();
    void RenderBackground(Renderer& renderer);
    void RenderUnitPreview(Renderer& renderer);
    void RenderPromotionOptions(Renderer& renderer);
    void RenderStatComparison(Renderer& renderer);
    
    Game& m_game;
    bool m_visible = false;
    
    // Current unit
    std::shared_ptr<Unit> m_unit;
    std::vector<PromotionOption> m_options;
    int m_selectedIndex = -1;
    
    // UI elements
    std::unique_ptr<Button> m_confirmButton;
    std::unique_ptr<Button> m_skipButton;
    std::vector<std::unique_ptr<Button>> m_optionButtons;
    
    // Callbacks
    std::function<void(std::shared_ptr<Unit>, const std::string&)> m_onPromote;
    std::function<void()> m_onSkip;
    std::function<void()> m_onClose;
    
    // Animation
    float m_fadeIn = 0.0f;
    float m_time = 0.0f;
    float m_previewPulse = 0.0f;
    
    // Layout
    int m_panelX = 140;
    int m_panelY = 100;
    int m_panelWidth = 1000;
    int m_panelHeight = 700;
};

} // namespace DDD
