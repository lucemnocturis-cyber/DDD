#include "VoiceSystem.h"
#include "../Utils/Logger.h"
#include "../Utils/Random.h"

#include <algorithm>
#include <cmath>

namespace DDD {

VoiceSystem& VoiceSystem::Instance() {
    static VoiceSystem instance;
    return instance;
}

void VoiceSystem::Initialize() {
    if (m_initialized) return;
    
    RegisterAllVoiceSets();
    RegisterDialogues();
    
    m_initialized = true;
    Logger::Info("VoiceSystem initialized with {} voice sets, {} dialogues",
                 m_voiceSets.size(), m_dialogues.size());
}

void VoiceSystem::Shutdown() {
    StopAll();
    m_voiceSets.clear();
    m_dialogues.clear();
    m_announcerLines.clear();
    m_initialized = false;
}

void VoiceSystem::Update(float deltaTime) {
    m_currentTime += deltaTime;
    
    // Update current playback
    if (m_isPlaying) {
        m_currentProgress += deltaTime;
        if (m_currentProgress >= m_currentDuration) {
            m_isPlaying = false;
            m_currentSubtitle.clear();
        }
    }
    
    // Update dialogue
    if (m_dialoguePlaying) {
        m_dialogueTimer += deltaTime;
        const DialogueLine* line = GetCurrentDialogueLine();
        if (line && m_dialogueTimer >= line->duration) {
            AdvanceDialogue();
        }
    }
    
    // Process voice queue
    if (!m_isPlaying && !m_dialoguePlaying) {
        ProcessQueue();
    }
    
    // Update cooldowns
    UpdateCooldowns(deltaTime);
}

void VoiceSystem::RegisterVoiceSet(const VoiceSet& set) {
    m_voiceSets[set.id] = set;
}

const VoiceSet* VoiceSystem::GetVoiceSet(const std::string& id) const {
    auto it = m_voiceSets.find(id);
    return it != m_voiceSets.end() ? &it->second : nullptr;
}

void VoiceSystem::RegisterDialogue(const DialogueSequence& dialogue) {
    m_dialogues[dialogue.id] = dialogue;
}

const DialogueSequence* VoiceSystem::GetDialogue(const std::string& id) const {
    auto it = m_dialogues.find(id);
    return it != m_dialogues.end() ? &it->second : nullptr;
}

void VoiceSystem::PlayVoice(const std::string& voiceSetId, VoiceTrigger trigger) {
    if (!CanPlayVoice(voiceSetId, trigger)) return;
    
    const VoiceSet* set = GetVoiceSet(voiceSetId);
    if (!set) return;
    
    const VoiceLine* line = SelectVoiceLine(*set, trigger);
    if (!line) return;
    
    // Check if current voice is interruptible
    if (m_isPlaying) {
        const VoiceSet* currentSet = GetVoiceSet(m_currentVoiceSetId);
        if (currentSet) {
            const VoiceLine* currentLine = SelectVoiceLine(*currentSet, m_currentTrigger);
            if (currentLine && !currentLine->interruptible) {
                QueueVoice(voiceSetId, trigger, line->priority);
                return;
            }
        }
    }
    
    PlayVoiceImmediate(voiceSetId, trigger);
}

void VoiceSystem::PlayVoiceImmediate(const std::string& voiceSetId, VoiceTrigger trigger) {
    const VoiceSet* set = GetVoiceSet(voiceSetId);
    if (!set) return;
    
    const VoiceLine* line = SelectVoiceLine(*set, trigger);
    if (!line) return;
    
    m_isPlaying = true;
    m_currentVoiceSetId = voiceSetId;
    m_currentTrigger = trigger;
    m_currentDuration = line->duration;
    m_currentProgress = 0.0f;
    m_currentSubtitle = line->text;
    
    // Set cooldown
    std::string cooldownKey = voiceSetId + "_" + std::to_string(static_cast<int>(trigger));
    m_cooldowns[cooldownKey] = line->cooldown;
    
    Logger::Info("Playing voice: {} - {}", set->name, line->text);
}

void VoiceSystem::QueueVoice(const std::string& voiceSetId, VoiceTrigger trigger, float priority) {
    QueuedVoice queued;
    queued.voiceSetId = voiceSetId;
    queued.trigger = trigger;
    queued.priority = priority;
    queued.timestamp = m_currentTime;
    
    m_voiceQueue.push_back(queued);
    
    // Sort by priority
    std::sort(m_voiceQueue.begin(), m_voiceQueue.end(),
        [](const QueuedVoice& a, const QueuedVoice& b) {
            return a.priority > b.priority;
        });
}

void VoiceSystem::PlayAnnouncer(AnnouncerType type) {
    if (!m_announcerEnabled) return;
    
    auto it = m_announcerLines.find(type);
    if (it == m_announcerLines.end() || it->second.empty()) return;
    
    // Select random line
    int idx = Random::Range(0, static_cast<int>(it->second.size()) - 1);
    const VoiceLine& line = it->second[idx];
    
    // Announcer has high priority
    if (m_isPlaying) {
        StopCurrentVoice();
    }
    
    m_isPlaying = true;
    m_currentVoiceSetId = "announcer";
    m_currentDuration = line.duration;
    m_currentProgress = 0.0f;
    m_currentSubtitle = line.text;
    
    Logger::Info("Announcer: {}", line.text);
}

void VoiceSystem::StartDialogue(const std::string& dialogueId) {
    const DialogueSequence* dialogue = GetDialogue(dialogueId);
    if (!dialogue || dialogue->lines.empty()) return;
    
    StopAll();
    
    m_dialoguePlaying = true;
    m_currentDialogueId = dialogueId;
    m_currentDialogueIndex = 0;
    m_dialogueTimer = 0.0f;
    
    const DialogueLine& line = dialogue->lines[0];
    m_currentSubtitle = line.text;
    
    Logger::Info("Starting dialogue: {}", dialogue->name);
}

void VoiceSystem::AdvanceDialogue() {
    const DialogueSequence* dialogue = GetDialogue(m_currentDialogueId);
    if (!dialogue) {
        SkipDialogue();
        return;
    }
    
    m_currentDialogueIndex++;
    m_dialogueTimer = 0.0f;
    
    if (m_currentDialogueIndex >= static_cast<int>(dialogue->lines.size())) {
        SkipDialogue();
        return;
    }
    
    const DialogueLine& line = dialogue->lines[m_currentDialogueIndex];
    m_currentSubtitle = line.text;
}

void VoiceSystem::SkipDialogue() {
    m_dialoguePlaying = false;
    m_currentDialogueId.clear();
    m_currentDialogueIndex = 0;
    m_currentSubtitle.clear();
}

const DialogueLine* VoiceSystem::GetCurrentDialogueLine() const {
    if (!m_dialoguePlaying) return nullptr;
    
    const DialogueSequence* dialogue = GetDialogue(m_currentDialogueId);
    if (!dialogue || m_currentDialogueIndex >= static_cast<int>(dialogue->lines.size())) {
        return nullptr;
    }
    
    return &dialogue->lines[m_currentDialogueIndex];
}

void VoiceSystem::SetVoiceVolume(float volume) {
    m_voiceVolume = std::clamp(volume, 0.0f, 1.0f);
}

void VoiceSystem::SetAnnouncerVolume(float volume) {
    m_announcerVolume = std::clamp(volume, 0.0f, 1.0f);
}

float VoiceSystem::GetSubtitleProgress() const {
    if (!m_isPlaying || m_currentDuration <= 0) return 0.0f;
    return m_currentProgress / m_currentDuration;
}

void VoiceSystem::StopCurrentVoice() {
    m_isPlaying = false;
    m_currentSubtitle.clear();
}

void VoiceSystem::StopAll() {
    StopCurrentVoice();
    SkipDialogue();
    m_voiceQueue.clear();
}

bool VoiceSystem::CanPlayVoice(const std::string& voiceSetId, VoiceTrigger trigger) const {
    std::string cooldownKey = voiceSetId + "_" + std::to_string(static_cast<int>(trigger));
    auto it = m_cooldowns.find(cooldownKey);
    return it == m_cooldowns.end() || it->second <= 0.0f;
}

const VoiceLine* VoiceSystem::SelectVoiceLine(const VoiceSet& set, VoiceTrigger trigger) const {
    auto it = set.lines.find(trigger);
    if (it == set.lines.end() || it->second.empty()) return nullptr;
    
    const auto& lines = it->second;
    
    // Weighted random selection based on priority
    float totalPriority = 0.0f;
    for (const auto& line : lines) {
        totalPriority += line.priority;
    }
    
    float roll = Random::Range(0.0f, totalPriority);
    float cumulative = 0.0f;
    
    for (const auto& line : lines) {
        cumulative += line.priority;
        if (roll <= cumulative) {
            return &line;
        }
    }
    
    return &lines[0];
}

void VoiceSystem::ProcessQueue() {
    if (m_voiceQueue.empty()) return;
    
    // Remove expired entries
    m_voiceQueue.erase(
        std::remove_if(m_voiceQueue.begin(), m_voiceQueue.end(),
            [this](const QueuedVoice& q) {
                return (m_currentTime - q.timestamp) > QUEUE_TIMEOUT;
            }),
        m_voiceQueue.end()
    );
    
    if (m_voiceQueue.empty()) return;
    
    // Play highest priority
    const QueuedVoice& next = m_voiceQueue[0];
    PlayVoiceImmediate(next.voiceSetId, next.trigger);
    m_voiceQueue.erase(m_voiceQueue.begin());
}

void VoiceSystem::UpdateCooldowns(float deltaTime) {
    for (auto& [key, cooldown] : m_cooldowns) {
        if (cooldown > 0) {
            cooldown -= deltaTime;
        }
    }
}

// ===========================================================================
// VOICE REGISTRATION
// ===========================================================================

void VoiceSystem::RegisterAllVoiceSets() {
    RegisterUnitVoices();
    RegisterBossVoices();
    RegisterAnnouncerVoices();
}

void VoiceSystem::RegisterUnitVoices() {
    // Mage voice set
    VoiceSet mageVoice;
    mageVoice.id = "voice_mage";
    mageVoice.name = "Mage";
    mageVoice.characterClass = "Mage";
    mageVoice.pitchBase = 1.0f;
    
    mageVoice.lines[VoiceTrigger::Summon] = {
        {"mage_summon_1", "The arcane answers my call!", "mage_summon_1.wav", 1.5f, 1.0f, 5.0f, true},
        {"mage_summon_2", "Magic flows through me!", "mage_summon_2.wav", 1.3f, 1.0f, 5.0f, true}
    };
    mageVoice.lines[VoiceTrigger::Select] = {
        {"mage_select_1", "What is your command?", "mage_select_1.wav", 1.2f, 1.0f, 2.0f, true},
        {"mage_select_2", "I am ready.", "mage_select_2.wav", 1.0f, 1.0f, 2.0f, true}
    };
    mageVoice.lines[VoiceTrigger::Attack] = {
        {"mage_attack_1", "Feel the burn!", "mage_attack_1.wav", 1.0f, 1.0f, 1.0f, true},
        {"mage_attack_2", "Ignite!", "mage_attack_2.wav", 0.8f, 1.0f, 1.0f, true}
    };
    mageVoice.lines[VoiceTrigger::AbilityUse] = {
        {"mage_ability_1", "Witness true power!", "mage_ability_1.wav", 1.3f, 1.5f, 3.0f, false},
        {"mage_ability_2", "By the ancient flames!", "mage_ability_2.wav", 1.5f, 1.5f, 3.0f, false}
    };
    mageVoice.lines[VoiceTrigger::CriticalHit] = {
        {"mage_crit_1", "Devastating!", "mage_crit_1.wav", 1.0f, 2.0f, 5.0f, false}
    };
    mageVoice.lines[VoiceTrigger::Kill] = {
        {"mage_kill_1", "Another falls to magic.", "mage_kill_1.wav", 1.2f, 1.0f, 3.0f, true},
        {"mage_kill_2", "Reduced to cinders.", "mage_kill_2.wav", 1.3f, 1.0f, 3.0f, true}
    };
    mageVoice.lines[VoiceTrigger::Damaged] = {
        {"mage_hit_1", "Ugh!", "mage_hit_1.wav", 0.5f, 1.0f, 1.0f, true},
        {"mage_hit_2", "That stings!", "mage_hit_2.wav", 0.7f, 1.0f, 1.0f, true}
    };
    mageVoice.lines[VoiceTrigger::LowHealth] = {
        {"mage_low_1", "I need healing!", "mage_low_1.wav", 1.2f, 2.0f, 10.0f, false}
    };
    mageVoice.lines[VoiceTrigger::Death] = {
        {"mage_death_1", "The flames... fade...", "mage_death_1.wav", 1.5f, 3.0f, 0.0f, false}
    };
    mageVoice.lines[VoiceTrigger::LevelUp] = {
        {"mage_level_1", "My power grows!", "mage_level_1.wav", 1.3f, 2.0f, 0.0f, false}
    };
    RegisterVoiceSet(mageVoice);
    
    // Soldier voice set
    VoiceSet soldierVoice;
    soldierVoice.id = "voice_soldier";
    soldierVoice.name = "Soldier";
    soldierVoice.characterClass = "Soldier";
    soldierVoice.pitchBase = 0.9f;
    
    soldierVoice.lines[VoiceTrigger::Summon] = {
        {"soldier_summon_1", "Reporting for duty!", "soldier_summon_1.wav", 1.3f, 1.0f, 5.0f, true},
        {"soldier_summon_2", "Ready to fight!", "soldier_summon_2.wav", 1.2f, 1.0f, 5.0f, true}
    };
    soldierVoice.lines[VoiceTrigger::Select] = {
        {"soldier_select_1", "Orders?", "soldier_select_1.wav", 0.8f, 1.0f, 2.0f, true},
        {"soldier_select_2", "At your service.", "soldier_select_2.wav", 1.0f, 1.0f, 2.0f, true}
    };
    soldierVoice.lines[VoiceTrigger::Attack] = {
        {"soldier_attack_1", "For glory!", "soldier_attack_1.wav", 1.0f, 1.0f, 1.0f, true},
        {"soldier_attack_2", "Take this!", "soldier_attack_2.wav", 0.8f, 1.0f, 1.0f, true},
        {"soldier_attack_3", "Ha!", "soldier_attack_3.wav", 0.5f, 1.0f, 0.5f, true}
    };
    soldierVoice.lines[VoiceTrigger::CriticalHit] = {
        {"soldier_crit_1", "Critical strike!", "soldier_crit_1.wav", 1.2f, 2.0f, 5.0f, false}
    };
    soldierVoice.lines[VoiceTrigger::Kill] = {
        {"soldier_kill_1", "Enemy down!", "soldier_kill_1.wav", 1.0f, 1.0f, 3.0f, true},
        {"soldier_kill_2", "One less to worry about.", "soldier_kill_2.wav", 1.3f, 1.0f, 3.0f, true}
    };
    soldierVoice.lines[VoiceTrigger::Damaged] = {
        {"soldier_hit_1", "I can take it!", "soldier_hit_1.wav", 0.8f, 1.0f, 1.5f, true},
        {"soldier_hit_2", "Argh!", "soldier_hit_2.wav", 0.5f, 1.0f, 1.0f, true}
    };
    soldierVoice.lines[VoiceTrigger::Death] = {
        {"soldier_death_1", "Tell my family... I fought well...", "soldier_death_1.wav", 2.0f, 3.0f, 0.0f, false}
    };
    RegisterVoiceSet(soldierVoice);
    
    // Healer voice set
    VoiceSet healerVoice;
    healerVoice.id = "voice_healer";
    healerVoice.name = "Healer";
    healerVoice.characterClass = "Healer";
    healerVoice.pitchBase = 1.1f;
    
    healerVoice.lines[VoiceTrigger::Summon] = {
        {"healer_summon_1", "Light guide us!", "healer_summon_1.wav", 1.3f, 1.0f, 5.0f, true}
    };
    healerVoice.lines[VoiceTrigger::Select] = {
        {"healer_select_1", "Who needs aid?", "healer_select_1.wav", 1.0f, 1.0f, 2.0f, true},
        {"healer_select_2", "I am here.", "healer_select_2.wav", 0.9f, 1.0f, 2.0f, true}
    };
    healerVoice.lines[VoiceTrigger::AbilityUse] = {
        {"healer_ability_1", "Be healed!", "healer_ability_1.wav", 1.0f, 1.5f, 2.0f, false},
        {"healer_ability_2", "Light, restore them!", "healer_ability_2.wav", 1.3f, 1.5f, 2.0f, false}
    };
    healerVoice.lines[VoiceTrigger::Healed] = {
        {"healer_heal_1", "That should help.", "healer_heal_1.wav", 1.0f, 1.0f, 3.0f, true}
    };
    healerVoice.lines[VoiceTrigger::Damaged] = {
        {"healer_hit_1", "Protect me!", "healer_hit_1.wav", 0.8f, 1.5f, 2.0f, true}
    };
    healerVoice.lines[VoiceTrigger::Death] = {
        {"healer_death_1", "The light... fades...", "healer_death_1.wav", 1.5f, 3.0f, 0.0f, false}
    };
    RegisterVoiceSet(healerVoice);
    
    // Rogue voice set
    VoiceSet rogueVoice;
    rogueVoice.id = "voice_rogue";
    rogueVoice.name = "Rogue";
    rogueVoice.characterClass = "Rogue";
    rogueVoice.pitchBase = 1.0f;
    
    rogueVoice.lines[VoiceTrigger::Summon] = {
        {"rogue_summon_1", "From the shadows...", "rogue_summon_1.wav", 1.2f, 1.0f, 5.0f, true}
    };
    rogueVoice.lines[VoiceTrigger::Select] = {
        {"rogue_select_1", "Quietly now...", "rogue_select_1.wav", 1.0f, 1.0f, 2.0f, true},
        {"rogue_select_2", "What's the target?", "rogue_select_2.wav", 1.1f, 1.0f, 2.0f, true}
    };
    rogueVoice.lines[VoiceTrigger::Attack] = {
        {"rogue_attack_1", "Surprise!", "rogue_attack_1.wav", 0.8f, 1.0f, 1.0f, true},
        {"rogue_attack_2", "Too slow!", "rogue_attack_2.wav", 0.9f, 1.0f, 1.0f, true}
    };
    rogueVoice.lines[VoiceTrigger::CriticalHit] = {
        {"rogue_crit_1", "Right in the back!", "rogue_crit_1.wav", 1.2f, 2.0f, 5.0f, false},
        {"rogue_crit_2", "Backstab!", "rogue_crit_2.wav", 1.0f, 2.0f, 5.0f, false}
    };
    rogueVoice.lines[VoiceTrigger::Kill] = {
        {"rogue_kill_1", "Never saw it coming.", "rogue_kill_1.wav", 1.2f, 1.0f, 3.0f, true}
    };
    rogueVoice.lines[VoiceTrigger::Death] = {
        {"rogue_death_1", "I've been... careless...", "rogue_death_1.wav", 1.5f, 3.0f, 0.0f, false}
    };
    RegisterVoiceSet(rogueVoice);
    
    // Archer voice set
    VoiceSet archerVoice;
    archerVoice.id = "voice_archer";
    archerVoice.name = "Archer";
    archerVoice.characterClass = "Archer";
    archerVoice.pitchBase = 1.05f;
    
    archerVoice.lines[VoiceTrigger::Summon] = {
        {"archer_summon_1", "Eyes on target!", "archer_summon_1.wav", 1.2f, 1.0f, 5.0f, true}
    };
    archerVoice.lines[VoiceTrigger::Attack] = {
        {"archer_attack_1", "Releasing!", "archer_attack_1.wav", 0.8f, 1.0f, 1.0f, true},
        {"archer_attack_2", "Fire!", "archer_attack_2.wav", 0.6f, 1.0f, 1.0f, true}
    };
    archerVoice.lines[VoiceTrigger::CriticalHit] = {
        {"archer_crit_1", "Bullseye!", "archer_crit_1.wav", 1.0f, 2.0f, 5.0f, false}
    };
    archerVoice.lines[VoiceTrigger::Kill] = {
        {"archer_kill_1", "Target eliminated.", "archer_kill_1.wav", 1.2f, 1.0f, 3.0f, true}
    };
    RegisterVoiceSet(archerVoice);
    
    // Tank voice set
    VoiceSet tankVoice;
    tankVoice.id = "voice_tank";
    tankVoice.name = "Tank";
    tankVoice.characterClass = "Tank";
    tankVoice.pitchBase = 0.85f;
    
    tankVoice.lines[VoiceTrigger::Summon] = {
        {"tank_summon_1", "I am your shield!", "tank_summon_1.wav", 1.3f, 1.0f, 5.0f, true}
    };
    tankVoice.lines[VoiceTrigger::Select] = {
        {"tank_select_1", "Standing firm.", "tank_select_1.wav", 1.0f, 1.0f, 2.0f, true}
    };
    tankVoice.lines[VoiceTrigger::Damaged] = {
        {"tank_hit_1", "Is that all?", "tank_hit_1.wav", 1.0f, 1.0f, 2.0f, true},
        {"tank_hit_2", "Hit me harder!", "tank_hit_2.wav", 1.1f, 1.0f, 2.0f, true}
    };
    tankVoice.lines[VoiceTrigger::Taunt] = {
        {"tank_taunt_1", "Come and get me!", "tank_taunt_1.wav", 1.3f, 1.5f, 5.0f, false},
        {"tank_taunt_2", "Over here, ugly!", "tank_taunt_2.wav", 1.2f, 1.5f, 5.0f, false}
    };
    tankVoice.lines[VoiceTrigger::Death] = {
        {"tank_death_1", "They got... through...", "tank_death_1.wav", 1.8f, 3.0f, 0.0f, false}
    };
    RegisterVoiceSet(tankVoice);
}

void VoiceSystem::RegisterBossVoices() {
    // Ancient Dragon
    VoiceSet dragonVoice;
    dragonVoice.id = "voice_dragon";
    dragonVoice.name = "Ancient Dragon";
    dragonVoice.characterClass = "Boss";
    dragonVoice.pitchBase = 0.7f;
    
    dragonVoice.lines[VoiceTrigger::BossEncounter] = {
        {"dragon_intro_1", "You dare enter MY domain?!", "dragon_intro_1.wav", 2.5f, 3.0f, 0.0f, false}
    };
    dragonVoice.lines[VoiceTrigger::Attack] = {
        {"dragon_attack_1", "BURN!", "dragon_attack_1.wav", 1.0f, 1.0f, 2.0f, false},
        {"dragon_attack_2", "Feel my wrath!", "dragon_attack_2.wav", 1.5f, 1.0f, 2.0f, false}
    };
    dragonVoice.lines[VoiceTrigger::AbilityUse] = {
        {"dragon_ability_1", "WITNESS DESTRUCTION!", "dragon_ability_1.wav", 2.0f, 2.0f, 5.0f, false}
    };
    dragonVoice.lines[VoiceTrigger::LowHealth] = {
        {"dragon_enrage_1", "You will PAY for this!", "dragon_enrage_1.wav", 2.0f, 3.0f, 30.0f, false}
    };
    dragonVoice.lines[VoiceTrigger::Death] = {
        {"dragon_death_1", "Impossible... I am... eternal...", "dragon_death_1.wav", 3.0f, 3.0f, 0.0f, false}
    };
    RegisterVoiceSet(dragonVoice);
    
    // Necrolord
    VoiceSet necroVoice;
    necroVoice.id = "voice_necrolord";
    necroVoice.name = "Necrolord";
    necroVoice.characterClass = "Boss";
    necroVoice.pitchBase = 0.8f;
    
    necroVoice.lines[VoiceTrigger::BossEncounter] = {
        {"necro_intro_1", "Join my army... in death!", "necro_intro_1.wav", 2.3f, 3.0f, 0.0f, false}
    };
    necroVoice.lines[VoiceTrigger::AbilityUse] = {
        {"necro_ability_1", "Rise, my minions!", "necro_ability_1.wav", 1.8f, 2.0f, 5.0f, false},
        {"necro_ability_2", "Death comes for you!", "necro_ability_2.wav", 1.5f, 2.0f, 5.0f, false}
    };
    necroVoice.lines[VoiceTrigger::Kill] = {
        {"necro_kill_1", "Another soul for my collection.", "necro_kill_1.wav", 2.0f, 1.5f, 5.0f, true}
    };
    necroVoice.lines[VoiceTrigger::Death] = {
        {"necro_death_1", "Death... is not the end...", "necro_death_1.wav", 2.5f, 3.0f, 0.0f, false}
    };
    RegisterVoiceSet(necroVoice);
    
    // Shadow King
    VoiceSet shadowVoice;
    shadowVoice.id = "voice_shadow_king";
    shadowVoice.name = "Shadow King";
    shadowVoice.characterClass = "Boss";
    shadowVoice.pitchBase = 0.75f;
    
    shadowVoice.lines[VoiceTrigger::BossEncounter] = {
        {"shadow_intro_1", "You stand before the ruler of darkness!", "shadow_intro_1.wav", 2.8f, 3.0f, 0.0f, false}
    };
    shadowVoice.lines[VoiceTrigger::Attack] = {
        {"shadow_attack_1", "Embrace the void!", "shadow_attack_1.wav", 1.5f, 1.0f, 2.0f, false}
    };
    shadowVoice.lines[VoiceTrigger::LowHealth] = {
        {"shadow_enrage_1", "The shadows will consume ALL!", "shadow_enrage_1.wav", 2.5f, 3.0f, 30.0f, false}
    };
    shadowVoice.lines[VoiceTrigger::Death] = {
        {"shadow_death_1", "The darkness... never truly dies...", "shadow_death_1.wav", 3.0f, 3.0f, 0.0f, false}
    };
    RegisterVoiceSet(shadowVoice);
}

void VoiceSystem::RegisterAnnouncerVoices() {
    m_announcerLines[AnnouncerType::WaveStart] = {
        {"ann_wave_1", "Wave incoming!", "announcer_wave_1.wav", 1.2f, 1.0f, 0.0f, true},
        {"ann_wave_2", "Here they come!", "announcer_wave_2.wav", 1.3f, 1.0f, 0.0f, true}
    };
    
    m_announcerLines[AnnouncerType::WaveComplete] = {
        {"ann_clear_1", "Wave complete!", "announcer_clear_1.wav", 1.3f, 1.0f, 0.0f, true},
        {"ann_clear_2", "Area secured!", "announcer_clear_2.wav", 1.2f, 1.0f, 0.0f, true}
    };
    
    m_announcerLines[AnnouncerType::BossIncoming] = {
        {"ann_boss_1", "Boss approaching!", "announcer_boss_1.wav", 1.8f, 2.0f, 0.0f, false},
        {"ann_boss_2", "A powerful enemy appears!", "announcer_boss_2.wav", 2.0f, 2.0f, 0.0f, false}
    };
    
    m_announcerLines[AnnouncerType::FirstBlood] = {
        {"ann_first_1", "First blood!", "announcer_first_1.wav", 1.2f, 1.5f, 0.0f, true}
    };
    
    m_announcerLines[AnnouncerType::MultiKill] = {
        {"ann_multi_1", "Multi-kill!", "announcer_multi_1.wav", 1.3f, 1.5f, 0.0f, true},
        {"ann_multi_2", "Devastating!", "announcer_multi_2.wav", 1.2f, 1.5f, 0.0f, true}
    };
    
    m_announcerLines[AnnouncerType::Unstoppable] = {
        {"ann_unstop_1", "Unstoppable!", "announcer_unstop_1.wav", 1.5f, 2.0f, 0.0f, false}
    };
    
    m_announcerLines[AnnouncerType::PlayerTurn] = {
        {"ann_player_1", "Your turn!", "announcer_player_1.wav", 1.0f, 1.0f, 0.0f, true}
    };
    
    m_announcerLines[AnnouncerType::EnemyTurn] = {
        {"ann_enemy_1", "Enemy turn!", "announcer_enemy_1.wav", 1.0f, 1.0f, 0.0f, true}
    };
    
    m_announcerLines[AnnouncerType::LowHealth] = {
        {"ann_low_1", "Unit critical!", "announcer_low_1.wav", 1.2f, 1.5f, 5.0f, true}
    };
    
    m_announcerLines[AnnouncerType::AlliesDown] = {
        {"ann_allies_1", "All allies down!", "announcer_allies_1.wav", 1.5f, 2.0f, 0.0f, false}
    };
    
    m_announcerLines[AnnouncerType::Victory] = {
        {"ann_victory_1", "Victory!", "announcer_victory_1.wav", 1.5f, 2.0f, 0.0f, false},
        {"ann_victory_2", "You are victorious!", "announcer_victory_2.wav", 1.8f, 2.0f, 0.0f, false}
    };
    
    m_announcerLines[AnnouncerType::Defeat] = {
        {"ann_defeat_1", "Defeat...", "announcer_defeat_1.wav", 1.5f, 2.0f, 0.0f, false}
    };
    
    m_announcerLines[AnnouncerType::ComboHit] = {
        {"ann_combo_1", "Combo!", "announcer_combo_1.wav", 0.8f, 1.0f, 2.0f, true}
    };
}

void VoiceSystem::RegisterDialogues() {
    // Dragon encounter dialogue
    RegisterDialogue({
        "dragon_intro", "Dragon Introduction",
        {
            {"narrator", "A massive shadow looms over the battlefield...", "", 3.0f, ""},
            {"voice_dragon", "You dare enter MY domain?!", "dragon_intro_1.wav", 2.5f, "angry"},
            {"voice_dragon", "I have slumbered for a thousand years... and you WAKE me?!", "dragon_intro_2.wav", 3.0f, "furious"},
            {"narrator", "The Ancient Dragon prepares to attack!", "", 2.0f, ""}
        },
        true, true
    });
    
    // Necrolord encounter
    RegisterDialogue({
        "necrolord_intro", "Necrolord Introduction",
        {
            {"narrator", "An unnatural chill fills the air...", "", 2.5f, ""},
            {"voice_necrolord", "Ah, fresh souls for my collection.", "necro_intro_1.wav", 2.3f, "sinister"},
            {"voice_necrolord", "Join my army... in eternal servitude!", "necro_intro_2.wav", 2.5f, "commanding"}
        },
        true, true
    });
    
    // Shadow King encounter
    RegisterDialogue({
        "shadow_king_intro", "Shadow King Introduction",
        {
            {"narrator", "Darkness engulfs the chamber...", "", 2.5f, ""},
            {"voice_shadow_king", "You stand before the ruler of all darkness!", "shadow_intro_1.wav", 2.8f, "menacing"},
            {"voice_shadow_king", "Your light will be extinguished!", "shadow_intro_2.wav", 2.3f, "threatening"}
        },
        true, true
    });
    
    // Victory dialogue
    RegisterDialogue({
        "victory_dialogue", "Victory",
        {
            {"narrator", "The enemy has been vanquished!", "", 2.0f, ""},
            {"narrator", "Your tactics have proven superior.", "", 2.5f, ""}
        },
        true, false
    });
}

} // namespace DDD
