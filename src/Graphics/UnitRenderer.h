#pragma once

#include "Sprite.h"
#include "../Gameplay/Unit.h"

#include <SDL2/SDL.h>
#include <memory>
#include <unordered_map>
#include <string>

namespace DDD {

class Renderer;
class Unit;

/**
 * Unit visual state for animations
 */
enum class UnitVisualState {
    Idle,
    Moving,
    Attacking,
    TakingDamage,
    Dying,
    Dead
};

/**
 * UnitRenderer - handles drawing units with sprites and animations
 * Falls back to colored shapes if sprites aren't loaded
 */
class UnitRenderer {
public:
    UnitRenderer();
    ~UnitRenderer();
    
    /**
     * Initialize renderer (load placeholder textures)
     */
    void Initialize(SDL_Renderer* sdlRenderer);
    
    /**
     * Render a unit at screen position
     */
    void RenderUnit(Renderer& renderer, const Unit& unit, int x, int y, int cellSize);
    
    /**
     * Update animations
     */
    void Update(float deltaTime);
    
    /**
     * Set unit's visual state
     */
    void SetUnitState(const Unit* unit, UnitVisualState state);
    
    /**
     * Get color for unit class
     */
    static SDL_Color GetClassColor(const std::string& className);
    
    /**
     * Get icon character for unit class (for fallback rendering)
     */
    static char GetClassIcon(const std::string& className);
    
private:
    void RenderUnitFallback(Renderer& renderer, const Unit& unit, int x, int y, int cellSize);
    void RenderHealthBar(Renderer& renderer, const Unit& unit, int x, int y, int cellSize);
    void RenderStatusIndicators(Renderer& renderer, const Unit& unit, int x, int y, int cellSize);
    
    SDL_Renderer* m_sdlRenderer = nullptr;
    
    // Unit state tracking for animations
    std::unordered_map<const Unit*, UnitVisualState> m_unitStates;
    std::unordered_map<const Unit*, float> m_unitAnimTimers;
    
    // Cached textures (when available)
    std::unordered_map<std::string, SDL_Texture*> m_classTextures;
    
    // Animation time
    float m_globalTime = 0.0f;
};

} // namespace DDD
