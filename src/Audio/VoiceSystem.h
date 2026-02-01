#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

namespace DDD {

/**
 * Voice line trigger events
 */
enum class VoiceTrigger {
    // Unit actions
    Summon,             // When unit is summoned
    Select,             // When unit is selected
    Move,               // When unit moves
    Attack,             // When attacking
    AbilityUse,         // When using ability
    CriticalHit,        // On critical hit
    Kill,               // When killing enemy
    MultiKill,          // Multiple kills in one turn
    
    // Damage/Death
    Damaged,            // When taking damage
    LowHealth,          // Below 25% HP
    Death,              // When dying
    Revive,             // When revived
    
    // Status effects
    Buffed,             // Positive effect applied
    Debuffed,           // Negative effect applied
    Healed,             // When healed
    
    // Special
    LevelUp,            // On level up
    Promotion,          // On class promotion
    BossEncounter,      // Meeting a boss
    Victory,            // Battle won
    Defeat,             // Battle lost
    
    // Idle/Ambient
    Idle,               // Random idle chatter
    Taunt               // Taunting enemies
};

/**
 * Voice line definition
 */
struct VoiceLine {
    std::string id;
    std::string text;           // Subtitle text
    std::string audioFile;      // Audio file name
    float duration;             // Duration in seconds
    float priority;             // Higher = more likely to play
    float cooldown;             // Minimum time between plays
    bool interruptible;         // Can be cut off
};

/**
 * Voice set (collection of lines for a character/class)
 */
struct VoiceSet {
    std::string id;
    std::string name;
    std::string characterClass;
    
    // Lines organized by trigger
    std::unordered_map<VoiceTrigger, std::vector<VoiceLine>> lines;
    
    // Voice properties
    float pitchBase = 1.0f;
    float pitchVariance = 0.05f;
    float volumeBase = 1.0f;
};

/**
 * Boss dialogue sequence
 */
struct DialogueLine {
    std::string speakerId;
    std::string text;
    std::string audioFile;
    float duration;
    std::string emotion;        // For portrait animation
};

struct DialogueSequence {
    std::string id;
    std::string name;
    std::vector<DialogueLine> lines;
    bool skippable = true;
    bool pauseGameplay = true;
};

/**
 * Announcer line types
 */
enum class AnnouncerType {
    WaveStart,
    WaveComplete,
    BossIncoming,
    FirstBlood,
    MultiKill,
    Unstoppable,
    PlayerTurn,
    EnemyTurn,
    LowHealth,
    AlliesDown,
    Victory,
    Defeat,
    ShopOpen,
    TimeWarning,
    ComboHit
};

/**
 * Voice queue entry
 */
struct QueuedVoice {
    std::string voiceSetId;
    VoiceTrigger trigger;
    float priority;
    float timestamp;
};

/**
 * VoiceSystem - manages all voice playback
 */
class VoiceSystem {
public:
    static VoiceSystem& Instance();
    
    void Initialize();
    void Shutdown();
    void Update(float deltaTime);
    
    // Voice set management
    void RegisterVoiceSet(const VoiceSet& set);
    const VoiceSet* GetVoiceSet(const std::string& id) const;
    
    // Dialogue management
    void RegisterDialogue(const DialogueSequence& dialogue);
    const DialogueSequence* GetDialogue(const std::string& id) const;
    
    // Voice playback
    void PlayVoice(const std::string& voiceSetId, VoiceTrigger trigger);
    void PlayVoiceImmediate(const std::string& voiceSetId, VoiceTrigger trigger);
    void QueueVoice(const std::string& voiceSetId, VoiceTrigger trigger, float priority);
    
    // Announcer
    void PlayAnnouncer(AnnouncerType type);
    void SetAnnouncerEnabled(bool enabled) { m_announcerEnabled = enabled; }
    bool IsAnnouncerEnabled() const { return m_announcerEnabled; }
    
    // Dialogue sequences
    void StartDialogue(const std::string& dialogueId);
    void AdvanceDialogue();
    void SkipDialogue();
    bool IsDialoguePlaying() const { return m_dialoguePlaying; }
    const DialogueLine* GetCurrentDialogueLine() const;
    
    // Volume control
    void SetVoiceVolume(float volume);
    float GetVoiceVolume() const { return m_voiceVolume; }
    void SetAnnouncerVolume(float volume);
    float GetAnnouncerVolume() const { return m_announcerVolume; }
    
    // Subtitles
    void SetSubtitlesEnabled(bool enabled) { m_subtitlesEnabled = enabled; }
    bool AreSubtitlesEnabled() const { return m_subtitlesEnabled; }
    std::string GetCurrentSubtitle() const { return m_currentSubtitle; }
    float GetSubtitleProgress() const;
    
    // Interruption
    void StopCurrentVoice();
    void StopAll();
    
    // Cooldown management
    bool CanPlayVoice(const std::string& voiceSetId, VoiceTrigger trigger) const;
    
private:
    VoiceSystem() = default;
    
    void RegisterAllVoiceSets();
    void RegisterUnitVoices();
    void RegisterBossVoices();
    void RegisterAnnouncerVoices();
    void RegisterDialogues();
    
    const VoiceLine* SelectVoiceLine(const VoiceSet& set, VoiceTrigger trigger) const;
    void ProcessQueue();
    void UpdateCooldowns(float deltaTime);
    
    // Registered content
    std::unordered_map<std::string, VoiceSet> m_voiceSets;
    std::unordered_map<std::string, DialogueSequence> m_dialogues;
    std::unordered_map<AnnouncerType, std::vector<VoiceLine>> m_announcerLines;
    
    // Voice queue
    std::vector<QueuedVoice> m_voiceQueue;
    
    // Current playback
    bool m_isPlaying = false;
    std::string m_currentVoiceSetId;
    VoiceTrigger m_currentTrigger;
    float m_currentDuration = 0.0f;
    float m_currentProgress = 0.0f;
    std::string m_currentSubtitle;
    
    // Dialogue state
    bool m_dialoguePlaying = false;
    std::string m_currentDialogueId;
    int m_currentDialogueIndex = 0;
    float m_dialogueTimer = 0.0f;
    
    // Cooldown tracking
    std::unordered_map<std::string, float> m_cooldowns;
    
    // Settings
    float m_voiceVolume = 1.0f;
    float m_announcerVolume = 1.0f;
    bool m_subtitlesEnabled = true;
    bool m_announcerEnabled = true;
    
    // Timing
    float m_currentTime = 0.0f;
    static constexpr float QUEUE_TIMEOUT = 2.0f;
    
    bool m_initialized = false;
};

} // namespace DDD
