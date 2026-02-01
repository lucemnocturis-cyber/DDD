#include "ResourceManager.h"
#include "../Utils/Logger.h"

#include <SDL2/SDL_image.h>
#include <filesystem>

namespace DDD {

ResourceManager::ResourceManager() = default;

ResourceManager::~ResourceManager() {
    ClearCache();
}

bool ResourceManager::Initialize(SDL_Renderer* renderer) {
    m_renderer = renderer;
    
    // Determine assets path
    char* basePath = SDL_GetBasePath();
    if (basePath) {
        m_assetsPath = std::string(basePath) + "assets/";
        SDL_free(basePath);
    } else {
        m_assetsPath = "assets/";
    }
    
    Logger::Info("Assets path: {}", m_assetsPath);
    return true;
}

TexturePtr ResourceManager::LoadTexture(const std::string& path) {
    // Check cache first
    auto it = m_textures.find(path);
    if (it != m_textures.end()) {
        return it->second;
    }
    
    // Build full path
    std::string fullPath = m_assetsPath + path;
    
    // Load texture
    SDL_Surface* surface = IMG_Load(fullPath.c_str());
    if (!surface) {
        Logger::Error("Failed to load texture '{}': {}", fullPath, IMG_GetError());
        return nullptr;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_FreeSurface(surface);
    
    if (!texture) {
        Logger::Error("Failed to create texture from '{}': {}", fullPath, SDL_GetError());
        return nullptr;
    }
    
    // Cache and return
    TexturePtr ptr(texture, TextureDeleter);
    m_textures[path] = ptr;
    Logger::Debug("Loaded texture: {}", path);
    return ptr;
}

TexturePtr ResourceManager::LoadTextureFromMemory(const std::string& key, const void* data, size_t size) {
    // Check cache first
    auto it = m_textures.find(key);
    if (it != m_textures.end()) {
        return it->second;
    }
    
    // Load from memory
    SDL_RWops* rw = SDL_RWFromConstMem(data, static_cast<int>(size));
    if (!rw) {
        Logger::Error("Failed to create RWops for texture '{}': {}", key, SDL_GetError());
        return nullptr;
    }
    
    SDL_Surface* surface = IMG_Load_RW(rw, 1);  // 1 = auto-close RWops
    if (!surface) {
        Logger::Error("Failed to load texture from memory '{}': {}", key, IMG_GetError());
        return nullptr;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_FreeSurface(surface);
    
    if (!texture) {
        Logger::Error("Failed to create texture from memory '{}': {}", key, SDL_GetError());
        return nullptr;
    }
    
    // Cache and return
    TexturePtr ptr(texture, TextureDeleter);
    m_textures[key] = ptr;
    Logger::Debug("Loaded texture from memory: {}", key);
    return ptr;
}

FontPtr ResourceManager::LoadFont(const std::string& path, int size) {
    // Create cache key with size
    std::string key = path + "_" + std::to_string(size);
    
    // Check cache
    auto it = m_fonts.find(key);
    if (it != m_fonts.end()) {
        return it->second;
    }
    
    // Build full path
    std::string fullPath = m_assetsPath + path;
    
    // Load font
    TTF_Font* font = TTF_OpenFont(fullPath.c_str(), size);
    if (!font) {
        Logger::Error("Failed to load font '{}' size {}: {}", fullPath, size, TTF_GetError());
        return nullptr;
    }
    
    // Cache and return
    FontPtr ptr(font, FontDeleter);
    m_fonts[key] = ptr;
    Logger::Debug("Loaded font: {} size {}", path, size);
    return ptr;
}

SoundPtr ResourceManager::LoadSound(const std::string& path) {
    // Check cache
    auto it = m_sounds.find(path);
    if (it != m_sounds.end()) {
        return it->second;
    }
    
    // Build full path
    std::string fullPath = m_assetsPath + path;
    
    // Load sound
    Mix_Chunk* chunk = Mix_LoadWAV(fullPath.c_str());
    if (!chunk) {
        Logger::Error("Failed to load sound '{}': {}", fullPath, Mix_GetError());
        return nullptr;
    }
    
    // Cache and return
    SoundPtr ptr(chunk, SoundDeleter);
    m_sounds[path] = ptr;
    Logger::Debug("Loaded sound: {}", path);
    return ptr;
}

MusicPtr ResourceManager::LoadMusic(const std::string& path) {
    // Check cache
    auto it = m_music.find(path);
    if (it != m_music.end()) {
        return it->second;
    }
    
    // Build full path
    std::string fullPath = m_assetsPath + path;
    
    // Load music
    Mix_Music* music = Mix_LoadMUS(fullPath.c_str());
    if (!music) {
        Logger::Error("Failed to load music '{}': {}", fullPath, Mix_GetError());
        return nullptr;
    }
    
    // Cache and return
    MusicPtr ptr(music, MusicDeleter);
    m_music[path] = ptr;
    Logger::Debug("Loaded music: {}", path);
    return ptr;
}

bool ResourceManager::GetTextureSize(const TexturePtr& texture, int& width, int& height) {
    if (!texture) return false;
    return SDL_QueryTexture(texture.get(), nullptr, nullptr, &width, &height) == 0;
}

void ResourceManager::ClearCache() {
    m_textures.clear();
    m_fonts.clear();
    m_sounds.clear();
    m_music.clear();
    Logger::Info("Resource cache cleared");
}

void ResourceManager::TextureDeleter(SDL_Texture* texture) {
    if (texture) SDL_DestroyTexture(texture);
}

void ResourceManager::FontDeleter(TTF_Font* font) {
    if (font) TTF_CloseFont(font);
}

void ResourceManager::SoundDeleter(Mix_Chunk* chunk) {
    if (chunk) Mix_FreeChunk(chunk);
}

void ResourceManager::MusicDeleter(Mix_Music* music) {
    if (music) Mix_FreeMusic(music);
}

} // namespace DDD
