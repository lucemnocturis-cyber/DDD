#include "CloudSaveSystem.h"
#include "SteamManager.h"
#include "../Utils/Logger.h"

#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <filesystem>

// In production, would use nlohmann/json
// For now, simple serialization

namespace DDD {

CloudSaveSystem& CloudSaveSystem::Instance() {
    static CloudSaveSystem instance;
    return instance;
}

void CloudSaveSystem::Initialize() {
    if (m_initialized) return;
    
    // Create save directories
    std::filesystem::create_directories(m_savePath);
    std::filesystem::create_directories(m_backupPath);
    
    // Check Steam Cloud availability
    if (SteamManager::Instance().IsInitialized()) {
        // In production: SteamRemoteStorage()->IsCloudEnabledForAccount()
        m_cloudEnabled = true;
    }
    
    // Load save slot cache
    UpdateSaveSlotCache();
    
    m_initialized = true;
    Logger::Info("CloudSaveSystem initialized with {} save slots", m_saveSlots.size());
}

void CloudSaveSystem::Shutdown() {
    // Final auto-save if pending
    if (m_hasAutoSavePending) {
        SaveGame(AUTO_SAVE_SLOT, m_pendingAutoSave);
    }
    
    // Sync with cloud
    if (m_cloudEnabled) {
        SyncWithCloud();
    }
    
    m_initialized = false;
}

void CloudSaveSystem::Update(float deltaTime) {
    // Auto-save timer
    if (m_autoSaveEnabled && m_hasAutoSavePending) {
        m_autoSaveTimer += deltaTime;
        if (m_autoSaveTimer >= m_autoSaveInterval) {
            m_autoSaveTimer = 0.0f;
            SaveGame(AUTO_SAVE_SLOT, m_pendingAutoSave);
            m_hasAutoSavePending = false;
            Logger::Info("Auto-save completed");
        }
    }
}

bool CloudSaveSystem::SaveGame(int slotIndex, const SaveData& data) {
    if (slotIndex < 0 || (slotIndex >= MAX_SAVE_SLOTS && 
        slotIndex != QUICK_SAVE_SLOT && slotIndex != AUTO_SAVE_SLOT)) {
        Logger::Warning("Invalid save slot: {}", slotIndex);
        return false;
    }
    
    // Create mutable copy for checksum
    SaveData saveData = data;
    saveData.timestamp = static_cast<uint64_t>(std::time(nullptr));
    
    // Serialize
    std::string serialized = SerializeSaveData(saveData);
    
    // Calculate checksum
    saveData.checksum = CalculateChecksum(serialized);
    serialized = SerializeSaveData(saveData);  // Re-serialize with checksum
    
    // Write to local file
    std::string filepath = GetSaveFilePath(slotIndex);
    if (!WriteToFile(filepath, serialized)) {
        Logger::Error("Failed to write save file: {}", filepath);
        if (m_saveCallback) m_saveCallback(false, slotIndex);
        return false;
    }
    
    // Upload to Steam Cloud
    if (m_cloudEnabled) {
        std::string cloudFilename = "save_" + std::to_string(slotIndex) + ".sav";
        WriteToCloud(cloudFilename, serialized);
    }
    
    // Update cache
    UpdateSaveSlotCache();
    
    Logger::Info("Game saved to slot {}", slotIndex);
    if (m_saveCallback) m_saveCallback(true, slotIndex);
    
    return true;
}

bool CloudSaveSystem::LoadGame(int slotIndex, SaveData& outData) {
    if (!SaveExists(slotIndex)) {
        Logger::Warning("Save slot {} does not exist", slotIndex);
        if (m_loadCallback) m_loadCallback(false, slotIndex);
        return false;
    }
    
    // Try cloud first if enabled
    std::string data;
    bool loaded = false;
    
    if (m_cloudEnabled) {
        std::string cloudFilename = "save_" + std::to_string(slotIndex) + ".sav";
        loaded = ReadFromCloud(cloudFilename, data);
    }
    
    // Fall back to local
    if (!loaded) {
        std::string filepath = GetSaveFilePath(slotIndex);
        loaded = ReadFromFile(filepath, data);
    }
    
    if (!loaded) {
        Logger::Error("Failed to read save slot {}", slotIndex);
        if (m_loadCallback) m_loadCallback(false, slotIndex);
        return false;
    }
    
    // Deserialize
    if (!DeserializeSaveData(data, outData)) {
        Logger::Error("Failed to parse save data for slot {}", slotIndex);
        if (m_loadCallback) m_loadCallback(false, slotIndex);
        return false;
    }
    
    // Validate checksum
    uint32_t storedChecksum = outData.checksum;
    outData.checksum = 0;
    std::string serialized = SerializeSaveData(outData);
    uint32_t calculatedChecksum = CalculateChecksum(serialized);
    outData.checksum = storedChecksum;
    
    if (storedChecksum != calculatedChecksum) {
        Logger::Warning("Save checksum mismatch for slot {} (stored: {}, calculated: {})",
                       slotIndex, storedChecksum, calculatedChecksum);
        // Continue anyway but warn
    }
    
    Logger::Info("Game loaded from slot {}", slotIndex);
    if (m_loadCallback) m_loadCallback(true, slotIndex);
    
    return true;
}

bool CloudSaveSystem::DeleteSave(int slotIndex) {
    std::string filepath = GetSaveFilePath(slotIndex);
    
    // Delete local file
    if (std::filesystem::exists(filepath)) {
        std::filesystem::remove(filepath);
    }
    
    // Delete from cloud
    if (m_cloudEnabled) {
        std::string cloudFilename = "save_" + std::to_string(slotIndex) + ".sav";
        // In production: SteamRemoteStorage()->FileDelete(cloudFilename.c_str());
    }
    
    UpdateSaveSlotCache();
    Logger::Info("Deleted save slot {}", slotIndex);
    
    return true;
}

bool CloudSaveSystem::SaveExists(int slotIndex) const {
    std::string filepath = GetSaveFilePath(slotIndex);
    return std::filesystem::exists(filepath);
}

void CloudSaveSystem::EnableAutoSave(bool enabled, float intervalSeconds) {
    m_autoSaveEnabled = enabled;
    m_autoSaveInterval = intervalSeconds;
    m_autoSaveTimer = 0.0f;
}

void CloudSaveSystem::TriggerAutoSave() {
    m_hasAutoSavePending = true;
    m_autoSaveTimer = m_autoSaveInterval;  // Trigger immediately
}

bool CloudSaveSystem::QuickSave(const SaveData& data) {
    return SaveGame(QUICK_SAVE_SLOT, data);
}

bool CloudSaveSystem::QuickLoad(SaveData& outData) {
    return LoadGame(QUICK_SAVE_SLOT, outData);
}

bool CloudSaveSystem::SaveProfile(const ProfileData& data) {
    std::string serialized = SerializeProfileData(data);
    std::string filepath = GetProfileFilePath();
    
    if (!WriteToFile(filepath, serialized)) {
        Logger::Error("Failed to save profile");
        return false;
    }
    
    if (m_cloudEnabled) {
        WriteToCloud("profile.dat", serialized);
    }
    
    Logger::Info("Profile saved");
    return true;
}

bool CloudSaveSystem::LoadProfile(ProfileData& outData) {
    std::string data;
    bool loaded = false;
    
    if (m_cloudEnabled) {
        loaded = ReadFromCloud("profile.dat", data);
    }
    
    if (!loaded) {
        std::string filepath = GetProfileFilePath();
        loaded = ReadFromFile(filepath, data);
    }
    
    if (!loaded) {
        // No profile exists, use defaults
        outData = ProfileData();
        return true;
    }
    
    return DeserializeProfileData(data, outData);
}

std::vector<SaveSlot> CloudSaveSystem::GetAllSaveSlots() const {
    return m_saveSlots;
}

SaveSlot CloudSaveSystem::GetSaveSlotInfo(int slotIndex) const {
    for (const auto& slot : m_saveSlots) {
        if (slot.slotIndex == slotIndex) {
            return slot;
        }
    }
    
    // Return empty slot
    SaveSlot empty;
    empty.slotIndex = slotIndex;
    empty.filename = "";
    return empty;
}

int CloudSaveSystem::GetNextAvailableSlot() const {
    for (int i = 0; i < MAX_SAVE_SLOTS; ++i) {
        if (!SaveExists(i)) {
            return i;
        }
    }
    return -1;  // All slots full
}

void CloudSaveSystem::SyncWithCloud() {
    if (!m_cloudEnabled) {
        m_syncStatus = CloudSyncStatus::Disabled;
        return;
    }
    
    m_syncStatus = CloudSyncStatus::Syncing;
    
    // In production, would use Steam Cloud API
    // SteamRemoteStorage()->FileExists(), FileRead(), FileWrite()
    
    CheckForConflicts();
    
    if (m_hasConflict) {
        m_syncStatus = CloudSyncStatus::Conflict;
    } else {
        m_syncStatus = CloudSyncStatus::Synced;
    }
    
    if (m_syncCallback) {
        m_syncCallback(m_syncStatus);
    }
    
    Logger::Info("Cloud sync complete, status: {}", static_cast<int>(m_syncStatus));
}

void CloudSaveSystem::ForceUploadToCloud() {
    if (!m_cloudEnabled) return;
    
    for (int i = 0; i < MAX_SAVE_SLOTS; ++i) {
        if (SaveExists(i)) {
            std::string data;
            std::string filepath = GetSaveFilePath(i);
            if (ReadFromFile(filepath, data)) {
                std::string cloudFilename = "save_" + std::to_string(i) + ".sav";
                WriteToCloud(cloudFilename, data);
            }
        }
    }
    
    // Upload profile
    std::string profileData;
    if (ReadFromFile(GetProfileFilePath(), profileData)) {
        WriteToCloud("profile.dat", profileData);
    }
    
    Logger::Info("Forced upload to cloud complete");
}

void CloudSaveSystem::ForceDownloadFromCloud() {
    if (!m_cloudEnabled) return;
    
    for (int i = 0; i < MAX_SAVE_SLOTS; ++i) {
        std::string cloudFilename = "save_" + std::to_string(i) + ".sav";
        std::string data;
        if (ReadFromCloud(cloudFilename, data)) {
            std::string filepath = GetSaveFilePath(i);
            WriteToFile(filepath, data);
        }
    }
    
    // Download profile
    std::string profileData;
    if (ReadFromCloud("profile.dat", profileData)) {
        WriteToFile(GetProfileFilePath(), profileData);
    }
    
    UpdateSaveSlotCache();
    Logger::Info("Forced download from cloud complete");
}

void CloudSaveSystem::SetCloudEnabled(bool enabled) {
    m_cloudEnabled = enabled;
    if (enabled) {
        SyncWithCloud();
    }
}

void CloudSaveSystem::SetConflictResolution(ConflictResolution resolution) {
    m_conflictResolution = resolution;
}

void CloudSaveSystem::ResolveConflict(bool useCloud) {
    if (!m_hasConflict) return;
    
    if (useCloud) {
        ForceDownloadFromCloud();
    } else {
        ForceUploadToCloud();
    }
    
    m_hasConflict = false;
    m_syncStatus = CloudSyncStatus::Synced;
}

bool CloudSaveSystem::CreateBackup(int slotIndex) {
    if (!SaveExists(slotIndex)) return false;
    
    std::string source = GetSaveFilePath(slotIndex);
    std::string backup = GetBackupFilePath(slotIndex);
    
    try {
        std::filesystem::copy_file(source, backup, 
            std::filesystem::copy_options::overwrite_existing);
        Logger::Info("Created backup for slot {}", slotIndex);
        return true;
    } catch (const std::exception& e) {
        Logger::Error("Failed to create backup: {}", e.what());
        return false;
    }
}

bool CloudSaveSystem::RestoreFromBackup(int slotIndex) {
    std::string backup = GetBackupFilePath(slotIndex);
    if (!std::filesystem::exists(backup)) {
        Logger::Warning("No backup exists for slot {}", slotIndex);
        return false;
    }
    
    std::string dest = GetSaveFilePath(slotIndex);
    
    try {
        std::filesystem::copy_file(backup, dest,
            std::filesystem::copy_options::overwrite_existing);
        UpdateSaveSlotCache();
        Logger::Info("Restored backup for slot {}", slotIndex);
        return true;
    } catch (const std::exception& e) {
        Logger::Error("Failed to restore backup: {}", e.what());
        return false;
    }
}

std::vector<std::string> CloudSaveSystem::GetBackupList() const {
    std::vector<std::string> backups;
    
    for (const auto& entry : std::filesystem::directory_iterator(m_backupPath)) {
        if (entry.is_regular_file()) {
            backups.push_back(entry.path().filename().string());
        }
    }
    
    return backups;
}

bool CloudSaveSystem::ExportSave(int slotIndex, const std::string& filepath) {
    if (!SaveExists(slotIndex)) return false;
    
    std::string data;
    std::string source = GetSaveFilePath(slotIndex);
    if (!ReadFromFile(source, data)) return false;
    
    return WriteToFile(filepath, data);
}

bool CloudSaveSystem::ImportSave(const std::string& filepath, int slotIndex) {
    std::string data;
    if (!ReadFromFile(filepath, data)) return false;
    
    // Validate it's a valid save
    SaveData temp;
    if (!DeserializeSaveData(data, temp)) {
        Logger::Error("Invalid save file format");
        return false;
    }
    
    std::string dest = GetSaveFilePath(slotIndex);
    if (!WriteToFile(dest, data)) return false;
    
    UpdateSaveSlotCache();
    return true;
}

bool CloudSaveSystem::ValidateSave(int slotIndex) const {
    if (!SaveExists(slotIndex)) return false;
    
    SaveData data;
    std::string filepath = GetSaveFilePath(slotIndex);
    std::string content;
    
    if (!ReadFromFile(filepath, content)) return false;
    if (!const_cast<CloudSaveSystem*>(this)->DeserializeSaveData(content, data)) return false;
    
    return ValidateSaveData(data);
}

bool CloudSaveSystem::ValidateSaveData(const SaveData& data) const {
    // Basic validation
    if (data.version == 0) return false;
    if (data.currentWave < 1 || data.currentWave > 100) return false;
    if (data.goldAmount < 0) return false;
    
    return true;
}

void CloudSaveSystem::PrintSaveInfo(int slotIndex) const {
    SaveSlot slot = GetSaveSlotInfo(slotIndex);
    
    Logger::Info("=== Save Slot {} ===", slotIndex);
    Logger::Info("Name: {}", slot.displayName);
    Logger::Info("Date: {}", slot.dateString);
    Logger::Info("Wave: {}", slot.waveNumber);
    Logger::Info("Gold: {}", slot.goldAmount);
    Logger::Info("Difficulty: {}", slot.difficulty);
    Logger::Info("Play Time: {:.1f} hours", slot.playTime / 3600.0f);
    Logger::Info("Cloud Synced: {}", slot.isCloudSynced);
}

uint64_t CloudSaveSystem::GetCloudQuotaUsed() const {
    // In production: SteamRemoteStorage()->GetQuota()
    return 0;
}

uint64_t CloudSaveSystem::GetCloudQuotaTotal() const {
    // In production: SteamRemoteStorage()->GetQuota()
    return 100 * 1024 * 1024;  // 100 MB typical
}

// ===========================================================================
// PRIVATE METHODS
// ===========================================================================

std::string CloudSaveSystem::GetSaveFilePath(int slotIndex) const {
    if (slotIndex == QUICK_SAVE_SLOT) {
        return m_savePath + "quicksave.sav";
    } else if (slotIndex == AUTO_SAVE_SLOT) {
        return m_savePath + "autosave.sav";
    }
    return m_savePath + "save_" + std::to_string(slotIndex) + ".sav";
}

std::string CloudSaveSystem::GetProfileFilePath() const {
    return m_savePath + "profile.dat";
}

std::string CloudSaveSystem::GetBackupFilePath(int slotIndex) const {
    return m_backupPath + "save_" + std::to_string(slotIndex) + "_backup.sav";
}

bool CloudSaveSystem::WriteToFile(const std::string& filepath, const std::string& data) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;
    
