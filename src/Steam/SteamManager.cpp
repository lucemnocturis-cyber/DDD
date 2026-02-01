#include "SteamManager.h"
#include "../Utils/Logger.h"

// In production, this would include:
// #include <steam/steam_api.h>

#include <algorithm>

namespace DDD {

SteamManager& SteamManager::Instance() {
    static SteamManager instance;
    return instance;
}

bool SteamManager::Initialize() {
    if (m_initialized) return true;
    
    m_status = SteamStatus::Initializing;
    
    // In production, this would call:
    // if (!SteamAPI_Init()) {
    //     m_status = SteamStatus::NoSteamClient;
    //     return false;
    // }
    
    // Simulate Steam initialization for development
    // In production, check if running through Steam client
    m_runningInSteam = false;  // Would check SteamAPI_RestartAppIfNecessary
    
    // For development, simulate successful init
    m_appId = SteamAppIds::DungeonDiceDuelists;
    
    // Load user info
    LoadUserInfo();
    
    // Load friends list
    LoadFriendsList();
    
    // Check for DLC
    // In production: SteamApps()->BIsDlcInstalled(dlcAppId)
    
    m_status = SteamStatus::Ready;
    m_initialized = true;
    
    Logger::Info("SteamManager initialized (Development Mode)");
    Logger::Info("  User: {} (ID: {})", m_currentUser.displayName, m_currentUser.steamId);
    Logger::Info("  Friends: {}", m_friends.size());
    
    return true;
}

void SteamManager::Shutdown() {
    if (!m_initialized) return;
    
    ClearRichPresence();
    
    // In production:
    // SteamAPI_Shutdown();
    
    m_status = SteamStatus::NotInitialized;
    m_initialized = false;
    
    Logger::Info("SteamManager shutdown");
}

void SteamManager::Update() {
    if (!m_initialized) return;
    
    // In production:
    // SteamAPI_RunCallbacks();
    
    UpdateCallbacks();
}

std::vector<SteamFriend> SteamManager::GetFriends() const {
    return m_friends;
}

int SteamManager::GetOnlineFriendCount() const {
    int count = 0;
    for (const auto& f : m_friends) {
        if (f.isOnline) count++;
    }
    return count;
}

bool SteamManager::IsFriend(uint64_t steamId) const {
    for (const auto& f : m_friends) {
        if (f.steamId == steamId) return true;
    }
    return false;
}

void SteamManager::SetRichPresence(const RichPresenceData& data) {
    m_richPresence = data;
    
    // In production, would call SteamFriends()->SetRichPresence for each key
    SetRichPresenceKey("status", data.status);
    SetRichPresenceKey("steam_display", data.status);
    
    if (!data.details.empty()) {
        SetRichPresenceKey("details", data.details);
    }
    
    if (data.currentWave > 0) {
        SetRichPresenceKey("wave", std::to_string(data.currentWave) + "/" + std::to_string(data.maxWaves));
    }
    
    if (!data.gameMode.empty()) {
        SetRichPresenceKey("gamemode", data.gameMode);
    }
    
    if (!data.difficulty.empty()) {
        SetRichPresenceKey("difficulty", data.difficulty);
    }
}

void SteamManager::ClearRichPresence() {
    m_richPresenceKeys.clear();
    m_richPresence = RichPresenceData();
    
    // In production:
    // SteamFriends()->ClearRichPresence();
}

void SteamManager::SetRichPresenceKey(const std::string& key, const std::string& value) {
    m_richPresenceKeys[key] = value;
    
    // In production:
    // SteamFriends()->SetRichPresence(key.c_str(), value.c_str());
}

void SteamManager::ActivateOverlay(const std::string& dialog) {
    if (!m_overlayEnabled) return;
    
    Logger::Info("Activating Steam overlay: {}", dialog.empty() ? "default" : dialog);
    
    // In production:
    // SteamFriends()->ActivateGameOverlay(dialog.c_str());
    
    // Simulate overlay activation
    OnOverlayActivated(true);
}

void SteamManager::ActivateOverlayToWebPage(const std::string& url) {
    if (!m_overlayEnabled) return;
    
    Logger::Info("Opening Steam overlay to: {}", url);
    
    // In production:
    // SteamFriends()->ActivateGameOverlayToWebPage(url.c_str());
}

void SteamManager::ActivateOverlayToStore(uint32_t appId) {
    if (!m_overlayEnabled) return;
    
    if (appId == 0) appId = m_appId;
    
    Logger::Info("Opening Steam store page for app: {}", appId);
    
    // In production:
    // SteamFriends()->ActivateGameOverlayToStore(appId, k_EOverlayToStoreFlag_None);
}

void SteamManager::ActivateOverlayToUser(const std::string& dialog, uint64_t steamId) {
    if (!m_overlayEnabled) return;
    
    Logger::Info("Opening Steam overlay to user {} ({})", steamId, dialog);
    
    // In production:
    // CSteamID id(steamId);
    // SteamFriends()->ActivateGameOverlayToUser(dialog.c_str(), id);
}

void SteamManager::SetOverlayNotificationPosition(NotificationPosition position) {
    m_notificationPosition = position;
    
    // In production:
    // ENotificationPosition pos = k_EPositionBottomRight;
    // switch (position) { ... }
    // SteamUtils()->SetOverlayNotificationPosition(pos);
}

bool SteamManager::IsDlcInstalled(uint32_t dlcAppId) const {
    return std::find(m_installedDlc.begin(), m_installedDlc.end(), dlcAppId) != m_installedDlc.end();
}

std::vector<uint32_t> SteamManager::GetInstalledDlc() const {
    return m_installedDlc;
}

void SteamManager::InstallDlc(uint32_t dlcAppId) {
    Logger::Info("Requesting DLC installation: {}", dlcAppId);
    
    // In production:
    // SteamApps()->InstallDLC(dlcAppId);
}

std::string SteamManager::GetAppInstallDir() const {
    // In production:
    // char path[1024];
    // SteamApps()->GetAppInstallDir(m_appId, path, sizeof(path));
    // return std::string(path);
    
    return "./";  // Development fallback
}

std::string SteamManager::GetCurrentBetaName() const {
    // In production:
    // char name[256];
    // if (SteamApps()->GetCurrentBetaName(name, sizeof(name))) {
    //     return std::string(name);
    // }
    
    return "";  // No beta
}

bool SteamManager::IsAppInstalled(uint32_t appId) const {
    // In production:
    // return SteamApps()->BIsAppInstalled(appId);
    
    return appId == m_appId;
}

bool SteamManager::IsSubscribedApp(uint32_t appId) const {
    // In production:
    // return SteamApps()->BIsSubscribedApp(appId);
    
    return appId == m_appId;
}

void SteamManager::PrintDebugInfo() const {
    Logger::Info("=== Steam Debug Info ===");
    Logger::Info("Status: {}", static_cast<int>(m_status));
    Logger::Info("App ID: {}", m_appId);
    Logger::Info("Language: {}", m_language);
    Logger::Info("User: {} ({})", m_currentUser.displayName, m_currentUser.steamId);
    Logger::Info("Friends: {} ({} online)", m_friends.size(), GetOnlineFriendCount());
    Logger::Info("Overlay enabled: {}", m_overlayEnabled);
    Logger::Info("Running in Steam: {}", m_runningInSteam);
    Logger::Info("Rich Presence Keys:");
    for (const auto& [key, value] : m_richPresenceKeys) {
        Logger::Info("  {}: {}", key, value);
    }
}

// ===========================================================================
// PRIVATE METHODS
// ===========================================================================

void SteamManager::LoadUserInfo() {
    // In production, would use SteamUser() and SteamFriends()
    // CSteamID steamId = SteamUser()->GetSteamID();
    // const char* name = SteamFriends()->GetPersonaName();
    
    // Development simulation
    m_currentUser.steamId = 76561198000000001;  // Fake Steam ID
    m_currentUser.displayName = "DeveloperPlayer";
    m_currentUser.playerLevel = 25;
    m_currentUser.isOnline = true;
    m_currentUser.countryCode = "US";
    
    // Get language
    // In production: SteamApps()->GetCurrentGameLanguage()
    m_language = "english";
}

void SteamManager::LoadFriendsList() {
    // In production:
    // int friendCount = SteamFriends()->GetFriendCount(k_EFriendFlagImmediate);
    // for (int i = 0; i < friendCount; i++) {
    //     CSteamID friendId = SteamFriends()->GetFriendByIndex(i, k_EFriendFlagImmediate);
    //     ...
    // }
    
    // Development simulation - add some fake friends
    m_friends.clear();
    
    m_friends.push_back({
        76561198000000002,
        "FriendOne",
        true,
        false,
        ""
    });
    
    m_friends.push_back({
        76561198000000003,
        "FriendTwo",
        true,
        true,
        "Dungeon Dice Duelists"
    });
    
    m_friends.push_back({
        76561198000000004,
        "FriendThree",
        false,
        false,
        ""
    });
    
    m_friends.push_back({
        76561198000000005,
        "FriendFour",
        true,
        false,
        "Another Game"
    });
}

void SteamManager::UpdateCallbacks() {
    // In production, Steam callbacks would be processed here
    // This includes:
    // - Overlay activation/deactivation
    // - Friend chat messages
    // - Game join requests
    // - Achievement notifications
    // - etc.
}

void SteamManager::OnOverlayActivated(bool active) {
    m_overlayActive = active;
    
    Logger::Info("Steam overlay {}", active ? "activated" : "deactivated");
    
    if (m_overlayCallback) {
        m_overlayCallback(active);
    }
}

void SteamManager::OnGameJoinRequested(uint64_t friendSteamId, const std::string& connectString) {
    Logger::Info("Game join request from friend {} with connect string: {}", 
                 friendSteamId, connectString);
    
    if (m_joinRequestCallback) {
        m_joinRequestCallback(friendSteamId);
    }
}

} // namespace DDD
