#include "BossSystem.h"
#include "Unit.h"
#include "Board.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/TextRenderer.h"
#include "../Utils/Logger.h"
#include "../Utils/Random.h"

#include <algorithm>
#include <cmath>

namespace DDD {

BossSystem& BossSystem::Instance() {
    static BossSystem instance;
    return instance;
}

void BossSystem::Initialize() {
    if (m_initialized) return;
    
    RegisterAllBosses();
    
    m_initialized = true;
    Logger::Info("BossSystem initialized with {} bosses", m_bosses.size());
}

void BossSystem::RegisterBoss(const BossDef& def) {
    m_bosses[def.id] = def;
}

const BossDef* BossSystem::GetBossDef(const std::string& id) const {
    auto it = m_bosses.find(id);
    return it != m_bosses.end() ? &it->second : nullptr;
}

void BossSystem::StartBossEncounter(std::shared_ptr<Unit> bossUnit, const std::string& bossId) {
    const BossDef* def = GetBossDef(bossId);
    if (!def) {
        Logger::Warning("Unknown boss ID: {}", bossId);
        return;
    }
    
    m_activeBoss.unit = bossUnit;
    m_activeBoss.definition = def;
    m_activeBoss.currentPhase = 0;
    m_activeBoss.isEnraged = false;
    m_activeBoss.turnCount = 0;
    m_activeBoss.summonedUnits.clear();
    
    // Initialize cooldowns
    m_activeBoss.attackCooldowns.clear();
    for (const auto& attack : def->attacks) {
        m_activeBoss.attackCooldowns.push_back(0);
    }
    
    // Show phase 1 announcement
    if (!def->phases.empty()) {
        m_activeBoss.showingPhaseTransition = true;
        m_activeBoss.phaseTransitionTimer = 3.0f;
        m_activeBoss.phaseAnnouncement = def->phases[0].announcement;
    }
    
    Logger::Info("Boss encounter started: {} - {}", def->name, def->title);
}

void BossSystem::EndBossEncounter() {
    m_activeBoss.unit = nullptr;
    m_activeBoss.definition = nullptr;
    m_activeBoss.summonedUnits.clear();
    Logger::Info("Boss encounter ended");
}

void BossSystem::UpdateBossState() {
    if (!IsBossActive()) return;
    
    CheckPhaseTransition();
}

void BossSystem::CheckPhaseTransition() {
    if (!m_activeBoss.unit || !m_activeBoss.definition) return;
    
    const auto& stats = m_activeBoss.unit->GetStats();
    float hpPercent = static_cast<float>(stats.hp) / static_cast<float>(stats.maxHp);
    
    const auto& phases = m_activeBoss.definition->phases;
    
    // Check if we should advance to next phase
    for (size_t i = m_activeBoss.currentPhase + 1; i < phases.size(); ++i) {
        if (hpPercent <= phases[i].healthThreshold) {
            m_activeBoss.currentPhase = static_cast<int>(i);
            
            // Apply stat multiplier
            auto& mutableStats = m_activeBoss.unit->GetMutableStats();
            mutableStats.atk = static_cast<int>(mutableStats.atk * phases[i].statMultiplier);
            
            // Show announcement
            m_activeBoss.showingPhaseTransition = true;
            m_activeBoss.phaseTransitionTimer = 3.0f;
            m_activeBoss.phaseAnnouncement = phases[i].announcement;
            
            Logger::Info("Boss entered phase {}: {}", i + 1, phases[i].name);
            break;
        }
    }
}

BossSystem::BossAction BossSystem::SelectBossAction(Board& board) {
    BossAction action;
    action.type = BossAttackType::SingleTarget;
    action.damage = 0;
    
    if (!IsBossActive()) return action;
    
    // Check for enrage
    if (m_activeBoss.definition->hasEnrage && 
        m_activeBoss.turnCount >= m_activeBoss.definition->enrageTurn &&
        !m_activeBoss.isEnraged) {
        m_activeBoss.isEnraged = true;
        Logger::Info("Boss has ENRAGED!");
    }
    
    // Select best available attack
    int attackIndex = SelectBestAttack(board);
    if (attackIndex < 0 || attackIndex >= static_cast<int>(m_activeBoss.definition->attacks.size())) {
        // Default attack
        action.attackName = "Attack";
        action.damage = m_activeBoss.unit->GetStats().atk;
        return action;
    }
    
    const BossAttack& attack = m_activeBoss.definition->attacks[attackIndex];
    action.type = attack.type;
    action.attackName = attack.name;
    action.damage = attack.damage;
    
    // Apply enrage multiplier
    if (m_activeBoss.isEnraged) {
        action.damage = static_cast<int>(action.damage * m_activeBoss.definition->enrageMultiplier);
    }
    
    // Apply phase multiplier
    if (m_activeBoss.currentPhase < static_cast<int>(m_activeBoss.definition->phases.size())) {
        float mult = m_activeBoss.definition->phases[m_activeBoss.currentPhase].statMultiplier;
        action.damage = static_cast<int>(action.damage * mult);
    }
    
    // Find target
    action.targetPos = FindBestTarget(board, attack);
    action.affectedCells = CalculateAffectedCells(attack, action.targetPos, board);
    
    // Handle summons
    if (attack.type == BossAttackType::SummonMinions) {
        int currentSummons = static_cast<int>(m_activeBoss.summonedUnits.size());
        int maxSummons = m_activeBoss.definition->maxSummons;
        
        if (currentSummons < maxSummons) {
            // Pick random summon type
            const auto& summonIds = m_activeBoss.definition->summonIds;
            if (!summonIds.empty()) {
                action.summonIds.push_back(summonIds[Random::Range(0, static_cast<int>(summonIds.size()) - 1)]);
            }
        }
    }
    
    // Put attack on cooldown
    m_activeBoss.attackCooldowns[attackIndex] = attack.cooldown;
    
    return action;
}

int BossSystem::SelectBestAttack(Board& board) {
    if (!m_activeBoss.definition) return -1;
    
    const auto& attacks = m_activeBoss.definition->attacks;
    std::vector<int> availableAttacks;
    
    // Find attacks off cooldown
    for (size_t i = 0; i < attacks.size(); ++i) {
        if (i < m_activeBoss.attackCooldowns.size() && m_activeBoss.attackCooldowns[i] <= 0) {
            availableAttacks.push_back(static_cast<int>(i));
        }
    }
    
    if (availableAttacks.empty()) return -1;
    
    // Priority: AoE if multiple targets, summon if low minions, single target otherwise
    // Count player units
    int playerUnits = 0;
    for (int y = 0; y < board.GetHeight(); ++y) {
        for (int x = 0; x < board.GetWidth(); ++x) {
            auto unit = board.GetUnitAt({x, y});
            if (unit && unit->GetOwner() == Owner::Player) {
                playerUnits++;
            }
        }
    }
    
    // Check if we need summons
    int currentSummons = static_cast<int>(m_activeBoss.summonedUnits.size());
    bool needsSummons = currentSummons < m_activeBoss.definition->maxSummons / 2;
    
    // Score each attack
    int bestAttack = availableAttacks[0];
    int bestScore = 0;
    
    for (int idx : availableAttacks) {
        const BossAttack& attack = attacks[idx];
        int score = attack.damage;
        
        // Prefer AoE against grouped enemies
        if (attack.type == BossAttackType::AreaOfEffect && playerUnits >= 2) {
            score += 50;
        }
        
        // Prefer summons when needed
        if (attack.type == BossAttackType::SummonMinions && needsSummons) {
            score += 40;
        }
        
        // Prefer heal when low HP
        if (attack.type == BossAttackType::Heal) {
            const auto& stats = m_activeBoss.unit->GetStats();
            float hpPercent = static_cast<float>(stats.hp) / static_cast<float>(stats.maxHp);
            if (hpPercent < 0.3f) {
                score += 60;
            }
        }
        
        if (score > bestScore) {
            bestScore = score;
            bestAttack = idx;
        }
    }
    
    return bestAttack;
}

Position BossSystem::FindBestTarget(Board& board, const BossAttack& attack) {
    Position bestTarget = m_activeBoss.unit->GetPosition();
    int bestValue = -1;
    
    // Find player units
    std::vector<std::shared_ptr<Unit>> playerUnits;
    for (int y = 0; y < board.GetHeight(); ++y) {
        for (int x = 0; x < board.GetWidth(); ++x) {
            auto unit = board.GetUnitAt({x, y});
            if (unit && unit->GetOwner() == Owner::Player && !unit->IsDead()) {
                playerUnits.push_back(unit);
            }
        }
    }
    
    if (playerUnits.empty()) return bestTarget;
    
    for (const auto& target : playerUnits) {
        Position pos = target->GetPosition();
        int value = 0;
        
        // Prefer low HP targets
        value += (100 - (target->GetStats().hp * 100 / target->GetStats().maxHp));
        
        // For AoE, count nearby units
        if (attack.type == BossAttackType::AreaOfEffect) {
            for (const auto& other : playerUnits) {
                if (other != target) {
                    int dist = std::abs(other->GetPosition().x - pos.x) + 
                               std::abs(other->GetPosition().y - pos.y);
                    if (dist <= attack.radius) {
                        value += 30;
                    }
                }
            }
        }
        
        if (value > bestValue) {
            bestValue = value;
            bestTarget = pos;
        }
    }
    
    return bestTarget;
}

std::vector<Position> BossSystem::CalculateAffectedCells(const BossAttack& attack, const Position& target, Board& board) {
    std::vector<Position> cells;
    cells.push_back(target);
    
    switch (attack.type) {
        case BossAttackType::Cleave:
            // Hit target and adjacent cells
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    if (dx == 0 && dy == 0) continue;
                    Position adj = {target.x + dx, target.y + dy};
                    if (board.IsValidPosition(adj)) {
                        cells.push_back(adj);
                    }
                }
            }
            break;
            