    file.write(data.c_str(), data.size());
    return file.good();
}

bool CloudSaveSystem::ReadFromFile(const std::string& filepath, std::string& outData) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    outData = buffer.str();
    return true;
}

bool CloudSaveSystem::WriteToCloud(const std::string& filename, const std::string& data) {
    if (!m_cloudEnabled) return false;
    
    // In production:
    // return SteamRemoteStorage()->FileWrite(filename.c_str(), data.c_str(), data.size());
    
    return true;  // Simulated success
}

bool CloudSaveSystem::ReadFromCloud(const std::string& filename, std::string& outData) {
    if (!m_cloudEnabled) return false;
    
    // In production:
    // int32 size = SteamRemoteStorage()->GetFileSize(filename.c_str());
    // if (size == 0) return false;
    // char* buffer = new char[size];
    // SteamRemoteStorage()->FileRead(filename.c_str(), buffer, size);
    // outData = std::string(buffer, size);
    // delete[] buffer;
    
    return false;  // No cloud data in simulation
}

std::string CloudSaveSystem::SerializeSaveData(const SaveData& data) const {
    // Simple text serialization (would use JSON in production)
    std::stringstream ss;
    ss << "VERSION:" << data.version << "\n";
    ss << "CHECKSUM:" << data.checksum << "\n";
    ss << "TIMESTAMP:" << data.timestamp << "\n";
    ss << "PLAYTIME:" << data.totalPlayTime << "\n";
    ss << "WAVE:" << data.currentWave << "\n";
    ss << "GOLD:" << data.goldAmount << "\n";
    ss << "SCORE:" << data.score << "\n";
    ss << "DIFFICULTY:" << data.difficulty << "\n";
    ss << "GAMEMODE:" << data.gameMode << "\n";
    ss << "UNITS:" << data.playerUnits.size() << "\n";
    for (const auto& unit : data.playerUnits) {
        ss << "UNIT:" << unit.classId << "," << unit.level << "," 
           << unit.currentHp << "," << unit.maxHp << "\n";
    }
    ss << "STATS:" << data.totalEnemiesKilled << "," << data.totalDamageDealt 
       << "," << data.totalGoldEarned << "," << data.gamesPlayed 
       << "," << data.gamesWon << "\n";
    return ss.str();
}

