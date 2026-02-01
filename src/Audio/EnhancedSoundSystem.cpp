#include "EnhancedSoundSystem.h"
#include "../Utils/Logger.h"
#include "../Utils/Random.h"

#include <algorithm>
#include <cmath>

namespace DDD {

EnhancedSoundSystem& EnhancedSoundSystem::Instance() {
    static EnhancedSoundSystem instance;
    return instance;
}

void EnhancedSoundSystem::Initialize() {
    if (m_initialized) return;
    
    // Initialize category volumes
    m_categoryVolumes[SoundCategory::Master] = 1.0f;
    m_categoryVolumes[SoundCategory::Music] = 0.7f;
    m_categoryVolumes[SoundCategory::SFX] = 1.0f;
    m_categoryVolumes[SoundCategory::UI] = 0.8f;
    m_categoryVolumes[SoundCategory::Ambient] = 0.5f;
    m_categoryVolumes[SoundCategory::Voice] = 1.0f;
    m_categoryVolumes[SoundCategory::Combat] = 1.0f;
    
    RegisterAllSounds();
    
    m_initialized = true;
    Logger::Info("EnhancedSoundSystem initialized with {} sounds, {} layered, {} events",
                 m_sounds.size(), m_layeredSounds.size(), m_events.size());
}

void EnhancedSoundSystem::Shutdown() {
    StopAllSounds();
    m_sounds.clear();
    m_layeredSounds.clear();
    m_events.clear();
    m_activeInstances.clear();
    m_initialized = false;
}

void EnhancedSoundSystem::Update(float deltaTime) {
    if (m_paused) return;
    
    m_currentTime += deltaTime;
    
    UpdateFades(deltaTime);
    UpdateAmbient(deltaTime);
    UpdatePositionalAudio();
    
    // Clean up finished sounds
    m_activeInstances.erase(
        std::remove_if(m_activeInstances.begin(), m_activeInstances.end(),
            [](const SoundInstance& inst) {
                return inst.channel < 0 || inst.fadeProgress <= 0.0f;
            }),
        m_activeInstances.end()
    );
}

void EnhancedSoundSystem::SetCategoryVolume(SoundCategory category, float volume) {
    m_categoryVolumes[category] = std::clamp(volume, 0.0f, 1.0f);
}

float EnhancedSoundSystem::GetCategoryVolume(SoundCategory category) const {
    auto it = m_categoryVolumes.find(category);
    return it != m_categoryVolumes.end() ? it->second : 1.0f;
}

void EnhancedSoundSystem::SetMasterVolume(float volume) {
    m_masterVolume = std::clamp(volume, 0.0f, 1.0f);
}

int EnhancedSoundSystem::PlaySound(const std::string& soundId) {
    auto it = m_sounds.find(soundId);
    if (it == m_sounds.end()) {
        Logger::Warning("Sound not found: {}", soundId);
        return -1;
    }
    
    const SoundDef& def = it->second;
    
    // Check cooldown
    if (def.cooldown > 0 && (m_currentTime - def.lastPlayTime) < def.cooldown) {
        return -1;
    }
    
    // Find channel
    int channel = FindFreeChannel(def.priority);
    if (channel < 0) {
        Logger::Warning("No free channels for sound: {}", soundId);
        return -1;
    }
    
    // Create instance
    SoundInstance instance;
    instance.soundId = soundId;
    instance.channel = channel;
    instance.volume = def.baseVolume;
    instance.pitch = def.basePitch;
    instance.isPositional = false;
    
    // Handle fade in
    if (def.fadeInTime > 0) {
        instance.isFadingIn = true;
        instance.fadeProgress = 0.0f;
        instance.fadeTime = def.fadeInTime;
    }
    
    m_activeInstances.push_back(instance);
    
    // Update last play time
    const_cast<SoundDef&>(def).lastPlayTime = m_currentTime;
    
    float finalVolume = GetFinalVolume(def, instance.volume * instance.fadeProgress);
    Logger::Info("Playing sound: {} on channel {} at volume {:.2f}", soundId, channel, finalVolume);
    
    return channel;
}

int EnhancedSoundSystem::PlaySoundAt(const std::string& soundId, const Position& position) {
    int channel = PlaySound(soundId);
    if (channel >= 0) {
        // Find instance and set position
        for (auto& inst : m_activeInstances) {
            if (inst.channel == channel) {
                inst.position = position;
                inst.isPositional = true;
                break;
            }
        }
    }
    return channel;
}

int EnhancedSoundSystem::PlaySoundWithPitch(const std::string& soundId, float pitch) {
    int channel = PlaySound(soundId);
    if (channel >= 0) {
        for (auto& inst : m_activeInstances) {
            if (inst.channel == channel) {
                inst.pitch = pitch;
                break;
            }
        }
    }
    return channel;
}

int EnhancedSoundSystem::PlaySoundVariation(const std::string& soundId) {
    auto it = m_sounds.find(soundId);
    if (it == m_sounds.end()) return -1;
    
    const SoundDef& def = it->second;
    std::string actualSound = SelectVariation(def);
    
    int channel = PlaySound(actualSound.empty() ? soundId : actualSound);
    if (channel >= 0 && !actualSound.empty()) {
        // Apply random pitch/volume variation
        for (auto& inst : m_activeInstances) {
            if (inst.channel == channel) {
                inst.pitch = Random::Range(def.variation.pitchMin, def.variation.pitchMax);
                inst.volume *= Random::Range(def.variation.volumeMin, def.variation.volumeMax);
                break;
            }
        }
    }
    return channel;
}

int EnhancedSoundSystem::PlayLayeredSound(const std::string& layeredId) {
    auto it = m_layeredSounds.find(layeredId);
    if (it == m_layeredSounds.end()) {
        Logger::Warning("Layered sound not found: {}", layeredId);
        return -1;
    }
    
    const LayeredSound& layered = it->second;
    int mainChannel = -1;
    
    for (size_t i = 0; i < layered.layers.size(); ++i) {
        const SoundLayer& layer = layered.layers[i];
        
        // TODO: Implement delay for layers
        int channel = PlaySound(layer.soundId);
        
        if (i == 0) mainChannel = channel;
        
        // Adjust volume for layer
        if (channel >= 0) {
            for (auto& inst : m_activeInstances) {
                if (inst.channel == channel) {
                    inst.volume *= layer.volumeScale;
                    break;
                }
            }
        }
    }
    
    return mainChannel;
}

void EnhancedSoundSystem::StopSound(int channel) {
    for (auto& inst : m_activeInstances) {
        if (inst.channel == channel) {
            inst.channel = -1;  // Mark for removal
            break;
        }
    }
}

void EnhancedSoundSystem::StopAllSounds() {
    for (auto& inst : m_activeInstances) {
        inst.channel = -1;
    }
    m_activeInstances.clear();
}

void EnhancedSoundSystem::StopCategory(SoundCategory category) {
    for (auto& inst : m_activeInstances) {
        auto it = m_sounds.find(inst.soundId);
        if (it != m_sounds.end() && it->second.category == category) {
            inst.channel = -1;
        }
    }
}

void EnhancedSoundSystem::PauseAll() {
    m_paused = true;
}

void EnhancedSoundSystem::ResumeAll() {
    m_paused = false;
}

void EnhancedSoundSystem::FadeIn(int channel, float duration) {
    for (auto& inst : m_activeInstances) {
        if (inst.channel == channel) {
            inst.isFadingIn = true;
            inst.isFadingOut = false;
            inst.fadeTime = duration;
            inst.fadeProgress = 0.0f;
            break;
        }
    }
}

void EnhancedSoundSystem::FadeOut(int channel, float duration) {
    for (auto& inst : m_activeInstances) {
        if (inst.channel == channel) {
            inst.isFadingIn = false;
            inst.isFadingOut = true;
            inst.fadeTime = duration;
            break;
        }
    }
}

void EnhancedSoundSystem::CrossFade(int fromChannel, int toChannel, float duration) {
    FadeOut(fromChannel, duration);
    FadeIn(toChannel, duration);
}

void EnhancedSoundSystem::TriggerEvent(const std::string& eventName) {
    auto it = m_events.find(eventName);
    if (it == m_events.end()) return;
    
    const SoundEvent& event = it->second;
    
    // Probability check
    if (Random::Range(0.0f, 1.0f) > event.probability) return;
    
    // Count currently playing from this event
    int currentCount = 0;
    for (const auto& inst : m_activeInstances) {
        if (std::find(event.soundIds.begin(), event.soundIds.end(), inst.soundId) != event.soundIds.end()) {
            currentCount++;
        }
    }
    
    if (currentCount >= event.maxSimultaneous) return;
    
    // Pick random sound
    if (!event.soundIds.empty()) {
        int idx = Random::Range(0, static_cast<int>(event.soundIds.size()) - 1);
        PlaySoundVariation(event.soundIds[idx]);
    }
}

void EnhancedSoundSystem::RegisterEvent(const SoundEvent& event) {
    m_events[event.eventName] = event;
}

void EnhancedSoundSystem::SetAmbientZone(const std::string& zoneId) {
    if (m_currentAmbientZone == zoneId) return;
    
    // Stop current ambient
    if (m_ambientLoopChannel >= 0) {
        FadeOut(m_ambientLoopChannel, 2.0f);
    }
    
    m_currentAmbientZone = zoneId;
    m_ambientTimer = 0.0f;
    
    // Start new ambient
    auto it = m_ambientZones.find(zoneId);
    if (it != m_ambientZones.end() && !it->second.loopSound.empty()) {
        m_ambientLoopChannel = PlaySound(it->second.loopSound);
        FadeIn(m_ambientLoopChannel, 2.0f);
    }
    
    Logger::Info("Ambient zone changed to: {}", zoneId);
}

void EnhancedSoundSystem::StopAmbient() {
    if (m_ambientLoopChannel >= 0) {
        FadeOut(m_ambientLoopChannel, 1.0f);
        m_ambientLoopChannel = -1;
    }
    m_currentAmbientZone.clear();
}

void EnhancedSoundSystem::SetListenerPosition(const Position& pos) {
    m_listenerPos = pos;
}

void EnhancedSoundSystem::RegisterSound(const SoundDef& def) {
    m_sounds[def.id] = def;
}

void EnhancedSoundSystem::RegisterLayeredSound(const LayeredSound& layered) {
    m_layeredSounds[layered.id] = layered;
}

void EnhancedSoundSystem::RegisterAmbientZone(const AmbientZone& zone) {
    m_ambientZones[zone.id] = zone;
}

bool EnhancedSoundSystem::IsSoundPlaying(const std::string& soundId) const {
    for (const auto& inst : m_activeInstances) {
        if (inst.soundId == soundId && inst.channel >= 0) {
            return true;
        }
    }
    return false;
}

int EnhancedSoundSystem::GetActiveSoundCount() const {
    return static_cast<int>(m_activeInstances.size());
}

// ===========================================================================
// PRIVATE METHODS
// ===========================================================================

float EnhancedSoundSystem::CalculatePositionalVolume(const Position& soundPos) const {
    float dx = static_cast<float>(soundPos.x - m_listenerPos.x);
    float dy = static_cast<float>(soundPos.y - m_listenerPos.y);
    float distance = std::sqrt(dx * dx + dy * dy);
    
    // Simple linear falloff
    float maxDist = 10.0f;  // Board units
    float volume = 1.0f - (distance / maxDist);
    
    return std::clamp(volume, 0.0f, 1.0f);
}

float EnhancedSoundSystem::GetFinalVolume(const SoundDef& def, float instanceVolume) const {
    float volume = instanceVolume;
    volume *= GetCategoryVolume(def.category);
    volume *= m_masterVolume;
    return std::clamp(volume, 0.0f, 1.0f);
}

std::string EnhancedSoundSystem::SelectVariation(const SoundDef& def) const {
    if (def.variation.alternates.empty()) return "";
    
    int idx = Random::Range(0, static_cast<int>(def.variation.alternates.size()));
    if (idx == 0) return "";  // Use original
    return def.variation.alternates[idx - 1];
}

int EnhancedSoundSystem::FindFreeChannel(SoundPriority priority) const {
    // Simple implementation - return next available
    std::vector<bool> usedChannels(MAX_CHANNELS, false);
    
    for (const auto& inst : m_activeInstances) {
        if (inst.channel >= 0 && inst.channel < MAX_CHANNELS) {
            usedChannels[inst.channel] = true;
        }
    }
    
    for (int i = 0; i < MAX_CHANNELS; ++i) {
        if (!usedChannels[i]) return i;
    }
    
    // If all channels used, steal lowest priority if we're higher
    if (priority >= SoundPriority::High) {
        // Find lowest priority sound
        int lowestChannel = -1;
        SoundPriority lowestPriority = SoundPriority::Critical;
        
        for (const auto& inst : m_activeInstances) {
            auto it = m_sounds.find(inst.soundId);
            if (it != m_sounds.end() && it->second.priority < lowestPriority) {
                lowestPriority = it->second.priority;
                lowestChannel = inst.channel;
            }
        }
        
        if (lowestPriority < priority) {
            return lowestChannel;
        }
    }
    
    return -1;
}

void EnhancedSoundSystem::UpdateFades(float deltaTime) {
    for (auto& inst : m_activeInstances) {
        if (inst.isFadingIn) {
            inst.fadeProgress += deltaTime / inst.fadeTime;
            if (inst.fadeProgress >= 1.0f) {
                inst.fadeProgress = 1.0f;
                inst.isFadingIn = false;
            }
        } else if (inst.isFadingOut) {
            inst.fadeProgress -= deltaTime / inst.fadeTime;
            if (inst.fadeProgress <= 0.0f) {
                inst.fadeProgress = 0.0f;
                inst.isFadingOut = false;
                inst.channel = -1;  // Mark for removal
            }
        }
    }
}

void EnhancedSoundSystem::UpdateAmbient(float deltaTime) {
    if (m_currentAmbientZone.empty()) return;
    
    auto it = m_ambientZones.find(m_currentAmbientZone);
    if (it == m_ambientZones.end()) return;
    
    const AmbientZone& zone = it->second;
    
    m_ambientTimer += deltaTime;
    
    if (m_ambientTimer >= zone.randomInterval) {
        m_ambientTimer = 0.0f;
        
        // Random chance to play ambient sound
        if (Random::Range(0.0f, 1.0f) < zone.randomChance && !zone.randomSounds.empty()) {
            int idx = Random::Range(0, static_cast<int>(zone.randomSounds.size()) - 1);
            PlaySound(zone.randomSounds[idx]);
        }
    }
}

void EnhancedSoundSystem::UpdatePositionalAudio() {
    for (auto& inst : m_activeInstances) {
        if (inst.isPositional) {
            float posVolume = CalculatePositionalVolume(inst.position);
            // Would apply to actual audio channel here
        }
    }
}

// ===========================================================================
// SOUND REGISTRATION
// ===========================================================================

void EnhancedSoundSystem::RegisterAllSounds() {
    RegisterCombatSounds();
    RegisterUISounds();
    RegisterAmbientSounds();
    RegisterAbilitySounds();
}

void EnhancedSoundSystem::RegisterCombatSounds() {
    // Attacks
    RegisterSound({
        "attack_sword", "Sword Attack", SoundCategory::Combat, SoundPriority::Normal,
        1.0f, 1.0f, 0.0f, 10.0f, false, 0.0f, 0.0f,
        {"attack_sword", 0.9f, 1.1f, 0.9f, 1.0f, {"attack_sword_2", "attack_sword_3"}}
    });
    
    RegisterSound({
        "attack_magic", "Magic Attack", SoundCategory::Combat, SoundPriority::Normal,
        1.0f, 1.0f, 0.0f, 15.0f, false, 0.0f, 0.0f,
        {"attack_magic", 0.95f, 1.05f, 0.9f, 1.0f, {"attack_magic_2"}}
    });
    
    RegisterSound({
        "attack_arrow", "Arrow Shot", SoundCategory::Combat, SoundPriority::Normal,
        0.9f, 1.0f, 0.0f, 12.0f, false, 0.0f, 0.0f,
        {"attack_arrow", 0.9f, 1.1f, 0.85f, 1.0f, {"attack_arrow_2", "attack_arrow_3"}}
    });
    
    // Impacts
    RegisterSound({
        "hit_light", "Light Hit", SoundCategory::Combat, SoundPriority::Normal,
        0.8f, 1.0f, 0.0f, 8.0f, false, 0.0f, 0.0f,
        {"hit_light", 0.85f, 1.15f, 0.8f, 1.0f, {"hit_light_2", "hit_light_3"}}
    });
    
    RegisterSound({
        "hit_heavy", "Heavy Hit", SoundCategory::Combat, SoundPriority::Normal,
        1.0f, 0.9f, 0.0f, 10.0f, false, 0.0f, 0.0f,
        {"hit_heavy", 0.9f, 1.1f, 0.9f, 1.0f, {"hit_heavy_2"}}
    });
    
    RegisterSound({
        "hit_critical", "Critical Hit", SoundCategory::Combat, SoundPriority::High,
        1.0f, 1.0f, 0.0f, 15.0f, false, 0.0f, 0.0f,
        {"hit_critical", 0.95f, 1.05f, 0.95f, 1.0f, {}}
    });
    
    // Deaths
    RegisterSound({
        "death_enemy", "Enemy Death", SoundCategory::Combat, SoundPriority::Normal,
        1.0f, 1.0f, 0.0f, 12.0f, false, 0.0f, 0.0f,
        {"death_enemy", 0.9f, 1.1f, 0.9f, 1.0f, {"death_enemy_2", "death_enemy_3"}}
    });
    
    RegisterSound({
        "death_ally", "Ally Death", SoundCategory::Combat, SoundPriority::High,
        1.0f, 1.0f, 0.0f, 20.0f, false, 0.0f, 0.0f,
        {"death_ally", 1.0f, 1.0f, 1.0f, 1.0f, {}}
    });
    
    RegisterSound({
        "death_boss", "Boss Death", SoundCategory::Combat, SoundPriority::Critical,
        1.0f, 1.0f, 0.0f, 30.0f, false, 0.0f, 0.2f,
        {"death_boss", 1.0f, 1.0f, 1.0f, 1.0f, {}}
    });
    
    // Layered combat sounds
    RegisterLayeredSound({
        "explosion_large", "Large Explosion",
        {
            {"explosion_base", 1.0f, 0.0f, true},
            {"explosion_debris", 0.7f, 0.05f, true},
            {"explosion_rumble", 0.5f, 0.1f, true}
        },
        SoundCategory::Combat, SoundPriority::High
    });
    
    // Events
    RegisterEvent({"on_attack", {"attack_sword", "attack_sword_2", "attack_sword_3"}, 1.0f, 2});
    RegisterEvent({"on_hit", {"hit_light", "hit_light_2", "hit_light_3"}, 1.0f, 3});
    RegisterEvent({"on_crit", {"hit_critical"}, 1.0f, 1});
    RegisterEvent({"on_enemy_death", {"death_enemy", "death_enemy_2", "death_enemy_3"}, 1.0f, 2});
}

void EnhancedSoundSystem::RegisterUISounds() {
    RegisterSound({
        "ui_click", "UI Click", SoundCategory::UI, SoundPriority::Normal,
        0.7f, 1.0f, 0.0f, 0.0f, false, 0.0f, 0.0f,
        {"ui_click", 0.95f, 1.05f, 0.9f, 1.0f, {}}
    });
    
    RegisterSound({
        "ui_hover", "UI Hover", SoundCategory::UI, SoundPriority::Low,
        0.4f, 1.1f, 0.0f, 0.0f, false, 0.0f, 0.0f,
        {"ui_hover", 0.98f, 1.02f, 0.9f, 1.0f, {}}
    });
    
    RegisterSound({
        "ui_confirm", "UI Confirm", SoundCategory::UI, SoundPriority::Normal,
        0.8f, 1.0f, 0.0f, 0.0f, false, 0.0f, 0.0f,
        {"ui_confirm", 1.0f, 1.0f, 1.0f, 1.0f, {}}
    });
    
    RegisterSound({
        "ui_cancel", "UI Cancel", SoundCategory::UI, SoundPriority::Normal,
        0.8f, 0.9f, 0.0f, 0.0f, false, 0.0f, 0.0f,
        {"ui_cancel", 1.0f, 1.0f, 1.0f, 1.0f, {}}
    });
    
    RegisterSound({
        "ui_error", "UI Error", SoundCategory::UI, SoundPriority::Normal,
        0.9f, 1.0f, 0.0f, 0.0f, false, 0.0f, 0.0f,
        {"ui_error", 1.0f, 1.0f, 1.0f, 1.0f, {}}
    });
    
    RegisterSound({
        "dice_roll", "Dice Roll", SoundCategory::UI, SoundPriority::Normal,
        0.9f, 1.0f, 0.0f, 0.0f, false, 0.0f, 0.0f,
        {"dice_roll", 0.9f, 1.1f, 0.9f, 1.0f, {"dice_roll_2", "dice_roll_3"}}
    });
    
    RegisterSound({
        "dice_place", "Dice Place", SoundCategory::UI, SoundPriority::Normal,
        0.8f, 1.0f, 0.0f, 0.0f, false, 0.0f, 0.0f,
        {"dice_place", 0.95f, 1.05f, 0.9f, 1.0f, {}}
    });
    
    RegisterSound({
        "level_up", "Level Up", SoundCategory::UI, SoundPriority::High,
        1.0f, 1.0f, 0.0f, 0.0f, false, 0.0f, 0.0f,
        {"level_up", 1.0f, 1.0f, 1.0f, 1.0f, {}}
    });
    
    RegisterSound({
        "wave_complete", "Wave Complete", SoundCategory::UI, SoundPriority::High,
        1.0f, 1.0f, 0.0f, 0.0f, false, 0.0f, 0.0f,
        {"wave_complete", 1.0f, 1.0f, 1.0f, 1.0f, {}}
    });
    
    RegisterSound({
        "gold_collect", "Gold Collect", SoundCategory::UI, SoundPriority::Normal,
        0.7f, 1.0f, 0.0f, 0.0f, false, 0.0f, 0.0f,
        {"gold_collect", 0.9f, 1.2f, 0.8f, 1.0f, {"gold_collect_2"}}
    });
}

void EnhancedSoundSystem::RegisterAmbientSounds() {
    // Dungeon ambient zone
    RegisterAmbientZone({
        "dungeon",
        "ambient_dungeon_loop",
        {"ambient_drip", "ambient_creak", "ambient_wind", "ambient_distant_roar"},
        8.0f,
        0.2f
    });
    
    // Battle ambient zone
    RegisterAmbientZone({
        "battle",
        "ambient_battle_loop",
        {"ambient_clash", "ambient_shout"},
        5.0f,
        0.3f
    });
    
    // Boss battle ambient
    RegisterAmbientZone({
        "boss_battle",
        "ambient_boss_loop",
        {"ambient_rumble", "ambient_roar"},
        4.0f,
        0.4f
    });
    
    // Menu ambient
    RegisterAmbientZone({
        "menu",
        "ambient_menu_loop",
        {},
        0.0f,
        0.0f
    });
}

void EnhancedSoundSystem::RegisterAbilitySounds() {
    // Fire abilities
    RegisterSound({
        "ability_fireball", "Fireball", SoundCategory::Combat, SoundPriority::High,
        1.0f, 1.0f, 0.0f, 15.0f, false, 0.0f, 0.0f,
        {"ability_fireball", 0.95f, 1.05f, 0.95f, 1.0f, {}}
    });
    
    RegisterSound({
        "ability_meteor", "Meteor", SoundCategory::Combat, SoundPriority::Critical,
        1.0f, 0.9f, 0.0f, 20.0f, false, 0.0f, 0.0f,
        {"ability_meteor", 1.0f, 1.0f, 1.0f, 1.0f, {}}
    });
    
    // Ice abilities
    RegisterSound({
        "ability_ice_shard", "Ice Shard", SoundCategory::Combat, SoundPriority::Normal,
        0.9f, 1.1f, 0.0f, 12.0f, false, 0.0f, 0.0f,
        {"ability_ice_shard", 0.95f, 1.05f, 0.9f, 1.0f, {}}
    });
    
    RegisterSound({
        "ability_freeze", "Freeze", SoundCategory::Combat, SoundPriority::Normal,
        0.9f, 1.0f, 0.0f, 10.0f, false, 0.0f, 0.0f,
        {"ability_freeze", 1.0f, 1.0f, 1.0f, 1.0f, {}}
    });
    
    // Healing
    RegisterSound({
        "ability_heal", "Heal", SoundCategory::Combat, SoundPriority::Normal,
        0.9f, 1.0f, 0.0f, 10.0f, false, 0.0f, 0.0f,
        {"ability_heal", 0.95f, 1.05f, 0.9f, 1.0f, {"ability_heal_2"}}
    });
    
    RegisterSound({
        "ability_mass_heal", "Mass Heal", SoundCategory::Combat, SoundPriority::High,
        1.0f, 1.0f, 0.0f, 15.0f, false, 0.0f, 0.0f,
        {"ability_mass_heal", 1.0f, 1.0f, 1.0f, 1.0f, {}}
    });
    
    // Buffs/Debuffs
    RegisterSound({
        "ability_buff", "Buff Apply", SoundCategory::Combat, SoundPriority::Normal,
        0.8f, 1.0f, 0.0f, 8.0f, false, 0.0f, 0.0f,
        {"ability_buff", 0.95f, 1.05f, 0.9f, 1.0f, {}}
    });
    
    RegisterSound({
        "ability_debuff", "Debuff Apply", SoundCategory::Combat, SoundPriority::Normal,
        0.8f, 0.9f, 0.0f, 8.0f, false, 0.0f, 0.0f,
        {"ability_debuff", 0.95f, 1.05f, 0.9f, 1.0f, {}}
    });
    
    // Boss abilities
    RegisterSound({
        "ability_boss_roar", "Boss Roar", SoundCategory::Combat, SoundPriority::Critical,
        1.0f, 0.8f, 0.0f, 30.0f, false, 0.0f, 0.2f,
        {"ability_boss_roar", 0.95f, 1.05f, 1.0f, 1.0f, {}}
    });
    
    RegisterSound({
        "ability_dragon_breath", "Dragon Breath", SoundCategory::Combat, SoundPriority::Critical,
        1.0f, 1.0f, 0.0f, 25.0f, false, 0.0f, 0.0f,
        {"ability_dragon_breath", 1.0f, 1.0f, 1.0f, 1.0f, {}}
    });
}

} // namespace DDD
