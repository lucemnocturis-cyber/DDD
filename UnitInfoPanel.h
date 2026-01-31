#pragma once

#include "../Graphics/TextRenderer.h"
#include <SDL2/SDL.h>
#include <memory>

namespace DDD {

class Renderer;
class Unit;

/**
 * UnitInfoPanel - displays detailed info about selected unit
 */
class UnitInfoPanel {
public:
    UnitInfoPanel();
    ~UnitInfoPanel() = default;
    
    /**
     * Update panel
     */
    void Update(float deltaTime);
    
    /**
     * Render the panel
     */
    void Render(Renderer& renderer);
    
    /**
     * Set the unit to display
     */
    void SetUnit(std::shared_ptr<Unit> unit);
    
    /**
     * Clear the displayed unit
     */
    void ClearUnit();
    
    /**
     * Check if showing a unit
     */
    bool HasUnit() const { return m_unit != nullptr; }
    
    // Positioning
    void SetPosition(int x, int y) { m_x = x; m_y = y; }
    void SetVisible(bool visible) { m_visible = visible; }
    bool IsVisible() const { return m_visible; }
    
    static constexpr int PANEL_WIDTH = 200;
    static constexpr int PANEL_HEIGHT = 200;
    
private:
    void RenderBackground(Renderer& renderer);
    void RenderUnitName(Renderer& renderer);
    void RenderHealthBar(Renderer& renderer);
    void RenderStats(Renderer& renderer);
    void RenderStatusIcons(Renderer& renderer);
    void RenderAbilityInfo(Renderer& renderer);
    
    SDL_Color GetClassColor() const;
    
    std::shared_ptr<Unit> m_unit;
    
    int m_x = 0;
    int m_y = 0;
    bool m_visible = true;
    
    // Animation
    float m_time = 0.0f;
    float m_slideIn = 0.0f;
};

} // namespace DDD
