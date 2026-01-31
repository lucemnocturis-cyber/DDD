#pragma once

#include "../Utils/Math.h"
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace DDD {

class Unit;
class Board;
class Renderer;

/**
 * Item categories
 */
enum class ItemCategory {
    Consumable,     // One-time use items
    Equipment,      // Permanent stat boosts
    PowerUp,        // Temporary field pickups
    Relic,          // Powerful run-wide effects
    Currency        // Gold, gems, etc
};

/**
 * Item rarity
 */
enum class ItemRarity {
    Common,
    Uncommon,
    Rare,
    Epic,
    Legendary
};

/**
 * Item effect types
 */
enum class ItemEffectType {
    // Instant effects
    HealHP,
    HealAllHP,
    DamageEnemy,
    DamageAllEnemies,
    
    // Stat modifications
    BoostATK,
    BoostDEF,
    BoostMOV,
    BoostRNG,
    BoostMaxHP,
    
    // Special effects
    Revive,
    Cleanse,
    Teleport,
    Shield,
    Invincibility,
    ExtraTurn,
    
    // Resource effects
    GainGold,
    GainExp,
    RefreshDice,
    ReduceCooldowns,
    
    // Run modifiers (relics)
    BonusDamage,
    BonusHealing,
    BonusGold,
    BonusExp,
    StartingGold,
    ExtraLife
};

/**
 * Item effect definition
 */
struct ItemEffect {
    ItemEffectType type;
    int value;              // Amount/duration
    float multiplier;       // For percentage effects
    int duration;           // Turns (0 = permanent/instant)
};

/**
 * Item definition
 */
struct ItemDef {
    std::string id;
    std::string name;
    std::string description;
    std::string iconName;
    
    ItemCategory category;
    ItemRarity rarity;
    
    int buyPrice;
    int sellPrice;
    
    std::vector<ItemEffect> effects;
    
    // Targeting
    bool targetsSelf;
    bool targetsAlly;
    bool targetsEnemy;
    bool targetsCell;
    int targetRange;
    
    // Visual
    SDL_Color color;
    char symbol;
};

/**
 * Item instance (can have unique properties)
 */
struct Item {
    std::string defId;
    int quantity;
    int charges;        // For limited-use equipment
    bool isEquipped;
    
    // Unique modifiers (for procedural items)
    int bonusValue;
    std::string suffix; // "of Power", "of Speed", etc
};

/**
 * Field pickup (spawned on board)
 */
struct FieldPickup {
    std::string itemId;
    Position position;
    float bobOffset;    // Animation
    float glowIntensity;
    bool isCollected;
};

/**
 * Player inventory
 */
class Inventory {
public:
    static constexpr int MAX_ITEMS = 20;
    static constexpr int MAX_EQUIPPED = 6;
    static constexpr int MAX_RELICS = 5;
    
    bool AddItem(const Item& item);
    bool RemoveItem(const std::string& itemId, int quantity = 1);
    bool HasItem(const std::string& itemId) const;
    int GetItemCount(const std::string& itemId) const;
    
    bool EquipItem(const std::string& itemId);
    bool UnequipItem(const std::string& itemId);
    
    const std::vector<Item>& GetItems() const { return m_items; }
    const std::vector<Item>& GetEquipped() const { return m_equipped; }
    const std::vector<Item>& GetRelics() const { return m_relics; }
    
    int GetGold() const { return m_gold; }
    void AddGold(int amount) { m_gold += amount; }
    bool SpendGold(int amount);
    
private:
    std::vector<Item> m_items;
    std::vector<Item> m_equipped;
    std::vector<Item> m_relics;
    int m_gold = 0;
};

/**
 * ItemSystem - manages all items and effects
 */
class ItemSystem {
public:
    static ItemSystem& Instance();
    
    void Initialize();
    
    // Item definitions
    const ItemDef* GetItemDef(const std::string& id) const;
    std::vector<std::string> GetItemsByCategory(ItemCategory category) const;
    std::vector<std::string> GetItemsByRarity(ItemRarity rarity) const;
    
    // Item usage
    bool CanUseItem(const std::string& itemId, const Unit* user, const Unit* target) const;
    bool UseItem(const std::string& itemId, Unit* user, Unit* target, Board* board);
    
    // Field pickups
    void SpawnPickup(const std::string& itemId, const Position& pos);
    void SpawnRandomPickup(const Position& pos, int waveNumber);
    void UpdatePickups(float deltaTime);
    bool CollectPickup(const Position& pos, Inventory& inventory);
    const std::vector<FieldPickup>& GetPickups() const { return m_pickups; }
    void ClearPickups();
    
    // Shop generation
    std::vector<std::string> GenerateShopItems(int waveNumber, int count) const;
    
    // Relic effects (called during gameplay)
    float GetRelicBonusDamage(const Inventory& inventory) const;
    float GetRelicBonusHealing(const Inventory& inventory) const;
    float GetRelicBonusGold(const Inventory& inventory) const;
    float GetRelicBonusExp(const Inventory& inventory) const;
    
    // Rendering
    void RenderPickup(Renderer& renderer, const FieldPickup& pickup, int cellX, int cellY, int cellSize);
    void RenderItem(Renderer& renderer, const std::string& itemId, int x, int y, int size);
    
    // Utility
    static SDL_Color GetRarityColor(ItemRarity rarity);
    static std::string GetRarityName(ItemRarity rarity);
    
private:
    ItemSystem() = default;
    
    void RegisterItem(const ItemDef& def);
    void RegisterConsumables();
    void RegisterEquipment();
    void RegisterPowerUps();
    void RegisterRelics();
    
    void ApplyEffect(const ItemEffect& effect, Unit* user, Unit* target, Board* board);
    
    std::unordered_map<std::string, ItemDef> m_items;
    std::vector<FieldPickup> m_pickups;
    bool m_initialized = false;
};

} // namespace DDD