        case BossAttackType::AreaOfEffect:
            // Hit all cells in radius
            for (int dx = -attack.radius; dx <= attack.radius; ++dx) {
                for (int dy = -attack.radius; dy <= attack.radius; ++dy) {
                    if (std::abs(dx) + std::abs(dy) <= attack.radius) {
                        Position aoe = {target.x + dx, target.y + dy};
                        if (board.IsValidPosition(aoe)) {
                            cells.push_back(aoe);
                        }
                    }
                }
            }
            break;
            
        case BossAttackType::LineAttack: {
            // Hit all cells in a line from boss to target
            Position bossPos = m_activeBoss.unit->GetPosition();
            int dx = (target.x > bossPos.x) ? 1 : (target.x < bossPos.x) ? -1 : 0;
            int dy = (target.y > bossPos.y) ? 1 : (target.y < bossPos.y) ? -1 : 0;
            
            Position current = bossPos;
            for (int i = 0; i < attack.range; ++i) {
                current.x += dx;
                current.y += dy;
                if (board.IsValidPosition(current)) {
                    cells.push_back(current);
                }
            }
            break;
        }
        
        default:
            // Single target, already added
            break;
    }
    
    return cells;
}

void BossSystem::OnBossTurnEnd() {
    if (!IsBossActive()) return;
    
    m_activeBoss.turnCount++;
    
    // Reduce cooldowns
    for (auto& cd : m_activeBoss.attackCooldowns) {
        if (cd > 0) cd--;
    }
    
    // Clean up dead summons
    m_activeBoss.summonedUnits.erase(
        std::remove_if(m_activeBoss.summonedUnits.begin(), m_activeBoss.summonedUnits.end(),
            [](const std::shared_ptr<Unit>& u) { return !u || u->IsDead(); }),
        m_activeBoss.summonedUnits.end()
    );
}

