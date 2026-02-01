#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace DDD {

/**
 * Save slot info
 */
struct SaveSlot {
    int slotIndex;
    std::string filename;
    std::string displayName;
    uint64_t timestamp;         // Unix timestamp
    std::string dateString;     // Human readable
    int waveNumber;
    int goldAmount;
    std::string difficulty;
    std::string gameMode;
    float playTime;             // In seconds
    bool isAutoSave;
    bool isCloudSynced;
    uint32_t checksum;
};

/**
 * Game save data structure
 */
struct SaveData {
    // Version
    uint32_t version = 1;
    uint32_t checksum = 0;
    
    // Meta
    uint64_t timestamp = 0;
    float totalPlayTime = 0.0f;
    
    // Game state
    int currentWave = 1;
    int goldAmount = 100;
    int score = 0;
    std::string difficulty = "normal";
    std::string gameMode = "campaign";
    
    // Player units
    struct UnitSave {
        std::string classId;
        std::string name;
        int level;
        int experience;
        int currentHp;
        int maxHp;
        int gridX, gridY;
        std::vector<std::string> abilities;
        std::vector<std::string> equipment;
    };
    std::vector<UnitSave> playerUnits;
    
    // Inventory
    struct InventoryItem {
        std::string itemId;
        int quantity;
    };
    std::vector<InventoryItem> inventory;
    
    // Dice collection
    struct DiceSave {
        std::string diceId;
        int quantity;
    };
    std::vector<DiceSave> diceCollection;
    
    // Unlocks
    std::vector<std::string> unlockedClasses;
    std::vector<std::string> unlockedDice;
    std::vector<std::string> unlockedItems;
    
    // Statistics
    int totalEnemiesKilled = 0;
    int totalDamageDealt = 0;
    int totalGoldEarned = 0;
    int gamesPlayed = 0;
    int gamesWon = 0;
    
    // Settings (saved per-slot for convenience)
    float masterVolume = 1.0f;
    float musicVolume = 0.7f;
    float sfxVolume = 1.0f;
    bool subtitlesEnabled = true;
};

/**
 * Profile data (global, not per-save)
 */
struct ProfileData {
    uint32_t version = 1;
    
    // Lifetime stats
    float totalPlayTime = 0.0f;
    int totalGamesPlayed = 0;
    int totalGamesWon = 0;
    int totalEnemiesKilled = 0;
    int highestWaveReached = 0;
    int highestScore = 0;
    
    // Unlocks (persistent across saves)
    std::vector<std::string> unlockedAchievements;
    std::vector<std::string> unlockedTitles;
    std::vector<std::string> unlockedSkins;
    
    // Preferences
    std::string lastDifficulty = "normal";
    std::string lastGameMode = "campaign";
    int lastSaveSlot = 0;
    
    // Settings
    float masterVolume = 1.0f;
    float musicVolume = 0.7f;
    float sfxVolume = 1.0f;
    float voiceVolume = 1.0f;
    bool fullscreen = true;
    int resolutionWidth = 1920;
    int resolutionHeight = 1080;
    bool vsync = true;
    bool subtitlesEnabled = true;
    std::string language = "english";
};

/**
 * Cloud sync status
 */
enum class CloudSyncStatus {
    Synced,
    Syncing,
    Pending,
    Conflict,
    Error,
    Disabled
};

/**
 * Sync conflict resolution
 */
enum class ConflictResolution {
    UseLocal,
    UseCloud,
    UseNewest,
    AskUser
};

/**
 * CloudSaveSystem - manages save games with Steam Cloud
 */
class CloudSaveSystem {
public:
    static CloudSaveSystem& Instance();
    
    void Initialize();
    void Shutdown();
    void Update(float deltaTime);
    
    // Save operations
    bool SaveGame(int slotIndex, const SaveData& data);
    bool LoadGame(int slotIndex, SaveData& outData);
    bool DeleteSave(int slotIndex);
    bool SaveExists(int slotIndex) const;
    
    // Auto-save
    void EnableAutoSave(bool enabled, float intervalSeconds = 60.0f);
    void TriggerAutoSave();
    bool IsAutoSaveEnabled() const { return m_autoSaveEnabled; }
    
    // Quick save/load
    bool QuickSave(const SaveData& data);
    bool QuickLoad(SaveData& outData);
    
    // Profile (global data)
    bool SaveProfile(const ProfileData& data);
    bool LoadProfile(ProfileData& outData);
    
