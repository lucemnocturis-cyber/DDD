#include "InputManager.h"
#include "../Utils/Logger.h"

namespace DDD {

bool InputManager::Initialize() {
    Logger::Info("InputManager initialized");
    return true;
}

void InputManager::ProcessEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_MOUSEMOTION:
            m_mousePos.x = event.motion.x;
            m_mousePos.y = event.motion.y;
            break;
            
        case SDL_MOUSEBUTTONDOWN:
            m_mouseButtonsDown.insert(event.button.button);
            m_mouseButtonsPressed.insert(event.button.button);
            break;
            
        case SDL_MOUSEBUTTONUP:
            m_mouseButtonsDown.erase(event.button.button);
            break;
            
        case SDL_KEYDOWN:
            if (!event.key.repeat) {
                m_keysDown.insert(event.key.keysym.scancode);
                m_keysPressed.insert(event.key.keysym.scancode);
            }
            break;
            
        case SDL_KEYUP:
            m_keysDown.erase(event.key.keysym.scancode);
            break;
    }
}

void InputManager::Update() {
    // Clear single-frame states
    m_mouseButtonsPressed.clear();
    m_keysPressed.clear();
}

bool InputManager::IsMouseButtonDown(int button) const {
    return m_mouseButtonsDown.count(button) > 0;
}

bool InputManager::IsMouseButtonPressed(int button) const {
    return m_mouseButtonsPressed.count(button) > 0;
}

bool InputManager::IsKeyDown(SDL_Scancode key) const {
    return m_keysDown.count(key) > 0;
}

bool InputManager::IsKeyPressed(SDL_Scancode key) const {
    return m_keysPressed.count(key) > 0;
}

} // namespace DDD
