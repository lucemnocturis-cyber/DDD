#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <cstdint>
#include <functional>

namespace DDD {

/**
 * Music track identifiers
 */
enum class MusicTrack {
    None,
    Menu,       // Calm, ambient
    Battle,     // Energetic, rhythmic
    BossBattle, // Intense, dramatic
    Victory,    // Triumphant jingle
    Defeat,     // Somber, short
    Shop        // Relaxed, pleasant
};

/**
 * Musical scale/mode for procedural generation
 */
enum class MusicalMode {
    Major,      // Happy, bright
    Minor,      // Sad, dark
    Dorian,     // Medieval, mysterious
    Pentatonic  // Simple, Eastern
};

/**
 * MusicManager - generates and plays procedural background music
 * Uses simple synthesis to create looping background tracks
 */
class MusicManager {
public:
    MusicManager();
    ~MusicManager();
    
    /**
     * Initialize music system
     */
    bool Initialize();
    
    /**
     * Shutdown music system
     */
    void Shutdown();
    
    /**
     * Play a music track
     * @param track Track to play
     * @param fadeIn Fade in duration in seconds (0 for immediate)
     */
    void Play(MusicTrack track, float fadeIn = 0.5f);
    
    /**
     * Stop current music
     * @param fadeOut Fade out duration in seconds
     */
    void Stop(float fadeOut = 0.5f);
    
    /**
     * Pause/resume music
     */
    void Pause();
    void Resume();
    
    /**
     * Set music volume (0.0 to 1.0)
     */
    void SetVolume(float volume);
    float GetVolume() const { return m_targetVolume; }
    
    /**
     * Mute/unmute
     */
    void SetMuted(bool muted);
    bool IsMuted() const { return m_muted; }
    
    /**
     * Update (for fading)
     */
    void Update(float deltaTime);
    
    /**
     * Check current state
     */
    bool IsPlaying() const { return m_playing; }
    MusicTrack GetCurrentTrack() const { return m_currentTrack; }
    
private:
    // Audio generation
    void GenerateTrack(MusicTrack track);
    void GenerateMenuMusic();
    void GenerateBattleMusic();
    void GenerateBossMusic();
    void GenerateVictoryMusic();
    void GenerateDefeatMusic();
    void GenerateShopMusic();
    
    // Synthesis helpers
    void AddNote(float frequency, float startTime, float duration, 
                 float volume = 0.3f, int waveform = 1);
    void AddChord(const std::vector<float>& frequencies, float startTime, 
                  float duration, float volume = 0.2f);
    void AddDrum(float startTime, int type = 0, float volume = 0.4f);
    void AddBass(float frequency, float startTime, float duration, float volume = 0.35f);
    
    float NoteToFreq(int note, int octave = 4);
    std::vector<int> GetScale(MusicalMode mode);
    
    static void AudioCallback(void* userdata, uint8_t* stream, int len);
    void MixAudio(uint8_t* stream, int len);
    
    // Audio state
    bool m_initialized = false;
    bool m_playing = false;
    bool m_paused = false;
    bool m_muted = false;
    
    SDL_AudioDeviceID m_audioDevice = 0;
    SDL_AudioSpec m_audioSpec;
    
    // Current track
    MusicTrack m_currentTrack = MusicTrack::None;
    MusicTrack m_pendingTrack = MusicTrack::None;
    std::vector<int16_t> m_trackData;
    size_t m_playPosition = 0;
    
    // Volume/fading
    float m_currentVolume = 0.0f;
    float m_targetVolume = 0.5f;
    float m_fadeSpeed = 0.0f;
    
    // Track buffer for generation
    std::vector<float> m_mixBuffer;
    int m_sampleRate = 44100;
    float m_trackLength = 0.0f;  // In seconds
};

// Global music manager
extern MusicManager* g_musicManager;

} // namespace DDD
