#include "DynamicMusicSystem.h"
#include "../Utils/Logger.h"
#include "../Utils/Random.h"

#include <algorithm>
#include <cmath>

namespace DDD {

DynamicMusicSystem& DynamicMusicSystem::Instance() {
    static DynamicMusicSystem instance;
    return instance;
}

void DynamicMusicSystem::Initialize() {
    if (m_initialized) return;
    
    RegisterAllTracks();
    RegisterAllCues();
    
    m_initialized = true;
    Logger::Info("DynamicMusicSystem initialized with {} tracks, {} cues",
                 m_tracks.size(), m_cues.size());
}

void DynamicMusicSystem::Shutdown() {
    StopMusic(0.0f);
    m_tracks.clear();
    m_cues.clear();
    m_initialized = false;
}

void DynamicMusicSystem::Update(float deltaTime) {
    if (!m_isPlaying || m_isPaused) return;
    
    // Update playback position
    const MusicTrack* track = GetTrack(m_currentTrackId);
    if (track && track->trackLength > 0) {
        m_playbackPosition += deltaTime;
        if (m_playbackPosition >= track->trackLength) {
            m_playbackPosition = std::fmod(m_playbackPosition, track->trackLength);
        }
    }
    
    // Update automatic intensity
    if (m_automaticIntensity) {
        UpdateAutomaticIntensity();
    }
    
    // Update intensity transition
    UpdateIntensityTransition(deltaTime);
    
    // Update layer volumes
    UpdateLayerVolumes(deltaTime);
    
    // Process track transition
    if (m_isTransitioning) {
        ProcessTransition(deltaTime);
    }
}

void DynamicMusicSystem::RegisterTrack(const MusicTrack& track) {
    m_tracks[track.id] = track;
}

void DynamicMusicSystem::RegisterCue(const MusicCue& cue) {
    m_cues[cue.id] = cue;
}

const MusicTrack* DynamicMusicSystem::GetTrack(const std::string& id) const {
    auto it = m_tracks.find(id);
    return it != m_tracks.end() ? &it->second : nullptr;
}

void DynamicMusicSystem::PlayTrack(const std::string& trackId, TransitionType transition) {
    if (trackId == m_currentTrackId && m_isPlaying) return;
    
    const MusicTrack* track = GetTrack(trackId);
    if (!track) {
        Logger::Warning("Music track not found: {}", trackId);
        return;
    }
    
    if (!m_isPlaying || transition == TransitionType::Instant) {
        // Immediate switch
        m_currentTrackId = trackId;
        m_playbackPosition = 0.0f;
        m_isPlaying = true;
        m_isTransitioning = false;
        
        // Initialize layer volumes
        m_layerVolumes.clear();
        for (const auto& layer : track->layers) {
            m_layerVolumes[layer.id] = GetIntensityLayerVolume(layer);
        }
        
        Logger::Info("Playing music track: {}", track->name);
    } else {
        // Start transition
        m_pendingTrackId = trackId;
        m_isTransitioning = true;
        m_transitionType = transition;
        m_transitionProgress = 0.0f;
        m_transitionDuration = 2.0f;
        
        Logger::Info("Transitioning to music track: {}", track->name);
    }
}

void DynamicMusicSystem::StopMusic(float fadeTime) {
    if (!m_isPlaying) return;
    
    if (fadeTime <= 0.0f) {
        m_isPlaying = false;
        m_currentTrackId.clear();
    } else {
        SetTargetIntensity(MusicIntensity::Silent, fadeTime);
    }
}

void DynamicMusicSystem::PauseMusic() {
    m_isPaused = true;
}

void DynamicMusicSystem::ResumeMusic() {
    m_isPaused = false;
}

void DynamicMusicSystem::SetIntensity(MusicIntensity intensity) {
    m_currentIntensity = intensity;
    m_targetIntensity = intensity;
    m_intensityTransitionProgress = 1.0f;
}

void DynamicMusicSystem::SetTargetIntensity(MusicIntensity intensity, float transitionTime) {
    m_targetIntensity = intensity;
    m_intensityTransitionTime = transitionTime;
    m_intensityTransitionProgress = 0.0f;
}

void DynamicMusicSystem::UpdateCombatState(const CombatIntensityState& state) {
    m_combatState = state;
}

void DynamicMusicSystem::OnCombatStart() {
    m_inCombat = true;
    if (m_automaticIntensity) {
        SetTargetIntensity(MusicIntensity::Medium, 1.5f);
    }
}

void DynamicMusicSystem::OnCombatEnd() {
    m_inCombat = false;
    if (m_automaticIntensity) {
        SetTargetIntensity(MusicIntensity::Low, 3.0f);
    }
}

void DynamicMusicSystem::OnBossEncounter() {
    TriggerCue("boss_encounter");
}

void DynamicMusicSystem::OnBossDefeated() {
    TriggerCue("boss_defeated");
}

void DynamicMusicSystem::OnDamageDealt(float amount) {
    m_combatState.damageDealtThisTurn += amount;
}

void DynamicMusicSystem::OnDamageTaken(float amount) {
    m_combatState.damageTakenThisTurn += amount;
    
    // Spike intensity briefly on big damage
    if (amount > 20 && m_automaticIntensity) {
        if (m_currentIntensity < MusicIntensity::High) {
            SetTargetIntensity(MusicIntensity::High, 0.5f);
        }
    }
}

void DynamicMusicSystem::OnUnitDeath(bool isEnemy) {
    if (!isEnemy && m_automaticIntensity) {
        // Ally death increases tension
        SetTargetIntensity(MusicIntensity::High, 0.3f);
    }
}

void DynamicMusicSystem::TriggerCue(const std::string& cueId) {
    auto it = m_cues.find(cueId);
    if (it == m_cues.end()) {
        Logger::Warning("Music cue not found: {}", cueId);
        return;
    }
    
    const MusicCue& cue = it->second;
    
    // Change track if specified
    if (!cue.trackId.empty()) {
        PlayTrack(cue.trackId, cue.transition);
    }
    
    // Set intensity
    SetTargetIntensity(cue.intensity, cue.transitionTime);
    
    Logger::Info("Music cue triggered: {}", cue.name);
}

void DynamicMusicSystem::SetLayerVolume(const std::string& layerId, float volume) {
    m_layerVolumes[layerId] = std::clamp(volume, 0.0f, 1.0f);
}

void DynamicMusicSystem::FadeLayer(const std::string& layerId, float targetVolume, float duration) {
    // Would need per-layer fade tracking for full implementation
    m_layerVolumes[layerId] = targetVolume;
}

void DynamicMusicSystem::MuteLayer(MusicLayerType type) {
    m_mutedLayerTypes[type] = true;
}

void DynamicMusicSystem::UnmuteLayer(MusicLayerType type) {
    m_mutedLayerTypes[type] = false;
}

void DynamicMusicSystem::SetMasterVolume(float volume) {
    m_masterVolume = std::clamp(volume, 0.0f, 1.0f);
}

int DynamicMusicSystem::GetCurrentBeat() const {
    const MusicTrack* track = GetTrack(m_currentTrackId);
    if (!track || track->bpm <= 0) return 0;
    
    float beatsPerSecond = track->bpm / 60.0f;
    return static_cast<int>(m_playbackPosition * beatsPerSecond);
}

int DynamicMusicSystem::GetCurrentBar() const {
    const MusicTrack* track = GetTrack(m_currentTrackId);
    if (!track || track->beatsPerBar <= 0) return 0;
    
    return GetCurrentBeat() / track->beatsPerBar;
}

// ===========================================================================
// PRIVATE METHODS
// ===========================================================================

void DynamicMusicSystem::UpdateLayerVolumes(float deltaTime) {
    const MusicTrack* track = GetTrack(m_currentTrackId);
    if (!track) return;
    
    for (const auto& layer : track->layers) {
        float targetVolume = GetIntensityLayerVolume(layer);
        
        // Check if layer type is muted
        auto muteIt = m_mutedLayerTypes.find(layer.type);
        if (muteIt != m_mutedLayerTypes.end() && muteIt->second) {
            targetVolume = 0.0f;
        }
        
        // Smooth transition
        float& currentVolume = m_layerVolumes[layer.id];
        float diff = targetVolume - currentVolume;
        float fadeAmount = layer.fadeSpeed * deltaTime;
        
        if (std::abs(diff) < fadeAmount) {
            currentVolume = targetVolume;
        } else {
            currentVolume += (diff > 0 ? fadeAmount : -fadeAmount);
        }
    }
}

void DynamicMusicSystem::UpdateIntensityTransition(float deltaTime) {
    if (m_intensityTransitionProgress >= 1.0f) return;
    
    m_intensityTransitionProgress += deltaTime / m_intensityTransitionTime;
    
    if (m_intensityTransitionProgress >= 1.0f) {
        m_intensityTransitionProgress = 1.0f;
        m_currentIntensity = m_targetIntensity;
    }
}

void DynamicMusicSystem::UpdateAutomaticIntensity() {
    if (!m_inCombat) {
        // Gradually decrease intensity when not in combat
        if (m_combatState.turnsSinceLastCombat > 2) {
            if (m_targetIntensity > MusicIntensity::Ambient) {
                SetTargetIntensity(MusicIntensity::Ambient, 5.0f);
            }
        }
        return;
    }
    
    MusicIntensity calculated = CalculateIntensityFromCombat();
    
    // Only change if significantly different
    int currentLevel = static_cast<int>(m_targetIntensity);
    int calculatedLevel = static_cast<int>(calculated);
    
    if (std::abs(calculatedLevel - currentLevel) >= 1) {
        float transitionTime = (calculatedLevel > currentLevel) ? 1.0f : 3.0f;
        SetTargetIntensity(calculated, transitionTime);
    }
    
    // Reset per-turn tracking
    m_combatState.damageDealtThisTurn = 0.0f;
    m_combatState.damageTakenThisTurn = 0.0f;
}

void DynamicMusicSystem::ProcessTransition(float deltaTime) {
    m_transitionProgress += deltaTime / m_transitionDuration;
    
    if (m_transitionProgress >= 1.0f) {
        // Complete transition
        m_currentTrackId = m_pendingTrackId;
        m_pendingTrackId.clear();
        m_isTransitioning = false;
        m_transitionProgress = 0.0f;
        
        // Reset playback position for new track
        if (m_transitionType != TransitionType::Crossfade) {
            m_playbackPosition = 0.0f;
        }
        
        // Initialize layer volumes for new track
        const MusicTrack* track = GetTrack(m_currentTrackId);
        if (track) {
            m_layerVolumes.clear();
            for (const auto& layer : track->layers) {
                m_layerVolumes[layer.id] = GetIntensityLayerVolume(layer);
            }
        }
    }
}

MusicIntensity DynamicMusicSystem::CalculateIntensityFromCombat() const {
    if (m_combatState.bossPresent) {
        return MusicIntensity::Boss;
    }
    
    // Calculate threat level
    float threatLevel = 0.0f;
    
    // More enemies = more intense
    threatLevel += m_combatState.enemyCount * 0.1f;
    
    // Low ally HP = more intense
    threatLevel += (1.0f - m_combatState.totalAllyHpPercent) * 0.3f;
    
    // Recent damage taken = more intense
    threatLevel += std::min(m_combatState.damageTakenThisTurn / 50.0f, 0.3f);
    
    // Outnumbered = more intense
    if (m_combatState.allyCount > 0) {
        float ratio = static_cast<float>(m_combatState.enemyCount) / m_combatState.allyCount;
        if (ratio > 2.0f) threatLevel += 0.2f;
    }
    
    // Map threat to intensity
    if (threatLevel >= 0.8f) return MusicIntensity::High;
    if (threatLevel >= 0.5f) return MusicIntensity::Medium;
    if (threatLevel >= 0.2f) return MusicIntensity::Low;
    return MusicIntensity::Ambient;
}

float DynamicMusicSystem::GetIntensityLayerVolume(const MusicLayer& layer) const {
    int currentLevel = static_cast<int>(m_currentIntensity);
    int targetLevel = static_cast<int>(m_targetIntensity);
    
    // Interpolate during transition
    float effectiveLevel = currentLevel + (targetLevel - currentLevel) * m_intensityTransitionProgress;
    
    int minLevel = static_cast<int>(layer.minIntensity);
    int maxLevel = static_cast<int>(layer.maxIntensity);
    
    if (effectiveLevel < minLevel) return 0.0f;
    if (effectiveLevel > maxLevel) return layer.baseVolume;
    
    // Gradual fade in/out at boundaries
    if (effectiveLevel - minLevel < 1.0f) {
        return layer.baseVolume * (effectiveLevel - minLevel);
    }
    
    return layer.baseVolume;
}

// ===========================================================================
// TRACK REGISTRATION
// ===========================================================================

void DynamicMusicSystem::RegisterAllTracks() {
    // Menu Music
    RegisterTrack({
        "menu_theme", "Main Menu Theme", "menu",
        90.0f, 4, 64.0f,
        {
            {"menu_base", "Base Pad", MusicLayerType::Base, 0.8f, 0.0f, 0.0f, 1.0f,
             MusicIntensity::Silent, MusicIntensity::Boss, true, true},
            {"menu_melody", "Melody", MusicLayerType::Melody, 0.7f, 0.0f, 0.0f, 1.5f,
             MusicIntensity::Ambient, MusicIntensity::Boss, true, true},
            {"menu_strings", "Strings", MusicLayerType::Strings, 0.5f, 0.0f, 0.0f, 2.0f,
             MusicIntensity::Low, MusicIntensity::Boss, true, true}
        },
        {0, 16, 32, 48}
    });
    
    // Exploration/Preparation Music
    RegisterTrack({
        "exploration", "Dungeon Exploration", "exploration",
        100.0f, 4, 48.0f,
        {
            {"explore_base", "Ambient Base", MusicLayerType::Base, 0.6f, 0.0f, 0.0f, 1.0f,
             MusicIntensity::Silent, MusicIntensity::Boss, true, true},
            {"explore_melody", "Light Melody", MusicLayerType::Melody, 0.5f, 0.0f, 0.0f, 2.0f,
             MusicIntensity::Ambient, MusicIntensity::Medium, true, true},
            {"explore_perc", "Light Percussion", MusicLayerType::Percussion, 0.4f, 0.0f, 0.0f, 1.5f,
             MusicIntensity::Low, MusicIntensity::High, true, true}
        },
        {0, 12, 24, 36}
    });
    
    // Combat Music
    RegisterTrack({
        "combat_normal", "Combat Theme", "combat",
        140.0f, 4, 32.0f,
        {
            {"combat_base", "Combat Base", MusicLayerType::Base, 0.7f, 0.0f, 0.0f, 0.5f,
             MusicIntensity::Silent, MusicIntensity::Boss, true, true},
            {"combat_drums_light", "Light Drums", MusicLayerType::Percussion, 0.6f, 0.0f, 0.0f, 1.0f,
             MusicIntensity::Low, MusicIntensity::High, true, true},
            {"combat_drums_heavy", "Heavy Drums", MusicLayerType::Percussion, 0.8f, 0.0f, 0.0f, 0.8f,
             MusicIntensity::Medium, MusicIntensity::Boss, true, true},
            {"combat_melody", "Combat Melody", MusicLayerType::Melody, 0.7f, 0.0f, 0.0f, 1.5f,
             MusicIntensity::Medium, MusicIntensity::Boss, true, true},
            {"combat_bass", "Combat Bass", MusicLayerType::Bass, 0.6f, 0.0f, 0.0f, 1.0f,
             MusicIntensity::Low, MusicIntensity::Boss, true, true},
            {"combat_strings", "Tension Strings", MusicLayerType::Strings, 0.5f, 0.0f, 0.0f, 2.0f,
             MusicIntensity::High, MusicIntensity::Boss, true, true},
            {"combat_brass", "Brass Hits", MusicLayerType::Brass, 0.7f, 0.0f, 0.0f, 1.0f,
             MusicIntensity::High, MusicIntensity::Boss, true, true}
        },
        {0, 8, 16, 24}
    });
    
    // Intense Combat
    RegisterTrack({
        "combat_intense", "Intense Combat", "combat",
        160.0f, 4, 24.0f,
        {
            {"intense_base", "Intense Base", MusicLayerType::Base, 0.8f, 0.0f, 0.0f, 0.3f,
             MusicIntensity::Silent, MusicIntensity::Boss, true, true},
            {"intense_drums", "Driving Drums", MusicLayerType::Percussion, 0.9f, 0.0f, 0.0f, 0.5f,
             MusicIntensity::Medium, MusicIntensity::Boss, true, true},
            {"intense_melody", "Aggressive Melody", MusicLayerType::Melody, 0.8f, 0.0f, 0.0f, 1.0f,
             MusicIntensity::High, MusicIntensity::Boss, true, true},
            {"intense_choir", "Dramatic Choir", MusicLayerType::Choir, 0.6f, 0.0f, 0.0f, 2.0f,
             MusicIntensity::High, MusicIntensity::Boss, true, true}
        },
        {0, 6, 12, 18}
    });
    
    // Boss Music
    RegisterTrack({
        "boss_battle", "Boss Battle", "boss",
        150.0f, 4, 48.0f,
        {
            {"boss_base", "Epic Base", MusicLayerType::Base, 0.9f, 0.0f, 0.0f, 0.3f,
             MusicIntensity::Silent, MusicIntensity::Boss, true, true},
            {"boss_drums", "Epic Drums", MusicLayerType::Percussion, 1.0f, 0.0f, 0.0f, 0.5f,
             MusicIntensity::Low, MusicIntensity::Boss, true, true},
            {"boss_melody", "Boss Theme", MusicLayerType::Melody, 0.9f, 0.0f, 0.0f, 1.0f,
             MusicIntensity::Medium, MusicIntensity::Boss, true, true},
            {"boss_strings", "Dramatic Strings", MusicLayerType::Strings, 0.7f, 0.0f, 0.0f, 1.5f,
             MusicIntensity::Medium, MusicIntensity::Boss, true, true},
            {"boss_brass", "Powerful Brass", MusicLayerType::Brass, 0.8f, 0.0f, 0.0f, 1.0f,
             MusicIntensity::High, MusicIntensity::Boss, true, true},
            {"boss_choir", "Epic Choir", MusicLayerType::Choir, 0.8f, 0.0f, 0.0f, 2.0f,
             MusicIntensity::High, MusicIntensity::Boss, true, true}
        },
        {0, 12, 24, 36}
    });
    
    // Victory Music
    RegisterTrack({
        "victory", "Victory Fanfare", "victory",
        120.0f, 4, 16.0f,
        {
            {"victory_fanfare", "Victory Fanfare", MusicLayerType::Brass, 1.0f, 0.0f, 0.0f, 0.5f,
             MusicIntensity::Silent, MusicIntensity::Boss, true, true},
            {"victory_strings", "Triumphant Strings", MusicLayerType::Strings, 0.8f, 0.0f, 0.0f, 1.0f,
             MusicIntensity::Silent, MusicIntensity::Boss, true, true}
        },
        {0, 8}
    });
    
    // Defeat Music
    RegisterTrack({
        "defeat", "Defeat Theme", "defeat",
        80.0f, 4, 16.0f,
        {
            {"defeat_base", "Somber Base", MusicLayerType::Base, 0.7f, 0.0f, 0.0f, 1.0f,
             MusicIntensity::Silent, MusicIntensity::Boss, true, true},
            {"defeat_strings", "Sad Strings", MusicLayerType::Strings, 0.6f, 0.0f, 0.0f, 2.0f,
             MusicIntensity::Silent, MusicIntensity::Boss, true, true}
        },
        {0, 8}
    });
    
    // Shop Music
    RegisterTrack({
        "shop", "Shop Theme", "shop",
        110.0f, 4, 32.0f,
        {
            {"shop_base", "Cozy Base", MusicLayerType::Base, 0.6f, 0.0f, 0.0f, 1.0f,
             MusicIntensity::Silent, MusicIntensity::Boss, true, true},
            {"shop_melody", "Cheerful Melody", MusicLayerType::Melody, 0.7f, 0.0f, 0.0f, 1.5f,
             MusicIntensity::Ambient, MusicIntensity::Boss, true, true}
        },
        {0, 8, 16, 24}
    });
}

void DynamicMusicSystem::RegisterAllCues() {
    // Game state cues
    RegisterCue({
        "game_start", "Game Start",
        "exploration", MusicIntensity::Ambient,
        TransitionType::FadeOutIn, 2.0f, ""
    });
    
    RegisterCue({
        "combat_start", "Combat Begin",
        "combat_normal", MusicIntensity::Medium,
        TransitionType::OnBar, 1.0f, ""
    });
    
    RegisterCue({
        "combat_end", "Combat End",
        "exploration", MusicIntensity::Low,
        TransitionType::Crossfade, 3.0f, ""
    });
    
    RegisterCue({
        "boss_encounter", "Boss Encounter",
        "boss_battle", MusicIntensity::Boss,
        TransitionType::Stinger, 1.5f, "stinger_boss"
    });
    
    RegisterCue({
        "boss_defeated", "Boss Defeated",
        "victory", MusicIntensity::Victory,
        TransitionType::Instant, 0.0f, ""
    });
    
    RegisterCue({
        "wave_complete", "Wave Complete",
        "", MusicIntensity::Low,
        TransitionType::Crossfade, 2.0f, ""
    });
    
    RegisterCue({
        "shop_enter", "Enter Shop",
        "shop", MusicIntensity::Ambient,
        TransitionType::Crossfade, 1.5f, ""
    });
    
    RegisterCue({
        "shop_exit", "Exit Shop",
        "exploration", MusicIntensity::Low,
        TransitionType::Crossfade, 1.5f, ""
    });
    
    RegisterCue({
        "victory", "Victory",
        "victory", MusicIntensity::Victory,
        TransitionType::FadeOutIn, 1.0f, ""
    });
    
    RegisterCue({
        "defeat", "Defeat",
        "defeat", MusicIntensity::Defeat,
        TransitionType::FadeOutIn, 2.0f, ""
    });
    
    RegisterCue({
        "intensity_spike", "Intensity Spike",
        "", MusicIntensity::High,
        TransitionType::Instant, 0.5f, ""
    });
    
    RegisterCue({
        "danger_low_hp", "Low HP Warning",
        "", MusicIntensity::High,
        TransitionType::Crossfade, 0.5f, ""
    });
}

} // namespace DDD
