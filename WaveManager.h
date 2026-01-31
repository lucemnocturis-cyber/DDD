#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <string>

namespace DDD {

class Game;
class Dice;

/**
 * Wave configuration
 */
struct WaveConfig {
    int waveNumber = 1;
    int enemyCount = 1;
    int maxTier = 0;
    int goldBonus = 10;
    int scoreBonus = 100;
    bool isBossWave = false;
    std::string bossClass;
};

/**
 * WaveManager - handles enemy wave spawning and progression
 */
class WaveManager {
public:
    explicit WaveManager(Game& game);
    ~WaveManager();
    
    /**
     * Start a new wave
     */
    void StartWave(int waveNumber);
    
    /**
     * Called when all enemies are defeated
     */
    void OnWaveComplete();
    
    /**
     * Get current wave configuration
     */
    const WaveConfig& GetCurrentConfig() const { return m_currentConfig; }
    
    /**
     * Get wave difficulty (0-3)
     */
    int GetDifficulty() const { return m_difficulty; }
    
    /**
     * Get total enemies spawned this wave
     */
    int GetEnemiesSpawned() const { return m_enemiesSpawned; }
    
    /**
     * Get rewards for completing current wave
     */
    int GetWaveGoldReward() const;
    int GetWaveScoreReward() const;
    
    /**
     * Get bonus dice rewards (every 5 waves)
     */
    std::vector<std::shared_ptr<Dice>> GetBonusDiceRewards() const;
    
    // Callbacks
    using WaveStartCallback = std::function<void(int waveNumber, int enemyCount)>;
    void SetOnWaveStart(WaveStartCallback cb) { m_onWaveStart = cb; }
    
    using WaveCompleteCallback = std::function<void(int waveNumber, int goldReward, int scoreReward)>;
    void SetOnWaveComplete(WaveCompleteCallback cb) { m_onWaveComplete = cb; }
    
private:
    WaveConfig GenerateWaveConfig(int waveNumber);
    void SpawnEnemies(const WaveConfig& config);
    std::vector<std::shared_ptr<Dice>> CreateEnemyDice(int count, int maxTier, bool includeBoss, const std::string& bossClass);
    std::string GetRandomEnemyClass(int maxTier);
    
    Game& m_game;
    WaveConfig m_currentConfig;
    int m_difficulty = 0;
    int m_enemiesSpawned = 0;
    
    // Callbacks
    WaveStartCallback m_onWaveStart;
    WaveCompleteCallback m_onWaveComplete;
};

} // namespace DDD
