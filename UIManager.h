#pragma once

#include <memory>
#include <vector>

namespace DDD {

class Game;
class Renderer;
class UIElement;
class HUD;
class MainMenu;
class DicePanel;
class UnitInfoPanel;
class GameOverScreen;
class SettingsPanel;
class PromotionScreen;

class UIManager {
public:
    explicit UIManager(Game& game);
    ~UIManager();
    
    bool Initialize();
    void Update(float deltaTime);
    void Render(Renderer& renderer);
    
    // Mouse handling
    void OnMouseMove(int x, int y);
    void OnMouseDown();
    void OnMouseUp();
    
    // Component access
    HUD* GetHUD() const { return m_hud.get(); }
    MainMenu* GetMainMenu() const { return m_mainMenu.get(); }
    DicePanel* GetDicePanel() const { return m_dicePanel.get(); }
    UnitInfoPanel* GetUnitInfoPanel() const { return m_unitInfoPanel.get(); }
    GameOverScreen* GetGameOverScreen() const { return m_gameOverScreen.get(); }
    SettingsPanel* GetSettingsPanel() const { return m_settingsPanel.get(); }
    PromotionScreen* GetPromotionScreen() const { return m_promotionScreen.get(); }
    
    // UI state methods
    void ShowMainMenu();
    void ShowBattleUI();
    void ShowWaveRewardUI();
    void ShowShopUI();
    void ShowPromotionUI();
    void ShowPromotionUIForUnit(std::shared_ptr<class Unit> unit);
    void ShowGameOverUI(bool victory);
    void ShowSettingsUI();
    void ShowPauseMenu();
    void HideAll();
    
    // Battle UI helpers
    void GivePlayerStartingDice();
    void ShowUnitInfo(std::shared_ptr<class Unit> unit);
    void HideUnitInfo();
    
private:
    Game& m_game;
    std::unique_ptr<HUD> m_hud;
    std::unique_ptr<MainMenu> m_mainMenu;
    std::unique_ptr<DicePanel> m_dicePanel;
    std::unique_ptr<UnitInfoPanel> m_unitInfoPanel;
    std::unique_ptr<GameOverScreen> m_gameOverScreen;
    std::unique_ptr<SettingsPanel> m_settingsPanel;
    std::unique_ptr<PromotionScreen> m_promotionScreen;
    std::vector<std::shared_ptr<UIElement>> m_elements;
    
    // Mouse state
    int m_mouseX = 0;
    int m_mouseY = 0;
    bool m_mouseDown = false;
    
    bool m_showMainMenu = false;
    bool m_showBattleUI = false;
    bool m_showGameOver = false;
    bool m_showSettings = false;
    bool m_showPromotion = false;
};

} // namespace DDD
