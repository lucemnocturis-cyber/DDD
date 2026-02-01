#include "SoundManager.h"
#include "../Utils/Logger.h"

#include <cmath>
#include <algorithm>
#include <cstring>

namespace DDD {

// Global instance
SoundManager* g_soundManager = nullptr;

// Constants
static constexpr int SAMPLE_RATE = 44100;
static constexpr int CHANNELS = 1;
static constexpr int BUFFER_SIZE = 2048;

SoundManager::SoundManager() {
    m_playing.resize(8);  // Max 8 simultaneous sounds
    for (auto& s : m_playing) {
        s.active = false;
    }
}

SoundManager::~SoundManager() {
    Shutdown();
}

bool SoundManager::Initialize() {
    if (m_initialized) return true;
    
    // Initialize SDL audio if not already done
    if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            Logger::Error("Failed to initialize SDL audio: {}", SDL_GetError());
            return false;
        }
    }
    
    // Set up audio spec
    SDL_AudioSpec desired;
    SDL_memset(&desired, 0, sizeof(desired));
    desired.freq = SAMPLE_RATE;
    desired.format = AUDIO_S16SYS;
    desired.channels = CHANNELS;
    desired.samples = BUFFER_SIZE;
    desired.callback = AudioCallback;
    desired.userdata = this;
    
    m_audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &m_audioSpec, 0);
    if (m_audioDevice == 0) {
        Logger::Warning("Failed to open audio device: {}. Sound disabled.", SDL_GetError());
        return false;
    }
    
    // Generate all default sounds
    CreateDefaultSounds();
    
    // Start audio playback
    SDL_PauseAudioDevice(m_audioDevice, 0);
    
    m_initialized = true;
    g_soundManager = this;
    
    Logger::Info("SoundManager initialized ({}Hz, {} channels)", m_audioSpec.freq, m_audioSpec.channels);
    return true;
}

void SoundManager::Shutdown() {
    if (m_audioDevice != 0) {
        SDL_CloseAudioDevice(m_audioDevice);
        m_audioDevice = 0;
    }
    m_sounds.clear();
    m_initialized = false;
    if (g_soundManager == this) {
        g_soundManager = nullptr;
    }
}

void SoundManager::Play(SoundID sound) {
    if (!m_initialized || m_muted) return;
    
    auto it = m_sounds.find(sound);
    if (it == m_sounds.end()) {
        Logger::Warning("Sound not found: {}", static_cast<int>(sound));
        return;
    }
    
    // Find free slot
    SDL_LockAudioDevice(m_audioDevice);
    for (auto& s : m_playing) {
        if (!s.active) {
            s.id = sound;
            s.position = 0;
            s.active = true;
            break;
        }
    }
    SDL_UnlockAudioDevice(m_audioDevice);
}

void SoundManager::PlayCustom(const SoundParams& params) {
    if (!m_initialized || m_muted) return;
    
    SDL_LockAudioDevice(m_audioDevice);
    m_customBuffer.clear();
    GenerateSoundData(params, m_customBuffer);
    m_customPosition = 0;
    m_customPlaying = true;
    SDL_UnlockAudioDevice(m_audioDevice);
}

void SoundManager::SetVolume(float volume) {
    m_masterVolume = std::clamp(volume, 0.0f, 1.0f);
}

void SoundManager::AudioCallback(void* userdata, uint8_t* stream, int len) {
    SoundManager* manager = static_cast<SoundManager*>(userdata);
    manager->MixAudio(stream, len);
}

