#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>

namespace DDD {

/**
 * SpriteSheet - a texture divided into uniform tiles
 */
class SpriteSheet {
public:
    SpriteSheet();
    ~SpriteSheet();
    
    /**
     * Initialize with texture and tile dimensions
     */
    void Initialize(SDL_Texture* texture, int tileWidth, int tileHeight);
    
    /**
     * Get source rect for a specific tile index
     */
    SDL_Rect GetTileRect(int index) const;
    
    /**
     * Get source rect for tile at column, row
     */
    SDL_Rect GetTileRect(int col, int row) const;
    
    /**
     * Get number of tiles in each direction
     */
    int GetColumns() const { return m_columns; }
    int GetRows() const { return m_rows; }
    int GetTileCount() const { return m_columns * m_rows; }
    
    /**
     * Get tile dimensions
     */
    int GetTileWidth() const { return m_tileWidth; }
    int GetTileHeight() const { return m_tileHeight; }
    
    /**
     * Get texture
     */
    SDL_Texture* GetTexture() const { return m_texture; }
    
private:
    SDL_Texture* m_texture = nullptr;
    int m_textureWidth = 0;
    int m_textureHeight = 0;
    int m_tileWidth = 32;
    int m_tileHeight = 32;
    int m_columns = 1;
    int m_rows = 1;
};

} // namespace DDD
