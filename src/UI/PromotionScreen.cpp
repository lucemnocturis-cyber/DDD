#include "PromotionScreen.h"
#include "Button.h"
#include "../Core/Game.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/TextRenderer.h"
#include "../Graphics/UnitRenderer.h"
#include "../Gameplay/Unit.h"
#include "../Gameplay/UnitDatabase.h"
#include "../Audio/SoundManager.h"
#include "../Utils/Logger.h"

#include <cmath>
#include <algorithm>

namespace DDD {

PromotionScreen::PromotionScreen(Game& game)
    : m_game(game)
{
}

PromotionScreen::~PromotionScreen() = default;

void PromotionScreen::Initialize() {
    CreateButtons();
    Logger::Info("PromotionScreen initialized");
}

void PromotionScreen::CreateButtons() {
    int centerX = m_panelX + m_panelWidth / 2;
    int bottomY = m_panelY + m_panelHeight - 60;
    
    // Confirm button
    m_confirmButton = std::make_unique<Button>("PROMOTE", centerX - 120, bottomY, 200, 50);
    m_confirmButton->SetCentered(true);
    m_confirmButton->SetStyle(ButtonStyle::Success);
    m_confirmButton->SetCallback([this]() {
        ConfirmPromotion();
    });
    
    // Skip button
    m_skipButton = std::make_unique<Button>("SKIP", centerX + 120, bottomY, 200, 50);
    m_skipButton->SetCentered(true);
    m_skipButton->SetStyle(ButtonStyle::Secondary);
    m_skipButton->SetCallback([this]() {
        if (m_onSkip) m_onSkip();
        Hide();
        if (m_onClose) m_onClose();
    });
}

void PromotionScreen::SetUnit(std::shared_ptr<Unit> unit) {
    m_unit = unit;
    m_selectedIndex = -1;
    LoadPromotionOptions();
}

void PromotionScreen::LoadPromotionOptions() {
    m_options.clear();
    m_optionButtons.clear();
    
    if (!m_unit) return;
    
    const auto& promotionNames = m_unit->GetPromotionOptions();
    auto& db = UnitDatabase::Instance();
    
    int optionY = m_panelY + 280;
    int optionHeight = 120;
    int optionWidth = 450;
    int spacing = 20;
    
    for (size_t i = 0; i < promotionNames.size() && i < 2; ++i) {
        const UnitClassDef* def = db.GetClassDef(promotionNames[i]);
        if (!def) continue;
        
        PromotionOption option;
        option.className = def->className;
        option.description = def->description;
        option.hp = def->hp;
        option.atk = def->atk;
        option.def = def->def;
        option.mov = def->mov;
        option.rng = def->rng;
        option.abilityName = def->abilityName;
        m_options.push_back(option);
        
        // Create option button
        int optionX = m_panelX + 50 + static_cast<int>(i) * (optionWidth + spacing);
        auto btn = std::make_unique<Button>(def->className, optionX + optionWidth / 2, optionY + 20, optionWidth - 20, 40);
        btn->SetCentered(true);
        btn->SetStyle(ButtonStyle::Primary);
        
        int index = static_cast<int>(i);
        btn->SetCallback([this, index]() {
            SelectOption(index);
        });
        
        m_optionButtons.push_back(std::move(btn));
    }
    
    // Auto-select first option if only one
    if (m_options.size() == 1) {
        SelectOption(0);
    }
}

void PromotionScreen::SelectOption(int index) {
    if (index < 0 || index >= static_cast<int>(m_options.size())) return;
    
    // Deselect all
    for (auto& opt : m_options) {
        opt.selected = false;
    }
    
    // Select new
    m_selectedIndex = index;
    m_options[index].selected = true;
    
    // Update button styles
    for (size_t i = 0; i < m_optionButtons.size(); ++i) {
        if (static_cast<int>(i) == index) {
            m_optionButtons[i]->SetStyle(ButtonStyle::Success);
        } else {
            m_optionButtons[i]->SetStyle(ButtonStyle::Primary);
        }
    }
    
    PlaySound(SoundID::ButtonClick);
}

void PromotionScreen::ConfirmPromotion() {
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_options.size())) {
        PlaySound(SoundID::Error);
        return;
    }
    
    const std::string& newClass = m_options[m_selectedIndex].className;
    
    if (m_onPromote && m_unit) {
        m_onPromote(m_unit, newClass);
    }
    
    PlaySound(SoundID::LevelUp);
    Hide();
    if (m_onClose) m_onClose();
}

