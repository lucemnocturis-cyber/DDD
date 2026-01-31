#include "ParticleSystem.h"
#include "Renderer.h"
#include "../Utils/Random.h"
#include "../Utils/Logger.h"

#include <cmath>
#include <algorithm>

namespace DDD {

// ============================================================================
// ParticleEmitter
// ============================================================================

ParticleEmitter::ParticleEmitter() {
    m_particles.reserve(100);
}

void ParticleEmitter::Configure(const EmitterConfig& config) {
    m_config = config;
}

void ParticleEmitter::SetPosition(float x, float y) {
    m_x = x;
    m_y = y;
}

void ParticleEmitter::Start() {
    m_emitting = true;
    m_emitTimer = 0.0f;
    m_durationTimer = 0.0f;
}

void ParticleEmitter::Stop() {
    m_emitting = false;
}

void ParticleEmitter::Burst(int count) {
    for (int i = 0; i < count; ++i) {
        EmitParticle();
    }
}

void ParticleEmitter::EmitParticle() {
    Particle p;
    
    // Position with variance
    float angle = Random::GetFloat(0.0f, 2.0f * 3.14159f);
    float radius = Random::GetFloat(0.0f, m_config.spawnRadius);
    p.x = m_x + std::cos(angle) * radius;
    p.y = m_y + std::sin(angle) * radius;
    
    // Velocity
    float speed = Random::GetFloat(m_config.minSpeed, m_config.maxSpeed);
    float velAngle = Random::GetFloat(m_config.minAngle, m_config.maxAngle) * 3.14159f / 180.0f;
    p.vx = std::cos(velAngle) * speed;
    p.vy = std::sin(velAngle) * speed;
    
    // Acceleration
    p.ax = m_config.gravityX;
    p.ay = m_config.gravityY;
    
    // Lifetime
    p.life = Random::GetFloat(m_config.minLife, m_config.maxLife);
    p.maxLife = p.life;
    
    // Size
    float sizeVar = Random::GetFloat(-m_config.sizeVariance, m_config.sizeVariance);
    p.startSize = m_config.startSize + sizeVar;
    p.endSize = m_config.endSize + sizeVar * 0.5f;
    p.size = p.startSize;
    
    // Rotation
    p.rotation = Random::GetFloat(0.0f, 360.0f);
    p.rotationSpeed = Random::GetFloat(m_config.minRotationSpeed, m_config.maxRotationSpeed);
    
    // Color
    if (m_config.randomColor) {
        p.startColor = {
            static_cast<uint8_t>(Random::GetInt(100, 255)),
            static_cast<uint8_t>(Random::GetInt(100, 255)),
            static_cast<uint8_t>(Random::GetInt(100, 255)),
            255
        };
        p.endColor = p.startColor;
        p.endColor.a = 0;
    } else {
        p.startColor = m_config.startColor;
        p.endColor = m_config.endColor;
    }
    
    p.active = true;
    m_particles.push_back(p);
}

void ParticleEmitter::Update(float deltaTime) {
    // Emit new particles
    if (m_emitting) {
        // Check duration
        if (m_config.duration > 0) {
            m_durationTimer += deltaTime;
            if (m_durationTimer >= m_config.duration) {
                m_emitting = false;
            }
        }
        
        // Emit based on rate
        if (m_config.emitRate > 0) {
            m_emitTimer += deltaTime;
            float emitInterval = 1.0f / m_config.emitRate;
            while (m_emitTimer >= emitInterval) {
                m_emitTimer -= emitInterval;
                EmitParticle();
            }
        }
        
        // Handle burst
        if (m_config.burstCount > 0) {
            Burst(m_config.burstCount);
            m_config.burstCount = 0;
            if (m_config.duration <= 0) {
                m_emitting = false;
            }
        }
    }
    
    // Update particles
    for (auto& p : m_particles) {
        if (!p.active) continue;
        
        // Update life
        p.life -= deltaTime;
        if (p.life <= 0) {
            p.active = false;
            continue;
        }
        
        // Update physics
        p.vx += p.ax * deltaTime;
        p.vy += p.ay * deltaTime;
        p.x += p.vx * deltaTime;
        p.y += p.vy * deltaTime;
        
        // Update rotation
        p.rotation += p.rotationSpeed * deltaTime;
        
        // Update size (lerp)
        float lifeRatio = p.life / p.maxLife;
        p.size = p.endSize + (p.startSize - p.endSize) * lifeRatio;
    }
    
    // Remove dead particles
    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(),
            [](const Particle& p) { return !p.active; }),
        m_particles.end()
    );
}

