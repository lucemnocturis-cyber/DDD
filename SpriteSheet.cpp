#include "SpriteSheet.h"
#include "../Utils/Logger.h"

namespace DDD {

SpriteSheet::SpriteSheet() = default;
SpriteSheet::~SpriteSheet() = default;

void SpriteSheet::Initialize(SDL_Texture* texture, int tileWidth, int tileHeight) {
    m_texture = texture;
    m_tileWidth = tileWidth;
    m_tileHeight = tileHeight;
    
    if (texture) {
        SDL_QueryTexture(texture, nullptr, nullptr, &m_textureWidth, &m_textureHeight);
        m_columns = m_textureWidth / tileWidth;
        m_rows = m_textureHeight / tileHeight;
        
        Logger::Debug("SpriteSheet initialized: {}x{} tiles ({}x{} px each)",
                     m_columns, m_rows, tileWidth, tileHeight);
    }
}

SDL_Rect SpriteSheet::GetTileRect(int index) const {
    int col = index % m_columns;
    int row = index / m_columns;
    return GetTileRect(col, row);
}

SDL_Rect SpriteSheet::GetTileRect(int col, int row) const {
    SDL_Rect rect;
    rect.x = col * m_tileWidth;
    rect.y = row * m_tileHeight;
    rect.w = m_tileWidth;
    rect.h = m_tileHeight;
    return rect;
}

} // namespace DDD
