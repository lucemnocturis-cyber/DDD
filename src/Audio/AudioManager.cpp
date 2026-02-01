#include "AudioManager.h"
#include "../Utils/Logger.h"

namespace DDD {

AudioManager::~AudioManager() {
    Shutdown();
}

bool AudioManager::Initialize() {
    Logger::Info("AudioManager initialized");
    return true;
}

void AudioManager::Shutdown() {
    StopMusic();
    for (auto& [path, chunk] : m_sounds) {
        Mix_FreeChunk(chunk);
    }
    m_sounds.clear();
}

void AudioManager::PlayMusic(const std::string& path, int loops) {
    // TODO: Load and play music
    Logger::Info("Playing music: {}", path);
}

void AudioManager::StopMusic() {
    Mix_HaltMusic();
    if (m_currentMusic) {
        Mix_FreeMusic(m_currentMusic);
        m_currentMusic = nullptr;
    }
}

void AudioManager::PauseMusic() {
    Mix_PauseMusic();
}

void AudioManager::ResumeMusic() {
    Mix_ResumeMusic();
}

void AudioManager::SetMusicVolume(float volume) {
    m_musicVolume = volume;
    Mix_VolumeMusic(static_cast<int>(volume * MIX_MAX_VOLUME));
}

void AudioManager::PlaySound(const std::string& path) {
    // TODO: Load and play sound effect
    Logger::Debug("Playing sound: {}", path);
}

void AudioManager::SetSFXVolume(float volume) {
    m_sfxVolume = volume;
}

} // namespace DDD