bool CloudSaveSystem::DeserializeSaveData(const std::string& content, SaveData& outData) const {
    std::istringstream ss(content);
    std::string line;
    
    outData = SaveData();  // Reset to defaults
    
    while (std::getline(ss, line)) {
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) continue;
        
        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        
        if (key == "VERSION") outData.version = std::stoul(value);
        else if (key == "CHECKSUM") outData.checksum = std::stoul(value);
        else if (key == "TIMESTAMP") outData.timestamp = std::stoull(value);
        else if (key == "PLAYTIME") outData.totalPlayTime = std::stof(value);
        else if (key == "WAVE") outData.currentWave = std::stoi(value);
        else if (key == "GOLD") outData.goldAmount = std::stoi(value);
        else if (key == "SCORE") outData.score = std::stoi(value);
        else if (key == "DIFFICULTY") outData.difficulty = value;
        else if (key == "GAMEMODE") outData.gameMode = value;
    }
    
    return outData.version > 0;
}

std::string CloudSaveSystem::SerializeProfileData(const ProfileData& data) const {
    std::stringstream ss;
    ss << "VERSION:" << data.version << "\n";
    ss << "PLAYTIME:" << data.totalPlayTime << "\n";
    ss << "GAMESPLAYED:" << data.totalGamesPlayed << "\n";
    ss << "GAMESWON:" << data.totalGamesWon << "\n";
    ss << "HIGHWAVE:" << data.highestWaveReached << "\n";
    ss << "HIGHSCORE:" << data.highestScore << "\n";
    ss << "VOLUME:" << data.masterVolume << "," << data.musicVolume 
       << "," << data.sfxVolume << "\n";
    ss << "DISPLAY:" << data.fullscreen << "," << data.resolutionWidth 
       << "," << data.resolutionHeight << "\n";
    return ss.str();
}

