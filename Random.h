#pragma once

#include <random>

namespace DDD {

/**
 * Random number generation utilities
 */
class Random {
public:
    /**
     * Initialize the random number generator
     */
    static void Initialize(unsigned int seed = 0) {
        if (seed == 0) {
            std::random_device rd;
            s_engine.seed(rd());
        } else {
            s_engine.seed(seed);
        }
    }
    
    /**
     * Get random integer in range [min, max]
     */
    static int GetInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(s_engine);
    }
    
    /**
     * Get random float in range [min, max]
     */
    static float GetFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(s_engine);
    }
    
    /**
     * Get random boolean with given probability of true
     */
    static bool GetBool(float probability = 0.5f) {
        return GetFloat(0.0f, 1.0f) < probability;
    }
    
    /**
     * Roll dice (1 to sides)
     */
    static int RollDice(int sides = 6) {
        return GetInt(1, sides);
    }
    
private:
    static std::mt19937 s_engine;
};

} // namespace DDD
