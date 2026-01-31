#pragma once

#include "Dice.h"
#include <vector>
#include <memory>

namespace DDD {

/**
 * GameState - tracks overall game progression
 */
struct GameState {
    // Player inventory
    std::vector<std::shared_ptr<Dice>> playerDice;
    std::vector<std::shared_ptr<Dice>> availableDice;
    
    // Resources
    int gold = 5;
    int score = 0;
    
    // Progression
    int currentWave = 1;
    int highestWave = 0;
    int totalWins = 0;
    
    // Session state
    bool isGameOver = false;
    bool isVictory = false;
    
    /**
     * Reset for new game
     */
    void Reset() {
        playerDice.clear();
        availableDice.clear();
        gold = 5;
        score = 0;
        currentWave = 1;
        isGameOver = false;
        isVictory = false;
    }
    
    /**
     * Add starting dice
     */
    void InitializeStartingDice() {
        playerDice.push_back(DiceFactory::CreateDice("Mage"));
        playerDice.push_back(DiceFactory::CreateDice("Soldier"));
        playerDice.push_back(DiceFactory::CreateDice("Rogue"));
        
        for (auto& dice : playerDice) {
            dice->Roll();
        }
    }
};

} // namespace DDD
