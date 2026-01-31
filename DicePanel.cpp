#include "DicePanel.h"
#include "../Core/Game.h"
#include "../Graphics/Renderer.h"
#include "../Gameplay/Dice.h"
#include "../Utils/Logger.h"

#include <algorithm>
#include <cmath>

namespace DDD {

DicePanel::DicePanel(Game& game)
    : m_game(game)
{
}

void DicePanel::Initialize() {
    // Create roll button
    m_rollButton = std::make_unique<Button>("ROLL (3)", 0, 0, PANEL_WIDTH - 20, 40);
    m_rollButton->SetStyle(ButtonStyle::Secondary);
    m_rollButton->SetFontSize(FontSize::Medium);
    m_rollButton->SetCallback([this]() {
        if (m_rollsRemaining > 0) {
            RollAllDice();
            m_rollsRemaining--;
            m_rollButton->SetText("ROLL (" + std::to_string(m_rollsRemaining) + ")");
            m_rollButton->SetEnabled(m_rollsRemaining > 0);
            
            if (m_onRollClicked) {
                m_onRollClicked();
            }
        }
    });
    
    UpdateLayout();
    Logger::Info("DicePanel initialized");
}

void DicePanel::Update(float deltaTime, int mouseX, int mouseY, bool mouseDown) {
    if (!m_visible) return;
    
    m_time += deltaTime;
    
    // Update dice cards
    for (auto& card : m_diceCards) {
        card->Update(deltaTime, mouseX, mouseY, mouseDown);
    }
    
    // Update roll button
    if (m_rollButton) {
        m_rollButton->Update(deltaTime, mouseX, mouseY, mouseDown);
    }
}

void DicePanel::Render(Renderer& renderer) {
    if (!m_visible) return;
    
    RenderPanelBackground(renderer);
    RenderTitle(renderer);
    RenderGoldDisplay(renderer);
    
    // Render dice cards
    for (auto& card : m_diceCards) {
        card->Render(renderer);
    }
    
    // Render roll button
    if (m_rollButton) {
        m_rollButton->Render(renderer);
    }
    
    // "Select a dice" hint if nothing selected
    if (!m_selectedDice && !m_diceCards.empty()) {
        float pulse = (std::sin(m_time * 3.0f) + 1.0f) * 0.5f;
        uint8_t alpha = static_cast<uint8_t>(100 + pulse * 100);
        SDL_Color hintColor = {200, 200, 220, alpha};
        
        int hintY = m_y + 60;
        renderer.DrawText("Click a dice", m_x + PANEL_WIDTH / 2, hintY, 
                         hintColor, FontSize::Small, TextAlign::Center);
        renderer.DrawText("to place it", m_x + PANEL_WIDTH / 2, hintY + 14, 
                         hintColor, FontSize::Small, TextAlign::Center);
    }
}

void DicePanel::SetDice(const std::vector<std::shared_ptr<Dice>>& dice) {
    m_diceCards.clear();
    m_selectedDice = nullptr;
    
    for (const auto& d : dice) {
        AddDice(d);
    }
    
    // Reset rolls for new hand
    m_rollsRemaining = 3;
    if (m_rollButton) {
        m_rollButton->SetText("ROLL (3)");
        m_rollButton->SetEnabled(true);
    }
    
    UpdateLayout();
}

void DicePanel::AddDice(std::shared_ptr<Dice> dice) {
    if (m_diceCards.size() >= MAX_DICE) {
        Logger::Warning("Cannot add more dice - hand is full");
        return;
    }
    
    auto card = std::make_unique<DiceCard>(dice);
    card->SetCallback([this](std::shared_ptr<Dice> d) {
        OnDiceCardClicked(d);
    });
    
    m_diceCards.push_back(std::move(card));
    UpdateLayout();
}

void DicePanel::RemoveDice(std::shared_ptr<Dice> dice) {
    auto it = std::find_if(m_diceCards.begin(), m_diceCards.end(),
        [&dice](const std::unique_ptr<DiceCard>& card) {
            return card->GetDice() == dice;
        });
    
    if (it != m_diceCards.end()) {
        if (m_selectedDice == dice) {
            m_selectedDice = nullptr;
        }
        m_diceCards.erase(it);
        UpdateLayout();
    }
}

void DicePanel::ClearDice() {
    m_diceCards.clear();
    m_selectedDice = nullptr;
}

void DicePanel::ClearSelection() {
    m_selectedDice = nullptr;
    for (auto& card : m_diceCards) {
        card->SetSelected(false);
    }
}

void DicePanel::RollAllDice() {
    for (auto& card : m_diceCards) {
        if (card->GetDice()) {
            card->GetDice()->Roll();
        }
    }
    Logger::Info("Rolled all dice");
}

void DicePanel::ResetRolls() {
    m_rollsRemaining = 3;
    if (m_rollButton) {
        m_rollButton->SetText("ROLL (" + std::to_string(m_rollsRemaining) + ")");
        m_rollButton->SetEnabled(true);
    }
    Logger::Info("Dice rolls reset to 3");
}

void DicePanel::UpdateLayout() {
    int cardX = m_x + (PANEL_WIDTH - DiceCard::DEFAULT_WIDTH) / 2;
    int cardY = m_y + 90;  // Leave room for title and gold display
    int cardSpacing = DiceCard::DEFAULT_HEIGHT + 10;
    
    for (size_t i = 0; i < m_diceCards.size(); i++) {
        m_diceCards[i]->SetPosition(cardX, cardY + static_cast<int>(i) * cardSpacing);
    }
    
    // Position roll button at the bottom
    if (m_rollButton) {
        int buttonY = cardY + static_cast<int>(m_diceCards.size()) * cardSpacing + 10;
        m_rollButton->SetPosition(m_x + 10, buttonY);
    }
}

void DicePanel::OnDiceCardClicked(std::shared_ptr<Dice> dice) {
    // Toggle selection
    if (m_selectedDice == dice) {
        // Deselect
        m_selectedDice = nullptr;
        for (auto& card : m_diceCards) {
            card->SetSelected(false);
        }
    } else {
        // Select this dice
        m_selectedDice = dice;
        for (auto& card : m_diceCards) {
            card->SetSelected(card->GetDice() == dice);
        }
        
        if (m_onDiceSelected) {
            m_onDiceSelected(dice);
        }
    }
    
    Logger::Debug("Dice {} {}", 
                  dice->GetClassName(), 
                  m_selectedDice ? "selected" : "deselected");
}

void DicePanel::RenderPanelBackground(Renderer& renderer) {
    // Calculate panel height based on content
    int contentHeight = 90 + static_cast<int>(m_diceCards.size()) * (DiceCard::DEFAULT_HEIGHT + 10) + 60;
    
    // Panel background
    SDL_Color panelBg = {25, 25, 35, 240};
    renderer.FillRect(m_x, m_y, PANEL_WIDTH, contentHeight, panelBg);
    
    // Border
    SDL_Color borderColor = {80, 80, 100, 255};
    renderer.DrawRect(m_x, m_y, PANEL_WIDTH, contentHeight, borderColor);
    renderer.DrawRect(m_x + 1, m_y + 1, PANEL_WIDTH - 2, contentHeight - 2, borderColor);
    
    // Inner shadow
    SDL_Color shadowColor = {0, 0, 0, 50};
    renderer.FillRect(m_x + 2, m_y + 2, PANEL_WIDTH - 4, 4, shadowColor);
}

void DicePanel::RenderTitle(Renderer& renderer) {
    // Title bar
    SDL_Color titleBg = {40, 40, 60, 255};
    renderer.FillRect(m_x + 2, m_y + 2, PANEL_WIDTH - 4, 30, titleBg);
    
    // Title text
    renderer.DrawTextWithShadow("YOUR DICE", m_x + PANEL_WIDTH / 2, m_y + 8, 
                                 Colors::Gold, FontSize::Medium, TextAlign::Center);
    
    // Decorative line
    SDL_Color lineColor = Colors::Gold;
    lineColor.a = 150;
    renderer.FillRect(m_x + 20, m_y + 32, PANEL_WIDTH - 40, 2, lineColor);
}

void DicePanel::RenderGoldDisplay(Renderer& renderer) {
    int goldY = m_y + 42;
    
    // Gold icon (simple coin)
    SDL_Color coinColor = Colors::Gold;
    renderer.FillRect(m_x + 15, goldY + 2, 16, 16, coinColor);
    renderer.FillRect(m_x + 17, goldY + 4, 12, 12, Colors::Orange);
    
    // Gold amount
    std::string goldText = std::to_string(m_game.GetPlayerGold());
    renderer.DrawTextWithShadow(goldText, m_x + 40, goldY, Colors::Gold, FontSize::Large, TextAlign::Left);
    
    // "GOLD" label
    renderer.DrawText("GOLD", m_x + 45 + static_cast<int>(goldText.length()) * 12, goldY + 4, 
                      Colors::Gray, FontSize::Small, TextAlign::Left);
}

} // namespace DDD
