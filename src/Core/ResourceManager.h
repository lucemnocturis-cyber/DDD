#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace DDD {

/**
 * Resource handle types
 */
using TexturePtr = std::shared_ptr<SDL_Texture>;
using FontPtr = std::shared_ptr<TTF_Font>;
using SoundPtr = std::shared_ptr<Mix_Chunk>;
using MusicPtr = std::shared_ptr<Mix_Music>;

/**
 * ResourceManager - handles loading and caching of game assets
 */
class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();
    
    /**
     * Initialize the resource manager
     */
    bool Initialize(SDL_Renderer* renderer);
    
    /**
     * Load a texture from file
     */
    TexturePtr LoadTexture(const std::string& path);
    
    /**
     * Load a texture from memory (e.g., embedded data)
     */
    TexturePtr LoadTextureFromMemory(const std::string& key, const void* data, size_t size);
    
    /**
     * Load a font at a specific size
     */
    FontPtr LoadFont(const std::string& path, int size);
    
    /**
     * Load a sound effect
     */
    SoundPtr LoadSound(const std::string& path);
    
    /**
     * Load music
     */
    MusicPtr LoadMusic(const std::string& path);
    
    /**
     * Get texture dimensions
     */
    bool GetTextureSize(const TexturePtr& texture, int& width, int& height);
    
    /**
     * Clear all cached resources
     */
    void ClearCache();
    
    /**
     * Get the assets base path
     */
    const std::string& GetAssetsPath() const { return m_assetsPath; }
    
private:
    SDL_Renderer* m_renderer = nullptr;
    std::string m_assetsPath;
    
    // Resource caches
    std::unordered_map<std::string, TexturePtr> m_textures;
    std::unordered_map<std::string, FontPtr> m_fonts;
    std::unordered_map<std::string, SoundPtr> m_sounds;
    std::unordered_map<std::string, MusicPtr> m_music;
    
    // Custom deleters for SDL resources
    static void TextureDeleter(SDL_Texture* texture);
    static void FontDeleter(TTF_Font* font);
    static void SoundDeleter(Mix_Chunk* chunk);
    static void MusicDeleter(Mix_Music* music);
};

} // namespace DDD
