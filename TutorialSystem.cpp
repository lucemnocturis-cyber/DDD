#include "TutorialSystem.h"
#include "../Core/Game.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/TextRenderer.h"
#include "../Gameplay/Board.h"
#include "../Gameplay/TurnManager.h"
#include "../Gameplay/WaveManager.h"
#include "../UI/UIManager.h"
#include "../UI/DicePanel.h"
#include "../Utils/Logger.h"

#include <cmath>
#include <algorithm>

namespace DDD {

TutorialSystem::TutorialSystem(Game& game)
    : m_game(game)
{
}

void TutorialSystem::Initialize() {
    CreateHints();
    m_hintsShown.resize(static_cast<size_t>(HintType::COUNT), false);
    Logger::Info("TutorialSystem initialized with {} hints", m_hints.size());
}

void TutorialSystem::CreateHints() {
    m_hints.clear();
    
    // Welcome hint
    m_hints.push_back({
        HintType::Welcome,
        "Welcome, Commander!",
        "Lead your forces to victory by summoning units and capturing territory.",
        "Click anywhere to continue...",
        8.0f, true, false
    });
    
    // Dice explanation
    m_hints.push_back({
        HintType::DiceExplanation,
        "Dice Summoning",
        "Your dice panel shows available units. Each die summons a different unit type.",
        "Dice have cooldowns after use.",
        6.0f, true, false
    });
    
    // Place dice hint
    m_hints.push_back({
        HintType::PlaceDice,
        "Place Your Units",
        "Click a die, then click a blue territory cell to summon that unit.",
        "Units can only be placed on your controlled territory.",
        7.0f, true, true
    });
    
    // Select unit hint
    m_hints.push_back({
        HintType::SelectUnit,
        "Select a Unit",
        "Click on one of your units to select it. You'll see where it can move.",
        "Blue cells show valid movement destinations.",
        6.0f, true, false
    });
    
    // Move unit hint
    m_hints.push_back({
        HintType::MoveUnit,
        "Move Your Unit",
        "Click a highlighted cell to move your selected unit there.",
        "Units can move once per turn.",
        5.0f, true, false
    });
    
    // Attack hint
    m_hints.push_back({
        HintType::AttackEnemy,
        "Attack!",
        "Move adjacent to an enemy to attack. Red highlights show attack targets.",
        "Deal damage based on ATK minus enemy DEF.",
        6.0f, true, false
    });
    
    // End turn hint
    m_hints.push_back({
        HintType::EndTurn,
        "End Your Turn",
        "When you're done, click 'End Turn' or press SPACE.",
        "The enemy will then take their turn.",
        5.0f, true, false
    });
    
    // Territory control
    m_hints.push_back({
        HintType::TerritoryControl,
        "Control Territory",
        "Your units expand your territory. More territory = more summoning spots!",
        "Capture the enemy's territory to weaken them.",
        6.0f, true, false
    });
    
    // Wave complete
    m_hints.push_back({
        HintType::WaveComplete,
        "Wave Complete!",
        "Excellent! Defeat all enemies to complete each wave.",
        "Waves get harder - prepare wisely!",
        5.0f, true, false
    });
    
    // Low health warning
    m_hints.push_back({
        HintType::LowHealth,
        "Unit in Danger!",
        "One of your units has low health. Consider retreating or healing.",
        "",
        4.0f, true, false
    });
    
    // Enemy approaching
    m_hints.push_back({
        HintType::EnemyApproaching,
        "Enemies Advancing!",
        "Enemy units are approaching your territory. Defend your position!",
        "",
        4.0f, true, false
    });
    
    // Can promote
    m_hints.push_back({
        HintType::CanPromote,
        "Promotion Available!",
        "A unit has gained enough experience to promote to a stronger class!",
        "Check the promotion screen after this wave.",
        5.0f, true, false
    });
    
    // Out of dice
    m_hints.push_back({
        HintType::OutOfDice,
        "Dice on Cooldown",
        "All your dice are on cooldown. Focus on moving existing units.",
        "Dice recharge over time.",
        4.0f, true, false
    });
    
    // Low gold
    m_hints.push_back({
        HintType::GoldLow,
        "Low on Gold",
        "You're running low on gold. Defeat enemies to earn more!",
        "",
        4.0f, true, false
    });
}

TutorialHint* TutorialSystem::GetHint(HintType type) {
    for (auto& hint : m_hints) {
        if (hint.type == type) {
            return &hint;
        }
    }
    return nullptr;
}

void TutorialSystem::Update(float deltaTime) {
    if (!m_enabled || !m_showingHint) return;
    
    // Update pulse animation
    m_pulseTimer += deltaTime * 3.0f;
    m_arrowBob = std::sin(m_pulseTimer) * 8.0f;
    
    // Handle fade in
    if (m_fadeIn < 1.0f && !m_dismissing) {
        m_fadeIn += deltaTime * 4.0f;
        if (m_fadeIn > 1.0f) m_fadeIn = 1.0f;
    }
    
    // Handle dismissing
    if (m_dismissing) {
        m_fadeOut += deltaTime * 5.0f;
        if (m_fadeOut >= 1.0f) {
            m_showingHint = false;
            m_dismissing = false;
        }
        return;
    }
    
    // Update display timer
    m_displayTimer -= deltaTime;
    if (m_displayTimer <= 0 && !m_currentHint.dismissOnClick) {
        DismissHint();
    }
}

void TutorialSystem::Render(Renderer& renderer) {
    if (!m_enabled || !m_showingHint) return;
    
    // Calculate alpha
    float alpha = m_fadeIn;
    if (m_dismissing) {
        alpha = 1.0f - m_fadeOut;
    }
    uint8_t a = static_cast<uint8_t>(alpha * 255);
    
    // Center panel
    m_panelX = (renderer.GetWidth() - m_panelWidth) / 2;
    m_panelY = renderer.GetHeight() - m_panelHeight - 100;
    
    // Draw arrow if needed
    if (m_currentHint.hasArrow) {
        int arrowX = m_currentHint.arrowX;
        int arrowY = m_currentHint.arrowY + static_cast<int>(m_arrowBob);
        
        // Draw pulsing circle at target
        float pulse = (std::sin(m_pulseTimer * 2.0f) + 1.0f) / 2.0f;
        uint8_t pulseAlpha = static_cast<uint8_t>(pulse * 150 * alpha);
        
        renderer.DrawCircle(m_currentHint.arrowX, m_currentHint.arrowY, 
                           20 + static_cast<int>(pulse * 10), {255, 220, 100, pulseAlpha});
        renderer.DrawCircle(m_currentHint.arrowX, m_currentHint.arrowY, 
                           15 + static_cast<int>(pulse * 5), {255, 220, 100, pulseAlpha});
        
        // Draw arrow pointing down
        SDL_Color arrowColor = {255, 220, 100, a};
        int ax = arrowX;
        int ay = arrowY - 40;
        
        // Arrow body
        renderer.FillRect(ax - 3, ay - 30, 6, 25, arrowColor);
        
        // Arrow head (triangle approximation with rectangles)
        for (int i = 0; i < 10; ++i) {
            int w = 10 - i;
            renderer.FillRect(ax - w, ay - 5 + i, w * 2, 1, arrowColor);
        }
    }
    
    // Panel background with gradient effect
    SDL_Color bgColor = {20, 25, 40, static_cast<uint8_t>(a * 0.95f)};
    SDL_Color borderColor = {100, 120, 180, a};
    SDL_Color accentColor = {70, 100, 160, a};
    
    // Outer glow
    renderer.FillRect(m_panelX - 2, m_panelY - 2, m_panelWidth + 4, m_panelHeight + 4, accentColor);
    
    // Main panel
    renderer.FillRect(m_panelX, m_panelY, m_panelWidth, m_panelHeight, bgColor);
    renderer.DrawRect(m_panelX, m_panelY, m_panelWidth, m_panelHeight, borderColor);
    
    // Inner border
    renderer.DrawRect(m_panelX + 3, m_panelY + 3, m_panelWidth - 6, m_panelHeight - 6, 
                     {50, 60, 90, a});
    
    // Title bar
    renderer.FillRect(m_panelX, m_panelY, m_panelWidth, 35, {40, 50, 80, a});
    renderer.FillRect(m_panelX, m_panelY + 33, m_panelWidth, 2, accentColor);
    
    // Icon (lightbulb shape)
    int iconX = m_panelX + 15;
    int iconY = m_panelY + 10;
    SDL_Color iconColor = {255, 220, 100, a};
    renderer.FillRect(iconX + 4, iconY, 8, 12, iconColor);
    renderer.FillRect(iconX + 2, iconY + 2, 12, 8, iconColor);
    renderer.FillRect(iconX + 5, iconY + 12, 6, 3, iconColor);
    
    // Title
    SDL_Color titleColor = {255, 230, 150, a};
    renderer.GetTextRenderer()->RenderText(m_currentHint.title, 
                                           m_panelX + 35, m_panelY + 8, 
                                           FontSize::Large, titleColor);
    
    // Message
    SDL_Color textColor = {220, 220, 230, a};
    renderer.GetTextRenderer()->RenderText(m_currentHint.message, 
                                           m_panelX + 15, m_panelY + 50, 
                                           FontSize::Medium, textColor);
    
    // Subtext
    if (!m_currentHint.subtext.empty()) {
        SDL_Color subtextColor = {150, 150, 170, a};
        renderer.GetTextRenderer()->RenderText(m_currentHint.subtext, 
                                               m_panelX + 15, m_panelY + 80, 
                                               FontSize::Small, subtextColor);
    }
    
    // Dismiss hint (bottom)
    if (m_currentHint.dismissOnClick) {
        SDL_Color dismissColor = {120, 120, 140, a};
        std::string dismissText = "Click to continue...";
        renderer.GetTextRenderer()->RenderText(dismissText, 
                                               m_panelX + m_panelWidth / 2 - 60, 
                                               m_panelY + m_panelHeight - 25, 
                                               FontSize::Small, dismissColor);
    } else {
        // Show timer
        int remaining = static_cast<int>(m_displayTimer) + 1;
        std::string timerText = "(" + std::to_string(remaining) + "s)";
        SDL_Color timerColor = {100, 100, 120, a};
        renderer.GetTextRenderer()->RenderText(timerText, 
                                               m_panelX + m_panelWidth - 40, 
                                               m_panelY + m_panelHeight - 25, 
                                               FontSize::Small, timerColor);
    }
}

void TutorialSystem::ShowHint(HintType type) {
    if (!m_enabled) return;
    
    // Don't show same hint twice (except for contextual ones)
    if (HasShownHint(type) && static_cast<int>(type) < static_cast<int>(HintType::LowHealth)) {
        return;
    }
    
    TutorialHint* hint = GetHint(type);
    if (!hint) {
        Logger::Warning("Tutorial hint not found: {}", static_cast<int>(type));
        return;
    }
    
    m_currentHint = *hint;
    m_showingHint = true;
    m_displayTimer = m_currentHint.displayTime;
    m_fadeIn = 0.0f;
    m_fadeOut = 0.0f;
    m_dismissing = false;
    m_pulseTimer = 0.0f;
    
    MarkHintShown(type);
    Logger::Debug("Showing tutorial hint: {}", m_currentHint.title);
}

void TutorialSystem::ShowCustomHint(const std::string& title, const std::string& message, 
                                     float duration) {
    if (!m_enabled) return;
    
    m_currentHint = TutorialHint{
        HintType::Welcome,  // Type doesn't matter for custom
        title,
        message,
        "",
        duration,
        duration <= 0,  // Dismiss on click if no duration
        false
    };
    
    m_showingHint = true;
    m_displayTimer = duration > 0 ? duration : 10.0f;
    m_fadeIn = 0.0f;
    m_fadeOut = 0.0f;
    m_dismissing = false;
    m_pulseTimer = 0.0f;
}

void TutorialSystem::ShowHintWithArrow(HintType type, int targetX, int targetY) {
    ShowHint(type);
    if (m_showingHint) {
        m_currentHint.hasArrow = true;
        m_currentHint.arrowX = targetX;
        m_currentHint.arrowY = targetY;
    }
}

void TutorialSystem::DismissHint() {
    if (m_showingHint && !m_dismissing) {
        m_dismissing = true;
        m_fadeOut = 0.0f;
    }
}

bool TutorialSystem::OnClick(int x, int y) {
    if (!m_showingHint || !m_currentHint.dismissOnClick) {
        return false;
    }
    
    // Check if clicked on panel or anywhere
    DismissHint();
    return true;  // Consumed click
}

bool TutorialSystem::HasShownHint(HintType type) const {
    size_t idx = static_cast<size_t>(type);
    if (idx < m_hintsShown.size()) {
        return m_hintsShown[idx];
    }
    return false;
}

void TutorialSystem::MarkHintShown(HintType type) {
    size_t idx = static_cast<size_t>(type);
    if (idx < m_hintsShown.size()) {
        m_hintsShown[idx] = true;
    }
}

void TutorialSystem::Reset() {
    std::fill(m_hintsShown.begin(), m_hintsShown.end(), false);
    m_showingHint = false;
    m_firstGame = true;
    Logger::Debug("Tutorial hints reset");
}

void TutorialSystem::CheckContextualHints() {
    if (!m_enabled || m_showingHint) return;
    
    // Only check during battle
    if (m_game.GetCurrentState() != GameStateType::Battle) return;
    
    Board* board = m_game.GetBoard();
    if (!board) return;
    
    // Check for low health units
    auto& playerUnits = board->GetUnits(Owner::Player);
    for (const auto& unit : playerUnits) {
        if (unit->GetHP() < unit->GetMaxHP() * 0.25f) {
            if (!HasShownHint(HintType::LowHealth)) {
                ShowHint(HintType::LowHealth);
                return;
            }
        }
        
        // Check for promotion
        if (unit->CanPromote()) {
            if (!HasShownHint(HintType::CanPromote)) {
                ShowHint(HintType::CanPromote);
                return;
            }
        }
    }
    
    // Check if out of dice
    UIManager* ui = m_game.GetUIManager();
    if (ui && ui->GetDicePanel()) {
        // Could add a method to check available dice
    }
    
    // Check gold
    if (m_game.GetPlayerGold() < 10) {
        if (!HasShownHint(HintType::GoldLow)) {
            ShowHint(HintType::GoldLow);
            return;
        }
    }
}

} // namespace DDD