bool CloudSaveSystem::DeserializeProfileData(const std::string& content, ProfileData& outData) const {
    std::istringstream ss(content);
    std::string line;
    
    outData = ProfileData();
    
    while (std::getline(ss, line)) {
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) continue;
        
        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        
        if (key == "VERSION") outData.version = std::stoul(value);
        else if (key == "PLAYTIME") outData.totalPlayTime = std::stof(value);
        else if (key == "GAMESPLAYED") outData.totalGamesPlayed = std::stoi(value);
        else if (key == "GAMESWON") outData.totalGamesWon = std::stoi(value);
        else if (key == "HIGHWAVE") outData.highestWaveReached = std::stoi(value);
        else if (key == "HIGHSCORE") outData.highestScore = std::stoi(value);
    }
    
    return outData.version > 0;
}

uint32_t CloudSaveSystem::CalculateChecksum(const std::string& data) const {
    // Simple checksum (would use CRC32 in production)
    uint32_t checksum = 0;
    for (char c : data) {
        checksum = (checksum << 5) + checksum + static_cast<uint32_t>(c);
    }
    return checksum;
}

std::string CloudSaveSystem::GetTimestampString(uint64_t timestamp) const {
    std::time_t time = static_cast<std::time_t>(timestamp);
    std::tm* tm = std::localtime(&time);
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", tm);
    return std::string(buffer);
}

