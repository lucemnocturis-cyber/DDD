#include "LoadingScreen.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/TextRenderer.h"

#include <cmath>

namespace DDD {

void LoadingScreen::Render(Renderer& renderer) {
    int screenWidth = renderer.GetWidth();
    int screenHeight = renderer.GetHeight();
    
    // Background
    renderer.Clear({15, 15, 30, 255});
    
    // Title
    SDL_Color titleColor = {255, 215, 100, 255};
    renderer.GetTextRenderer()->RenderText("DUNGEON DICE DUELISTS", 
                                           screenWidth / 2 - 180, 
                                           screenHeight / 2 - 100,
                                           FontSize::XLarge, titleColor);
    
    // Loading bar background
    int barWidth = 400;
    int barHeight = 20;
    int barX = (screenWidth - barWidth) / 2;
    int barY = screenHeight / 2 + 20;
    
    SDL_Color barBg = {40, 40, 60, 255};
    renderer.FillRect(barX, barY, barWidth, barHeight, barBg);
    
    // Loading bar fill
    int fillWidth = static_cast<int>(barWidth * m_progress);
    SDL_Color barFill = {70, 130, 200, 255};
    renderer.FillRect(barX, barY, fillWidth, barHeight, barFill);
    
    // Border
    SDL_Color borderColor = {100, 100, 140, 255};
    renderer.DrawRect(barX, barY, barWidth, barHeight, borderColor);
    
    // Status text
    SDL_Color statusColor = {180, 180, 200, 255};
    renderer.GetTextRenderer()->RenderText(m_status, 
                                           screenWidth / 2 - 50, 
                                           barY + 35,
                                           FontSize::Small, statusColor);
    
    // Animated dots
    m_time += 0.016f;  // Approximate frame time
    int numDots = static_cast<int>(m_time * 3) % 4;
    std::string dots(numDots, '.');
    renderer.GetTextRenderer()->RenderText(dots, 
                                           screenWidth / 2 + 30, 
                                           barY + 35,
                                           FontSize::Small, statusColor);
}

} // namespace DDD
