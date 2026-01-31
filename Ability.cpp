#include "Ability.h"
#include "Unit.h"
#include "Board.h"
#include "../Utils/Logger.h"

namespace DDD {

// ============================================================================
// Base Ability
// ============================================================================

Ability::Ability() = default;
Ability::~Ability() = default;

bool Ability::CanUse(const Unit& caster, const Board& board) const {
    if (m_currentCooldown > 0) return false;
    if (caster.IsStunned()) return false;
    return true;
}

std::vector<Position> Ability::GetValidTargets(const Unit& caster, const Board& board) const {
    std::vector<Position> targets;
    const Position& pos = caster.GetPosition();
    
    switch (m_targetType) {
        case TargetType::Self:
            targets.push_back(pos);
            break;
            
        case TargetType::SingleEnemy:
            for (int dy = -m_range; dy <= m_range; ++dy) {
                for (int dx = -m_range; dx <= m_range; ++dx) {
                    int nx = pos.x + dx;
                    int ny = pos.y + dy;
                    if (!board.IsValidPosition(nx, ny)) continue;
                    if (board.GetDistance(pos, {nx, ny}) > m_range) continue;
                    
                    auto target = board.GetUnitAt(nx, ny);
                    if (target && target->GetOwner() != caster.GetOwner()) {
                        targets.push_back({nx, ny});
                    }
                }
            }
            break;
            
        case TargetType::SingleAlly:
            for (int dy = -m_range; dy <= m_range; ++dy) {
                for (int dx = -m_range; dx <= m_range; ++dx) {
                    int nx = pos.x + dx;
                    int ny = pos.y + dy;
                    if (!board.IsValidPosition(nx, ny)) continue;
                    if (board.GetDistance(pos, {nx, ny}) > m_range) continue;
                    
                    auto target = board.GetUnitAt(nx, ny);
                    if (target && target->GetOwner() == caster.GetOwner()) {
                        targets.push_back({nx, ny});
                    }
                }
            }
            break;
            
        case TargetType::Area:
            // Return all cells in range (for AoE)
            for (int dy = -m_range; dy <= m_range; ++dy) {
                for (int dx = -m_range; dx <= m_range; ++dx) {
                    int nx = pos.x + dx;
                    int ny = pos.y + dy;
                    if (board.IsValidPosition(nx, ny) && 
                        board.GetDistance(pos, {nx, ny}) <= m_range) {
                        targets.push_back({nx, ny});
                    }
                }
            }
            break;
            
        default:
            break;
    }
    
    return targets;
}

void Ability::TickCooldown() {
    if (m_currentCooldown > 0) {
        m_currentCooldown--;
    }
}

// ============================================================================
// Concrete Abilities
// ============================================================================

FireballAbility::FireballAbility() {
    m_name = "Fireball";
    m_description = "Launch a fireball dealing damage in a 2x2 area";
    m_type = AbilityType::Attack;
    m_targetType = TargetType::Area;
    m_cooldown = 2;
    m_range = 5;
    m_aoeRadius = 1;
}

void FireballAbility::Execute(Unit& caster, const std::vector<Unit*>& targets, Board& board) {
    int damage = caster.GetStats().atk;
    
    for (Unit* target : targets) {
        if (target && target->GetOwner() != caster.GetOwner()) {
            target->TakeDamage(damage);
            Logger::Info("{} hits {} with Fireball for {} damage",
                        caster.GetClassName(), target->GetClassName(), damage);
        }
    }
    
    m_currentCooldown = m_cooldown;
}

IceShardAbility::IceShardAbility() {
    m_name = "Ice Shard";
    m_description = "Fire an ice shard that damages and freezes the target";
    m_type = AbilityType::Attack;
    m_targetType = TargetType::SingleEnemy;
    m_cooldown = 1;
    m_range = 4;
}

void IceShardAbility::Execute(Unit& caster, const std::vector<Unit*>& targets, Board& board) {
    if (targets.empty()) return;
    
    Unit* target = targets[0];
    int damage = caster.GetStats().atk + 2;  // Bonus damage
    
    target->TakeDamage(damage);
    target->ApplyStun(1);  // Freeze for 1 turn
    
    Logger::Info("{} freezes {} with Ice Shard for {} damage",
                caster.GetClassName(), target->GetClassName(), damage);
    
    m_currentCooldown = m_cooldown;
}

ShieldBashAbility::ShieldBashAbility() {
    m_name = "Shield Bash";
    m_description = "Strike with your shield, dealing damage and pushing the enemy back";
    m_type = AbilityType::Attack;
    m_targetType = TargetType::SingleEnemy;
    m_cooldown = 1;
    m_range = 1;
}

void ShieldBashAbility::Execute(Unit& caster, const std::vector<Unit*>& targets, Board& board) {
    if (targets.empty()) return;
    
    Unit* target = targets[0];
    int damage = caster.GetStats().atk;
    
    target->TakeDamage(damage);
    
    // Calculate push direction
    Position casterPos = caster.GetPosition();
    Position targetPos = target->GetPosition();
    int dx = targetPos.x - casterPos.x;
    int dy = targetPos.y - casterPos.y;
    
    // Normalize to direction
    if (dx != 0) dx = dx / std::abs(dx);
    if (dy != 0) dy = dy / std::abs(dy);
    
    // Try to push
    Position newPos = {targetPos.x + dx, targetPos.y + dy};
    if (board.IsValidPosition(newPos.x, newPos.y) && !board.GetUnitAt(newPos.x, newPos.y)) {
        target->MoveTo(newPos);
        Logger::Info("{} pushes {} back with Shield Bash", 
                    caster.GetClassName(), target->GetClassName());
    }
    
    m_currentCooldown = m_cooldown;
}

BackstabAbility::BackstabAbility() {
    m_name = "Backstab";
    m_description = "Deal triple damage when attacking from behind";
    m_type = AbilityType::Attack;
    m_targetType = TargetType::SingleEnemy;
    m_cooldown = 2;
    m_range = 1;
}

void BackstabAbility::Execute(Unit& caster, const std::vector<Unit*>& targets, Board& board) {
    if (targets.empty()) return;
    
    Unit* target = targets[0];
    int baseDamage = caster.GetStats().atk;
    
    // Check if attacking from behind (simplified - just triple damage)
    int damage = baseDamage * 3;
    target->TakeDamage(damage);
    
    Logger::Info("{} backstabs {} for {} damage!",
                caster.GetClassName(), target->GetClassName(), damage);
    
    m_currentCooldown = m_cooldown;
}

PoisonAbility::PoisonAbility() {
    m_name = "Poison";
    m_description = "Apply poison that deals damage over 3 turns";
    m_type = AbilityType::Debuff;
    m_targetType = TargetType::SingleEnemy;
    m_cooldown = 1;
    m_range = 1;
}

void PoisonAbility::Execute(Unit& caster, const std::vector<Unit*>& targets, Board& board) {
    if (targets.empty()) return;
    
    Unit* target = targets[0];
    int poisonDamage = 3;
    int poisonTurns = 3;
    
    target->ApplyPoison(poisonDamage, poisonTurns);
    
    Logger::Info("{} poisons {} ({} damage for {} turns)",
                caster.GetClassName(), target->GetClassName(), poisonDamage, poisonTurns);
    
    m_currentCooldown = m_cooldown;
}

TauntAbility::TauntAbility() {
    m_name = "Taunt";
    m_description = "Force nearby enemies to attack you";
    m_type = AbilityType::Utility;
    m_targetType = TargetType::Self;
    m_cooldown = 2;
    m_range = 0;
}

void TauntAbility::Execute(Unit& caster, const std::vector<Unit*>& targets, Board& board) {
    // TODO: Implement taunt effect (requires AI modification)
    Logger::Info("{} taunts nearby enemies!", caster.GetClassName());
    m_currentCooldown = m_cooldown;
}

DefensiveStanceAbility::DefensiveStanceAbility() {
    m_name = "Defensive Stance";
    m_description = "Increase defense by 5 for 2 turns";
    m_type = AbilityType::Buff;
    m_targetType = TargetType::Self;
    m_cooldown = 3;
    m_range = 0;
}

void DefensiveStanceAbility::Execute(Unit& caster, const std::vector<Unit*>& targets, Board& board) {
    // Apply defense buff
    caster.GetMutableStats().def += 5;
    
    // TODO: Track buff duration and remove after 2 turns
    Logger::Info("{} enters defensive stance (+5 DEF)", caster.GetClassName());
    m_currentCooldown = m_cooldown;
}

// ============================================================================
// AbilityFactory
// ============================================================================

std::shared_ptr<Ability> AbilityFactory::CreateAbility(const std::string& name) {
    if (name == "Fireball") return std::make_shared<FireballAbility>();
    if (name == "Ice Shard") return std::make_shared<IceShardAbility>();
    if (name == "Shield Bash") return std::make_shared<ShieldBashAbility>();
    if (name == "Backstab") return std::make_shared<BackstabAbility>();
    if (name == "Poison") return std::make_shared<PoisonAbility>();
    if (name == "Taunt") return std::make_shared<TauntAbility>();
    if (name == "Defensive Stance") return std::make_shared<DefensiveStanceAbility>();
    
    Logger::Warning("Unknown ability: {}", name);
    return nullptr;
}

} // namespace DDD
