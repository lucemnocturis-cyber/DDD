#include "UIManager.h"
#include "UIElement.h"
#include "HUD.h"
#include "MainMenu.h"
#include "DicePanel.h"
#include "UnitInfoPanel.h"
#include "GameOverScreen.h"
#include "SettingsPanel.h"
#include "PromotionScreen.h"
#include "../Core/Game.h"
#include "../Gameplay/UnitDatabase.h"
#include "../Core/Engine.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/TransitionManager.h"
#include "../Gameplay/Dice.h"
#include "../Gameplay/Unit.h"
#include "../Gameplay/SelectionManager.h"
#include "../Utils/Logger.h"

namespace DDD {

UIManager::UIManager(Game& game) : m_game(game) {}
UIManager::~UIManager() = default;

bool UIManager::Initialize() {
    // Create HUD
    m_hud = std::make_unique<HUD>(m_game);
    
    // Create Main Menu
    m_mainMenu = std::make_unique<MainMenu>(m_game);
    
    // Set up main menu callbacks
    m_mainMenu->SetOnNewGame([this]() {
        Logger::Info("New Game clicked");
        m_game.TransitionToState(GameStateType::Battle, TransitionType::Fade);
    });
    
    m_mainMenu->SetOnSettings([this]() {
        Logger::Info("Settings clicked");
        m_game.ChangeState(GameStateType::Settings);
    });
    
    m_mainMenu->SetOnQuit([this]() {
        Logger::Info("Quit clicked");
        m_game.GetEngine().Stop();
    });
    
    // Create Dice Panel
    m_dicePanel = std::make_unique<DicePanel>(m_game);
    m_dicePanel->Initialize();
    m_dicePanel->SetPosition(10, 110);  // Left side, below HUD
    m_dicePanel->SetVisible(false);
    
    // Set up dice panel callbacks
    m_dicePanel->SetOnDiceSelected([this](std::shared_ptr<Dice> dice) {
        // When a dice is selected in the panel, tell the selection manager
        if (m_game.GetSelectionManager()) {
            m_game.GetSelectionManager()->SetSelectedDice(dice);
        }
        // Hide unit info when selecting a dice
        HideUnitInfo();
    });
    
    // Create Unit Info Panel (right side of screen)
    m_unitInfoPanel = std::make_unique<UnitInfoPanel>();
    m_unitInfoPanel->SetPosition(1280 - UnitInfoPanel::PANEL_WIDTH - 10, 110);
    m_unitInfoPanel->SetVisible(true);
    
    // Create Game Over Screen
    m_gameOverScreen = std::make_unique<GameOverScreen>(m_game);
    m_gameOverScreen->Initialize();
    m_gameOverScreen->SetOnMainMenu([this]() {
        Logger::Info("Returning to main menu");
        m_game.TransitionToState(GameStateType::MainMenu, TransitionType::Fade);
    });
    m_gameOverScreen->SetOnPlayAgain([this]() {
        Logger::Info("Starting new game");
        m_game.ResetGame();
        m_game.TransitionToState(GameStateType::Battle, TransitionType::Fade);
    });
    
    // Create Settings Panel
    m_settingsPanel = std::make_unique<SettingsPanel>(m_game);
    m_settingsPanel->Initialize();
    m_settingsPanel->SetOnClose([this]() {
        m_showSettings = false;
        // Return to previous state
        if (m_showMainMenu) {
            m_game.ChangeState(GameStateType::MainMenu);
        }
    });
    
    // Create Promotion Screen
    m_promotionScreen = std::make_unique<PromotionScreen>(m_game);
    m_promotionScreen->Initialize();
    m_promotionScreen->SetOnPromote([this](std::shared_ptr<Unit> unit, const std::string& newClass) {
        // Apply promotion using UnitDatabase
        auto& db = UnitDatabase::Instance();
        const UnitClassDef* def = db.GetClassDef(newClass);
        if (def && unit) {
            unit->Promote(newClass);
            
            // Update stats from new class
            UnitStats newStats;
            newStats.hp = def->hp;
            newStats.maxHp = def->hp;
            newStats.atk = def->atk;
            newStats.def = def->def;
            newStats.mov = def->mov;
            newStats.rng = def->rng;
            unit->SetStats(newStats);
            
            unit->SetUnfurlPattern(def->unfurlPattern);
            unit->SetPromotionOptions(def->promotionOptions);
            unit->SetExpToPromote(def->expToPromote);
            
            Logger::Info("Unit promoted to {}", newClass);
        }
    });
    m_promotionScreen->SetOnClose([this]() {
        m_showPromotion = false;
        m_game.ChangeState(GameStateType::Battle);
    });
    
    Logger::Info("UIManager initialized");
    return true;
}

void UIManager::OnMouseMove(int x, int y) {
    m_mouseX = x;
    m_mouseY = y;
}

void UIManager::OnMouseDown() {
    m_mouseDown = true;
}

void UIManager::OnMouseUp() {
    m_mouseDown = false;
}

void UIManager::Update(float deltaTime) {
    // Update Settings Panel (priority)
    if (m_settingsPanel && m_showSettings) {
        m_settingsPanel->Update(deltaTime, m_mouseX, m_mouseY, m_mouseDown);
        return;  // Settings panel captures all input
    }
    
    // Update Promotion Screen (priority)
    if (m_promotionScreen && m_showPromotion) {
        m_promotionScreen->Update(deltaTime, m_mouseX, m_mouseY, m_mouseDown);
        return;  // Promotion screen captures all input
    }
    
    // Update HUD
    if (m_hud && m_showBattleUI) {
        m_hud->Update(deltaTime);
    }
    
    // Update Main Menu
    if (m_mainMenu && m_showMainMenu) {
        m_mainMenu->Update(deltaTime, m_mouseX, m_mouseY, m_mouseDown);
    }
    
    // Update Dice Panel
    if (m_dicePanel && m_showBattleUI) {
        m_dicePanel->Update(deltaTime, m_mouseX, m_mouseY, m_mouseDown);
    }
    
    // Update Unit Info Panel
    if (m_unitInfoPanel && m_showBattleUI) {
        m_unitInfoPanel->Update(deltaTime);
    }
    
    // Update Game Over Screen
    if (m_gameOverScreen && m_showGameOver) {
        m_gameOverScreen->Update(deltaTime, m_mouseX, m_mouseY, m_mouseDown);
    }
    
    for (auto& element : m_elements) {
        element->Update(deltaTime);
    }
}

void UIManager::Render(Renderer& renderer) {
    // Render regular elements
    for (auto& element : m_elements) {
        if (element->IsVisible()) {
            element->Render(renderer);
        }
    }
    
    // Render Dice Panel during battle
    if (m_showBattleUI && m_dicePanel) {
        m_dicePanel->Render(renderer);
    }
    
    // Render Unit Info Panel during battle
    if (m_showBattleUI && m_unitInfoPanel) {
        m_unitInfoPanel->Render(renderer);
    }
    
    // Render HUD during battle (on top of dice panel)
    if (m_showBattleUI && m_hud) {
        m_hud->Render(renderer);
    }
    
    // Render main menu (covers everything)
    if (m_showMainMenu && m_mainMenu) {
        m_mainMenu->Render(renderer);
    }
    
    // Render game over screen (covers everything)
    if (m_showGameOver && m_gameOverScreen) {
        m_gameOverScreen->Render(renderer);
    }
    
    // Render settings panel (on top of everything)
    if (m_showSettings && m_settingsPanel) {
        m_settingsPanel->Render(renderer);
    }
    
    // Render promotion screen (on top of everything)
    if (m_showPromotion && m_promotionScreen) {
        m_promotionScreen->Render(renderer);
    }
}

void UIManager::ShowMainMenu() {
    HideAll();
    m_showMainMenu = true;
    if (m_mainMenu) {
        m_mainMenu->SetVisible(true);
    }
    Logger::Debug("Showing main menu");
}

void UIManager::ShowBattleUI() {
    HideAll();
    m_showBattleUI = true;
    
    // Show dice panel
    if (m_dicePanel) {
        m_dicePanel->SetVisible(true);
    }
    
    // Give player starting dice
    GivePlayerStartingDice();
    
    Logger::Debug("Showing battle UI");
}

void UIManager::GivePlayerStartingDice() {
    if (!m_dicePanel) return;
    
    // Create starting dice for the player
    std::vector<std::shared_ptr<Dice>> startingDice;
    
    // Create a Mage dice
    auto mage = std::make_shared<Dice>("Mage", 2);
    mage->AddFace({"Mage", {25, 12, 3, 2, 3}, {1, 1, 1, 1}, "Fireball"});
    mage->Roll();
    startingDice.push_back(mage);
    
    // Create a Soldier dice
    auto soldier = std::make_shared<Dice>("Soldier", 2);
    soldier->AddFace({"Soldier", {35, 8, 6, 3, 1}, {2, 0, 2, 0}, "Shield Bash"});
    soldier->Roll();
    startingDice.push_back(soldier);
    
    // Create a Rogue dice
    auto rogue = std::make_shared<Dice>("Rogue", 2);
    rogue->AddFace({"Rogue", {20, 15, 2, 4, 1}, {0, 2, 0, 2}, "Backstab"});
    rogue->Roll();
    startingDice.push_back(rogue);
    
    m_dicePanel->SetDice(startingDice);
    Logger::Info("Gave player {} starting dice", startingDice.size());
}

void UIManager::ShowWaveRewardUI() {
    m_showBattleUI = false;
    if (m_dicePanel) {
        m_dicePanel->SetVisible(false);
    }
    Logger::Debug("Showing wave reward UI");
}

void UIManager::ShowShopUI() {
    Logger::Debug("Showing shop UI");
}

void UIManager::ShowPromotionUI() {
    m_showPromotion = true;
    if (m_promotionScreen) {
        m_promotionScreen->Show();
    }
    Logger::Debug("Showing promotion UI");
}

/**
 * Show promotion UI for a specific unit
 */
void UIManager::ShowPromotionUIForUnit(std::shared_ptr<Unit> unit) {
    if (!unit || !unit->CanPromote()) return;
    
    m_showPromotion = true;
    if (m_promotionScreen) {
        m_promotionScreen->SetUnit(unit);
        m_promotionScreen->Show();
    }
    Logger::Debug("Showing promotion UI for {}", unit->GetClassName());
}

void UIManager::ShowGameOverUI(bool victory) {
    HideAll();
    m_showGameOver = true;
    
    if (m_gameOverScreen) {
        m_gameOverScreen->SetVictory(victory);
        m_gameOverScreen->SetVisible(true);
    }
    
    Logger::Debug("Showing game over UI (victory: {})", victory);
}

void UIManager::ShowSettingsUI() {
    m_showSettings = true;
    if (m_settingsPanel) {
        m_settingsPanel->Show();
    }
    Logger::Debug("Showing settings UI");
}

void UIManager::ShowPauseMenu() {
    Logger::Debug("Showing pause menu");
}

void UIManager::HideAll() {
    m_showMainMenu = false;
    m_showBattleUI = false;
    m_showGameOver = false;
    m_showSettings = false;
    if (m_mainMenu) {
        m_mainMenu->SetVisible(false);
    }
    if (m_dicePanel) {
        m_dicePanel->SetVisible(false);
    }
    if (m_gameOverScreen) {
        m_gameOverScreen->SetVisible(false);
    }
    if (m_settingsPanel) {
        m_settingsPanel->Hide();
    }
    HideUnitInfo();
}

void UIManager::ShowUnitInfo(std::shared_ptr<Unit> unit) {
    if (m_unitInfoPanel) {
        m_unitInfoPanel->SetUnit(unit);
    }
}

void UIManager::HideUnitInfo() {
    if (m_unitInfoPanel) {
        m_unitInfoPanel->ClearUnit();
    }
}

} // namespace DDD