void CloudSaveSystem::UpdateSaveSlotCache() {
    m_saveSlots.clear();
    
    for (int i = 0; i < MAX_SAVE_SLOTS; ++i) {
        if (SaveExists(i)) {
            SaveSlot slot;
            slot.slotIndex = i;
            slot.filename = GetSaveFilePath(i);
            
            // Load save to get info
            SaveData data;
            if (LoadGame(i, data)) {
                slot.timestamp = data.timestamp;
                slot.dateString = GetTimestampString(data.timestamp);
                slot.waveNumber = data.currentWave;
                slot.goldAmount = data.goldAmount;
                slot.difficulty = data.difficulty;
                slot.gameMode = data.gameMode;
                slot.playTime = data.totalPlayTime;
                slot.displayName = "Save " + std::to_string(i + 1) + " - Wave " + std::to_string(data.currentWave);
                slot.isAutoSave = false;
                slot.isCloudSynced = m_cloudEnabled;
                slot.checksum = data.checksum;
            }
            
            m_saveSlots.push_back(slot);
        }
    }
    
    // Check special slots
    if (SaveExists(AUTO_SAVE_SLOT)) {
        SaveSlot slot;
        slot.slotIndex = AUTO_SAVE_SLOT;
        slot.displayName = "Auto Save";
        slot.isAutoSave = true;
        m_saveSlots.push_back(slot);
    }
    
    if (SaveExists(QUICK_SAVE_SLOT)) {
        SaveSlot slot;
        slot.slotIndex = QUICK_SAVE_SLOT;
        slot.displayName = "Quick Save";
        slot.isAutoSave = false;
        m_saveSlots.push_back(slot);
    }
}

void CloudSaveSystem::CheckForConflicts() {
    // In production, compare local and cloud timestamps
    // For now, assume no conflicts
    m_hasConflict = false;
}

} // namespace DDD