std::vector<Position> BossSystem::GetThreatZone(Board& board) const {
    std::vector<Position> zone;
    
    if (!IsBossActive()) return zone;
    
    // Show potential attack range
    Position bossPos = m_activeBoss.unit->GetPosition();
    int maxRange = 3;  // Default threat range
    
    // Find max attack range
    if (m_activeBoss.definition) {
        for (const auto& attack : m_activeBoss.definition->attacks) {
            if (attack.range > maxRange) maxRange = attack.range;
        }
    }
    
    for (int dx = -maxRange; dx <= maxRange; ++dx) {
        for (int dy = -maxRange; dy <= maxRange; ++dy) {
            if (std::abs(dx) + std::abs(dy) <= maxRange) {
                Position threat = {bossPos.x + dx, bossPos.y + dy};
                if (board.IsValidPosition(threat)) {
                    zone.push_back(threat);
                }
            }
        }
    }
    
    return zone;
}

void BossSystem::Update(float deltaTime) {
    m_healthBarPulse += deltaTime * 3.0f;
    m_titleGlow += deltaTime * 2.0f;
    
    if (m_activeBoss.showingPhaseTransition) {
        m_activeBoss.phaseTransitionTimer -= deltaTime;
        if (m_activeBoss.phaseTransitionTimer <= 0) {
            m_activeBoss.showingPhaseTransition = false;
        }
    }
}