void SoundManager::MixAudio(uint8_t* stream, int len) {
    // Clear buffer
    std::memset(stream, 0, len);
    
    int16_t* output = reinterpret_cast<int16_t*>(stream);
    int numSamples = len / sizeof(int16_t);
    
    // Mix all playing sounds
    for (auto& s : m_playing) {
        if (!s.active) continue;
        
        auto it = m_sounds.find(s.id);
        if (it == m_sounds.end()) {
            s.active = false;
            continue;
        }
        
        const auto& buffer = it->second;
        for (int i = 0; i < numSamples && s.position < buffer.size(); ++i, ++s.position) {
            int32_t mixed = output[i] + static_cast<int32_t>(buffer[s.position] * m_masterVolume);
            output[i] = static_cast<int16_t>(std::clamp(mixed, -32768, 32767));
        }
        
        if (s.position >= buffer.size()) {
            s.active = false;
        }
    }
    
    // Mix custom sound if playing
    if (m_customPlaying) {
        for (int i = 0; i < numSamples && m_customPosition < m_customBuffer.size(); ++i, ++m_customPosition) {
            int32_t mixed = output[i] + static_cast<int32_t>(m_customBuffer[m_customPosition] * m_masterVolume);
            output[i] = static_cast<int16_t>(std::clamp(mixed, -32768, 32767));
        }
        if (m_customPosition >= m_customBuffer.size()) {
            m_customPlaying = false;
        }
    }
}

void SoundManager::GenerateSoundData(const SoundParams& params, std::vector<int16_t>& buffer) {
    int numSamples = static_cast<int>(params.duration * SAMPLE_RATE);
    buffer.resize(numSamples);
    
    float phase = 0.0f;
    float freq = params.frequency;
    
    for (int i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / SAMPLE_RATE;
        
        // Apply frequency slide
        freq = params.frequency + params.frequencySlide * t;
        
        // Apply vibrato
        if (params.vibratoDepth > 0) {
            freq += params.vibratoDepth * std::sin(2.0f * 3.14159f * params.vibratoSpeed * t);
        }
        
        // Generate waveform
        float sample = 0.0f;
        phase += freq / SAMPLE_RATE;
        if (phase >= 1.0f) phase -= 1.0f;
        
        switch (params.waveform) {
            case 0: // Square
                sample = phase < 0.5f ? 1.0f : -1.0f;
                break;
            case 1: // Sine
                sample = std::sin(2.0f * 3.14159f * phase);
                break;
            case 2: // Noise
                sample = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
                break;
            case 3: // Sawtooth
                sample = 2.0f * phase - 1.0f;
                break;
        }
        
        // Apply envelope (ADSR simplified to AD)
        float envelope = 1.0f;
        if (t < params.attack) {
            envelope = t / params.attack;
        } else if (t < params.attack + params.decay) {
            envelope = 1.0f - (t - params.attack) / params.decay * 0.5f;
        } else {
            float remaining = params.duration - t;
            float releaseTime = params.duration * 0.2f;
            if (remaining < releaseTime) {
                envelope = 0.5f * remaining / releaseTime;
            } else {
                envelope = 0.5f;
            }
        }
        
        sample *= envelope * params.volume;
        buffer[i] = static_cast<int16_t>(sample * 32767);
    }
}

