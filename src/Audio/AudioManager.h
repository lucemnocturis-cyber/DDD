#pragma once

#include <SDL2/SDL_mixer.h>
#include <string>
#include <unordered_map>
#include <memory>

namespace DDD {

class AudioManager {
public:
    AudioManager() = default;
    ~AudioManager();
    
    bool Initialize();
    void Shutdown();
    
    // Music
    void PlayMusic(const std::string& path, int loops = -1);
    void StopMusic();
    void PauseMusic();
    void ResumeMusic();
    void SetMusicVolume(float volume);
    
    // Sound effects
    void PlaySound(const std::string& path);
    void SetSFXVolume(float volume);
    
private:
    Mix_Music* m_currentMusic = nullptr;
    std::unordered_map<std::string, Mix_Chunk*> m_sounds;
    float m_musicVolume = 0.7f;
    float m_sfxVolume = 1.0f;
};

} // namespace DDD
