#include "ItemSystem.h"
#include "Unit.h"
#include "Board.h"
#include "../Graphics/Renderer.h"
#include "../Utils/Logger.h"
#include "../Utils/Random.h"

#include <algorithm>
#include <cmath>

namespace DDD {

// ===========================================================================
// INVENTORY
// ===========================================================================

bool Inventory::AddItem(const Item& item) {
    // Check for existing stack
    for (auto& existing : m_items) {
        if (existing.defId == item.defId && !existing.isEquipped) {
            existing.quantity += item.quantity;
            return true;
        }
    }
    
    // Add new item
    if (static_cast<int>(m_items.size()) < MAX_ITEMS) {
        m_items.push_back(item);
        return true;
    }
    
    return false;
}

bool Inventory::RemoveItem(const std::string& itemId, int quantity) {
    for (auto it = m_items.begin(); it != m_items.end(); ++it) {
        if (it->defId == itemId) {
            it->quantity -= quantity;
            if (it->quantity <= 0) {
                m_items.erase(it);
            }
            return true;
        }
    }
    return false;
}

bool Inventory::HasItem(const std::string& itemId) const {
    return GetItemCount(itemId) > 0;
}

int Inventory::GetItemCount(const std::string& itemId) const {
    for (const auto& item : m_items) {
        if (item.defId == itemId) {
            return item.quantity;
        }
    }
    return 0;
}

bool Inventory::EquipItem(const std::string& itemId) {
    if (static_cast<int>(m_equipped.size()) >= MAX_EQUIPPED) {
        return false;
    }
    
    for (auto& item : m_items) {
        if (item.defId == itemId && !item.isEquipped) {
            item.isEquipped = true;
            m_equipped.push_back(item);
            return true;
        }
    }
    return false;
}

bool Inventory::UnequipItem(const std::string& itemId) {
    for (auto it = m_equipped.begin(); it != m_equipped.end(); ++it) {
        if (it->defId == itemId) {
            // Find in main inventory and unequip
            for (auto& item : m_items) {
                if (item.defId == itemId && item.isEquipped) {
                    item.isEquipped = false;
                    break;
                }
            }
            m_equipped.erase(it);
            return true;
        }
    }
    return false;
}

bool Inventory::SpendGold(int amount) {
    if (m_gold >= amount) {
        m_gold -= amount;
        return true;
    }
    return false;
}

// ===========================================================================
// ITEM SYSTEM
// ===========================================================================

ItemSystem& ItemSystem::Instance() {
    static ItemSystem instance;
    return instance;
}

void ItemSystem::Initialize() {
    if (m_initialized) return;
    
    RegisterConsumables();
    RegisterEquipment();
    RegisterPowerUps();
    RegisterRelics();
    
    m_initialized = true;
    Logger::Info("ItemSystem initialized with {} items", m_items.size());
}

void ItemSystem::RegisterItem(const ItemDef& def) {
    m_items[def.id] = def;
}

const ItemDef* ItemSystem::GetItemDef(const std::string& id) const {
    auto it = m_items.find(id);
    return it != m_items.end() ? &it->second : nullptr;
}

std::vector<std::string> ItemSystem::GetItemsByCategory(ItemCategory category) const {
    std::vector<std::string> result;
    for (const auto& [id, def] : m_items) {
        if (def.category == category) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<std::string> ItemSystem::GetItemsByRarity(ItemRarity rarity) const {
    std::vector<std::string> result;
    for (const auto& [id, def] : m_items) {
        if (def.rarity == rarity) {
            result.push_back(id);
        }
    }
    return result;
}

bool ItemSystem::CanUseItem(const std::string& itemId, const Unit* user, const Unit* target) const {
    const ItemDef* def = GetItemDef(itemId);
    if (!def) return false;
    
    // Check targeting
    if (def->targetsSelf && user == target) return true;
    if (def->targetsAlly && target && target->GetOwner() == user->GetOwner()) return true;
    if (def->targetsEnemy && target && target->GetOwner() != user->GetOwner()) return true;
    if (def->targetsCell) return true;
    
    return !def->targetsSelf && !def->targetsAlly && !def->targetsEnemy;
}

bool ItemSystem::UseItem(const std::string& itemId, Unit* user, Unit* target, Board* board) {
    const ItemDef* def = GetItemDef(itemId);
    if (!def) return false;
    
    for (const auto& effect : def->effects) {
        ApplyEffect(effect, user, target, board);
    }
    
    Logger::Info("Used item: {}", def->name);
    return true;
}

void ItemSystem::ApplyEffect(const ItemEffect& effect, Unit* user, Unit* target, Board* board) {
    Unit* effectTarget = target ? target : user;
    if (!effectTarget) return;
    
    switch (effect.type) {
        case ItemEffectType::HealHP:
            effectTarget->Heal(effect.value);
            break;
            
        case ItemEffectType::HealAllHP:
            if (board) {
                for (int y = 0; y < board->GetHeight(); ++y) {
                    for (int x = 0; x < board->GetWidth(); ++x) {
                        auto unit = board->GetUnitAt({x, y});
                        if (unit && unit->GetOwner() == user->GetOwner()) {
                            unit->Heal(effect.value);
                        }
                    }
                }
            }
            break;
            
        case ItemEffectType::DamageEnemy:
            if (target && target->GetOwner() != user->GetOwner()) {
                target->TakeDamage(effect.value);
            }
            break;
            
        case ItemEffectType::DamageAllEnemies:
            if (board) {
                for (int y = 0; y < board->GetHeight(); ++y) {
                    for (int x = 0; x < board->GetWidth(); ++x) {
                        auto unit = board->GetUnitAt({x, y});
                        if (unit && unit->GetOwner() != user->GetOwner()) {
                            unit->TakeDamage(effect.value);
                        }
                    }
                }
            }
            break;
            
        case ItemEffectType::BoostATK: {
            auto& stats = effectTarget->GetMutableStats();
            stats.atk += effect.value;
            break;
        }
        
        case ItemEffectType::BoostDEF: {
            auto& stats = effectTarget->GetMutableStats();
            stats.def += effect.value;
            break;
        }
        
        case ItemEffectType::BoostMOV: {
            auto& stats = effectTarget->GetMutableStats();
            stats.mov += effect.value;
            break;
        }
        
        case ItemEffectType::BoostRNG: {
            auto& stats = effectTarget->GetMutableStats();
            stats.rng += effect.value;
            break;
        }
        
        case ItemEffectType::BoostMaxHP: {
            auto& stats = effectTarget->GetMutableStats();
            stats.maxHp += effect.value;
            stats.hp += effect.value;
            break;
        }
        
        case ItemEffectType::Shield:
            // Would need shield system in Unit
            break;
            
        case ItemEffectType::Cleanse:
            effectTarget->ClearStatusEffects();
            break;
            
        default:
            break;
    }
}

void ItemSystem::SpawnPickup(const std::string& itemId, const Position& pos) {
    FieldPickup pickup;
    pickup.itemId = itemId;
    pickup.position = pos;
    pickup.bobOffset = Random::Range(0.0f, 6.28f);
    pickup.glowIntensity = 1.0f;
    pickup.isCollected = false;
    m_pickups.push_back(pickup);
}

void ItemSystem::SpawnRandomPickup(const Position& pos, int waveNumber) {
    // Determine rarity
    int roll = Random::Range(0, 100);
    ItemRarity rarity;
    
    if (roll < 50 - waveNumber) {
        rarity = ItemRarity::Common;
    } else if (roll < 80 - waveNumber / 2) {
        rarity = ItemRarity::Uncommon;
    } else if (roll < 95) {
        rarity = ItemRarity::Rare;
    } else {
        rarity = ItemRarity::Epic;
    }
    
    // Get power-ups of that rarity
    std::vector<std::string> candidates;
    for (const auto& [id, def] : m_items) {
        if (def.category == ItemCategory::PowerUp && def.rarity == rarity) {
            candidates.push_back(id);
        }
    }
    
    if (candidates.empty()) {
        // Fallback to consumables
        candidates = GetItemsByCategory(ItemCategory::Consumable);
    }
    
    if (!candidates.empty()) {
        SpawnPickup(candidates[Random::Range(0, static_cast<int>(candidates.size()) - 1)], pos);
    }
}

void ItemSystem::UpdatePickups(float deltaTime) {
    for (auto& pickup : m_pickups) {
        pickup.bobOffset += deltaTime * 3.0f;
        pickup.glowIntensity = 0.7f + 0.3f * std::sin(pickup.bobOffset);
    }
}

bool ItemSystem::CollectPickup(const Position& pos, Inventory& inventory) {
    for (auto it = m_pickups.begin(); it != m_pickups.end(); ++it) {
        if (it->position == pos && !it->isCollected) {
            Item item;
            item.defId = it->itemId;
            item.quantity = 1;
            item.charges = -1;
            item.isEquipped = false;
            item.bonusValue = 0;
            
            const ItemDef* def = GetItemDef(it->itemId);
            if (def && def->category == ItemCategory::Currency) {
                // Direct gold pickup
                inventory.AddGold(def->effects[0].value);
            } else {
                inventory.AddItem(item);
            }
            
            m_pickups.erase(it);
            return true;
        }
    }
    return false;
}

void ItemSystem::ClearPickups() {
    m_pickups.clear();
}

std::vector<std::string> ItemSystem::GenerateShopItems(int waveNumber, int count) const {
    std::vector<std::string> result;
    
    // Build weighted pool
    std::vector<std::pair<std::string, int>> pool;
    
    for (const auto& [id, def] : m_items) {
        if (def.category == ItemCategory::Currency) continue;
        if (def.category == ItemCategory::PowerUp) continue;
        
        int weight = 10;
        switch (def.rarity) {
            case ItemRarity::Common: weight = 40 - waveNumber; break;
            case ItemRarity::Uncommon: weight = 30; break;
            case ItemRarity::Rare: weight = 20 + waveNumber; break;
            case ItemRarity::Epic: weight = 10 + waveNumber; break;
            case ItemRarity::Legendary: weight = waveNumber / 2; break;
        }
        
        if (weight > 0) {
            pool.push_back({id, weight});
        }
    }
    
    // Select items
    while (result.size() < static_cast<size_t>(count) && !pool.empty()) {
        int totalWeight = 0;
        for (const auto& [id, w] : pool) {
            totalWeight += w;
        }
        
        int roll = Random::Range(0, totalWeight - 1);
        int cumulative = 0;
        
        for (auto it = pool.begin(); it != pool.end(); ++it) {
            cumulative += it->second;
            if (roll < cumulative) {
                result.push_back(it->first);
                pool.erase(it);
                break;
            }
        }
    }
    
    return result;
}

float ItemSystem::GetRelicBonusDamage(const Inventory& inventory) const {
    float bonus = 1.0f;
    for (const auto& relic : inventory.GetRelics()) {
        const ItemDef* def = GetItemDef(relic.defId);
        if (def) {
            for (const auto& effect : def->effects) {
                if (effect.type == ItemEffectType::BonusDamage) {
                    bonus += effect.multiplier;
                }
            }
        }
    }
    return bonus;
}

float ItemSystem::GetRelicBonusHealing(const Inventory& inventory) const {
    float bonus = 1.0f;
    for (const auto& relic : inventory.GetRelics()) {
        const ItemDef* def = GetItemDef(relic.defId);
        if (def) {
            for (const auto& effect : def->effects) {
                if (effect.type == ItemEffectType::BonusHealing) {
                    bonus += effect.multiplier;
                }
            }
        }
    }
    return bonus;
}

float ItemSystem::GetRelicBonusGold(const Inventory& inventory) const {
    float bonus = 1.0f;
    for (const auto& relic : inventory.GetRelics()) {
        const ItemDef* def = GetItemDef(relic.defId);
        if (def) {
            for (const auto& effect : def->effects) {
                if (effect.type == ItemEffectType::BonusGold) {
                    bonus += effect.multiplier;
                }
            }
        }
    }
    return bonus;
}

float ItemSystem::GetRelicBonusExp(const Inventory& inventory) const {
    float bonus = 1.0f;
    for (const auto& relic : inventory.GetRelics()) {
        const ItemDef* def = GetItemDef(relic.defId);
        if (def) {
            for (const auto& effect : def->effects) {
                if (effect.type == ItemEffectType::BonusExp) {
                    bonus += effect.multiplier;
                }
            }
        }
    }
    return bonus;
}

void ItemSystem::RenderPickup(Renderer& renderer, const FieldPickup& pickup, int cellX, int cellY, int cellSize) {
    const ItemDef* def = GetItemDef(pickup.itemId);
    if (!def) return;
    
    // Bob animation
    float bob = std::sin(pickup.bobOffset) * 4.0f;
    int drawY = cellY + static_cast<int>(bob);
    
    // Glow effect
    SDL_Color glowColor = def->color;
    glowColor.a = static_cast<uint8_t>(pickup.glowIntensity * 100);
    
    int glowSize = 8;
    renderer.FillRect(cellX + cellSize/2 - glowSize - 2, drawY + cellSize/2 - glowSize - 2,
                      glowSize * 2 + 4, glowSize * 2 + 4, glowColor);
    
    // Item icon
    int iconSize = 20;
    int iconX = cellX + (cellSize - iconSize) / 2;
    int iconY = drawY + (cellSize - iconSize) / 2;
    
    renderer.FillRect(iconX, iconY, iconSize, iconSize, def->color);
    renderer.DrawRect(iconX, iconY, iconSize, iconSize, {255, 255, 255, 200});
}

void ItemSystem::RenderItem(Renderer& renderer, const std::string& itemId, int x, int y, int size) {
    const ItemDef* def = GetItemDef(itemId);
    if (!def) return;
    
    // Background with rarity border
    SDL_Color rarityColor = GetRarityColor(def->rarity);
    renderer.FillRect(x - 2, y - 2, size + 4, size + 4, rarityColor);
    renderer.FillRect(x, y, size, size, def->color);
    
    // Symbol
    // Would render def->symbol as text here
}

SDL_Color ItemSystem::GetRarityColor(ItemRarity rarity) {
    switch (rarity) {
        case ItemRarity::Common:    return {180, 180, 180, 255};
        case ItemRarity::Uncommon:  return {100, 200, 100, 255};
        case ItemRarity::Rare:      return {80, 140, 220, 255};
        case ItemRarity::Epic:      return {180, 80, 220, 255};
        case ItemRarity::Legendary: return {255, 180, 50, 255};
        default: return {200, 200, 200, 255};
    }
}

std::string ItemSystem::GetRarityName(ItemRarity rarity) {
    switch (rarity) {
        case ItemRarity::Common:    return "Common";
        case ItemRarity::Uncommon:  return "Uncommon";
        case ItemRarity::Rare:      return "Rare";
        case ItemRarity::Epic:      return "Epic";
        case ItemRarity::Legendary: return "Legendary";
        default: return "Unknown";
    }
}

// ===========================================================================
// CONSUMABLES
// ===========================================================================

void ItemSystem::RegisterConsumables() {
    // Health Potions
    RegisterItem({
        "potion_health_small", "Small Health Potion", "Restores 15 HP", "potion_red",
        ItemCategory::Consumable, ItemRarity::Common,
        25, 10,
        {{ItemEffectType::HealHP, 15, 0, 0}},
        true, true, false, false, 1,
        {220, 80, 80, 255}, 'h'
    });
    
    RegisterItem({
        "potion_health_medium", "Health Potion", "Restores 30 HP", "potion_red",
        ItemCategory::Consumable, ItemRarity::Uncommon,
        50, 20,
        {{ItemEffectType::HealHP, 30, 0, 0}},
        true, true, false, false, 1,
        {200, 60, 60, 255}, 'H'
    });
    
    RegisterItem({
        "potion_health_large", "Large Health Potion", "Restores 50 HP", "potion_red",
        ItemCategory::Consumable, ItemRarity::Rare,
        100, 40,
        {{ItemEffectType::HealHP, 50, 0, 0}},
        true, true, false, false, 1,
        {180, 40, 40, 255}, 'H'
    });
    
    // Team healing
    RegisterItem({
        "potion_mass_heal", "Mass Healing Potion", "Heals all allies for 20 HP", "potion_pink",
        ItemCategory::Consumable, ItemRarity::Rare,
        150, 60,
        {{ItemEffectType::HealAllHP, 20, 0, 0}},
        false, false, false, false, 0,
        {255, 150, 200, 255}, 'M'
    });
    
    // Damage items
    RegisterItem({
        "bomb_small", "Bomb", "Deals 20 damage to one enemy", "bomb",
        ItemCategory::Consumable, ItemRarity::Uncommon,
        40, 15,
        {{ItemEffectType::DamageEnemy, 20, 0, 0}},
        false, false, true, false, 3,
        {80, 80, 80, 255}, 'B'
    });
    
    RegisterItem({
        "bomb_large", "Mega Bomb", "Deals 40 damage to one enemy", "bomb",
        ItemCategory::Consumable, ItemRarity::Rare,
        80, 30,
        {{ItemEffectType::DamageEnemy, 40, 0, 0}},
        false, false, true, false, 3,
        {60, 60, 60, 255}, 'B'
    });
    
    // Cleansing
    RegisterItem({
        "antidote", "Antidote", "Removes all negative effects", "potion_green",
        ItemCategory::Consumable, ItemRarity::Uncommon,
        35, 12,
        {{ItemEffectType::Cleanse, 0, 0, 0}},
        true, true, false, false, 1,
        {100, 200, 100, 255}, 'A'
    });
}

// ===========================================================================
// EQUIPMENT
// ===========================================================================

void ItemSystem::RegisterEquipment() {
    // Attack boosters
    RegisterItem({
        "ring_power", "Ring of Power", "Permanently +3 ATK", "ring_red",
        ItemCategory::Equipment, ItemRarity::Uncommon,
        100, 40,
        {{ItemEffectType::BoostATK, 3, 0, 0}},
        true, false, false, false, 0,
        {200, 100, 100, 255}, 'r'
    });
    
    RegisterItem({
        "amulet_fury", "Amulet of Fury", "Permanently +5 ATK", "amulet_red",
        ItemCategory::Equipment, ItemRarity::Rare,
        200, 80,
        {{ItemEffectType::BoostATK, 5, 0, 0}},
        true, false, false, false, 0,
        {220, 80, 80, 255}, 'a'
    });
    
    // Defense boosters
    RegisterItem({
        "ring_protection", "Ring of Protection", "Permanently +3 DEF", "ring_blue",
        ItemCategory::Equipment, ItemRarity::Uncommon,
        100, 40,
        {{ItemEffectType::BoostDEF, 3, 0, 0}},
        true, false, false, false, 0,
        {100, 100, 200, 255}, 'r'
    });
    
    RegisterItem({
        "shield_charm", "Shield Charm", "Permanently +5 DEF", "charm_blue",
        ItemCategory::Equipment, ItemRarity::Rare,
        200, 80,
        {{ItemEffectType::BoostDEF, 5, 0, 0}},
        true, false, false, false, 0,
        {80, 80, 220, 255}, 's'
    });
    
    // HP boosters
    RegisterItem({
        "heart_crystal", "Heart Crystal", "Permanently +10 Max HP", "crystal_pink",
        ItemCategory::Equipment, ItemRarity::Uncommon,
        120, 50,
        {{ItemEffectType::BoostMaxHP, 10, 0, 0}},
        true, false, false, false, 0,
        {255, 150, 180, 255}, 'c'
    });
    
    RegisterItem({
        "heart_container", "Heart Container", "Permanently +25 Max HP", "heart_gold",
        ItemCategory::Equipment, ItemRarity::Rare,
        250, 100,
        {{ItemEffectType::BoostMaxHP, 25, 0, 0}},
        true, false, false, false, 0,
        {255, 200, 100, 255}, 'C'
    });
    
    // Movement
    RegisterItem({
        "boots_speed", "Boots of Speed", "Permanently +1 MOV", "boots_green",
        ItemCategory::Equipment, ItemRarity::Rare,
        180, 70,
        {{ItemEffectType::BoostMOV, 1, 0, 0}},
        true, false, false, false, 0,
        {100, 200, 100, 255}, 'b'
    });
    
    // Range
    RegisterItem({
        "scope", "Sniper Scope", "Permanently +1 RNG", "scope",
        ItemCategory::Equipment, ItemRarity::Rare,
        180, 70,
        {{ItemEffectType::BoostRNG, 1, 0, 0}},
        true, false, false, false, 0,
        {150, 150, 200, 255}, 'S'
    });
}

// ===========================================================================
// POWER-UPS (Field pickups)
// ===========================================================================

void ItemSystem::RegisterPowerUps() {
    // Gold drops
    RegisterItem({
        "gold_small", "Gold Coin", "Worth 10 gold", "coin",
        ItemCategory::Currency, ItemRarity::Common,
        0, 0,
        {{ItemEffectType::GainGold, 10, 0, 0}},
        false, false, false, false, 0,
        {255, 200, 50, 255}, '$'
    });
    
    RegisterItem({
        "gold_medium", "Gold Pile", "Worth 25 gold", "coins",
        ItemCategory::Currency, ItemRarity::Uncommon,
        0, 0,
        {{ItemEffectType::GainGold, 25, 0, 0}},
        false, false, false, false, 0,
        {255, 210, 80, 255}, '$'
    });
    
    RegisterItem({
        "gold_large", "Gold Chest", "Worth 50 gold", "chest",
        ItemCategory::Currency, ItemRarity::Rare,
        0, 0,
        {{ItemEffectType::GainGold, 50, 0, 0}},
        false, false, false, false, 0,
        {255, 220, 100, 255}, '$'
    });
    
    // Health pickups
    RegisterItem({
        "pickup_health", "Health Orb", "Instantly restores 20 HP", "orb_red",
        ItemCategory::PowerUp, ItemRarity::Common,
        0, 0,
        {{ItemEffectType::HealHP, 20, 0, 0}},
        true, false, false, false, 0,
        {255, 100, 100, 255}, '+'
    });
    
    // Stat boosts (temporary)
    RegisterItem({
        "pickup_power", "Power Orb", "Temporarily +5 ATK for 3 turns", "orb_orange",
        ItemCategory::PowerUp, ItemRarity::Uncommon,
        0, 0,
        {{ItemEffectType::BoostATK, 5, 0, 3}},
        true, false, false, false, 0,
        {255, 150, 50, 255}, '!'
    });
    
    RegisterItem({
        "pickup_shield", "Shield Orb", "Temporarily +5 DEF for 3 turns", "orb_blue",
        ItemCategory::PowerUp, ItemRarity::Uncommon,
        0, 0,
        {{ItemEffectType::BoostDEF, 5, 0, 3}},
        true, false, false, false, 0,
        {100, 150, 255, 255}, '!'
    });
}

// ===========================================================================
// RELICS (Run-wide bonuses)
// ===========================================================================

void ItemSystem::RegisterRelics() {
    RegisterItem({
        "relic_warriors_crest", "Warrior's Crest", "All damage increased by 15%", "crest_red",
        ItemCategory::Relic, ItemRarity::Rare,
        300, 100,
        {{ItemEffectType::BonusDamage, 0, 0.15f, 0}},
        false, false, false, false, 0,
        {200, 80, 80, 255}, 'W'
    });
    
    RegisterItem({
        "relic_healers_charm", "Healer's Charm", "All healing increased by 20%", "charm_green",
        ItemCategory::Relic, ItemRarity::Rare,
        300, 100,
        {{ItemEffectType::BonusHealing, 0, 0.20f, 0}},
        false, false, false, false, 0,
        {100, 200, 100, 255}, 'H'
    });
    
    RegisterItem({
        "relic_gold_idol", "Golden Idol", "Gold drops increased by 25%", "idol_gold",
        ItemCategory::Relic, ItemRarity::Rare,
        250, 80,
        {{ItemEffectType::BonusGold, 0, 0.25f, 0}},
        false, false, false, false, 0,
        {255, 200, 50, 255}, 'G'
    });
    
    RegisterItem({
        "relic_exp_tome", "Tome of Knowledge", "Experience gained increased by 20%", "tome_purple",
        ItemCategory::Relic, ItemRarity::Rare,
        280, 90,
        {{ItemEffectType::BonusExp, 0, 0.20f, 0}},
        false, false, false, false, 0,
        {180, 100, 220, 255}, 'T'
    });
    
    RegisterItem({
        "relic_phoenix_feather", "Phoenix Feather", "Revive once per battle with 50% HP", "feather_orange",
        ItemCategory::Relic, ItemRarity::Epic,
        500, 150,
        {{ItemEffectType::ExtraLife, 1, 0.5f, 0}},
        false, false, false, false, 0,
        {255, 150, 50, 255}, 'P'
    });
    
    RegisterItem({
        "relic_starting_purse", "Heavy Purse", "Start each battle with +50 gold", "purse_gold",
        ItemCategory::Relic, ItemRarity::Epic,
        400, 120,
        {{ItemEffectType::StartingGold, 50, 0, 0}},
        false, false, false, false, 0,
        {200, 180, 100, 255}, '$'
    });
    
    RegisterItem({
        "relic_champion_belt", "Champion's Belt", "+25% damage and +10% healing", "belt_gold",
        ItemCategory::Relic, ItemRarity::Legendary,
        800, 250,
        {{ItemEffectType::BonusDamage, 0, 0.25f, 0}, {ItemEffectType::BonusHealing, 0, 0.10f, 0}},
        false, false, false, false, 0,
        {255, 220, 100, 255}, 'C'
    });
}

} // namespace DDD
