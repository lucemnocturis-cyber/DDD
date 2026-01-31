#pragma once

#include <SDL2/SDL.h>
#include "../Utils/Math.h"
#include <unordered_set>

namespace DDD {

class InputManager {
public:
    InputManager() = default;
    ~InputManager() = default;
    
    bool Initialize();
    void ProcessEvent(const SDL_Event& event);
    void Update();
    
    // Mouse state
    Position GetMousePosition() const { return m_mousePos; }
    bool IsMouseButtonDown(int button) const;
    bool IsMouseButtonPressed(int button) const;
    
    // Keyboard state
    bool IsKeyDown(SDL_Scancode key) const;
    bool IsKeyPressed(SDL_Scancode key) const;
    
private:
    Position m_mousePos;
    std::unordered_set<int> m_mouseButtonsDown;
    std::unordered_set<int> m_mouseButtonsPressed;
    std::unordered_set<SDL_Scancode> m_keysDown;
    std::unordered_set<SDL_Scancode> m_keysPressed;
};

} // namespace DDD
