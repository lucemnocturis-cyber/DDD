#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace DDD {

/**
 * Target platform
 */
enum class Platform {
    Windows,
    macOS,
    Linux,
    All
};

/**
 * Build configuration
 */
enum class BuildConfig {
    Debug,
    Release,
    Distribution    // Release with all optimizations
};

/**
 * Distribution channel
 */
enum class DistributionChannel {
    Steam,
    GOG,
    EpicGames,
    Itch,
    Direct          // Direct download
};

/**
 * Build target info
 */
struct BuildTarget {
    Platform platform;
    BuildConfig config;
    std::string outputDir;
    std::string executableName;
    std::string iconPath;
    bool includeDebugSymbols;
    bool stripSymbols;
    bool enableOptimizations;
    bool enableLTO;             // Link-time optimization
};

/**
 * Asset bundle
 */
struct AssetBundle {
    std::string bundleId;
    std::string name;
    std::vector<std::string> includedPaths;
    std::vector<std::string> excludedPatterns;
    bool compress;
    bool encrypt;
    std::string outputPath;
};

/**
 * Distribution package
 */
struct DistributionPackage {
    std::string packageId;
    std::string version;
    DistributionChannel channel;
    Platform platform;
    std::string outputPath;
    std::vector<std::string> includedFiles;
    std::vector<std::string> excludedFiles;
    std::string installerType;  // "zip", "exe", "dmg", "appimage"
    bool signPackage;
    std::string signIdentity;
};

/**
 * Version info
 */
struct VersionInfo {
    int major = 1;
    int minor = 0;
    int patch = 0;
    int build = 1;
    std::string prerelease = "";    // "alpha", "beta", "rc1"
    std::string metadata = "";       // Build metadata
    
    std::string ToString() const {
        std::string ver = std::to_string(major) + "." + 
                         std::to_string(minor) + "." + 
                         std::to_string(patch);
        if (!prerelease.empty()) ver += "-" + prerelease;
        if (!metadata.empty()) ver += "+" + metadata;
        return ver;
    }
    
    std::string ToFileVersion() const {
        return std::to_string(major) + "." + 
               std::to_string(minor) + "." + 
               std::to_string(patch) + "." +
               std::to_string(build);
    }
};

/**
 * Steam depot configuration
 */
struct SteamDepot {
    uint32_t depotId;
    std::string contentRoot;
    std::vector<std::string> fileMapping;
    std::vector<std::string> excludePatterns;
    Platform platform;
};

/**
 * Steam build configuration
 */
struct SteamBuildConfig {
    uint32_t appId;
    std::string description;
    std::string contentRoot;
    std::string buildOutput;
    std::vector<SteamDepot> depots;
    bool setLive;
    std::string branch;         // "default", "beta", "staging"
};

/**
 * Build result
 */
struct BuildResult {
    bool success;
    std::string outputPath;
    std::string errorMessage;
    float durationSeconds;
    size_t outputSize;
    std::vector<std::string> warnings;
};

/**
 * BuildSystem - manages builds and distribution
 */
class BuildSystem {
public:
    static BuildSystem& Instance();
    
    void Initialize();
    void Shutdown();
    
    // Version management
    void SetVersion(const VersionInfo& version);
    VersionInfo GetVersion() const { return m_version; }
    std::string GetVersionString() const { return m_version.ToString(); }
    void IncrementBuild();
    void IncrementPatch();
    void IncrementMinor();
    void IncrementMajor();
    
    // Build targets
    void AddBuildTarget(const BuildTarget& target);
    void RemoveBuildTarget(Platform platform, BuildConfig config);
    std::vector<BuildTarget> GetBuildTargets() const;
    BuildTarget GetBuildTarget(Platform platform, BuildConfig config) const;
    
    // Building
    BuildResult Build(Platform platform, BuildConfig config);
    BuildResult BuildAll();
    void CancelBuild();
    bool IsBuildInProgress() const { return m_buildInProgress; }
    float GetBuildProgress() const { return m_buildProgress; }
    
    // Asset bundling
    void AddAssetBundle(const AssetBundle& bundle);
    BuildResult BuildAssetBundles();
    void SetAssetCompression(bool enabled, int level = 6);
    void SetAssetEncryption(bool enabled, const std::string& key = "");
    
    // Distribution
    void AddDistributionPackage(const DistributionPackage& package);
    BuildResult CreateDistributionPackage(const std::string& packageId);
    BuildResult CreateAllDistributionPackages();
    
    // Steam-specific
    void SetSteamConfig(const SteamBuildConfig& config);
    BuildResult BuildSteamDepots();
    BuildResult UploadToSteam(const std::string& branch = "default");
    std::string GenerateSteamVDF() const;
    
    // Code signing
    void SetCodeSigningIdentity(const std::string& identity);
    void SetCodeSigningCertificate(const std::string& certPath, const std::string& password);
    bool SignExecutable(const std::string& executablePath);
    bool VerifySignature(const std::string& executablePath);
    
    // Installer creation
    BuildResult CreateWindowsInstaller(const std::string& outputPath);
    BuildResult CreateMacDMG(const std::string& outputPath);
    BuildResult CreateLinuxAppImage(const std::string& outputPath);
    
    // Validation
    bool ValidateBuild(const std::string& buildPath);
    bool ValidateAssets(const std::string& assetPath);
    std::vector<std::string> GetMissingDependencies(Platform platform);
    
    // Clean
    void CleanBuildDirectory();
    void CleanIntermediateFiles();
    void CleanAll();
    
    // Callbacks
    using BuildProgressCallback = std::function<void(float progress, const std::string& status)>;
    using BuildCompleteCallback = std::function<void(const BuildResult& result)>;
    
    void SetProgressCallback(BuildProgressCallback callback) { m_progressCallback = callback; }
    void SetCompleteCallback(BuildCompleteCallback callback) { m_completeCallback = callback; }
    
    // Paths
    void SetBuildOutputPath(const std::string& path);
    void SetIntermediatePath(const std::string& path);
    std::string GetBuildOutputPath() const { return m_buildOutputPath; }
    
private:
    BuildSystem() = default;
    
    void RegisterDefaultTargets();
    std::string GetPlatformName(Platform platform) const;
    std::string GetConfigName(BuildConfig config) const;
    void UpdateProgress(float progress, const std::string& status);
    
    // Version
    VersionInfo m_version;
    
    // Build targets
    std::vector<BuildTarget> m_buildTargets;
    std::vector<AssetBundle> m_assetBundles;
    std::vector<DistributionPackage> m_distPackages;
    
    // Steam
    SteamBuildConfig m_steamConfig;
    
    // Build state
    bool m_buildInProgress = false;
    bool m_buildCancelled = false;
    float m_buildProgress = 0.0f;
    std::string m_currentStatus;
    
    // Paths
    std::string m_buildOutputPath = "./build/";
    std::string m_intermediatePath = "./build/intermediate/";
    
    // Asset settings
    bool m_compressAssets = true;
    int m_compressionLevel = 6;
    bool m_encryptAssets = false;
    std::string m_encryptionKey;
    
    // Code signing
    std::string m_signingIdentity;
    std::string m_certPath;
    std::string m_certPassword;
    
    // Callbacks
    BuildProgressCallback m_progressCallback;
    BuildCompleteCallback m_completeCallback;
    
    bool m_initialized = false;
};

} // namespace DDD
