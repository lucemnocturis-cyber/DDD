#include "Unit.h"
#include "Ability.h"
#include "../Graphics/Renderer.h"
#include "../Utils/Logger.h"

#include <algorithm>

namespace DDD {

Unit::Unit() = default;
Unit::~Unit() = default;

void Unit::Update(float deltaTime) {
    m_animationTime += deltaTime;
}

void Unit::Render(Renderer& renderer, int screenX, int screenY, int cellSize) {
    // Draw unit base (placeholder - would use sprites in full version)
    SDL_Color unitColor;
    
    // Color based on owner
    if (m_owner == Owner::Player) {
        unitColor = {52, 152, 219, 255};  // Blue
    } else {
        unitColor = {231, 76, 60, 255};   // Red
    }
    
    // Draw unit circle
    int padding = 4;
    int unitSize = cellSize - padding * 2;
    renderer.FillRect(screenX + padding, screenY + padding, unitSize, unitSize, unitColor);
    
    // Draw HP bar
    int hpBarHeight = 4;
    int hpBarY = screenY + cellSize - hpBarHeight - 2;
    float hpPercent = static_cast<float>(m_stats.hp) / m_stats.maxHp;
    
    // HP bar background (red)
    renderer.FillRect(screenX + 2, hpBarY, cellSize - 4, hpBarHeight, {200, 50, 50, 255});
    
    // HP bar fill (green)
    int hpBarWidth = static_cast<int>((cellSize - 4) * hpPercent);
    renderer.FillRect(screenX + 2, hpBarY, hpBarWidth, hpBarHeight, {50, 200, 50, 255});
    
    // Draw tier indicator (small dots)
    for (int i = 0; i < m_tier; ++i) {
        int dotX = screenX + 4 + i * 6;
        int dotY = screenY + 2;
        renderer.FillRect(dotX, dotY, 4, 4, {255, 215, 0, 255});  // Gold dots
    }
}

void Unit::TakeDamage(int damage) {
    int actualDamage = std::max(1, damage - m_stats.def);
    m_stats.hp = std::max(0, m_stats.hp - actualDamage);
    
    Logger::Info("{} takes {} damage (HP: {}/{})", 
                 m_className, actualDamage, m_stats.hp, m_stats.maxHp);
    
    if (IsDead()) {
        Logger::Info("{} has been defeated!", m_className);
    }
}

void Unit::Heal(int amount) {
    int oldHp = m_stats.hp;
    m_stats.hp = std::min(m_stats.maxHp, m_stats.hp + amount);
    int actualHeal = m_stats.hp - oldHp;
    
    Logger::Info("{} heals for {} (HP: {}/{})", 
                 m_className, actualHeal, m_stats.hp, m_stats.maxHp);
}

void Unit::MoveTo(const Position& newPos) {
    Logger::Info("{} moves from ({},{}) to ({},{})", 
                 m_className, m_position.x, m_position.y, newPos.x, newPos.y);
    m_position = newPos;
    m_hasMoved = true;
}

void Unit::GainExp(int amount) {
    m_exp += amount;
    Logger::Info("{} gains {} EXP (total: {}/{})", 
                 m_className, amount, m_exp, m_expToPromote);
}

bool Unit::CanPromote() const {
    return m_exp >= m_expToPromote && !m_promotionOptions.empty();
}

void Unit::Promote(const std::string& newClassName) {
    Logger::Info("{} promotes to {}!", m_className, newClassName);
    
    // Store old stats for comparison
    int oldMaxHp = m_stats.maxHp;
    float hpPercent = static_cast<float>(m_stats.hp) / static_cast<float>(m_stats.maxHp);
    
    m_className = newClassName;
    m_tier++;
    m_exp = 0;
    
    // Stats will be updated by the calling code using UnitDatabase
    // This allows proper stat inheritance and bonuses
    
    // Restore HP percentage after stat change
    m_stats.hp = static_cast<int>(hpPercent * m_stats.maxHp);
    if (m_stats.hp < 1) m_stats.hp = 1;
}

void Unit::AddAbility(std::shared_ptr<Ability> ability) {
    m_abilities.push_back(ability);
    Logger::Info("{} learned ability: {}", m_className, ability->GetName());
}

void Unit::RemoveAbility(const std::string& abilityName) {
    auto it = std::remove_if(m_abilities.begin(), m_abilities.end(),
        [&abilityName](const std::shared_ptr<Ability>& ability) {
            return ability->GetName() == abilityName;
        });
    
    if (it != m_abilities.end()) {
        m_abilities.erase(it, m_abilities.end());
        Logger::Info("{} lost ability: {}", m_className, abilityName);
    }
}

void Unit::ProcessStatusEffects() {
    // Process poison
    if (m_poisonTurns > 0) {
        TakeDamage(m_poisonDamage);
        m_poisonTurns--;
        Logger::Info("{} takes {} poison damage ({} turns remaining)", 
                     m_className, m_poisonDamage, m_poisonTurns);
    }
    
    // Process stun
    if (m_stunTurns > 0) {
        m_stunTurns--;
        Logger::Info("{} is stunned ({} turns remaining)", m_className, m_stunTurns);
    }
}

std::string Unit::GetAbilityName() const {
    if (!m_abilities.empty()) {
        return m_abilities[0]->GetName();
    }
    return "";
}

} // namespace DDD
