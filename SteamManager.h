#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace DDD {

/**
 * Steam initialization status
 */
enum class SteamStatus {
    NotInitialized,
    Initializing,
    Ready,
    Failed,
    NoSteamClient,
    AppIdMismatch
};

/**
 * Steam user info
 */
struct SteamUserInfo {
    uint64_t steamId = 0;
    std::string displayName;
    std::string avatarUrl;
    int playerLevel = 0;
    bool isOnline = false;
    std::string countryCode;
};

/**
 * Steam friend info
 */
struct SteamFriend {
    uint64_t steamId = 0;
    std::string displayName;
    bool isOnline = false;
    bool isPlayingThisGame = false;
    std::string currentGame;
};

/**
 * Rich presence keys
 */
struct RichPresenceData {
    std::string status;         // "In Menu", "In Combat", "Boss Fight"
    std::string details;        // "Wave 15/20", "Fighting Ancient Dragon"
    int currentWave = 0;
    int maxWaves = 20;
    int unitsAlive = 0;
    std::string gameMode;
    std::string difficulty;
};

/**
 * Overlay notification position
 */
enum class NotificationPosition {
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};

/**
 * SteamManager - Steam SDK integration
 */
class SteamManager {
public:
    static SteamManager& Instance();
    
    // Initialization
    bool Initialize();
    void Shutdown();
    void Update();  // Call every frame for callbacks
    
    // Status
    SteamStatus GetStatus() const { return m_status; }
    bool IsInitialized() const { return m_status == SteamStatus::Ready; }
    bool IsOverlayEnabled() const { return m_overlayEnabled; }
    bool IsOverlayActive() const { return m_overlayActive; }
    
    // User info
    const SteamUserInfo& GetCurrentUser() const { return m_currentUser; }
    uint64_t GetSteamId() const { return m_currentUser.steamId; }
    std::string GetDisplayName() const { return m_currentUser.displayName; }
    std::string GetLanguage() const { return m_language; }
    
    // Friends
    std::vector<SteamFriend> GetFriends() const;
    int GetFriendCount() const { return static_cast<int>(m_friends.size()); }
    int GetOnlineFriendCount() const;
    bool IsFriend(uint64_t steamId) const;
    
    // Rich Presence
    void SetRichPresence(const RichPresenceData& data);
    void ClearRichPresence();
    void SetRichPresenceKey(const std::string& key, const std::string& value);
    
    // Overlay
    void ActivateOverlay(const std::string& dialog = "");  // "", "Friends", "Community", "Players", "Settings", "OfficialGameGroup", "Stats", "Achievements"
    void ActivateOverlayToWebPage(const std::string& url);
    void ActivateOverlayToStore(uint32_t appId = 0);  // 0 = this game
    void ActivateOverlayToUser(const std::string& dialog, uint64_t steamId);  // "steamid", "chat", "jointrade", "stats", "achievements"
    void SetOverlayNotificationPosition(NotificationPosition position);
    
    // DLC
    bool IsDlcInstalled(uint32_t dlcAppId) const;
    std::vector<uint32_t> GetInstalledDlc() const;
    void InstallDlc(uint32_t dlcAppId);
    
    // App info
    uint32_t GetAppId() const { return m_appId; }
    std::string GetAppInstallDir() const;
    std::string GetCurrentBetaName() const;
    bool IsAppInstalled(uint32_t appId) const;
    bool IsSubscribedApp(uint32_t appId) const;
    
    // Callbacks
    using OverlayCallback = std::function<void(bool active)>;
    void SetOverlayCallback(OverlayCallback callback) { m_overlayCallback = callback; }
    
    using GameJoinRequestCallback = std::function<void(uint64_t friendSteamId)>;
    void SetGameJoinRequestCallback(GameJoinRequestCallback callback) { m_joinRequestCallback = callback; }
    
    // Debug
    bool IsRunningInSteam() const { return m_runningInSteam; }
    void PrintDebugInfo() const;
    
private:
    SteamManager() = default;
    
    void LoadUserInfo();
    void LoadFriendsList();
    void UpdateCallbacks();
    
    // Simulated Steam callbacks (would be real Steam callbacks in production)
    void OnOverlayActivated(bool active);
    void OnGameJoinRequested(uint64_t friendSteamId, const std::string& connectString);
    
    SteamStatus m_status = SteamStatus::NotInitialized;
    uint32_t m_appId = 0;
    std::string m_language = "english";
    
    // User data
    SteamUserInfo m_currentUser;
    std::vector<SteamFriend> m_friends;
    
    // Rich presence
    RichPresenceData m_richPresence;
    std::unordered_map<std::string, std::string> m_richPresenceKeys;
    
    // Overlay
    bool m_overlayEnabled = true;
    bool m_overlayActive = false;
    NotificationPosition m_notificationPosition = NotificationPosition::BottomRight;
    
    // DLC tracking
    std::vector<uint32_t> m_installedDlc;
    
    // Callbacks
    OverlayCallback m_overlayCallback;
    GameJoinRequestCallback m_joinRequestCallback;
    
    // State
    bool m_runningInSteam = false;
    bool m_initialized = false;
};

/**
 * Steam App IDs (would be real IDs in production)
 */
namespace SteamAppIds {
    constexpr uint32_t DungeonDiceDuelists = 0;  // Placeholder
    constexpr uint32_t DLC_ExpansionPack1 = 0;
    constexpr uint32_t DLC_SoundtrackDLC = 0;
}

} // namespace DDD
