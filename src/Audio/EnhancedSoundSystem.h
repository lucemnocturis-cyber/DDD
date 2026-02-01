#pragma once

#include "../Utils/Math.h"
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <queue>

namespace DDD {

/**
 * Sound categories for mixing
 */
enum class SoundCategory {
    Master,
    Music,
    SFX,
    UI,
    Ambient,
    Voice,
    Combat
};

/**
 * Sound priority levels
 */
enum class SoundPriority {
    Low,        // Can be cut off freely
    Normal,     // Standard sounds
    High,       // Important gameplay sounds
    Critical    // Never cut off (boss roars, etc)
};

/**
 * Sound variation data
 */
struct SoundVariation {
    std::string id;
    float pitchMin = 0.9f;
    float pitchMax = 1.1f;
    float volumeMin = 0.9f;
    float volumeMax = 1.0f;
    std::vector<std::string> alternates;  // Random alternate sounds
};

/**
 * Sound definition
 */
struct SoundDef {
    std::string id;
    std::string name;
    SoundCategory category;
    SoundPriority priority;
    
    float baseVolume = 1.0f;
    float basePitch = 1.0f;
    float minDistance = 0.0f;    // For positional audio
    float maxDistance = 100.0f;
    
    bool loop = false;
    float fadeInTime = 0.0f;
    float fadeOutTime = 0.0f;
    
    SoundVariation variation;
    
    // Cooldown to prevent spam
    float cooldown = 0.0f;
    float lastPlayTime = -999.0f;
};

/**
 * Active sound instance
 */
struct SoundInstance {
    std::string soundId;
    int channel = -1;
    float volume = 1.0f;
    float pitch = 1.0f;
    float fadeProgress = 1.0f;
    bool isFadingIn = false;
    bool isFadingOut = false;
    float fadeTime = 0.0f;
    Position position;
    bool isPositional = false;
};

/**
 * Sound layer for complex sounds
 */
struct SoundLayer {
    std::string soundId;
    float volumeScale = 1.0f;
    float delay = 0.0f;
    bool followMain = true;
};

/**
 * Layered sound (multiple sounds combined)
 */
struct LayeredSound {
    std::string id;
    std::string name;
    std::vector<SoundLayer> layers;
    SoundCategory category;
    SoundPriority priority;
};

/**
 * Sound event (triggered by game events)
 */
struct SoundEvent {
    std::string eventName;
    std::vector<std::string> soundIds;  // Possible sounds to play
    float probability = 1.0f;
    int maxSimultaneous = 3;
};

/**
 * Ambient sound zone
 */
struct AmbientZone {
    std::string id;
    std::string loopSound;
    std::vector<std::string> randomSounds;
    float randomInterval = 5.0f;
    float randomChance = 0.3f;
};

/**
 * EnhancedSoundSystem - advanced audio management
 */
class EnhancedSoundSystem {
public:
    static EnhancedSoundSystem& Instance();
    
    void Initialize();
    void Shutdown();
    void Update(float deltaTime);
    
    // Volume control
    void SetCategoryVolume(SoundCategory category, float volume);
    float GetCategoryVolume(SoundCategory category) const;
    void SetMasterVolume(float volume);
    float GetMasterVolume() const { return m_masterVolume; }
    
    // Basic playback
    int PlaySound(const std::string& soundId);
    int PlaySoundAt(const std::string& soundId, const Position& position);
    int PlaySoundWithPitch(const std::string& soundId, float pitch);
    int PlaySoundVariation(const std::string& soundId);
    
    // Layered sounds
    int PlayLayeredSound(const std::string& layeredId);
    
    // Sound control
    void StopSound(int channel);
    void StopAllSounds();
    void StopCategory(SoundCategory category);
    void PauseAll();
    void ResumeAll();
    
    // Fading
    void FadeIn(int channel, float duration);
    void FadeOut(int channel, float duration);
    void CrossFade(int fromChannel, int toChannel, float duration);
    
    // Events
    void TriggerEvent(const std::string& eventName);
    void RegisterEvent(const SoundEvent& event);
    
    // Ambient
    void SetAmbientZone(const std::string& zoneId);
    void StopAmbient();
    
    // Listener position (for positional audio)
    void SetListenerPosition(const Position& pos);
    
    // Sound registration
    void RegisterSound(const SoundDef& def);
    void RegisterLayeredSound(const LayeredSound& layered);
    void RegisterAmbientZone(const AmbientZone& zone);
    
    // Queries
    bool IsSoundPlaying(const std::string& soundId) const;
    int GetActiveSoundCount() const;
    
private:
    EnhancedSoundSystem() = default;
    
    void RegisterAllSounds();
    void RegisterCombatSounds();
    void RegisterUISounds();
    void RegisterAmbientSounds();
    void RegisterAbilitySounds();
    
    float CalculatePositionalVolume(const Position& soundPos) const;
    float GetFinalVolume(const SoundDef& def, float instanceVolume) const;
    std::string SelectVariation(const SoundDef& def) const;
    int FindFreeChannel(SoundPriority priority) const;
    
    void UpdateFades(float deltaTime);
    void UpdateAmbient(float deltaTime);
    void UpdatePositionalAudio();
    
    // Registered sounds
    std::unordered_map<std::string, SoundDef> m_sounds;
    std::unordered_map<std::string, LayeredSound> m_layeredSounds;
    std::unordered_map<std::string, SoundEvent> m_events;
    std::unordered_map<std::string, AmbientZone> m_ambientZones;
    
    // Active instances
    std::vector<SoundInstance> m_activeInstances;
    
    // Volume levels
    float m_masterVolume = 1.0f;
    std::unordered_map<SoundCategory, float> m_categoryVolumes;
    
    // Listener
    Position m_listenerPos = {0, 0};
    
    // Ambient state
    std::string m_currentAmbientZone;
    float m_ambientTimer = 0.0f;
    int m_ambientLoopChannel = -1;
    
    // Timing
    float m_currentTime = 0.0f;
    
    bool m_initialized = false;
    bool m_paused = false;
    
    static constexpr int MAX_CHANNELS = 32;
};

} // namespace DDD
