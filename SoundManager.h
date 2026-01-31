#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace DDD {

/**
 * Sound effect identifiers
 */
enum class SoundID {
    // UI Sounds
    ButtonClick,
    ButtonHover,
    MenuOpen,
    MenuClose,
    
    // Combat Sounds
    Attack,
    AttackCrit,
    AttackMiss,
    Damage,
    DamageCrit,
    Death,
    Heal,
    
    // Dice Sounds
    DiceRoll,
    DiceSelect,
    DicePlaced,
    
    // Unit Sounds
    UnitSelect,
    UnitMove,
    UnitSpawn,
    
    // Game Events
    TurnStart,
    TurnEnd,
    WaveStart,
    WaveComplete,
    Victory,
    Defeat,
    LevelUp,
    GoldPickup,
    
    // Misc
    Error,
    Confirm,
    Cancel,
    
    COUNT  // Keep last for array sizing
};

/**
 * Procedural sound parameters
 */
struct SoundParams {
    float frequency = 440.0f;      // Hz
    float duration = 0.1f;         // seconds
    float volume = 0.5f;           // 0.0 to 1.0
    float attack = 0.01f;          // Attack time
    float decay = 0.1f;            // Decay time
    int waveform = 0;              // 0=square, 1=sine, 2=noise, 3=sawtooth
    float frequencySlide = 0.0f;   // Hz per second (pitch bend)
    float vibratoDepth = 0.0f;     // Frequency modulation amount
    float vibratoSpeed = 0.0f;     // Vibrato cycles per second
};

/**
 * SoundManager - handles all game sound effects
 * Uses procedural audio generation for lightweight, dependency-free sounds
 */
class SoundManager {
public:
    SoundManager();
    ~SoundManager();
    
    /**
     * Initialize audio system
     */
    bool Initialize();
    
    /**
     * Shutdown audio system
     */
    void Shutdown();
    
    /**
     * Play a named sound effect
     */
    void Play(SoundID sound);
    
    /**
     * Play a custom procedural sound
     */
    void PlayCustom(const SoundParams& params);
    
    /**
     * Set master volume (0.0 to 1.0)
     */
    void SetVolume(float volume);
    float GetVolume() const { return m_masterVolume; }
    
    /**
     * Mute/unmute all sounds
     */
    void SetMuted(bool muted) { m_muted = muted; }
    bool IsMuted() const { return m_muted; }
    
    /**
     * Check if audio is available
     */
    bool IsAvailable() const { return m_initialized; }
    
private:
    void GenerateSoundData(const SoundParams& params, std::vector<int16_t>& buffer);
    void CreateDefaultSounds();
    
    static void AudioCallback(void* userdata, uint8_t* stream, int len);
    void MixAudio(uint8_t* stream, int len);
    
    bool m_initialized = false;
    bool m_muted = false;
    float m_masterVolume = 0.8f;
    
    SDL_AudioDeviceID m_audioDevice = 0;
    SDL_AudioSpec m_audioSpec;
    
    // Pre-generated sound buffers
    std::unordered_map<SoundID, std::vector<int16_t>> m_sounds;
    
    // Currently playing sounds
    struct PlayingSound {
        SoundID id;
        size_t position;
        bool active;
    };
    std::vector<PlayingSound> m_playing;
    
    // Custom sound buffer for one-off procedural sounds
    std::vector<int16_t> m_customBuffer;
    size_t m_customPosition = 0;
    bool m_customPlaying = false;
};

// Global sound manager instance
extern SoundManager* g_soundManager;

/**
 * Convenience function to play sounds
 */
inline void PlaySound(SoundID sound) {
    if (g_soundManager) {
        g_soundManager->Play(sound);
    }
}

} // namespace DDD
