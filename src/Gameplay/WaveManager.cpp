#include "WaveManager.h"
#include "../Core/Game.h"
#include "Board.h"
#include "Dice.h"
#include "../Utils/Random.h"
#include "../Utils/Logger.h"

namespace DDD {

WaveManager::WaveManager(Game& game)
    : m_game(game)
{
}

WaveManager::~WaveManager() = default;

WaveConfig WaveManager::GenerateWaveConfig(int waveNumber) {
    WaveConfig config;
    config.waveNumber = waveNumber;
    
    // Difficulty scaling
    if (waveNumber <= 3) {
        // Tutorial waves
        config.enemyCount = waveNumber;
        config.maxTier = 0;
        config.goldBonus = 10 + waveNumber * 5;
        config.scoreBonus = 50 + waveNumber * 25;
        m_difficulty = 0;
    }
    else if (waveNumber <= 5) {
        // Easy waves
        config.enemyCount = 2 + (waveNumber - 3);
        config.maxTier = 0;
        config.goldBonus = 20 + waveNumber * 5;
        config.scoreBonus = 100 + waveNumber * 25;
        m_difficulty = 0;
    }
    else if (waveNumber <= 10) {
        // Medium waves - introduce Tier 1 enemies
        config.enemyCount = 3 + (waveNumber - 5) / 2;
        config.maxTier = 1;
        config.goldBonus = 30 + waveNumber * 8;
        config.scoreBonus = 150 + waveNumber * 30;
        m_difficulty = 1;
    }
    else if (waveNumber <= 15) {
        // Hard waves
        config.enemyCount = 4 + (waveNumber - 10) / 2;
        config.maxTier = 1;
        config.goldBonus = 50 + waveNumber * 10;
        config.scoreBonus = 250 + waveNumber * 40;
        m_difficulty = 2;
    }
    else {
        // Very hard waves
        config.enemyCount = 5 + (waveNumber - 15) / 3;
        config.maxTier = 2;
        config.goldBonus = 75 + waveNumber * 12;
        config.scoreBonus = 400 + waveNumber * 50;
        m_difficulty = 3;
    }
    
    // Boss waves every 5 waves
    if (waveNumber % 5 == 0) {
        config.isBossWave = true;
        config.goldBonus *= 2;
        config.scoreBonus *= 2;
        
        // Select boss based on wave
        if (waveNumber == 5) {
            config.bossClass = "Knight";
        } else if (waveNumber == 10) {
            config.bossClass = "Elementalist";
        } else if (waveNumber == 15) {
            config.bossClass = "Assassin";
        } else {
            // Random boss for later waves
            std::vector<std::string> bossClasses = {"Knight", "Elementalist", "Assassin"};
            config.bossClass = bossClasses[Random::GetInt(0, 2)];
        }
    }
    
    return config;
}

void WaveManager::StartWave(int waveNumber) {
    m_currentConfig = GenerateWaveConfig(waveNumber);
    
    Logger::Info("=== WAVE {} {} ===", 
                 waveNumber, 
                 m_currentConfig.isBossWave ? "(BOSS WAVE)" : "");
    Logger::Info("Enemies: {}, Max Tier: {}, Difficulty: {}",
                 m_currentConfig.enemyCount, m_currentConfig.maxTier, m_difficulty);
    
    SpawnEnemies(m_currentConfig);
    
    if (m_onWaveStart) {
        m_onWaveStart(waveNumber, m_enemiesSpawned);
    }
}

void WaveManager::SpawnEnemies(const WaveConfig& config) {
    auto enemyDice = CreateEnemyDice(
        config.enemyCount, 
        config.maxTier, 
        config.isBossWave, 
        config.bossClass
    );
    
    auto* board = m_game.GetBoard();
    m_enemiesSpawned = 0;
    
    // Enemy spawn zone is top-right area
    int spawnStartX = Board::WIDTH - 5;
    int spawnStartY = 0;
    
    for (auto& dice : enemyDice) {
        bool placed = false;
        
        // Try to place in spawn zone
        for (int attempts = 0; attempts < 50 && !placed; ++attempts) {
            int x = spawnStartX + Random::GetInt(0, 4);
            int y = spawnStartY + Random::GetInt(0, 4);
            
            if (board->IsValidPlacement(*dice, x, y, Owner::Enemy)) {
                board->PlaceDice(dice, x, y, Owner::Enemy);
                placed = true;
                m_enemiesSpawned++;
                
                Logger::Info("Spawned {} at ({}, {})", dice->GetClassName(), x, y);
            }
        }
        
        // Fallback: try any valid position in enemy territory
        if (!placed) {
            for (int y = 0; y < Board::HEIGHT / 2 && !placed; ++y) {
                for (int x = Board::WIDTH / 2; x < Board::WIDTH && !placed; ++x) {
                    if (board->IsValidPlacement(*dice, x, y, Owner::Enemy)) {
                        board->PlaceDice(dice, x, y, Owner::Enemy);
                        placed = true;
                        m_enemiesSpawned++;
                    }
                }
            }
        }
        
        if (!placed) {
            Logger::Warning("Could not place enemy unit: {}", dice->GetClassName());
        }
    }
    
    Logger::Info("Spawned {} enemies for wave {}", m_enemiesSpawned, config.waveNumber);
}

std::vector<std::shared_ptr<Dice>> WaveManager::CreateEnemyDice(
    int count, int maxTier, bool includeBoss, const std::string& bossClass) 
{
    std::vector<std::shared_ptr<Dice>> dice;
    
    // Add boss first if boss wave
    if (includeBoss && !bossClass.empty()) {
        auto bossDice = DiceFactory::CreateDice(bossClass);
        if (bossDice) {
            bossDice->Roll();
            dice.push_back(bossDice);
            count--;  // Boss counts as one enemy
            Logger::Info("Boss spawning: {}", bossClass);
        }
    }
    
    // Add regular enemies
    for (int i = 0; i < count; ++i) {
        std::string className = GetRandomEnemyClass(maxTier);
        auto enemyDice = DiceFactory::CreateDice(className);
        
        if (enemyDice) {
            enemyDice->Roll();
            dice.push_back(enemyDice);
        }
    }
    
    return dice;
}

std::string WaveManager::GetRandomEnemyClass(int maxTier) {
    // Base classes
    std::vector<std::string> tier0Classes = {"Mage", "Soldier", "Rogue"};
    
    // Tier 1 classes
    std::vector<std::string> tier1Classes = {"Knight", "Elementalist", "Assassin"};
    
    // Build pool based on max tier
    std::vector<std::string> pool = tier0Classes;
    
    if (maxTier >= 1) {
        // 40% chance for tier 1 enemy
        if (Random::GetFloat(0.0f, 1.0f) < 0.4f) {
            pool = tier1Classes;
        }
    }
    
    int index = Random::GetInt(0, static_cast<int>(pool.size()) - 1);
    return pool[index];
}

void WaveManager::OnWaveComplete() {
    int goldReward = GetWaveGoldReward();
    int scoreReward = GetWaveScoreReward();
    
    m_game.AddGold(goldReward);
    m_game.AddScore(scoreReward);
    
    Logger::Info("Wave {} complete! +{} Gold, +{} Score",
                 m_currentConfig.waveNumber, goldReward, scoreReward);
    
    if (m_onWaveComplete) {
        m_onWaveComplete(m_currentConfig.waveNumber, goldReward, scoreReward);
    }
}

int WaveManager::GetWaveGoldReward() const {
    return m_currentConfig.goldBonus;
}

int WaveManager::GetWaveScoreReward() const {
    return m_currentConfig.scoreBonus;
}

std::vector<std::shared_ptr<Dice>> WaveManager::GetBonusDiceRewards() const {
    std::vector<std::shared_ptr<Dice>> rewards;
    
    // Every 5 waves, offer a bonus dice
    if (m_currentConfig.waveNumber % 5 == 0) {
        auto baseClasses = DiceFactory::GetBaseClasses();
        
        // Offer 2 dice choices
        for (int i = 0; i < 2; ++i) {
            int idx = Random::GetInt(0, static_cast<int>(baseClasses.size()) - 1);
            auto dice = DiceFactory::CreateDice(baseClasses[idx]);
            if (dice) {
                dice->Roll();
                rewards.push_back(dice);
            }
        }
    }
    
    return rewards;
}

} // namespace DDD