void PromotionScreen::Show() {
    m_visible = true;
    m_fadeIn = 0.0f;
    m_time = 0.0f;
    Logger::Debug("Showing promotion screen");
}

void PromotionScreen::Hide() {
    m_visible = false;
    m_unit = nullptr;
    m_options.clear();
    m_optionButtons.clear();
    m_selectedIndex = -1;
}

void PromotionScreen::Update(float deltaTime, int mouseX, int mouseY, bool mouseDown) {
    if (!m_visible) return;
    
    m_time += deltaTime;
    m_previewPulse = std::sin(m_time * 3.0f) * 0.5f + 0.5f;
    
    // Fade in
    if (m_fadeIn < 1.0f) {
        m_fadeIn += deltaTime * 3.0f;
        if (m_fadeIn > 1.0f) m_fadeIn = 1.0f;
    }
    
    // Update buttons
    for (auto& btn : m_optionButtons) {
        btn->Update(deltaTime, mouseX, mouseY, mouseDown);
    }
    
    if (m_confirmButton) m_confirmButton->Update(deltaTime, mouseX, mouseY, mouseDown);
    if (m_skipButton) m_skipButton->Update(deltaTime, mouseX, mouseY, mouseDown);
}

void PromotionScreen::Render(Renderer& renderer) {
    if (!m_visible) return;
    
    RenderBackground(renderer);
    RenderUnitPreview(renderer);
    RenderPromotionOptions(renderer);
    RenderStatComparison(renderer);
    
    // Render buttons
    for (auto& btn : m_optionButtons) {
        btn->Render(renderer);
    }
    
    if (m_confirmButton) m_confirmButton->Render(renderer);
    if (m_skipButton) m_skipButton->Render(renderer);
}

void PromotionScreen::RenderBackground(Renderer& renderer) {
    int screenWidth = renderer.GetWidth();
    int screenHeight = renderer.GetHeight();
    
    // Dark overlay
    uint8_t alpha = static_cast<uint8_t>(220 * m_fadeIn);
    renderer.FillRect(0, 0, screenWidth, screenHeight, {10, 10, 20, alpha});
    
    // Main panel
    SDL_Color panelBg = {25, 25, 40, static_cast<uint8_t>(250 * m_fadeIn)};
    renderer.FillRect(m_panelX, m_panelY, m_panelWidth, m_panelHeight, panelBg);
    
    // Border
    SDL_Color borderColor = {255, 200, 50, static_cast<uint8_t>(255 * m_fadeIn)};
    renderer.DrawRect(m_panelX, m_panelY, m_panelWidth, m_panelHeight, borderColor);
    renderer.DrawRect(m_panelX + 2, m_panelY + 2, m_panelWidth - 4, m_panelHeight - 4, borderColor);
    
    // Title bar
    renderer.FillRect(m_panelX, m_panelY, m_panelWidth, 50, {40, 35, 60, static_cast<uint8_t>(255 * m_fadeIn)});
    renderer.FillRect(m_panelX, m_panelY + 48, m_panelWidth, 2, borderColor);
    
    // Title
    SDL_Color titleColor = {255, 220, 100, static_cast<uint8_t>(255 * m_fadeIn)};
    renderer.GetTextRenderer()->RenderText("UNIT PROMOTION", m_panelX + m_panelWidth / 2 - 100, 
                                           m_panelY + 12, FontSize::XLarge, titleColor);
    
    // Subtitle
    SDL_Color subtitleColor = {180, 180, 200, static_cast<uint8_t>(255 * m_fadeIn)};
    renderer.GetTextRenderer()->RenderText("Choose a new class for your unit", 
                                           m_panelX + m_panelWidth / 2 - 120, m_panelY + 60,
                                           FontSize::Medium, subtitleColor);
}

