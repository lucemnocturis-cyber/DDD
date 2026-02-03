#pragma once
#include <random>
#include <ctime>

namespace DDD {

/**
 * Random number generation utilities
 */
class Random {
public:
    /**
     * Seed the random number generator with a specific value
     */
    static void Seed(unsigned int seed);
    
    /**
     * Seed with current time
     */
    static void SeedWithTime();
    
    /**
     * Initialize the random number generator
     */
    static void Initialize(unsigned int seed = 0) {
        if (seed == 0) {
            SeedWithTime();
        } else {
            Seed(seed);
        }
    }
    
    /**
     * Get the random engine (auto-seeds if needed)
     */
    static std::mt19937& GetEngine();
    
    /**
     * Get random integer in range [min, max]
     */
    static int Range(int min, int max);
    
    /**
     * Get random float in range [min, max]
     */
    static float Range(float min, float max);
    
    /**
     * Get random integer in range [min, max] (alias for Range)
     */
    static int GetInt(int min, int max) {
        return Range(min, max);
    }
    
    /**
     * Get random float in range [min, max] (alias for Range)
     */
    static float Chance(float probability);