void BossSystem::Render(Renderer& renderer, int screenWidth, int screenHeight) {
    if (!IsBossActive()) return;
    
    // Boss health bar at top of screen
    int barWidth = 500;
    int barHeight = 30;
    int barX = (screenWidth - barWidth) / 2;
    int barY = 20;
    
    const auto& stats = m_activeBoss.unit->GetStats();
    float hpPercent = static_cast<float>(stats.hp) / static_cast<float>(stats.maxHp);
    
    // Background
    renderer.FillRect(barX - 4, barY - 4, barWidth + 8, barHeight + 8, {20, 20, 30, 255});
    renderer.FillRect(barX - 2, barY - 2, barWidth + 4, barHeight + 4, {40, 40, 60, 255});
    
    // Health bar background
    renderer.FillRect(barX, barY, barWidth, barHeight, {60, 20, 20, 255});
    
    // Health bar fill
    int fillWidth = static_cast<int>(barWidth * hpPercent);
    SDL_Color healthColor = {200, 50, 50, 255};
    if (m_activeBoss.isEnraged) {
        // Pulsing red when enraged
        float pulse = (std::sin(m_healthBarPulse * 2) + 1.0f) * 0.5f;
        healthColor.r = 200 + static_cast<uint8_t>(pulse * 55);
    }
    renderer.FillRect(barX, barY, fillWidth, barHeight, healthColor);
    
    // Phase markers
    for (const auto& phase : m_activeBoss.definition->phases) {
        int markerX = barX + static_cast<int>(barWidth * phase.healthThreshold);
        renderer.FillRect(markerX - 1, barY, 2, barHeight, {255, 255, 255, 150});
    }
    
    // Border
    renderer.DrawRect(barX, barY, barWidth, barHeight, {180, 150, 50, 255});
    
    // Boss name and title
    std::string bossName = m_activeBoss.definition->name;
    std::string bossTitle = m_activeBoss.definition->title;
    
    SDL_Color nameColor = {255, 200, 100, 255};
    float glow = (std::sin(m_titleGlow) + 1.0f) * 0.3f;
    nameColor.r = std::min(255, static_cast<int>(nameColor.r + glow * 50));
    
    renderer.GetTextRenderer()->RenderText(bossName, barX + barWidth / 2 - 60, barY - 25, 
                                           FontSize::Large, nameColor);
    
    SDL_Color titleColor = {200, 180, 150, 255};
    renderer.GetTextRenderer()->RenderText(bossTitle, barX + barWidth / 2 - 80, barY + barHeight + 5, 
                                           FontSize::Small, titleColor);
    
    // HP text
    std::string hpText = std::to_string(stats.hp) + " / " + std::to_string(stats.maxHp);
    renderer.GetTextRenderer()->RenderText(hpText, barX + barWidth / 2 - 40, barY + 5, 
                                           FontSize::Medium, {255, 255, 255, 255});
    
    // Enrage indicator
    if (m_activeBoss.isEnraged) {
        SDL_Color enrageColor = {255, 100, 100, 255};
        renderer.GetTextRenderer()->RenderText("ENRAGED!", barX + barWidth + 10, barY + 5, 
                                               FontSize::Medium, enrageColor);
    }
    
    // Phase transition announcement
    if (m_activeBoss.showingPhaseTransition) {
        float alpha = std::min(1.0f, m_activeBoss.phaseTransitionTimer);
        uint8_t a = static_cast<uint8_t>(alpha * 255);
        
        // Dark overlay
        renderer.FillRect(0, screenHeight / 2 - 50, screenWidth, 100, {0, 0, 0, static_cast<uint8_t>(a * 0.7f)});
        
        // Announcement text
        SDL_Color announceColor = {255, 220, 100, a};
        int textX = screenWidth / 2 - 150;
        int textY = screenHeight / 2 - 15;
        renderer.GetTextRenderer()->RenderText(m_activeBoss.phaseAnnouncement, textX, textY, 
                                               FontSize::XLarge, announceColor);
    }
}

// ===========================================================================
// BOSS DEFINITIONS
// ===========================================================================