void PromotionScreen::RenderUnitPreview(Renderer& renderer) {
    if (!m_unit) return;
    
    uint8_t alpha = static_cast<uint8_t>(255 * m_fadeIn);
    
    // Current unit section
    int previewX = m_panelX + 50;
    int previewY = m_panelY + 100;
    int previewW = 200;
    int previewH = 150;
    
    // Background
    renderer.FillRect(previewX, previewY, previewW, previewH, {35, 35, 50, alpha});
    renderer.DrawRect(previewX, previewY, previewW, previewH, {80, 80, 100, alpha});
    
    // "Current" label
    SDL_Color labelColor = {150, 150, 170, alpha};
    renderer.GetTextRenderer()->RenderText("CURRENT", previewX + 60, previewY + 5, FontSize::Small, labelColor);
    
    // Unit name
    SDL_Color nameColor = {255, 255, 255, alpha};
    renderer.GetTextRenderer()->RenderText(m_unit->GetClassName(), previewX + 10, previewY + 30, 
                                           FontSize::Large, nameColor);
    
    // Unit visual (simplified)
    int unitX = previewX + previewW / 2 - 30;
    int unitY = previewY + 60;
    int unitSize = 60;
    
    // Draw unit shape
    SDL_Color unitColor = {100, 150, 220, alpha};
    float pulse = m_previewPulse * 20;
    renderer.FillCircle(unitX + unitSize / 2, unitY + unitSize / 2, 
                        static_cast<int>(unitSize / 2 - 5 + pulse), unitColor);
    
    // Tier indicator
    std::string tierStr = "Tier " + std::to_string(m_unit->GetTier());
    SDL_Color tierColor = {200, 180, 100, alpha};
    renderer.GetTextRenderer()->RenderText(tierStr, previewX + 70, previewY + previewH - 25, 
                                           FontSize::Small, tierColor);
    
    // Arrow
    int arrowX = previewX + previewW + 20;
    int arrowY = previewY + previewH / 2;
    SDL_Color arrowColor = {255, 200, 50, alpha};
    
    // Draw arrow (pointing right)
    for (int i = 0; i < 30; ++i) {
        renderer.FillRect(arrowX + i, arrowY - 3, 2, 6, arrowColor);
    }
    // Arrow head
    for (int i = 0; i < 15; ++i) {
        renderer.FillRect(arrowX + 25 + i, arrowY - i, 2, 1, arrowColor);
        renderer.FillRect(arrowX + 25 + i, arrowY + i, 2, 1, arrowColor);
    }
}

void PromotionScreen::RenderPromotionOptions(Renderer& renderer) {
    uint8_t alpha = static_cast<uint8_t>(255 * m_fadeIn);
    
    int optionY = m_panelY + 280;
    int optionHeight = 120;
    int optionWidth = 450;
    int spacing = 20;
    
    for (size_t i = 0; i < m_options.size(); ++i) {
        const auto& opt = m_options[i];
        int optionX = m_panelX + 50 + static_cast<int>(i) * (optionWidth + spacing);
        
        // Option background
        SDL_Color bgColor = opt.selected ? 
            SDL_Color{50, 60, 80, alpha} : SDL_Color{35, 35, 50, alpha};
        renderer.FillRect(optionX, optionY, optionWidth, optionHeight, bgColor);
        
        // Border
        SDL_Color borderColor = opt.selected ? 
            SDL_Color{100, 200, 100, alpha} : SDL_Color{70, 70, 90, alpha};
        renderer.DrawRect(optionX, optionY, optionWidth, optionHeight, borderColor);
        
        if (opt.selected) {
            renderer.DrawRect(optionX + 1, optionY + 1, optionWidth - 2, optionHeight - 2, borderColor);
        }
        
        // Description
        SDL_Color descColor = {180, 180, 200, alpha};
        renderer.GetTextRenderer()->RenderText(opt.description, optionX + 10, optionY + 70, 
                                               FontSize::Small, descColor);
        
        // Ability
        if (!opt.abilityName.empty()) {
            SDL_Color abilityColor = {200, 180, 100, alpha};
            std::string abilityText = "Ability: " + opt.abilityName;
            renderer.GetTextRenderer()->RenderText(abilityText, optionX + 10, optionY + 95, 
                                                   FontSize::Small, abilityColor);
        }
    }
}