SDL_Color ParticleEmitter::LerpColor(const SDL_Color& a, const SDL_Color& b, float t) {
    return {
        static_cast<uint8_t>(a.r + (b.r - a.r) * t),
        static_cast<uint8_t>(a.g + (b.g - a.g) * t),
        static_cast<uint8_t>(a.b + (b.b - a.b) * t),
        static_cast<uint8_t>(a.a + (b.a - a.a) * t)
    };
}

void ParticleEmitter::Render(Renderer& renderer) {
    for (const auto& p : m_particles) {
        if (!p.active) continue;
        
        // Calculate color based on lifetime
        float lifeRatio = 1.0f - (p.life / p.maxLife);
        SDL_Color color = LerpColor(p.startColor, p.endColor, lifeRatio);
        
        int x = static_cast<int>(p.x - p.size / 2);
        int y = static_cast<int>(p.y - p.size / 2);
        int size = static_cast<int>(p.size);
        
        switch (m_config.shape) {
            case EmitterConfig::Shape::Circle:
                renderer.FillCircle(static_cast<int>(p.x), static_cast<int>(p.y), 
                                    size / 2, color);
                break;
                
            case EmitterConfig::Shape::Star: {
                // Simple star approximation with lines
                int cx = static_cast<int>(p.x);
                int cy = static_cast<int>(p.y);
                int r = size / 2;
                renderer.DrawLine(cx - r, cy, cx + r, cy, color);
                renderer.DrawLine(cx, cy - r, cx, cy + r, color);
                renderer.DrawLine(cx - r/2, cy - r/2, cx + r/2, cy + r/2, color);
                renderer.DrawLine(cx + r/2, cy - r/2, cx - r/2, cy + r/2, color);
                break;
            }
                
            case EmitterConfig::Shape::Square:
            default:
                renderer.FillRect(x, y, size, size, color);
                break;
        }
    }
}

// ============================================================================
// ParticleSystem
// ============================================================================

ParticleSystem::ParticleSystem() {
    // Configure damage effect
    m_damageConfig.burstCount = 8;
    m_damageConfig.minSpeed = 80.0f;
    m_damageConfig.maxSpeed = 150.0f;
    m_damageConfig.minAngle = 200.0f;  // Upward spread
    m_damageConfig.maxAngle = 340.0f;
    m_damageConfig.gravityY = 200.0f;  // Fall down
    m_damageConfig.minLife = 0.3f;
    m_damageConfig.maxLife = 0.6f;
    m_damageConfig.startSize = 6.0f;
    m_damageConfig.endSize = 2.0f;
    m_damageConfig.startColor = {255, 100, 100, 255};
    m_damageConfig.endColor = {255, 50, 50, 0};
    m_damageConfig.shape = EmitterConfig::Shape::Square;
    
    // Configure critical hit effect
    m_criticalConfig = m_damageConfig;
    m_criticalConfig.burstCount = 15;
    m_criticalConfig.minSpeed = 100.0f;
    m_criticalConfig.maxSpeed = 200.0f;
    m_criticalConfig.startSize = 8.0f;
    m_criticalConfig.startColor = {255, 200, 50, 255};
    m_criticalConfig.endColor = {255, 100, 0, 0};
    m_criticalConfig.shape = EmitterConfig::Shape::Star;
    
    // Configure heal effect
    m_healConfig.burstCount = 10;
    m_healConfig.minSpeed = 30.0f;
    m_healConfig.maxSpeed = 60.0f;
    m_healConfig.minAngle = 240.0f;  // Upward
    m_healConfig.maxAngle = 300.0f;
    m_healConfig.gravityY = -50.0f;  // Float up
    m_healConfig.minLife = 0.5f;
    m_healConfig.maxLife = 1.0f;
    m_healConfig.startSize = 5.0f;
    m_healConfig.endSize = 1.0f;
    m_healConfig.startColor = {100, 255, 100, 255};
    m_healConfig.endColor = {200, 255, 200, 0};
    m_healConfig.shape = EmitterConfig::Shape::Circle;
    
    // Configure death explosion
    m_deathConfig.burstCount = 25;
    m_deathConfig.spawnRadius = 10.0f;
    m_deathConfig.minSpeed = 50.0f;
    m_deathConfig.maxSpeed = 150.0f;
    m_deathConfig.minAngle = 0.0f;
    m_deathConfig.maxAngle = 360.0f;
    m_deathConfig.gravityY = 100.0f;
    m_deathConfig.minLife = 0.4f;
    m_deathConfig.maxLife = 0.8f;
    m_deathConfig.startSize = 10.0f;
    m_deathConfig.endSize = 3.0f;
    m_deathConfig.sizeVariance = 3.0f;
    m_deathConfig.shape = EmitterConfig::Shape::Square;
    
    // Configure territory capture
    m_captureConfig.burstCount = 12;
    m_captureConfig.minSpeed = 20.0f;
    m_captureConfig.maxSpeed = 50.0f;
    m_captureConfig.minAngle = 0.0f;
    m_captureConfig.maxAngle = 360.0f;
    m_captureConfig.gravityY = -30.0f;  // Float up
    m_captureConfig.minLife = 0.3f;
    m_captureConfig.maxLife = 0.5f;
    m_captureConfig.startSize = 6.0f;
    m_captureConfig.endSize = 2.0f;
    m_captureConfig.shape = EmitterConfig::Shape::Square;
    
    Logger::Info("ParticleSystem initialized");
}

