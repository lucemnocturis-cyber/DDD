#pragma once

#include <string>

namespace DDD {

class Renderer;

/**
 * LoadingScreen - displays during game initialization
 */
class LoadingScreen {
public:
    LoadingScreen() = default;
    
    void SetProgress(float progress) { m_progress = progress; }
    void SetStatus(const std::string& status) { m_status = status; }
    
    void Render(Renderer& renderer);
    
private:
    float m_progress = 0.0f;
    std::string m_status = "Loading...";
    float m_time = 0.0f;
};

} // namespace DDD