    // Slot management
    std::vector<SaveSlot> GetAllSaveSlots() const;
    SaveSlot GetSaveSlotInfo(int slotIndex) const;
    int GetNextAvailableSlot() const;
    int GetSaveSlotCount() const { return MAX_SAVE_SLOTS; }
    
    // Cloud sync
    void SyncWithCloud();
    void ForceUploadToCloud();
    void ForceDownloadFromCloud();
    CloudSyncStatus GetSyncStatus() const { return m_syncStatus; }
    bool IsCloudEnabled() const { return m_cloudEnabled; }
    void SetCloudEnabled(bool enabled);
    
    // Conflict handling
    void SetConflictResolution(ConflictResolution resolution);
    bool HasConflict() const { return m_hasConflict; }
    void ResolveConflict(bool useCloud);
    
    // Backup/restore
    bool CreateBackup(int slotIndex);
    bool RestoreFromBackup(int slotIndex);
    std::vector<std::string> GetBackupList() const;
    
    // Import/Export (for manual backup)
    bool ExportSave(int slotIndex, const std::string& filepath);
    bool ImportSave(const std::string& filepath, int slotIndex);
    
    // Validation
    bool ValidateSave(int slotIndex) const;
    bool ValidateSaveData(const SaveData& data) const;
    
    // Callbacks
    using SaveCompleteCallback = std::function<void(bool success, int slotIndex)>;
    using LoadCompleteCallback = std::function<void(bool success, int slotIndex)>;
    using SyncCompleteCallback = std::function<void(CloudSyncStatus status)>;
    using ConflictCallback = std::function<void(const SaveSlot& local, const SaveSlot& cloud)>;
    
    void SetSaveCallback(SaveCompleteCallback callback) { m_saveCallback = callback; }
    void SetLoadCallback(LoadCompleteCallback callback) { m_loadCallback = callback; }
    void SetSyncCallback(SyncCompleteCallback callback) { m_syncCallback = callback; }
    void SetConflictCallback(ConflictCallback callback) { m_conflictCallback = callback; }
    
    // Debug
    void PrintSaveInfo(int slotIndex) const;
    uint64_t GetCloudQuotaUsed() const;
    uint64_t GetCloudQuotaTotal() const;
    
private:
    CloudSaveSystem() = default;
    
    std::string GetSaveFilePath(int slotIndex) const;
    std::string GetProfileFilePath() const;
    std::string GetBackupFilePath(int slotIndex) const;
    
    bool WriteToFile(const std::string& filepath, const std::string& data);
    bool ReadFromFile(const std::string& filepath, std::string& outData);
    bool WriteToCloud(const std::string& filename, const std::string& data);
    bool ReadFromCloud(const std::string& filename, std::string& outData);
    
    std::string SerializeSaveData(const SaveData& data) const;
    bool DeserializeSaveData(const std::string& json, SaveData& outData) const;
    std::string SerializeProfileData(const ProfileData& data) const;
    bool DeserializeProfileData(const std::string& json, ProfileData& outData) const;
    
    uint32_t CalculateChecksum(const std::string& data) const;
    std::string GetTimestampString(uint64_t timestamp) const;
    
    void UpdateSaveSlotCache();
    void CheckForConflicts();
    
    static constexpr int MAX_SAVE_SLOTS = 10;
    static constexpr int QUICK_SAVE_SLOT = 99;
    static constexpr int AUTO_SAVE_SLOT = 98;
    
    // Save slot cache
    std::vector<SaveSlot> m_saveSlots;
    
    // Cloud state
    CloudSyncStatus m_syncStatus = CloudSyncStatus::Synced;
    bool m_cloudEnabled = true;
    bool m_hasConflict = false;
    ConflictResolution m_conflictResolution = ConflictResolution::UseNewest;
    
    // Auto-save
    bool m_autoSaveEnabled = true;
    float m_autoSaveInterval = 60.0f;
    float m_autoSaveTimer = 0.0f;
    SaveData m_pendingAutoSave;
    bool m_hasAutoSavePending = false;
    
    // Callbacks
    SaveCompleteCallback m_saveCallback;
    LoadCompleteCallback m_loadCallback;
    SyncCompleteCallback m_syncCallback;
    ConflictCallback m_conflictCallback;
    
    // Paths
    std::string m_savePath = "./saves/";
    std::string m_backupPath = "./saves/backups/";
    
    bool m_initialized = false;
};

} // namespace DDD
