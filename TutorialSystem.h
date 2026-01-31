#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace DDD {

class Game;
class Renderer;

/**
 * Tutorial hint types
 */
enum class HintType {
    // First game hints
    Welcome,
    DiceExplanation,
    PlaceDice,
    SelectUnit,
    MoveUnit,
    AttackEnemy,
    EndTurn,
    TerritoryControl,
    WaveComplete,
    
    // Contextual hints
    LowHealth,
    EnemyApproaching,
    CanPromote,
    OutOfDice,
    GoldLow,
    
    COUNT
};

/**
 * A single tutorial hint
 */
struct TutorialHint {
    HintType type;
    std::string title;
    std::string message;
    std::string subtext;
    float displayTime = 5.0f;
    bool dismissOnClick = true;
    bool hasArrow = false;
    int arrowX = 0;
    int arrowY = 0;
};

/**
 * TutorialSystem - manages tutorial hints and player guidance
 */
class TutorialSystem {
public:
    TutorialSystem(Game& game);
    ~TutorialSystem() = default;
    
    void Initialize();
    void Update(float deltaTime);
    void Render(Renderer& renderer);
    
    /**
     * Show a specific hint
     */
    void ShowHint(HintType type);
    
    /**
     * Show a custom hint
     */
    void ShowCustomHint(const std::string& title, const std::string& message, 
                        float duration = 5.0f);
    
    /**
     * Show hint with arrow pointing to location
     */
    void ShowHintWithArrow(HintType type, int targetX, int targetY);
    
    /**
     * Dismiss current hint
     */
    void DismissHint();
    
    /**
     * Check if hint is currently showing
     */
    bool IsShowingHint() const { return m_showingHint; }
    
    /**
     * Handle click (may dismiss hint)
     */
    bool OnClick(int x, int y);
    
    /**
     * Enable/disable tutorial
     */
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }
    
    /**
     * Check if a hint has been shown before
     */
    bool HasShownHint(HintType type) const;
    
    /**
     * Mark hint as shown
     */
    void MarkHintShown(HintType type);
    
    /**
     * Reset all hints (for new game)
     */
    void Reset();
    
    /**
     * Trigger contextual hints based on game state
     */
    void CheckContextualHints();
    
private:
    void CreateHints();
    TutorialHint* GetHint(HintType type);
    
    Game& m_game;
    bool m_enabled = true;
    bool m_firstGame = true;
    
    // Hint data
    std::vector<TutorialHint> m_hints;
    std::vector<bool> m_hintsShown;
    
    // Current display state
    bool m_showingHint = false;
    TutorialHint m_currentHint;
    float m_displayTimer = 0.0f;
    float m_fadeIn = 0.0f;
    float m_fadeOut = 0.0f;
    bool m_dismissing = false;
    
    // Animation
    float m_pulseTimer = 0.0f;
    float m_arrowBob = 0.0f;
    
    // Panel position
    int m_panelX = 0;
    int m_panelY = 0;
    int m_panelWidth = 400;
    int m_panelHeight = 150;
};

} // namespace DDD