void PromotionScreen::RenderStatComparison(Renderer& renderer) {
    if (!m_unit || m_selectedIndex < 0) return;
    
    uint8_t alpha = static_cast<uint8_t>(255 * m_fadeIn);
    const auto& newOpt = m_options[m_selectedIndex];
    const auto& oldStats = m_unit->GetStats();
    
    // Stat comparison section
    int statX = m_panelX + 50;
    int statY = m_panelY + 420;
    int statW = m_panelWidth - 100;
    int statH = 180;
    
    // Background
    renderer.FillRect(statX, statY, statW, statH, {35, 35, 50, alpha});
    renderer.DrawRect(statX, statY, statW, statH, {70, 70, 90, alpha});
    
    // Title
    SDL_Color titleColor = {255, 220, 100, alpha};
    renderer.GetTextRenderer()->RenderText("STAT CHANGES", statX + statW / 2 - 60, statY + 10, 
                                           FontSize::Medium, titleColor);
    
    // Stats
    struct StatInfo {
        std::string name;
        int oldVal;
        int newVal;
    };
    
    std::vector<StatInfo> stats = {
        {"HP", oldStats.maxHp, newOpt.hp},
        {"ATK", oldStats.atk, newOpt.atk},
        {"DEF", oldStats.def, newOpt.def},
        {"MOV", oldStats.mov, newOpt.mov},
        {"RNG", oldStats.rng, newOpt.rng}
    };
    
    int colWidth = (statW - 40) / 5;
    int rowY = statY + 50;
    
    for (size_t i = 0; i < stats.size(); ++i) {
        int colX = statX + 20 + static_cast<int>(i) * colWidth;
        const auto& stat = stats[i];
        
        // Stat name
        SDL_Color nameColor = {150, 150, 170, alpha};
        renderer.GetTextRenderer()->RenderText(stat.name, colX + colWidth / 2 - 15, rowY, 
                                               FontSize::Medium, nameColor);
        
        // Old value
        SDL_Color oldColor = {180, 180, 180, alpha};
        renderer.GetTextRenderer()->RenderText(std::to_string(stat.oldVal), colX + colWidth / 2 - 10, 
                                               rowY + 30, FontSize::Large, oldColor);
        
        // Arrow
        renderer.GetTextRenderer()->RenderText("->", colX + colWidth / 2 - 10, rowY + 60, 
                                               FontSize::Small, {150, 150, 150, alpha});
        
        // New value with color based on change
        int diff = stat.newVal - stat.oldVal;
        SDL_Color newColor;
        if (diff > 0) {
            newColor = {100, 255, 100, alpha};  // Green for increase
        } else if (diff < 0) {
            newColor = {255, 100, 100, alpha};  // Red for decrease
        } else {
            newColor = {200, 200, 200, alpha};  // White for same
        }
        
        renderer.GetTextRenderer()->RenderText(std::to_string(stat.newVal), colX + colWidth / 2 - 10, 
                                               rowY + 85, FontSize::Large, newColor);
        
        // Change indicator
        if (diff != 0) {
            std::string changeStr = (diff > 0 ? "+" : "") + std::to_string(diff);
            renderer.GetTextRenderer()->RenderText(changeStr, colX + colWidth / 2 - 15, rowY + 115, 
                                                   FontSize::Small, newColor);
        }
    }
}

} // namespace DDD
