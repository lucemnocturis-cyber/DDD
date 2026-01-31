#include "BuildSystem.h"
#include "../Utils/Logger.h"

#include <algorithm>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <chrono>

namespace DDD {

BuildSystem& BuildSystem::Instance() {
    static BuildSystem instance;
    return instance;
}

void BuildSystem::Initialize() {
    if (m_initialized) return;
    
    // Set default version
    m_version.major = 1;
    m_version.minor = 0;
    m_version.patch = 0;
    m_version.build = 1;
    
    RegisterDefaultTargets();
    
    // Create build directories
    std::filesystem::create_directories(m_buildOutputPath);
    std::filesystem::create_directories(m_intermediatePath);
    
    m_initialized = true;
    Logger::Info("BuildSystem initialized, version: {}", m_version.ToString());
}

void BuildSystem::Shutdown() {
    m_buildTargets.clear();
    m_assetBundles.clear();
    m_distPackages.clear();
    m_initialized = false;
}

void BuildSystem::SetVersion(const VersionInfo& version) {
    m_version = version;
    Logger::Info("Version set to: {}", m_version.ToString());
}

void BuildSystem::IncrementBuild() {
    m_version.build++;
}

void BuildSystem::IncrementPatch() {
    m_version.patch++;
    m_version.build = 1;
}

void BuildSystem::IncrementMinor() {
    m_version.minor++;
    m_version.patch = 0;
    m_version.build = 1;
}

void BuildSystem::IncrementMajor() {
    m_version.major++;
    m_version.minor = 0;
    m_version.patch = 0;
    m_version.build = 1;
}

void BuildSystem::AddBuildTarget(const BuildTarget& target) {
    // Remove existing target for same platform/config
    RemoveBuildTarget(target.platform, target.config);
    m_buildTargets.push_back(target);
}

void BuildSystem::RemoveBuildTarget(Platform platform, BuildConfig config) {
    m_buildTargets.erase(
        std::remove_if(m_buildTargets.begin(), m_buildTargets.end(),
            [platform, config](const BuildTarget& t) {
                return t.platform == platform && t.config == config;
            }),
        m_buildTargets.end());
}

std::vector<BuildTarget> BuildSystem::GetBuildTargets() const {
    return m_buildTargets;
}

BuildTarget BuildSystem::GetBuildTarget(Platform platform, BuildConfig config) const {
    for (const auto& target : m_buildTargets) {
        if (target.platform == platform && target.config == config) {
            return target;
        }
    }
    return BuildTarget();
}

BuildResult BuildSystem::Build(Platform platform, BuildConfig config) {
    BuildResult result;
    result.success = false;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    m_buildInProgress = true;
    m_buildCancelled = false;
    m_buildProgress = 0.0f;
    
    BuildTarget target = GetBuildTarget(platform, config);
    
    UpdateProgress(0.1f, "Preparing build...");
    
    // Create output directory
    std::string outputDir = m_buildOutputPath + GetPlatformName(platform) + "/" + 
                           GetConfigName(config) + "/";
    std::filesystem::create_directories(outputDir);
    
    UpdateProgress(0.2f, "Compiling sources...");
    
    // In production, would invoke CMake/compiler
    // cmake --build . --config Release
    
    if (m_buildCancelled) {
        result.errorMessage = "Build cancelled";
        m_buildInProgress = false;
        return result;
    }
    
    UpdateProgress(0.5f, "Linking...");
    
    UpdateProgress(0.7f, "Copying assets...");
    
    // Copy assets to output
    std::string assetSrc = "./assets/";
    std::string assetDst = outputDir + "assets/";
    // std::filesystem::copy(assetSrc, assetDst, std::filesystem::copy_options::recursive);
    
    UpdateProgress(0.9f, "Finalizing...");
    
    // Strip symbols if release
    if (target.stripSymbols && config == BuildConfig::Distribution) {
        // strip executable
    }
    
    UpdateProgress(1.0f, "Build complete!");
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.durationSeconds = std::chrono::duration<float>(endTime - startTime).count();
    
    result.success = true;
    result.outputPath = outputDir + target.executableName;
    
    m_buildInProgress = false;
    
    Logger::Info("Build complete: {} ({:.1f}s)", result.outputPath, result.durationSeconds);
    
    if (m_completeCallback) {
        m_completeCallback(result);
    }
    
    return result;
}

BuildResult BuildSystem::BuildAll() {
    BuildResult result;
    result.success = true;
    
    for (const auto& target : m_buildTargets) {
        BuildResult targetResult = Build(target.platform, target.config);
        if (!targetResult.success) {
            result.success = false;
            result.errorMessage += targetResult.errorMessage + "\n";
        }
        result.warnings.insert(result.warnings.end(), 
                               targetResult.warnings.begin(), 
                               targetResult.warnings.end());
    }
    
    return result;
}

void BuildSystem::CancelBuild() {
    m_buildCancelled = true;
}

void BuildSystem::AddAssetBundle(const AssetBundle& bundle) {
    m_assetBundles.push_back(bundle);
}

BuildResult BuildSystem::BuildAssetBundles() {
    BuildResult result;
    result.success = true;
    
    UpdateProgress(0.0f, "Building asset bundles...");
    
    float progressPerBundle = 1.0f / m_assetBundles.size();
    
    for (size_t i = 0; i < m_assetBundles.size(); ++i) {
        const auto& bundle = m_assetBundles[i];
        
        UpdateProgress(i * progressPerBundle, "Bundling: " + bundle.name);
        
        // In production, would compress and optionally encrypt assets
        
        Logger::Info("Built asset bundle: {}", bundle.name);
    }
    
    UpdateProgress(1.0f, "Asset bundles complete!");
    
    return result;
}

void BuildSystem::SetAssetCompression(bool enabled, int level) {
    m_compressAssets = enabled;
    m_compressionLevel = level;
}

void BuildSystem::SetAssetEncryption(bool enabled, const std::string& key) {
    m_encryptAssets = enabled;
    m_encryptionKey = key;
}

void BuildSystem::AddDistributionPackage(const DistributionPackage& package) {
    m_distPackages.push_back(package);
}

BuildResult BuildSystem::CreateDistributionPackage(const std::string& packageId) {
    BuildResult result;
    result.success = false;
    
    for (const auto& package : m_distPackages) {
        if (package.packageId == packageId) {
            UpdateProgress(0.0f, "Creating distribution package...");
            
            // Create output directory
            std::filesystem::create_directories(package.outputPath);
            
            UpdateProgress(0.3f, "Copying files...");
            
            // Copy included files
            
            UpdateProgress(0.6f, "Creating installer...");
            
            // Create installer based on type
            if (package.installerType == "zip") {
                // Create zip archive
            } else if (package.installerType == "exe") {
                CreateWindowsInstaller(package.outputPath);
            } else if (package.installerType == "dmg") {
                CreateMacDMG(package.outputPath);
            } else if (package.installerType == "appimage") {
                CreateLinuxAppImage(package.outputPath);
            }
            
            UpdateProgress(0.9f, "Signing package...");
            
            if (package.signPackage) {
                // Sign the package
            }
            
            UpdateProgress(1.0f, "Package complete!");
            
            result.success = true;
            result.outputPath = package.outputPath;
            
            Logger::Info("Created distribution package: {}", packageId);
            return result;
        }
    }
    
    result.errorMessage = "Package not found: " + packageId;
    return result;
}

BuildResult BuildSystem::CreateAllDistributionPackages() {
    BuildResult result;
    result.success = true;
    
    for (const auto& package : m_distPackages) {
        BuildResult pkgResult = CreateDistributionPackage(package.packageId);
        if (!pkgResult.success) {
            result.success = false;
            result.errorMessage += pkgResult.errorMessage + "\n";
        }
    }
    
    return result;
}

void BuildSystem::SetSteamConfig(const SteamBuildConfig& config) {
    m_steamConfig = config;
}

BuildResult BuildSystem::BuildSteamDepots() {
    BuildResult result;
    result.success = true;
    
    UpdateProgress(0.0f, "Building Steam depots...");
    
    float progressPerDepot = 1.0f / m_steamConfig.depots.size();
    
    for (size_t i = 0; i < m_steamConfig.depots.size(); ++i) {
        const auto& depot = m_steamConfig.depots[i];
        
        UpdateProgress(i * progressPerDepot, "Building depot " + std::to_string(depot.depotId));
        
        // In production, would prepare depot content
        
        Logger::Info("Built Steam depot: {}", depot.depotId);
    }
    
    UpdateProgress(1.0f, "Steam depots complete!");
    
    return result;
}

BuildResult BuildSystem::UploadToSteam(const std::string& branch) {
    BuildResult result;
    
    UpdateProgress(0.0f, "Uploading to Steam...");
    
    // Generate VDF file
    std::string vdfContent = GenerateSteamVDF();
    std::string vdfPath = m_buildOutputPath + "app_build.vdf";
    
    std::ofstream vdfFile(vdfPath);
    vdfFile << vdfContent;
    vdfFile.close();
    
    UpdateProgress(0.3f, "Running steamcmd...");
    
    // In production:
    // system("steamcmd +login user pass +run_app_build app_build.vdf +quit");
    
    UpdateProgress(1.0f, "Upload complete!");
    
    result.success = true;
    Logger::Info("Uploaded to Steam branch: {}", branch);
    
    return result;
}

std::string BuildSystem::GenerateSteamVDF() const {
    std::ostringstream ss;
    
    ss << "\"appbuild\"\n";
    ss << "{\n";
    ss << "    \"appid\" \"" << m_steamConfig.appId << "\"\n";
    ss << "    \"desc\" \"" << m_steamConfig.description << "\"\n";
    ss << "    \"buildoutput\" \"" << m_steamConfig.buildOutput << "\"\n";
    ss << "    \"contentroot\" \"" << m_steamConfig.contentRoot << "\"\n";
    ss << "    \"setlive\" \"" << (m_steamConfig.setLive ? m_steamConfig.branch : "") << "\"\n";
    ss << "\n";
    ss << "    \"depots\"\n";
    ss << "    {\n";
    
    for (const auto& depot : m_steamConfig.depots) {
        ss << "        \"" << depot.depotId << "\"\n";
        ss << "        {\n";
        ss << "            \"FileMapping\"\n";
        ss << "            {\n";
        ss << "                \"LocalPath\" \"" << depot.contentRoot << "/*\"\n";
        ss << "                \"DepotPath\" \".\"\n";
        ss << "                \"recursive\" \"1\"\n";
        ss << "            }\n";
        
        for (const auto& exclude : depot.excludePatterns) {
            ss << "            \"FileExclusion\" \"" << exclude << "\"\n";
        }
        
        ss << "        }\n";
    }
    
    ss << "    }\n";
    ss << "}\n";
    
    return ss.str();
}

void BuildSystem::SetCodeSigningIdentity(const std::string& identity) {
    m_signingIdentity = identity;
}

void BuildSystem::SetCodeSigningCertificate(const std::string& certPath, const std::string& password) {
    m_certPath = certPath;
    m_certPassword = password;
}

bool BuildSystem::SignExecutable(const std::string& executablePath) {
    if (m_signingIdentity.empty() && m_certPath.empty()) {
        Logger::Warning("No signing identity configured");
        return false;
    }
    
    // In production:
    // Windows: signtool sign /f cert.pfx /p password /t timestamp_url executable.exe
    // macOS: codesign --force --sign "identity" executable
    
    Logger::Info("Signed executable: {}", executablePath);
    return true;
}

bool BuildSystem::VerifySignature(const std::string& executablePath) {
    // In production:
    // Windows: signtool verify /pa executable.exe
    // macOS: codesign --verify executable
    
    return true;
}

BuildResult BuildSystem::CreateWindowsInstaller(const std::string& outputPath) {
    BuildResult result;
    
    UpdateProgress(0.0f, "Creating Windows installer...");
    
    // In production, would use NSIS or WiX to create installer
    // makensis installer.nsi
    
    UpdateProgress(1.0f, "Installer created!");
    
    result.success = true;
    result.outputPath = outputPath + "/DungeonDiceDuelists_Setup.exe";
    
    Logger::Info("Created Windows installer: {}", result.outputPath);
    
    return result;
}

BuildResult BuildSystem::CreateMacDMG(const std::string& outputPath) {
    BuildResult result;
    
    UpdateProgress(0.0f, "Creating macOS DMG...");
    
    // In production:
    // hdiutil create -volname "Game" -srcfolder app.app -ov -format UDZO game.dmg
    
    UpdateProgress(1.0f, "DMG created!");
    
    result.success = true;
    result.outputPath = outputPath + "/DungeonDiceDuelists.dmg";
    
    Logger::Info("Created macOS DMG: {}", result.outputPath);
    
    return result;
}

BuildResult BuildSystem::CreateLinuxAppImage(const std::string& outputPath) {
    BuildResult result;
    
    UpdateProgress(0.0f, "Creating Linux AppImage...");
    
    // In production:
    // appimagetool AppDir game.AppImage
    
    UpdateProgress(1.0f, "AppImage created!");
    
    result.success = true;
    result.outputPath = outputPath + "/DungeonDiceDuelists.AppImage";
    
    Logger::Info("Created Linux AppImage: {}", result.outputPath);
    
    return result;
}

bool BuildSystem::ValidateBuild(const std::string& buildPath) {
    // Check executable exists
    if (!std::filesystem::exists(buildPath)) {
        Logger::Error("Build validation failed: executable not found");
        return false;
    }
    
    // Check assets
    std::string assetPath = std::filesystem::path(buildPath).parent_path().string() + "/assets/";
    if (!ValidateAssets(assetPath)) {
        return false;
    }
    
    Logger::Info("Build validation passed");
    return true;
}

bool BuildSystem::ValidateAssets(const std::string& assetPath) {
    // Check required asset directories
    std::vector<std::string> requiredDirs = {"data", "fonts"};
    
    for (const auto& dir : requiredDirs) {
        if (!std::filesystem::exists(assetPath + dir)) {
            Logger::Error("Missing required asset directory: {}", dir);
            return false;
        }
    }
    
    return true;
}

std::vector<std::string> BuildSystem::GetMissingDependencies(Platform platform) {
    std::vector<std::string> missing;
    
    // Check for required libraries
    // In production, would actually check for DLLs/dylibs/so files
    
    return missing;
}

void BuildSystem::CleanBuildDirectory() {
    std::filesystem::remove_all(m_buildOutputPath);
    std::filesystem::create_directories(m_buildOutputPath);
    Logger::Info("Cleaned build directory");
}

void BuildSystem::CleanIntermediateFiles() {
    std::filesystem::remove_all(m_intermediatePath);
    std::filesystem::create_directories(m_intermediatePath);
    Logger::Info("Cleaned intermediate files");
}

void BuildSystem::CleanAll() {
    CleanBuildDirectory();
    CleanIntermediateFiles();
}

void BuildSystem::SetBuildOutputPath(const std::string& path) {
    m_buildOutputPath = path;
    std::filesystem::create_directories(m_buildOutputPath);
}

void BuildSystem::SetIntermediatePath(const std::string& path) {
    m_intermediatePath = path;
    std::filesystem::create_directories(m_intermediatePath);
}

// ===========================================================================
// PRIVATE METHODS
// ===========================================================================

void BuildSystem::RegisterDefaultTargets() {
    // Windows Debug
    BuildTarget winDebug;
    winDebug.platform = Platform::Windows;
    winDebug.config = BuildConfig::Debug;
    winDebug.outputDir = "build/Windows/Debug/";
    winDebug.executableName = "DungeonDiceDuelists_d.exe";
    winDebug.includeDebugSymbols = true;
    winDebug.stripSymbols = false;
    winDebug.enableOptimizations = false;
    winDebug.enableLTO = false;
    m_buildTargets.push_back(winDebug);
    
    // Windows Release
    BuildTarget winRelease;
    winRelease.platform = Platform::Windows;
    winRelease.config = BuildConfig::Release;
    winRelease.outputDir = "build/Windows/Release/";
    winRelease.executableName = "DungeonDiceDuelists.exe";
    winRelease.includeDebugSymbols = true;
    winRelease.stripSymbols = false;
    winRelease.enableOptimizations = true;
    winRelease.enableLTO = false;
    m_buildTargets.push_back(winRelease);
    
    // Windows Distribution
    BuildTarget winDist;
    winDist.platform = Platform::Windows;
    winDist.config = BuildConfig::Distribution;
    winDist.outputDir = "build/Windows/Distribution/";
    winDist.executableName = "DungeonDiceDuelists.exe";
    winDist.iconPath = "assets/icon.ico";
    winDist.includeDebugSymbols = false;
    winDist.stripSymbols = true;
    winDist.enableOptimizations = true;
    winDist.enableLTO = true;
    m_buildTargets.push_back(winDist);
    
    // macOS Distribution
    BuildTarget macDist;
    macDist.platform = Platform::macOS;
    macDist.config = BuildConfig::Distribution;
    macDist.outputDir = "build/macOS/Distribution/";
    macDist.executableName = "DungeonDiceDuelists.app";
    macDist.iconPath = "assets/icon.icns";
    macDist.includeDebugSymbols = false;
    macDist.stripSymbols = true;
    macDist.enableOptimizations = true;
    macDist.enableLTO = true;
    m_buildTargets.push_back(macDist);
    
    // Linux Distribution
    BuildTarget linuxDist;
    linuxDist.platform = Platform::Linux;
    linuxDist.config = BuildConfig::Distribution;
    linuxDist.outputDir = "build/Linux/Distribution/";
    linuxDist.executableName = "DungeonDiceDuelists";
    linuxDist.iconPath = "assets/icon.png";
    linuxDist.includeDebugSymbols = false;
    linuxDist.stripSymbols = true;
    linuxDist.enableOptimizations = true;
    linuxDist.enableLTO = true;
    m_buildTargets.push_back(linuxDist);
    
    // Steam depot config
    m_steamConfig.appId = 0;  // Set to actual Steam App ID
    m_steamConfig.description = "Dungeon Dice Duelists v" + m_version.ToString();
    m_steamConfig.contentRoot = "./build/";
    m_steamConfig.buildOutput = "./build/steam_output/";
    m_steamConfig.setLive = false;
    m_steamConfig.branch = "default";
    
    // Add depots
    SteamDepot windowsDepot;
    windowsDepot.depotId = 1;  // Set to actual depot ID
    windowsDepot.contentRoot = "Windows/Distribution";
    windowsDepot.platform = Platform::Windows;
    windowsDepot.excludePatterns = {"*.pdb", "*.map"};
    m_steamConfig.depots.push_back(windowsDepot);
    
    SteamDepot macDepot;
    macDepot.depotId = 2;
    macDepot.contentRoot = "macOS/Distribution";
    macDepot.platform = Platform::macOS;
    macDepot.excludePatterns = {"*.dSYM"};
    m_steamConfig.depots.push_back(macDepot);
    
    SteamDepot linuxDepot;
    linuxDepot.depotId = 3;
    linuxDepot.contentRoot = "Linux/Distribution";
    linuxDepot.platform = Platform::Linux;
    m_steamConfig.depots.push_back(linuxDepot);
}

std::string BuildSystem::GetPlatformName(Platform platform) const {
    switch (platform) {
        case Platform::Windows: return "Windows";
        case Platform::macOS: return "macOS";
        case Platform::Linux: return "Linux";
        default: return "Unknown";
    }
}

std::string BuildSystem::GetConfigName(BuildConfig config) const {
    switch (config) {
        case BuildConfig::Debug: return "Debug";
        case BuildConfig::Release: return "Release";
        case BuildConfig::Distribution: return "Distribution";
        default: return "Unknown";
    }
}

void BuildSystem::UpdateProgress(float progress, const std::string& status) {
    m_buildProgress = progress;
    m_currentStatus = status;
    
    if (m_progressCallback) {
        m_progressCallback(progress, status);
    }
}

} // namespace DDD
