#include "AbilityDatabase.h"
#include "Unit.h"
#include "Board.h"
#include "../Utils/Logger.h"
#include "../Utils/Random.h"

#include <algorithm>
#include <cmath>

namespace DDD {

AbilityDatabase& AbilityDatabase::Instance() {
    static AbilityDatabase instance;
    return instance;
}

void AbilityDatabase::Initialize() {
    if (m_initialized) return;
    
    RegisterMageAbilities();
    RegisterSoldierAbilities();
    RegisterRogueAbilities();
    RegisterHealerAbilities();
    RegisterTankAbilities();
    RegisterArcherAbilities();
    RegisterBossAbilities();
    
    m_initialized = true;
    Logger::Info("AbilityDatabase initialized with {} abilities", m_abilities.size());
}

void AbilityDatabase::RegisterAbility(const AbilityDef& def) {
    m_abilities[def.id] = def;
}

const AbilityDef* AbilityDatabase::GetAbilityDef(const std::string& id) const {
    auto it = m_abilities.find(id);
    return it != m_abilities.end() ? &it->second : nullptr;
}

std::vector<std::string> AbilityDatabase::GetAbilitiesByType(AbilityType type) const {
    std::vector<std::string> result;
    for (const auto& [id, def] : m_abilities) {
        if (def.type == type) {
            result.push_back(id);
        }
    }
    return result;
}

bool AbilityDatabase::CanUseAbility(const std::string& abilityId,
                                    const Unit& caster,
                                    const Board& board) const {
    const AbilityDef* def = GetAbilityDef(abilityId);
    if (!def) return false;
    
    // Check if caster is alive and can act
    if (caster.IsDead() || caster.IsStunned()) return false;
    
    // Check for valid targets
    auto targets = GetValidTargets(abilityId, caster, board);
    return !targets.empty() || def->targetType == TargetType::Self || 
           def->targetType == TargetType::None;
}

std::vector<Position> AbilityDatabase::GetValidTargets(const std::string& abilityId,
                                                       const Unit& caster,
                                                       const Board& board) const {
    std::vector<Position> targets;
    const AbilityDef* def = GetAbilityDef(abilityId);
    if (!def) return targets;
    
    Position casterPos = caster.GetPosition();
    Owner casterOwner = caster.GetOwner();
    
    // Self-targeting
    if (def->targetType == TargetType::Self) {
        targets.push_back(casterPos);
        return targets;
    }
    
    // Search board for valid targets
    for (int y = 0; y < 19; ++y) {  // Board height
        for (int x = 0; x < 13; ++x) {  // Board width
            Position pos = {x, y};
            int dist = std::abs(x - casterPos.x) + std::abs(y - casterPos.y);
            
            // Check range
            if (dist > def->range) continue;
            
            auto unit = board.GetUnitAt(pos.x, pos.y);
            
            switch (def->targetType) {
                case TargetType::SingleEnemy:
                    if (unit && unit->GetOwner() != casterOwner && !unit->IsDead()) {
                        targets.push_back(pos);
                    }
                    break;
                    
                case TargetType::SingleAlly:
                    if (unit && unit->GetOwner() == casterOwner && !unit->IsDead()) {
                        if (def->canTargetSelf || pos != casterPos) {
                            targets.push_back(pos);
                        }
                    }
                    break;
                    
                case TargetType::Area:
                case TargetType::Line:
                    // For AoE, any cell in range is valid
                    targets.push_back(pos);
                    break;
                    
                case TargetType::AllEnemies:
                case TargetType::AllAllies:
                    // These don't need specific targeting
                    targets.push_back(casterPos);
                    return targets;
                    
                default:
                    break;
            }
        }
    }
    
    return targets;
}

std::vector<Position> AbilityDatabase::GetAffectedCells(const std::string& abilityId,
                                                        const Position& casterPos,
                                                        const Position& targetPos,
                                                        const Board& board) const {
    std::vector<Position> cells;
    const AbilityDef* def = GetAbilityDef(abilityId);
    if (!def) return cells;
    
    cells.push_back(targetPos);
    
    // AoE radius
    if (def->aoeRadius > 0) {
        for (int dx = -def->aoeRadius; dx <= def->aoeRadius; ++dx) {
            for (int dy = -def->aoeRadius; dy <= def->aoeRadius; ++dy) {
                if (std::abs(dx) + std::abs(dy) <= def->aoeRadius) {
                    Position aoe = {targetPos.x + dx, targetPos.y + dy};
                    if (board.IsValidPosition(aoe.x, aoe.y) && aoe != targetPos) {
                        cells.push_back(aoe);
                    }
                }
            }
        }
    }
    
    // Line attack
    if (def->targetType == TargetType::Line) {
        int dx = (targetPos.x > casterPos.x) ? 1 : (targetPos.x < casterPos.x) ? -1 : 0;
        int dy = (targetPos.y > casterPos.y) ? 1 : (targetPos.y < casterPos.y) ? -1 : 0;
        
        Position current = casterPos;
        for (int i = 0; i < def->range; ++i) {
            current.x += dx;
            current.y += dy;
            if (board.IsValidPosition(current.x, current.y)) {
                cells.push_back(current);
            }
        }
    }
    
    return cells;
}

int AbilityDatabase::CalculateDamage(const AbilityDef& def, const Unit& caster, const Unit& target) const {
    int damage = def.baseDamage;
    
    // ATK scaling
    damage += static_cast<int>(caster.GetStats().atk * def.atkScaling);
    
    // Defense reduction (unless ignores defense)
    if (!def.ignoresDefense) {
        damage -= target.GetStats().def;
    }
    
    // Crit check
    float critChance = 0.1f + def.critBonus;
    if (def.guaranteedCrit || Random::Range(0, 100) < static_cast<int>(critChance * 100)) {
        damage = static_cast<int>(damage * 1.5f);
    }
    
    return std::max(1, damage);
}

int AbilityDatabase::CalculateHeal(const AbilityDef& def, const Unit& caster) const {
    int heal = def.baseHeal;
    heal += static_cast<int>(caster.GetStats().atk * def.atkScaling * 0.5f);
    return std::max(1, heal);
}

void AbilityDatabase::ApplyStatusEffect(Unit& target, StatusEffect effect, int duration, int power) {
    switch (effect) {
        case StatusEffect::Stun:
            target.ApplyStun(duration);
            break;
        case StatusEffect::Poison:
        case StatusEffect::Burn:
            target.ApplyPoison(power, duration);
            break;
        // Other effects would need additional Unit methods
        default:
            break;
    }
}

AbilityResult AbilityDatabase::ExecuteAbility(const std::string& abilityId,
                                               Unit& caster,
                                               const Position& targetPos,
                                               Board& board) {
    AbilityResult result;
    result.success = false;
    
    const AbilityDef* def = GetAbilityDef(abilityId);
    if (!def) {
        result.message = "Unknown ability";
        return result;
    }
    
    // Get affected cells
    auto affectedCells = GetAffectedCells(abilityId, caster.GetPosition(), targetPos, board);
    
    // Collect targets
    std::vector<Unit*> targets;
    for (const auto& pos : affectedCells) {
        auto unit = board.GetUnitAt(pos.x, pos.y);
        if (unit && !unit->IsDead()) {
            bool isEnemy = unit->GetOwner() != caster.GetOwner();
            
            // Filter based on ability targeting
            if (def->type == AbilityType::Attack || def->type == AbilityType::Debuff) {
                if (isEnemy || def->canTargetAllies) {
                    targets.push_back(unit.get());
                }
            } else if (def->type == AbilityType::Heal || def->type == AbilityType::Buff) {
                if (!isEnemy || def->canTargetEnemies) {
                    targets.push_back(unit.get());
                }
            }
        }
    }
    
    // Self-targeting abilities
    if (def->targetType == TargetType::Self) {
        targets.clear();
        targets.push_back(&caster);
    }
    
    // Execute effects
    for (Unit* target : targets) {
        // Damage
        if (def->baseDamage > 0 || def->atkScaling > 0) {
            int damage = CalculateDamage(*def, caster, *target);
            target->TakeDamage(damage);
            result.damageDealt.push_back({target, damage});
        }
        
        // Healing
        if (def->baseHeal > 0) {
            int heal = CalculateHeal(*def, caster);
            target->Heal(heal);
            result.healingDone.push_back({target, heal});
        }
        
        // Status effect
        if (def->appliesEffect != StatusEffect::None) {
            ApplyStatusEffect(*target, def->appliesEffect, def->effectDuration, def->effectPower);
            result.effectsApplied.push_back({target, def->appliesEffect});
        }
    }
    
    result.success = true;
    result.message = caster.GetClassName() + " uses " + def->name + "!";
    
    Logger::Info("{} uses {} on {} targets", caster.GetClassName(), def->name, targets.size());
    
    return result;
}

std::string AbilityDatabase::GetEffectName(StatusEffect effect) {
    switch (effect) {
        case StatusEffect::Stun: return "Stunned";
        case StatusEffect::Poison: return "Poisoned";
        case StatusEffect::Burn: return "Burning";
        case StatusEffect::Freeze: return "Frozen";
        case StatusEffect::Slow: return "Slowed";
        case StatusEffect::Weaken: return "Weakened";
        case StatusEffect::Vulnerability: return "Vulnerable";
        case StatusEffect::Regeneration: return "Regenerating";
        case StatusEffect::Shield: return "Shielded";
        case StatusEffect::AttackBoost: return "Attack Up";
        case StatusEffect::DefenseBoost: return "Defense Up";
        case StatusEffect::Haste: return "Haste";
        case StatusEffect::Taunt: return "Taunting";
        case StatusEffect::Stealth: return "Stealthed";
        case StatusEffect::Marked: return "Marked";
        default: return "None";
    }
}

SDL_Color AbilityDatabase::GetEffectColor(StatusEffect effect) {
    switch (effect) {
        case StatusEffect::Stun: return {255, 255, 100, 255};
        case StatusEffect::Poison: return {100, 200, 100, 255};
        case StatusEffect::Burn: return {255, 150, 50, 255};
        case StatusEffect::Freeze: return {100, 200, 255, 255};
        case StatusEffect::Slow: return {150, 150, 200, 255};
        case StatusEffect::Weaken: return {200, 100, 100, 255};
        case StatusEffect::Vulnerability: return {255, 100, 255, 255};
        case StatusEffect::Regeneration: return {100, 255, 150, 255};
        case StatusEffect::Shield: return {200, 200, 255, 255};
        case StatusEffect::AttackBoost: return {255, 100, 100, 255};
        case StatusEffect::DefenseBoost: return {100, 100, 255, 255};
        case StatusEffect::Haste: return {100, 255, 255, 255};
        case StatusEffect::Taunt: return {255, 200, 100, 255};
        case StatusEffect::Stealth: return {100, 100, 100, 255};
        case StatusEffect::Marked: return {255, 50, 50, 255};
        default: return {200, 200, 200, 255};
    }
}

// ===========================================================================
// MAGE ABILITIES
// ===========================================================================

void AbilityDatabase::RegisterMageAbilities() {
    RegisterAbility({
        "fireball", "Fireball", "Launches a ball of fire dealing AoE damage", "fire",
        AbilityType::Attack, TargetType::Area,
        15, 0, 0.8f, 3, 1, 3, 0,
        StatusEffect::Burn, 2, 5,
        false, false, true, false, false, 0.0f,
        "fire_burst", "fireball"
    });
    
    RegisterAbility({
        "chain_lightning", "Chain Lightning", "Electricity jumps between enemies", "lightning",
        AbilityType::Attack, TargetType::SingleEnemy,
        12, 0, 0.6f, 3, 2, 4, 0,
        StatusEffect::None, 0, 0,
        false, false, true, false, false, 0.1f,
        "lightning", "zap"
    });
    
    RegisterAbility({
        "ice_shard", "Ice Shard", "Freezes an enemy in place", "ice",
        AbilityType::Attack, TargetType::SingleEnemy,
        10, 0, 0.5f, 3, 0, 2, 0,
        StatusEffect::Freeze, 1, 0,
        false, false, true, false, false, 0.0f,
        "ice", "freeze"
    });
    
    RegisterAbility({
        "meteor", "Meteor", "Calls down a devastating meteor", "meteor",
        AbilityType::Attack, TargetType::Area,
        40, 0, 1.0f, 4, 2, 6, 0,
        StatusEffect::Burn, 3, 8,
        false, false, true, false, false, 0.2f,
        "explosion", "meteor"
    });
    
    RegisterAbility({
        "life_drain", "Life Drain", "Drains life from an enemy", "drain",
        AbilityType::Attack, TargetType::SingleEnemy,
        12, 8, 0.5f, 2, 0, 3, 0,
        StatusEffect::None, 0, 0,
        false, false, true, false, false, 0.0f,
        "dark", "drain"
    });
}

// ===========================================================================
// SOLDIER ABILITIES
// ===========================================================================

void AbilityDatabase::RegisterSoldierAbilities() {
    RegisterAbility({
        "shield_bash", "Shield Bash", "Bashes enemy, stunning them", "shield",
        AbilityType::Attack, TargetType::SingleEnemy,
        8, 0, 0.4f, 1, 0, 2, 0,
        StatusEffect::Stun, 1, 0,
        false, false, true, false, false, 0.0f,
        "impact", "bash"
    });
    
    RegisterAbility({
        "cleave", "Cleave", "Swings weapon hitting multiple enemies", "sword",
        AbilityType::Attack, TargetType::Area,
        12, 0, 0.7f, 1, 1, 2, 0,
        StatusEffect::None, 0, 0,
        false, false, true, false, false, 0.05f,
        "slash", "sword"
    });
    
    RegisterAbility({
        "rampage", "Rampage", "Goes berserk, dealing heavy damage", "rage",
        AbilityType::Attack, TargetType::SingleEnemy,
        25, 0, 1.2f, 1, 0, 4, 0,
        StatusEffect::None, 0, 0,
        false, false, true, false, false, 0.3f,
        "rage", "roar"
    });
    
    RegisterAbility({
        "war_cry", "War Cry", "Boosts attack of nearby allies", "cry",
        AbilityType::Buff, TargetType::Area,
        0, 0, 0.0f, 0, 2, 4, 0,
        StatusEffect::AttackBoost, 3, 5,
        true, true, false, false, false, 0.0f,
        "buff", "roar"
    });
}

// ===========================================================================
// ROGUE ABILITIES
// ===========================================================================

void AbilityDatabase::RegisterRogueAbilities() {
    RegisterAbility({
        "backstab", "Backstab", "Massive damage from behind", "dagger",
        AbilityType::Attack, TargetType::SingleEnemy,
        20, 0, 1.0f, 1, 0, 2, 0,
        StatusEffect::None, 0, 0,
        false, false, true, false, false, 0.5f,
        "slash", "stab"
    });
    
    RegisterAbility({
        "poison_blade", "Poison Blade", "Applies deadly poison", "poison",
        AbilityType::Attack, TargetType::SingleEnemy,
        8, 0, 0.4f, 1, 0, 2, 0,
        StatusEffect::Poison, 3, 6,
        false, false, true, false, false, 0.0f,
        "poison", "hit"
    });
    
    RegisterAbility({
        "smoke_bomb", "Smoke Bomb", "Blinds enemies, gaining stealth", "smoke",
        AbilityType::Debuff, TargetType::Area,
        0, 0, 0.0f, 2, 1, 4, 0,
        StatusEffect::Slow, 2, 0,
        true, false, true, false, false, 0.0f,
        "smoke", "poof"
    });
    
    RegisterAbility({
        "execute", "Execute", "Instant kill on low HP targets", "skull",
        AbilityType::Attack, TargetType::SingleEnemy,
        50, 0, 0.5f, 1, 0, 5, 0,
        StatusEffect::None, 0, 0,
        false, false, true, true, false, 0.0f,
        "death", "execute"
    });
    
    RegisterAbility({
        "death_mark", "Death Mark", "Marks target for guaranteed crits", "mark",
        AbilityType::Debuff, TargetType::SingleEnemy,
        0, 0, 0.0f, 3, 0, 4, 0,
        StatusEffect::Marked, 3, 0,
        false, false, true, false, false, 0.0f,
        "mark", "curse"
    });
}

// ===========================================================================
// HEALER ABILITIES
// ===========================================================================

void AbilityDatabase::RegisterHealerAbilities() {
    RegisterAbility({
        "heal", "Heal", "Restores HP to an ally", "heal",
        AbilityType::Heal, TargetType::SingleAlly,
        0, 20, 0.8f, 2, 0, 2, 0,
        StatusEffect::None, 0, 0,
        true, true, false, false, false, 0.0f,
        "heal", "heal"
    });
    
    RegisterAbility({
        "mass_heal", "Mass Heal", "Heals all nearby allies", "mass_heal",
        AbilityType::Heal, TargetType::Area,
        0, 15, 0.5f, 0, 2, 4, 0,
        StatusEffect::None, 0, 0,
        true, true, false, false, false, 0.0f,
        "heal_burst", "heal"
    });
    
    RegisterAbility({
        "divine_shield", "Divine Shield", "Shields an ally from damage", "shield",
        AbilityType::Buff, TargetType::SingleAlly,
        0, 0, 0.0f, 2, 0, 4, 0,
        StatusEffect::Shield, 2, 20,
        true, true, false, false, false, 0.0f,
        "shield", "buff"
    });
    
    RegisterAbility({
        "resurrection", "Resurrection", "Revives a fallen ally", "revive",
        AbilityType::Heal, TargetType::SingleAlly,
        0, 50, 0.0f, 2, 0, 8, 0,
        StatusEffect::None, 0, 0,
        false, true, false, false, false, 0.0f,
        "holy", "revive"
    });
    
    RegisterAbility({
        "purify", "Purify", "Removes negative effects from ally", "purify",
        AbilityType::Buff, TargetType::SingleAlly,
        0, 5, 0.0f, 2, 0, 2, 0,
        StatusEffect::None, 0, 0,
        true, true, false, false, false, 0.0f,
        "holy", "purify"
    });
}

// ===========================================================================
// TANK ABILITIES
// ===========================================================================

void AbilityDatabase::RegisterTankAbilities() {
    RegisterAbility({
        "taunt", "Taunt", "Forces enemies to attack you", "taunt",
        AbilityType::Debuff, TargetType::Area,
        0, 0, 0.0f, 0, 2, 3, 0,
        StatusEffect::Taunt, 2, 0,
        false, false, true, false, false, 0.0f,
        "roar", "taunt"
    });
    
    RegisterAbility({
        "fortify", "Fortify", "Greatly increases defense", "armor",
        AbilityType::Buff, TargetType::Self,
        0, 0, 0.0f, 0, 0, 3, 0,
        StatusEffect::DefenseBoost, 3, 10,
        true, false, false, false, false, 0.0f,
        "buff", "armor"
    });
    
    RegisterAbility({
        "inspire", "Inspire", "Boosts nearby allies' defense", "inspire",
        AbilityType::Buff, TargetType::Area,
        0, 0, 0.0f, 0, 2, 4, 0,
        StatusEffect::DefenseBoost, 2, 5,
        true, true, false, false, false, 0.0f,
        "buff", "inspire"
    });
    
    RegisterAbility({
        "earthquake", "Earthquake", "Damages and slows all nearby", "quake",
        AbilityType::Attack, TargetType::Area,
        20, 0, 0.6f, 0, 2, 5, 0,
        StatusEffect::Slow, 2, 0,
        false, false, true, false, false, 0.0f,
        "quake", "rumble"
    });
}

// ===========================================================================
// ARCHER ABILITIES
// ===========================================================================

void AbilityDatabase::RegisterArcherAbilities() {
    RegisterAbility({
        "aimed_shot", "Aimed Shot", "Precise shot with high crit chance", "aim",
        AbilityType::Attack, TargetType::SingleEnemy,
        15, 0, 0.8f, 5, 0, 2, 0,
        StatusEffect::None, 0, 0,
        false, false, true, false, false, 0.3f,
        "arrow", "bow"
    });
    
    RegisterAbility({
        "multishot", "Multishot", "Fires arrows at multiple targets", "arrows",
        AbilityType::Attack, TargetType::Area,
        10, 0, 0.5f, 4, 2, 3, 0,
        StatusEffect::None, 0, 0,
        false, false, true, false, false, 0.0f,
        "arrows", "bow"
    });
    
    RegisterAbility({
        "poison_arrow", "Poison Arrow", "Arrow coated in deadly poison", "poison_arrow",
        AbilityType::Attack, TargetType::SingleEnemy,
        8, 0, 0.4f, 4, 0, 2, 0,
        StatusEffect::Poison, 4, 5,
        false, false, true, false, false, 0.0f,
        "poison", "bow"
    });
    
    RegisterAbility({
        "headshot", "Headshot", "Devastating shot that ignores armor", "headshot",
        AbilityType::Attack, TargetType::SingleEnemy,
        30, 0, 1.0f, 5, 0, 4, 0,
        StatusEffect::None, 0, 0,
        false, false, true, true, true, 0.0f,
        "crit", "bow"
    });
    
    RegisterAbility({
        "arrow_storm", "Arrow Storm", "Rains arrows on a large area", "storm",
        AbilityType::Attack, TargetType::Area,
        12, 0, 0.4f, 4, 3, 5, 0,
        StatusEffect::None, 0, 0,
        false, false, true, false, false, 0.0f,
        "arrows", "storm"
    });
}

// ===========================================================================
// BOSS ABILITIES
// ===========================================================================

void AbilityDatabase::RegisterBossAbilities() {
    // Dragon abilities
    RegisterAbility({
        "fire_breath", "Fire Breath", "Breathes fire in a line", "fire",
        AbilityType::Attack, TargetType::Line,
        25, 0, 0.0f, 4, 0, 2, 0,
        StatusEffect::Burn, 2, 8,
        false, false, true, false, false, 0.0f,
        "fire_line", "roar"
    });
    
    // Necrolord abilities
    RegisterAbility({
        "raise_dead", "Raise Dead", "Summons undead minions", "summon",
        AbilityType::Summon, TargetType::None,
        0, 0, 0.0f, 0, 0, 3, 0,
        StatusEffect::None, 0, 0,
        false, false, false, false, false, 0.0f,
        "dark", "summon"
    });
    
    RegisterAbility({
        "soul_harvest", "Soul Harvest", "Drains life from all enemies", "dark",
        AbilityType::Attack, TargetType::AllEnemies,
        15, 10, 0.0f, 5, 0, 4, 0,
        StatusEffect::Weaken, 2, 3,
        false, false, true, false, false, 0.0f,
        "dark_burst", "drain"
    });
    
    // Titan abilities
    RegisterAbility({
        "titan_stomp", "Titan Stomp", "Shakes the ground violently", "quake",
        AbilityType::Attack, TargetType::Area,
        30, 0, 0.0f, 1, 2, 2, 0,
        StatusEffect::Stun, 1, 0,
        false, false, true, false, false, 0.0f,
        "quake", "stomp"
    });
    
    // Shadow King abilities
    RegisterAbility({
        "shadow_strike", "Shadow Strike", "Teleports and strikes", "shadow",
        AbilityType::Attack, TargetType::SingleEnemy,
        35, 0, 0.0f, 5, 0, 2, 0,
        StatusEffect::None, 0, 0,
        false, false, true, false, true, 0.0f,
        "shadow", "slash"
    });
}

} // namespace DDD