void BossSystem::RegisterAllBosses() {
    // ANCIENT DRAGON
    RegisterBoss({
        "boss_dragon",
        "Ancient Dragon",
        "Wyrm of Destruction",
        {
            {"Awakened", 1.0f, "Fire Breath", 20, 3, 1.0f, "The Dragon awakens!"},
            {"Furious", 0.6f, "Inferno", 30, 4, 1.3f, "The Dragon becomes FURIOUS!"},
            {"Desperate", 0.3f, "Apocalypse", 50, 5, 1.5f, "The Dragon's rage is UNLEASHED!"}
        },
        {
            {"Fire Breath", BossAttackType::LineAttack, 25, 4, 0, 2, 0, "fire", "Breathes fire in a line"},
            {"Tail Swipe", BossAttackType::Cleave, 18, 1, 0, 1, 0, "impact", "Hits nearby units"},
            {"Inferno", BossAttackType::AreaOfEffect, 15, 3, 2, 4, 0, "explosion", "Burns a large area"},
            {"Dragon's Roar", BossAttackType::Debuff, 0, 5, 3, 5, 0, "roar", "Weakens all enemies"}
        },
        {"imp", "imp"},
        3,
        true, 8, 1.5f
    });
    
    // THE NECROLORD
    RegisterBoss({
        "boss_necrolord",
        "The Necrolord",
        "Master of the Undead",
        {
            {"Awakened", 1.0f, "Death Bolt", 15, 2, 1.0f, "The Necrolord rises!"},
            {"Summoning", 0.5f, "Mass Raise", 0, 3, 1.2f, "The dead shall serve!"},
            {"Lich Form", 0.25f, "Soul Harvest", 40, 4, 1.4f, "TRUE POWER AWAKENS!"}
        },
        {
            {"Death Bolt", BossAttackType::SingleTarget, 20, 4, 0, 1, 0, "dark", "Dark magic bolt"},
            {"Raise Dead", BossAttackType::SummonMinions, 0, 0, 0, 3, 0, "summon", "Summons undead"},
            {"Life Drain", BossAttackType::SingleTarget, 15, 3, 0, 2, 0, "drain", "Drains HP, heals self"},
            {"Plague", BossAttackType::AreaOfEffect, 10, 4, 2, 4, 0, "poison", "Poisons an area"}
        },
        {"skeleton", "skeleton", "armored_skeleton"},
        5,
        true, 10, 1.4f
    });
    
    // TITAN
    RegisterBoss({
        "boss_titan",
        "Titan",
        "Ancient Giant",
        {
            {"Standing", 1.0f, "Stomp", 25, 2, 1.0f, "The Titan STANDS!"},
            {"Rampaging", 0.5f, "Earthquake", 35, 3, 1.3f, "The ground TREMBLES!"},
            {"Unstoppable", 0.2f, "Cataclysm", 50, 4, 1.6f, "NOTHING can stop it!"}
        },
        {
            {"Stomp", BossAttackType::AreaOfEffect, 30, 1, 1, 1, 0, "impact", "Stomps the ground"},
            {"Boulder Throw", BossAttackType::SingleTarget, 35, 5, 0, 2, 0, "rock", "Throws a massive boulder"},
            {"Earthquake", BossAttackType::AreaOfEffect, 20, 3, 3, 4, 0, "quake", "Shakes the entire area"},
            {"Regenerate", BossAttackType::Heal, 40, 0, 0, 5, 0, "heal", "Heals self"}
        },
        {"golem", "slime"},
        2,
        true, 6, 1.6f
    });
    
    // SHADOW KING
    RegisterBoss({
        "boss_shadow_king",
        "Shadow King",
        "Ruler of Darkness",
        {
            {"Emerging", 1.0f, "Shadow Strike", 20, 2, 1.0f, "Darkness descends..."},
            {"Hunting", 0.6f, "Assassinate", 40, 3, 1.2f, "You cannot hide!"},
            {"Shadow Form", 0.3f, "Oblivion", 60, 4, 1.5f, "EMBRACE THE VOID!"}
        },
        {
            {"Shadow Strike", BossAttackType::SingleTarget, 30, 3, 0, 1, 0, "shadow", "Quick shadow attack"},
            {"Assassinate", BossAttackType::SingleTarget, 50, 5, 0, 3, 0, "death", "Massive damage to one target"},
            {"Shadow Clones", BossAttackType::SummonMinions, 0, 0, 0, 4, 0, "clone", "Creates shadow clones"},
            {"Void Zone", BossAttackType::AreaOfEffect, 15, 4, 2, 3, 0, "void", "Creates damaging zone"}
        },
        {"shadow_assassin", "bandit"},
        4,
        true, 7, 1.5f
    });
}

} // namespace DDD