void SoundManager::CreateDefaultSounds() {
    // UI Sounds
    {
        SoundParams p;
        p.frequency = 800; p.duration = 0.05f; p.volume = 0.3f;
        p.waveform = 0; p.attack = 0.005f; p.decay = 0.03f;
        GenerateSoundData(p, m_sounds[SoundID::ButtonClick]);
    }
    {
        SoundParams p;
        p.frequency = 600; p.duration = 0.03f; p.volume = 0.15f;
        p.waveform = 1; p.attack = 0.005f; p.decay = 0.02f;
        GenerateSoundData(p, m_sounds[SoundID::ButtonHover]);
    }
    {
        SoundParams p;
        p.frequency = 400; p.duration = 0.15f; p.volume = 0.25f;
        p.waveform = 0; p.attack = 0.01f; p.decay = 0.1f;
        p.frequencySlide = 400;
        GenerateSoundData(p, m_sounds[SoundID::MenuOpen]);
    }
    {
        SoundParams p;
        p.frequency = 600; p.duration = 0.12f; p.volume = 0.25f;
        p.waveform = 0; p.attack = 0.01f; p.decay = 0.08f;
        p.frequencySlide = -300;
        GenerateSoundData(p, m_sounds[SoundID::MenuClose]);
    }
    
    // Combat Sounds
    {
        SoundParams p;
        p.frequency = 200; p.duration = 0.15f; p.volume = 0.4f;
        p.waveform = 2; p.attack = 0.005f; p.decay = 0.1f;
        GenerateSoundData(p, m_sounds[SoundID::Attack]);
    }
    {
        SoundParams p;
        p.frequency = 150; p.duration = 0.25f; p.volume = 0.5f;
        p.waveform = 2; p.attack = 0.005f; p.decay = 0.15f;
        p.frequencySlide = 100;
        GenerateSoundData(p, m_sounds[SoundID::AttackCrit]);
    }
    {
        SoundParams p;
        p.frequency = 300; p.duration = 0.1f; p.volume = 0.2f;
        p.waveform = 2; p.attack = 0.01f; p.decay = 0.08f;
        GenerateSoundData(p, m_sounds[SoundID::AttackMiss]);
    }
    {
        SoundParams p;
        p.frequency = 150; p.duration = 0.1f; p.volume = 0.35f;
        p.waveform = 0; p.attack = 0.005f; p.decay = 0.08f;
        p.frequencySlide = -80;
        GenerateSoundData(p, m_sounds[SoundID::Damage]);
    }
    {
        SoundParams p;
        p.frequency = 100; p.duration = 0.2f; p.volume = 0.45f;
        p.waveform = 0; p.attack = 0.005f; p.decay = 0.15f;
        p.frequencySlide = -50;
        GenerateSoundData(p, m_sounds[SoundID::DamageCrit]);
    }
    {
        SoundParams p;
        p.frequency = 200; p.duration = 0.4f; p.volume = 0.4f;
        p.waveform = 2; p.attack = 0.01f; p.decay = 0.35f;
        p.frequencySlide = -150;
        GenerateSoundData(p, m_sounds[SoundID::Death]);
    }
    {
        SoundParams p;
        p.frequency = 523; p.duration = 0.2f; p.volume = 0.3f;  // C5
        p.waveform = 1; p.attack = 0.02f; p.decay = 0.15f;
        p.frequencySlide = 200;
        GenerateSoundData(p, m_sounds[SoundID::Heal]);
    }
    
    // Dice Sounds
    {
        SoundParams p;
        p.frequency = 300; p.duration = 0.3f; p.volume = 0.25f;
        p.waveform = 2; p.attack = 0.01f; p.decay = 0.25f;
        p.vibratoDepth = 50; p.vibratoSpeed = 20;
        GenerateSoundData(p, m_sounds[SoundID::DiceRoll]);
    }
    {
        SoundParams p;
        p.frequency = 500; p.duration = 0.08f; p.volume = 0.25f;
        p.waveform = 0; p.attack = 0.005f; p.decay = 0.05f;
        GenerateSoundData(p, m_sounds[SoundID::DiceSelect]);
    }
    {
        SoundParams p;
        p.frequency = 350; p.duration = 0.15f; p.volume = 0.3f;
        p.waveform = 0; p.attack = 0.01f; p.decay = 0.1f;
        p.frequencySlide = -100;
        GenerateSoundData(p, m_sounds[SoundID::DicePlaced]);
    }
    
    // Unit Sounds
    {
        SoundParams p;
        p.frequency = 440; p.duration = 0.1f; p.volume = 0.25f;
        p.waveform = 1; p.attack = 0.01f; p.decay = 0.08f;
        GenerateSoundData(p, m_sounds[SoundID::UnitSelect]);
    }
    {
        SoundParams p;
        p.frequency = 250; p.duration = 0.12f; p.volume = 0.2f;
        p.waveform = 0; p.attack = 0.02f; p.decay = 0.08f;
        GenerateSoundData(p, m_sounds[SoundID::UnitMove]);
    }
    {
        SoundParams p;
        p.frequency = 300; p.duration = 0.25f; p.volume = 0.35f;
        p.waveform = 1; p.attack = 0.02f; p.decay = 0.2f;
        p.frequencySlide = 200;
        GenerateSoundData(p, m_sounds[SoundID::UnitSpawn]);
    }
    
    // Game Events
    {
        SoundParams p;
        p.frequency = 523; p.duration = 0.15f; p.volume = 0.3f;
        p.waveform = 1; p.attack = 0.01f; p.decay = 0.12f;
        GenerateSoundData(p, m_sounds[SoundID::TurnStart]);
    }
    {
        SoundParams p;
        p.frequency = 392; p.duration = 0.15f; p.volume = 0.25f;
        p.waveform = 1; p.attack = 0.01f; p.decay = 0.12f;
        GenerateSoundData(p, m_sounds[SoundID::TurnEnd]);
    }
    {
        SoundParams p;
        p.frequency = 440; p.duration = 0.4f; p.volume = 0.35f;
        p.waveform = 1; p.attack = 0.02f; p.decay = 0.3f;
        p.frequencySlide = 220;
        GenerateSoundData(p, m_sounds[SoundID::WaveStart]);
    }
    {
        // Victory fanfare - ascending notes
        SoundParams p;
        p.frequency = 523; p.duration = 0.5f; p.volume = 0.4f;
        p.waveform = 1; p.attack = 0.02f; p.decay = 0.4f;
        p.frequencySlide = 300;
        GenerateSoundData(p, m_sounds[SoundID::WaveComplete]);
    }
    {
        SoundParams p;
        p.frequency = 659; p.duration = 0.8f; p.volume = 0.45f;  // E5
        p.waveform = 1; p.attack = 0.05f; p.decay = 0.6f;
        p.vibratoDepth = 10; p.vibratoSpeed = 5;
        GenerateSoundData(p, m_sounds[SoundID::Victory]);
    }
    {
        SoundParams p;
        p.frequency = 200; p.duration = 0.8f; p.volume = 0.4f;
        p.waveform = 1; p.attack = 0.05f; p.decay = 0.7f;
        p.frequencySlide = -100;
        GenerateSoundData(p, m_sounds[SoundID::Defeat]);
    }
    {
        SoundParams p;
        p.frequency = 440; p.duration = 0.3f; p.volume = 0.35f;
        p.waveform = 1; p.attack = 0.02f; p.decay = 0.25f;
        p.frequencySlide = 440;
        GenerateSoundData(p, m_sounds[SoundID::LevelUp]);
    }
    {
        SoundParams p;
        p.frequency = 880; p.duration = 0.1f; p.volume = 0.25f;
        p.waveform = 1; p.attack = 0.005f; p.decay = 0.08f;
        GenerateSoundData(p, m_sounds[SoundID::GoldPickup]);
    }
    
    // Misc
    {
        SoundParams p;
        p.frequency = 200; p.duration = 0.15f; p.volume = 0.3f;
        p.waveform = 0; p.attack = 0.005f; p.decay = 0.1f;
        GenerateSoundData(p, m_sounds[SoundID::Error]);
    }
    {
        SoundParams p;
        p.frequency = 600; p.duration = 0.1f; p.volume = 0.3f;
        p.waveform = 0; p.attack = 0.005f; p.decay = 0.08f;
        GenerateSoundData(p, m_sounds[SoundID::Confirm]);
    }
    {
        SoundParams p;
        p.frequency = 400; p.duration = 0.1f; p.volume = 0.25f;
        p.waveform = 0; p.attack = 0.005f; p.decay = 0.08f;
        p.frequencySlide = -200;
        GenerateSoundData(p, m_sounds[SoundID::Cancel]);
    }
    
    Logger::Info("Created {} procedural sound effects", m_sounds.size());
}

} // namespace DDD
