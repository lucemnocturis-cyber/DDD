#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

namespace DDD {

/**
 * Music intensity levels
 */
enum class MusicIntensity {
    Silent,         // No music
    Ambient,        // Calm exploration
    Low,            // Light activity
    Medium,         // Normal combat
    High,           // Intense combat
    Boss,           // Boss encounters
    Victory,        // Win state
    Defeat          // Loss state
};

/**
 * Music layer types
 */
enum class MusicLayerType {
    Base,           // Always playing foundation
    Melody,         // Main melodic content
    Percussion,     // Drums and rhythm
    Bass,           // Bass line
    Strings,        // String section
    Brass,          // Brass/horn section
    Choir,          // Vocal elements
    SFX,            // Musical sound effects
    Stinger         // One-shot dramatic hits
};

/**
 * Music layer definition
 */
struct MusicLayer {
    std::string id;
    std::string name;
    MusicLayerType type;
    
    float baseVolume = 1.0f;
    float currentVolume = 0.0f;
    float targetVolume = 0.0f;
    float fadeSpeed = 1.0f;
    
    // Intensity thresholds
    MusicIntensity minIntensity = MusicIntensity::Silent;
    MusicIntensity maxIntensity = MusicIntensity::Boss;
    
    bool isLooping = true;
    bool syncToBase = true;     // Sync playback position to base layer
};

/**
 * Music track (collection of layers)
 */
struct MusicTrack {
    std::string id;
    std::string name;
    std::string context;        // "menu", "combat", "boss", etc.
    
    float bpm = 120.0f;
    int beatsPerBar = 4;
    float trackLength = 0.0f;   // In seconds
    
    std::vector<MusicLayer> layers;
    
    // Transition points (in beats)
    std::vector<int> transitionBeats;
};

/**
 * Music transition types
 */
enum class TransitionType {
    Instant,        // Immediate switch
    Crossfade,      // Smooth blend
    FadeOutIn,      // Fade out, then fade in
    OnBeat,         // Wait for beat boundary
    OnBar,          // Wait for bar boundary
    Stinger         // Play stinger, then transition
};

/**
 * Music cue (triggered event)
 */
struct MusicCue {
    std::string id;
    std::string name;
    std::string trackId;        // Track to switch to (empty = don't change)
    MusicIntensity intensity;   // Intensity to set
    TransitionType transition;
    float transitionTime = 1.0f;
    std::string stingerId;      // Stinger to play during transition
};

/**
 * Combat intensity tracker
 */
struct CombatIntensityState {
    int enemyCount = 0;
    int allyCount = 0;
    float totalEnemyHpPercent = 1.0f;
    float totalAllyHpPercent = 1.0f;
    bool bossPresent = false;
    int turnsSinceLastCombat = 0;
    float damageDealtThisTurn = 0.0f;
    float damageTakenThisTurn = 0.0f;
};

/**
 * DynamicMusicSystem - adaptive music management
 */
class DynamicMusicSystem {
public:
    static DynamicMusicSystem& Instance();
    
    void Initialize();
    void Shutdown();
    void Update(float deltaTime);
    
    // Track management
    void RegisterTrack(const MusicTrack& track);
    void RegisterCue(const MusicCue& cue);
    const MusicTrack* GetTrack(const std::string& id) const;
    
    // Playback control
    void PlayTrack(const std::string& trackId, TransitionType transition = TransitionType::Crossfade);
    void StopMusic(float fadeTime = 1.0f);
    void PauseMusic();
    void ResumeMusic();
    
    // Intensity control
    void SetIntensity(MusicIntensity intensity);
    MusicIntensity GetIntensity() const { return m_currentIntensity; }
    void SetTargetIntensity(MusicIntensity intensity, float transitionTime = 2.0f);
    
    // Combat tracking
    void UpdateCombatState(const CombatIntensityState& state);
    void OnCombatStart();
    void OnCombatEnd();
    void OnBossEncounter();
    void OnBossDefeated();
    void OnDamageDealt(float amount);
    void OnDamageTaken(float amount);
    void OnUnitDeath(bool isEnemy);
    
    // Cue triggering
    void TriggerCue(const std::string& cueId);
    
    // Layer control
    void SetLayerVolume(const std::string& layerId, float volume);
    void FadeLayer(const std::string& layerId, float targetVolume, float duration);
    void MuteLayer(MusicLayerType type);
    void UnmuteLayer(MusicLayerType type);
    
    // Volume
    void SetMasterVolume(float volume);
    float GetMasterVolume() const { return m_masterVolume; }
    
    // Queries
    bool IsPlaying() const { return m_isPlaying; }
    const std::string& GetCurrentTrackId() const { return m_currentTrackId; }
    float GetPlaybackPosition() const { return m_playbackPosition; }
    int GetCurrentBeat() const;
    int GetCurrentBar() const;
    
private:
    DynamicMusicSystem() = default;
    
    void RegisterAllTracks();
    void RegisterAllCues();
    
    void UpdateLayerVolumes(float deltaTime);
    void UpdateIntensityTransition(float deltaTime);
    void UpdateAutomaticIntensity();
    void ProcessTransition(float deltaTime);
    
    MusicIntensity CalculateIntensityFromCombat() const;
    float GetIntensityLayerVolume(const MusicLayer& layer) const;
    
    // Registered content
    std::unordered_map<std::string, MusicTrack> m_tracks;
    std::unordered_map<std::string, MusicCue> m_cues;
    
    // Current state
    std::string m_currentTrackId;
    std::string m_pendingTrackId;
    MusicIntensity m_currentIntensity = MusicIntensity::Ambient;
    MusicIntensity m_targetIntensity = MusicIntensity::Ambient;
    float m_intensityTransitionTime = 0.0f;
    float m_intensityTransitionProgress = 1.0f;
    
    // Playback state
    bool m_isPlaying = false;
    bool m_isPaused = false;
    float m_playbackPosition = 0.0f;
    float m_masterVolume = 0.7f;
    
    // Transition state
    bool m_isTransitioning = false;
    TransitionType m_transitionType = TransitionType::Crossfade;
    float m_transitionProgress = 0.0f;
    float m_transitionDuration = 1.0f;
    
    // Combat tracking
    CombatIntensityState m_combatState;
    bool m_inCombat = false;
    bool m_automaticIntensity = true;
    
    // Layer volumes for current track
    std::unordered_map<std::string, float> m_layerVolumes;
    std::unordered_map<MusicLayerType, bool> m_mutedLayerTypes;
    
    bool m_initialized = false;
};

} // namespace DDD