ParticleSystem::~ParticleSystem() = default;

void ParticleSystem::Update(float deltaTime) {
    // Update all emitters
    for (auto& emitter : m_emitters) {
        emitter->Update(deltaTime);
    }
    
    // Remove inactive emitters
    m_emitters.erase(
        std::remove_if(m_emitters.begin(), m_emitters.end(),
            [](const std::unique_ptr<ParticleEmitter>& e) { return !e->IsActive(); }),
        m_emitters.end()
    );
}

void ParticleSystem::Render(Renderer& renderer) {
    for (auto& emitter : m_emitters) {
        emitter->Render(renderer);
    }
}

void ParticleSystem::SpawnDamageEffect(float x, float y, int damage, bool isCritical) {
    auto emitter = std::make_unique<ParticleEmitter>();
    
    if (isCritical) {
        EmitterConfig config = m_criticalConfig;
        config.burstCount = 15 + damage / 2;  // More particles for bigger hits
        emitter->Configure(config);
    } else {
        EmitterConfig config = m_damageConfig;
        config.burstCount = 8 + damage / 3;
        emitter->Configure(config);
    }
    
    emitter->SetPosition(x, y);
    emitter->Start();
    
    m_emitters.push_back(std::move(emitter));
}

void ParticleSystem::SpawnHealEffect(float x, float y, int amount) {
    auto emitter = std::make_unique<ParticleEmitter>();
    
    EmitterConfig config = m_healConfig;
    config.burstCount = 10 + amount / 2;
    emitter->Configure(config);
    emitter->SetPosition(x, y);
    emitter->Start();
    
    m_emitters.push_back(std::move(emitter));
}

void ParticleSystem::SpawnDeathEffect(float x, float y, SDL_Color unitColor) {
    auto emitter = std::make_unique<ParticleEmitter>();
    
    EmitterConfig config = m_deathConfig;
    config.startColor = unitColor;
    config.endColor = {unitColor.r, unitColor.g, unitColor.b, 0};
    emitter->Configure(config);
    emitter->SetPosition(x, y);
    emitter->Start();
    
    m_emitters.push_back(std::move(emitter));
}

void ParticleSystem::SpawnCaptureEffect(float x, float y, SDL_Color ownerColor) {
    auto emitter = std::make_unique<ParticleEmitter>();
    
    EmitterConfig config = m_captureConfig;
    config.startColor = ownerColor;
    config.endColor = {ownerColor.r, ownerColor.g, ownerColor.b, 0};
    emitter->Configure(config);
    emitter->SetPosition(x, y);
    emitter->Start();
    
    m_emitters.push_back(std::move(emitter));
}

void ParticleSystem::SpawnBurst(float x, float y, const EmitterConfig& config) {
    auto emitter = std::make_unique<ParticleEmitter>();
    emitter->Configure(config);
    emitter->SetPosition(x, y);
    emitter->Start();
    
    m_emitters.push_back(std::move(emitter));
}

void ParticleSystem::Clear() {
    m_emitters.clear();
}

size_t ParticleSystem::GetTotalParticles() const {
    size_t total = 0;
    for (const auto& emitter : m_emitters) {
        total += emitter->GetParticleCount();
    }
    return total;
}

} // namespace DDD
