#pragma once

#include "Unit.h"
#include "../Utils/Math.h"

#include <string>
#include <array>
#include <vector>
#include <memory>

namespace DDD {

/**
 * A single face of a die (one of six possible stat configurations)
 */
struct DiceFace {
    int hp = 0;
    int atk = 0;
    int def = 0;
    int move = 0;
    int range = 1;
    std::array<int, 4> unfurl = {1, 1, 1, 1};  // [north, east, south, west]
    std::vector<std::string> abilities;
};

/**
 * Dice - represents a unit die that can be rolled and placed
 */
class Dice {
public:
    Dice();
    ~Dice();
    
    /**
     * Initialize dice from class data
     */
    void Initialize(const std::string& className, int tier, int cost,
                    const std::vector<DiceFace>& faces,
                    const std::vector<std::string>& promotionOptions);
    
    /**
     * Roll the dice to select a random face
     */
    void Roll();
    
    /**
     * Get the currently active face
     */
    const DiceFace& GetCurrentFace() const { return m_faces[m_currentFaceIndex]; }
    
    /**
     * Get all faces
     */
    const std::vector<DiceFace>& GetFaces() const { return m_faces; }
    
    /**
     * Get current face index (0-5)
     */
    int GetCurrentFaceIndex() const { return m_currentFaceIndex; }
    
    /**
     * Create a unit from this dice
     */
    std::shared_ptr<Unit> CreateUnit() const;
    
    // Accessors
    const std::string& GetClassName() const { return m_className; }
    int GetTier() const { return m_tier; }
    int GetCost() const { return m_cost; }
    const std::vector<std::string>& GetPromotionOptions() const { return m_promotionOptions; }
    
    // State
    bool IsPlaced() const { return m_isPlaced; }
    void SetPlaced(bool placed) { m_isPlaced = placed; }
    
private:
    std::string m_className;
    int m_tier = 0;
    int m_cost = 1;
    std::vector<DiceFace> m_faces;
    int m_currentFaceIndex = 0;
    std::vector<std::string> m_promotionOptions;
    bool m_isPlaced = false;
};

/**
 * DiceFactory - creates dice from class definitions
 */
class DiceFactory {
public:
    /**
     * Create a dice from class name
     */
    static std::shared_ptr<Dice> CreateDice(const std::string& className);
    
    /**
     * Get available base class names
     */
    static std::vector<std::string> GetBaseClasses();
    
    /**
     * Get promotion options for a class
     */
    static std::vector<std::string> GetPromotionOptions(const std::string& className);
};

} // namespace DDD
