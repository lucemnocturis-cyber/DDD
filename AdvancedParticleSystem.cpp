#include "AdvancedParticleSystem.h"
#include "../Utils/Logger.h"
#include "../Utils/Random.h"

#include <algorithm>
#include <cmath>

namespace DDD {

AdvancedParticleSystem& AdvancedParticleSystem::Instance() {
    static AdvancedParticleSystem instance;
    return instance;
}

void AdvancedParticleSystem::Initialize() {
    if (m_initialized) return;
    
    RegisterAllEmitters();
    RegisterAllEffects();
    
    m_initialized = true;
    Logger::Info("AdvancedParticleSystem initialized with {} emitters, {} effects",
                 m_emitterConfigs.size(), m_effects.size());
}

void AdvancedParticleSystem::Shutdown() {
    Clear();
    m_emitterConfigs.clear();
    m_effects.clear();
    m_initialized = false;
}

void AdvancedParticleSystem::Update(float deltaTime) {
    deltaTime *= m_globalTimeScale;
    
    for (auto& emitter : m_emitters) {
        if (emitter.active && !emitter.paused) {
            UpdateEmitter(emitter, deltaTime);
        }
    }
    
    // Remove inactive emitters with no particles
    m_emitters.erase(
        std::remove_if(m_emitters.begin(), m_emitters.end(),
            [](const EmitterInstance& e) {
                return !e.active && e.particles.empty();
            }),
        m_emitters.end()
    );
}

void AdvancedParticleSystem::Render(SDL_Renderer* renderer) {
    for (const auto& emitter : m_emitters) {
        const EmitterConfig* config = GetEmitterConfig(emitter.configId);
        if (!config) continue;
        
        // Set blend mode
        SDL_BlendMode blendMode = SDL_BLENDMODE_BLEND;
        if (config->blendMode == ParticleBlend::Additive) {
            blendMode = SDL_BLENDMODE_ADD;
        }
        SDL_SetRenderDrawBlendMode(renderer, blendMode);
        
        for (const auto& particle : emitter.particles) {
            if (particle.alive && particle.delay <= 0) {
                RenderParticle(renderer, particle, *config);
            }
        }
    }
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
}

void AdvancedParticleSystem::RegisterEmitter(const EmitterConfig& config) {
    m_emitterConfigs[config.id] = config;
}

const EmitterConfig* AdvancedParticleSystem::GetEmitterConfig(const std::string& id) const {
    auto it = m_emitterConfigs.find(id);
    return it != m_emitterConfigs.end() ? &it->second : nullptr;
}

void AdvancedParticleSystem::RegisterEffect(const ParticleEffect& effect) {
    m_effects[effect.id] = effect;
}

const ParticleEffect* AdvancedParticleSystem::GetEffect(const std::string& id) const {
    auto it = m_effects.find(id);
    return it != m_effects.end() ? &it->second : nullptr;
}

int AdvancedParticleSystem::SpawnEmitter(const std::string& configId, float x, float y) {
    return SpawnEmitterRotated(configId, x, y, 0.0f);
}

int AdvancedParticleSystem::SpawnEmitterRotated(const std::string& configId, float x, float y, float rotation) {
    const EmitterConfig* config = GetEmitterConfig(configId);
    if (!config) {
        Logger::Warning("Emitter config not found: {}", configId);
        return -1;
    }
    
    EmitterInstance instance;
    instance.instanceId = m_nextInstanceId++;
    instance.configId = configId;
    instance.x = x;
    instance.y = y;
    instance.rotation = rotation;
    instance.scale = 1.0f;
    instance.emissionAccum = 0.0f;
    instance.burstTimer = 0.0f;
    instance.lifetime = 0.0f;
    instance.active = true;
    instance.paused = false;
    
    instance.particles.reserve(config->maxParticles);
    
    // Initial burst
    if (config->burstCount > 0) {
        for (int i = 0; i < config->burstCount; ++i) {
            EmitParticle(instance, *config);
        }
    }
    
    m_emitters.push_back(std::move(instance));
    return instance.instanceId;
}

int AdvancedParticleSystem::SpawnEffect(const std::string& effectId, float x, float y) {
    const ParticleEffect* effect = GetEffect(effectId);
    if (!effect) {
        Logger::Warning("Effect not found: {}", effectId);
        return -1;
    }
    
    int firstId = -1;
    for (const auto& emitterId : effect->emitterIds) {
        int id = SpawnEmitter(emitterId, x, y);
        if (firstId < 0) firstId = id;
    }
    
    return firstId;
}

void AdvancedParticleSystem::SpawnBurst(const std::string& configId, float x, float y, int count) {
    const EmitterConfig* config = GetEmitterConfig(configId);
    if (!config) return;
    
    EmitterInstance instance;
    instance.instanceId = m_nextInstanceId++;
    instance.configId = configId;
    instance.x = x;
    instance.y = y;
    instance.rotation = 0.0f;
    instance.scale = 1.0f;
    instance.active = false;  // One-shot, immediately inactive
    instance.paused = false;
    
    for (int i = 0; i < count; ++i) {
        EmitParticle(instance, *config);
    }
    
    m_emitters.push_back(std::move(instance));
}

void AdvancedParticleSystem::StopEmitter(int instanceId) {
    for (auto& emitter : m_emitters) {
        if (emitter.instanceId == instanceId) {
            emitter.active = false;
            break;
        }
    }
}

void AdvancedParticleSystem::PauseEmitter(int instanceId) {
    for (auto& emitter : m_emitters) {
        if (emitter.instanceId == instanceId) {
            emitter.paused = true;
            break;
        }
    }
}

void AdvancedParticleSystem::ResumeEmitter(int instanceId) {
    for (auto& emitter : m_emitters) {
        if (emitter.instanceId == instanceId) {
            emitter.paused = false;
            break;
        }
    }
}

void AdvancedParticleSystem::SetEmitterPosition(int instanceId, float x, float y) {
    for (auto& emitter : m_emitters) {
        if (emitter.instanceId == instanceId) {
            emitter.x = x;
            emitter.y = y;
            break;
        }
    }
}

void AdvancedParticleSystem::SetEmitterScale(int instanceId, float scale) {
    for (auto& emitter : m_emitters) {
        if (emitter.instanceId == instanceId) {
            emitter.scale = scale;
            break;
        }
    }
}

void AdvancedParticleSystem::StopAll() {
    for (auto& emitter : m_emitters) {
        emitter.active = false;
    }
}

void AdvancedParticleSystem::PauseAll() {
    for (auto& emitter : m_emitters) {
        emitter.paused = true;
    }
}

void AdvancedParticleSystem::ResumeAll() {
    for (auto& emitter : m_emitters) {
        emitter.paused = false;
    }
}

void AdvancedParticleSystem::Clear() {
    m_emitters.clear();
}

int AdvancedParticleSystem::GetActiveParticleCount() const {
    int count = 0;
    for (const auto& emitter : m_emitters) {
        for (const auto& p : emitter.particles) {
            if (p.alive) count++;
        }
    }
    return count;
}

int AdvancedParticleSystem::GetActiveEmitterCount() const {
    int count = 0;
    for (const auto& emitter : m_emitters) {
        if (emitter.active) count++;
    }
    return count;
}

bool AdvancedParticleSystem::IsEmitterActive(int instanceId) const {
    for (const auto& emitter : m_emitters) {
        if (emitter.instanceId == instanceId) {
            return emitter.active;
        }
    }
    return false;
}

// ===========================================================================
// PRIVATE METHODS
// ===========================================================================

void AdvancedParticleSystem::UpdateEmitter(EmitterInstance& instance, float deltaTime) {
    const EmitterConfig* config = GetEmitterConfig(instance.configId);
    if (!config) return;
    
    instance.lifetime += deltaTime;
    
    // Check duration
    if (config->duration > 0 && instance.lifetime >= config->duration) {
        instance.active = false;
    }
    
    // Emit new particles
    if (instance.active && config->continuous) {
        instance.emissionAccum += deltaTime * config->emissionRate;
        while (instance.emissionAccum >= 1.0f && 
               static_cast<int>(instance.particles.size()) < config->maxParticles) {
            EmitParticle(instance, *config);
            instance.emissionAccum -= 1.0f;
        }
    }
    
    // Burst emission
    if (instance.active && config->burstInterval > 0) {
        instance.burstTimer += deltaTime;
        if (instance.burstTimer >= config->burstInterval) {
            instance.burstTimer = 0.0f;
            for (int i = 0; i < config->burstCount && 
                 static_cast<int>(instance.particles.size()) < config->maxParticles; ++i) {
                EmitParticle(instance, *config);
            }
        }
    }
    
    // Update particles
    for (auto& particle : instance.particles) {
        if (particle.alive) {
            UpdateParticle(particle, *config, deltaTime);
        }
    }
    
    // Remove dead particles
    instance.particles.erase(
        std::remove_if(instance.particles.begin(), instance.particles.end(),
            [](const AdvancedParticle& p) { return !p.alive; }),
        instance.particles.end()
    );
}

void AdvancedParticleSystem::UpdateParticle(AdvancedParticle& particle, const EmitterConfig& config, float deltaTime) {
    // Handle delay
    if (particle.delay > 0) {
        particle.delay -= deltaTime;
        return;
    }
    
    particle.lifetime += deltaTime;
    
    // Check death
    if (particle.lifetime >= particle.maxLifetime) {
        particle.alive = false;
        return;
    }
    
    float t = particle.lifetime / particle.maxLifetime;
    
    // Apply gravity
    particle.velX += config.gravityX * deltaTime;
    particle.velY += config.gravityY * deltaTime;
    
    // Apply modifiers
    ApplyModifiers(particle, config, deltaTime);
    
    // Apply drag
    if (config.drag > 0) {
        particle.velX *= (1.0f - config.drag * deltaTime);
        particle.velY *= (1.0f - config.drag * deltaTime);
    }
    
    // Update position
    particle.x += particle.velX * deltaTime;
    particle.y += particle.velY * deltaTime;
    
    // Update rotation
    particle.rotation += particle.rotationSpeed * deltaTime;
    
    // Update size
    particle.size = particle.startSize + (particle.endSize - particle.startSize) * t;
    
    // Update color
    if (!config.colorOverLife.empty()) {
        particle.color = GetColorAtTime(config.colorOverLife, t);
    } else {
        particle.color = LerpColor(particle.startColor, particle.endColor, t);
    }
    
    // Update trail
    if (config.trailEnabled) {
        particle.trail.insert(particle.trail.begin(), {particle.x, particle.y});
        while (static_cast<int>(particle.trail.size()) > particle.maxTrailLength) {
            particle.trail.pop_back();
        }
    }
}

void AdvancedParticleSystem::EmitParticle(EmitterInstance& instance, const EmitterConfig& config) {
    AdvancedParticle particle;
    
    // Position based on emitter shape
    float offsetX = 0, offsetY = 0;
    switch (config.shape) {
        case EmitterShape::Point:
            break;
        case EmitterShape::Circle: {
            float angle = GetRandomInRange(0, 360) * 3.14159f / 180.0f;
            offsetX = std::cos(angle) * config.shapeWidth * 0.5f;
            offsetY = std::sin(angle) * config.shapeHeight * 0.5f;
            break;
        }
        case EmitterShape::Disc: {
            float angle = GetRandomInRange(0, 360) * 3.14159f / 180.0f;
            float r = std::sqrt(GetRandomInRange(0, 1)) * config.shapeWidth * 0.5f;
            offsetX = std::cos(angle) * r;
            offsetY = std::sin(angle) * r;
            break;
        }
        case EmitterShape::Box:
            offsetX = GetRandomInRange(-config.shapeWidth * 0.5f, config.shapeWidth * 0.5f);
            offsetY = GetRandomInRange(-config.shapeHeight * 0.5f, config.shapeHeight * 0.5f);
            break;
        case EmitterShape::Line:
            offsetX = GetRandomInRange(0, config.shapeWidth) - config.shapeWidth * 0.5f;
            break;
        default:
            break;
    }
    
    particle.x = instance.x + offsetX * instance.scale;
    particle.y = instance.y + offsetY * instance.scale;
    
    // Velocity
    float angle = GetRandomInRange(config.angleMin, config.angleMax) * 3.14159f / 180.0f;
    angle += instance.rotation * 3.14159f / 180.0f;
    float speed = GetRandomInRange(config.speedMin, config.speedMax) * instance.scale;
    particle.velX = std::cos(angle) * speed;
    particle.velY = std::sin(angle) * speed;
    particle.accX = 0;
    particle.accY = 0;
    
    // Rotation
    particle.rotation = GetRandomInRange(config.rotationMin, config.rotationMax);
    particle.rotationSpeed = GetRandomInRange(config.rotationSpeedMin, config.rotationSpeedMax);
    
    // Size
    particle.startSize = GetRandomInRange(config.sizeMin, config.sizeMax) * instance.scale;
    particle.endSize = GetRandomInRange(config.sizeEndMin, config.sizeEndMax) * instance.scale;
    particle.size = particle.startSize;
    
    // Color
    particle.startColor = GetRandomColor(config.startColorMin, config.startColorMax);
    particle.endColor = GetRandomColor(config.endColorMin, config.endColorMax);
    particle.color = particle.startColor;
    particle.alpha = 1.0f;
    
    // Lifetime
    particle.maxLifetime = GetRandomInRange(config.lifetimeMin, config.lifetimeMax);
    particle.lifetime = 0.0f;
    particle.delay = 0.0f;
    
    // Shape
    particle.shape = config.particleShape;
    particle.spriteFrame = 0;
    particle.alive = true;
    
    // Trail
    particle.maxTrailLength = config.trailLength;
    
    instance.particles.push_back(particle);
}

void AdvancedParticleSystem::RenderParticle(SDL_Renderer* renderer, const AdvancedParticle& particle, const EmitterConfig& config) {
    // Render trail first
    if (config.trailEnabled && particle.trail.size() > 1) {
        float fadeStep = config.trailFadeRate / particle.trail.size();
        for (size_t i = 1; i < particle.trail.size(); ++i) {
            float alpha = (1.0f - i * fadeStep) * (particle.color.a / 255.0f);
            SDL_Color trailColor = particle.color;
            trailColor.a = static_cast<Uint8>(alpha * 255);
            
            float trailSize = particle.size * (1.0f - i * 0.1f);
            RenderShape(renderer, particle.shape, particle.trail[i].first, particle.trail[i].second,
                       trailSize, particle.rotation, trailColor);
        }
    }
    
    // Render main particle
    RenderShape(renderer, particle.shape, particle.x, particle.y, 
                particle.size, particle.rotation, particle.color);
}

void AdvancedParticleSystem::RenderShape(SDL_Renderer* renderer, ParticleShape shape, 
                                          float x, float y, float size, float rotation, const SDL_Color& color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    
    int halfSize = static_cast<int>(size * 0.5f);
    int ix = static_cast<int>(x);
    int iy = static_cast<int>(y);
    
    switch (shape) {
        case ParticleShape::Point:
            SDL_RenderDrawPoint(renderer, ix, iy);
            break;
            
        case ParticleShape::Circle: {
            // Filled circle approximation
            for (int dy = -halfSize; dy <= halfSize; ++dy) {
                for (int dx = -halfSize; dx <= halfSize; ++dx) {
                    if (dx*dx + dy*dy <= halfSize*halfSize) {
                        SDL_RenderDrawPoint(renderer, ix + dx, iy + dy);
                    }
                }
            }
            break;
        }
        
        case ParticleShape::Square: {
            SDL_Rect rect = {ix - halfSize, iy - halfSize, static_cast<int>(size), static_cast<int>(size)};
            SDL_RenderFillRect(renderer, &rect);
            break;
        }
        
        case ParticleShape::Triangle: {
            // Simple triangle pointing up
            for (int row = 0; row < static_cast<int>(size); ++row) {
                int width = row * 2;
                int startX = ix - row;
                for (int col = 0; col <= width; ++col) {
                    SDL_RenderDrawPoint(renderer, startX + col, iy + halfSize - row);
                }
            }
            break;
        }
        
        case ParticleShape::Star: {
            // 4-pointed star
            SDL_RenderDrawLine(renderer, ix - halfSize, iy, ix + halfSize, iy);
            SDL_RenderDrawLine(renderer, ix, iy - halfSize, ix, iy + halfSize);
            int diag = halfSize * 7 / 10;
            SDL_RenderDrawLine(renderer, ix - diag, iy - diag, ix + diag, iy + diag);
            SDL_RenderDrawLine(renderer, ix - diag, iy + diag, ix + diag, iy - diag);
            break;
        }
        
        case ParticleShape::Ring: {
            // Circle outline
            for (int angle = 0; angle < 360; angle += 10) {
                float rad = angle * 3.14159f / 180.0f;
                int px = ix + static_cast<int>(std::cos(rad) * halfSize);
                int py = iy + static_cast<int>(std::sin(rad) * halfSize);
                SDL_RenderDrawPoint(renderer, px, py);
            }
            break;
        }
        
        case ParticleShape::Spark: {
            // Elongated line in velocity direction
            SDL_RenderDrawLine(renderer, ix - halfSize, iy, ix + halfSize, iy);
            break;
        }
        
        case ParticleShape::Smoke: {
            // Soft circle with gradient (approximated)
            for (int dy = -halfSize; dy <= halfSize; ++dy) {
                for (int dx = -halfSize; dx <= halfSize; ++dx) {
                    float dist = std::sqrt(static_cast<float>(dx*dx + dy*dy));
                    if (dist <= halfSize) {
                        float alpha = (1.0f - dist / halfSize) * (color.a / 255.0f);
                        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 
                                              static_cast<Uint8>(alpha * 255));
                        SDL_RenderDrawPoint(renderer, ix + dx, iy + dy);
                    }
                }
            }
            break;
        }
        
        default:
            SDL_RenderDrawPoint(renderer, ix, iy);
            break;
    }
}

void AdvancedParticleSystem::ApplyModifiers(AdvancedParticle& particle, const EmitterConfig& config, float deltaTime) {
    for (const auto& mod : config.modifiers) {
        switch (mod) {
            case VelocityModifier::Vortex: {
                float dx = particle.x - config.attractPointX;
                float dy = particle.y - config.attractPointY;
                float perpX = -dy;
                float perpY = dx;
                float dist = std::sqrt(dx*dx + dy*dy);
                if (dist > 0.1f) {
                    particle.velX += (perpX / dist) * config.vortexStrength * deltaTime;
                    particle.velY += (perpY / dist) * config.vortexStrength * deltaTime;
                }
                break;
            }
            case VelocityModifier::Attract: {
                float dx = config.attractPointX - particle.x;
                float dy = config.attractPointY - particle.y;
                float dist = std::sqrt(dx*dx + dy*dy);
                if (dist > 0.1f) {
                    particle.velX += (dx / dist) * config.attractStrength * deltaTime;
                    particle.velY += (dy / dist) * config.attractStrength * deltaTime;
                }
                break;
            }
            case VelocityModifier::Turbulence: {
                float noise = std::sin(particle.x * config.turbulenceScale + particle.lifetime * config.turbulenceSpeed);
                particle.velX += noise * 50.0f * deltaTime;
                particle.velY += std::cos(particle.y * config.turbulenceScale) * 50.0f * deltaTime;
                break;
            }
            default:
                break;
        }
    }
}

SDL_Color AdvancedParticleSystem::LerpColor(const SDL_Color& a, const SDL_Color& b, float t) const {
    return {
        static_cast<Uint8>(a.r + (b.r - a.r) * t),
        static_cast<Uint8>(a.g + (b.g - a.g) * t),
        static_cast<Uint8>(a.b + (b.b - a.b) * t),
        static_cast<Uint8>(a.a + (b.a - a.a) * t)
    };
}

SDL_Color AdvancedParticleSystem::GetColorAtTime(const std::vector<ColorKey>& keys, float t) const {
    if (keys.empty()) return {255, 255, 255, 255};
    if (keys.size() == 1) return keys[0].color;
    
    for (size_t i = 0; i < keys.size() - 1; ++i) {
        if (t >= keys[i].time && t <= keys[i + 1].time) {
            float localT = (t - keys[i].time) / (keys[i + 1].time - keys[i].time);
            return LerpColor(keys[i].color, keys[i + 1].color, localT);
        }
    }
    
    return keys.back().color;
}

float AdvancedParticleSystem::GetRandomInRange(float min, float max) const {
    return Random::Range(min, max);
}

SDL_Color AdvancedParticleSystem::GetRandomColor(const SDL_Color& min, const SDL_Color& max) const {
    return {
        static_cast<Uint8>(Random::Range(static_cast<int>(min.r), static_cast<int>(max.r))),
        static_cast<Uint8>(Random::Range(static_cast<int>(min.g), static_cast<int>(max.g))),
        static_cast<Uint8>(Random::Range(static_cast<int>(min.b), static_cast<int>(max.b))),
        static_cast<Uint8>(Random::Range(static_cast<int>(min.a), static_cast<int>(max.a)))
    };
}

// ===========================================================================
// EMITTER REGISTRATION
// ===========================================================================

void AdvancedParticleSystem::RegisterAllEmitters() {
    // Fire emitter
    EmitterConfig fire;
    fire.id = "fire";
    fire.name = "Fire";
    fire.emissionRate = 30.0f;
    fire.particleShape = ParticleShape::Circle;
    fire.sizeMin = 8.0f; fire.sizeMax = 15.0f;
    fire.sizeEndMin = 2.0f; fire.sizeEndMax = 5.0f;
    fire.lifetimeMin = 0.5f; fire.lifetimeMax = 1.0f;
    fire.speedMin = 30.0f; fire.speedMax = 60.0f;
    fire.angleMin = 250.0f; fire.angleMax = 290.0f;
    fire.gravityY = -50.0f;
    fire.startColorMin = {255, 200, 50, 255}; fire.startColorMax = {255, 150, 0, 255};
    fire.endColorMin = {255, 50, 0, 0}; fire.endColorMax = {200, 0, 0, 0};
    fire.blendMode = ParticleBlend::Additive;
    RegisterEmitter(fire);
    
    // Smoke emitter
    EmitterConfig smoke;
    smoke.id = "smoke";
    smoke.name = "Smoke";
    smoke.emissionRate = 15.0f;
    smoke.particleShape = ParticleShape::Smoke;
    smoke.sizeMin = 15.0f; smoke.sizeMax = 25.0f;
    smoke.sizeEndMin = 30.0f; smoke.sizeEndMax = 50.0f;
    smoke.lifetimeMin = 1.5f; smoke.lifetimeMax = 3.0f;
    smoke.speedMin = 10.0f; smoke.speedMax = 30.0f;
    smoke.angleMin = 250.0f; smoke.angleMax = 290.0f;
    smoke.gravityY = -20.0f;
    smoke.startColorMin = {80, 80, 80, 150}; smoke.startColorMax = {120, 120, 120, 200};
    smoke.endColorMin = {60, 60, 60, 0}; smoke.endColorMax = {100, 100, 100, 0};
    smoke.modifiers = {VelocityModifier::Turbulence};
    smoke.turbulenceScale = 0.05f;
    RegisterEmitter(smoke);
    
    // Spark emitter
    EmitterConfig spark;
    spark.id = "spark";
    spark.name = "Sparks";
    spark.continuous = false;
    spark.burstCount = 20;
    spark.particleShape = ParticleShape::Spark;
    spark.sizeMin = 3.0f; spark.sizeMax = 8.0f;
    spark.sizeEndMin = 1.0f; spark.sizeEndMax = 2.0f;
    spark.lifetimeMin = 0.3f; spark.lifetimeMax = 0.8f;
    spark.speedMin = 100.0f; spark.speedMax = 250.0f;
    spark.angleMin = 0.0f; spark.angleMax = 360.0f;
    spark.gravityY = 200.0f;
    spark.startColorMin = {255, 255, 150, 255}; spark.startColorMax = {255, 200, 100, 255};
    spark.endColorMin = {255, 100, 0, 0}; spark.endColorMax = {255, 50, 0, 0};
    spark.trailEnabled = true;
    spark.trailLength = 5;
    RegisterEmitter(spark);
    
    // Blood emitter
    EmitterConfig blood;
    blood.id = "blood";
    blood.name = "Blood";
    blood.continuous = false;
    blood.burstCount = 15;
    blood.particleShape = ParticleShape::Circle;
    blood.sizeMin = 3.0f; blood.sizeMax = 8.0f;
    blood.sizeEndMin = 2.0f; blood.sizeEndMax = 5.0f;
    blood.lifetimeMin = 0.5f; blood.lifetimeMax = 1.0f;
    blood.speedMin = 80.0f; blood.speedMax = 150.0f;
    blood.angleMin = 0.0f; blood.angleMax = 360.0f;
    blood.gravityY = 300.0f;
    blood.startColorMin = {180, 0, 0, 255}; blood.startColorMax = {255, 0, 0, 255};
    blood.endColorMin = {100, 0, 0, 0}; blood.endColorMax = {150, 0, 0, 0};
    RegisterEmitter(blood);
    
    // Heal emitter
    EmitterConfig heal;
    heal.id = "heal";
    heal.name = "Healing";
    heal.emissionRate = 20.0f;
    heal.duration = 1.0f;
    heal.shape = EmitterShape::Disc;
    heal.shapeWidth = 40.0f;
    heal.particleShape = ParticleShape::Star;
    heal.sizeMin = 5.0f; heal.sizeMax = 10.0f;
    heal.sizeEndMin = 2.0f; heal.sizeEndMax = 5.0f;
    heal.lifetimeMin = 0.8f; heal.lifetimeMax = 1.5f;
    heal.speedMin = 20.0f; heal.speedMax = 50.0f;
    heal.angleMin = 250.0f; heal.angleMax = 290.0f;
    heal.startColorMin = {100, 255, 100, 255}; heal.startColorMax = {150, 255, 150, 255};
    heal.endColorMin = {50, 200, 50, 0}; heal.endColorMax = {100, 255, 100, 0};
    heal.blendMode = ParticleBlend::Additive;
    RegisterEmitter(heal);
    
    // Ice emitter
    EmitterConfig ice;
    ice.id = "ice";
    ice.name = "Ice";
    ice.continuous = false;
    ice.burstCount = 25;
    ice.particleShape = ParticleShape::Triangle;
    ice.sizeMin = 4.0f; ice.sizeMax = 10.0f;
    ice.sizeEndMin = 2.0f; ice.sizeEndMax = 5.0f;
    ice.lifetimeMin = 0.5f; ice.lifetimeMax = 1.2f;
    ice.speedMin = 60.0f; ice.speedMax = 120.0f;
    ice.angleMin = 0.0f; ice.angleMax = 360.0f;
    ice.rotationSpeedMin = -180.0f; ice.rotationSpeedMax = 180.0f;
    ice.startColorMin = {150, 200, 255, 255}; ice.startColorMax = {200, 230, 255, 255};
    ice.endColorMin = {100, 150, 255, 0}; ice.endColorMax = {150, 200, 255, 0};
    RegisterEmitter(ice);
    
    // Lightning emitter
    EmitterConfig lightning;
    lightning.id = "lightning";
    lightning.name = "Lightning";
    lightning.continuous = false;
    lightning.burstCount = 30;
    lightning.particleShape = ParticleShape::Spark;
    lightning.sizeMin = 2.0f; lightning.sizeMax = 6.0f;
    lightning.lifetimeMin = 0.1f; lightning.lifetimeMax = 0.3f;
    lightning.speedMin = 200.0f; lightning.speedMax = 400.0f;
    lightning.angleMin = 0.0f; lightning.angleMax = 360.0f;
    lightning.startColorMin = {200, 200, 255, 255}; lightning.startColorMax = {255, 255, 255, 255};
    lightning.endColorMin = {100, 100, 255, 0}; lightning.endColorMax = {150, 150, 255, 0};
    lightning.blendMode = ParticleBlend::Additive;
    lightning.trailEnabled = true;
    lightning.trailLength = 3;
    RegisterEmitter(lightning);
    
    // Explosion emitter
    EmitterConfig explosion;
    explosion.id = "explosion";
    explosion.name = "Explosion";
    explosion.continuous = false;
    explosion.burstCount = 50;
    explosion.particleShape = ParticleShape::Circle;
    explosion.sizeMin = 5.0f; explosion.sizeMax = 15.0f;
    explosion.sizeEndMin = 20.0f; explosion.sizeEndMax = 40.0f;
    explosion.lifetimeMin = 0.3f; explosion.lifetimeMax = 0.8f;
    explosion.speedMin = 150.0f; explosion.speedMax = 300.0f;
    explosion.angleMin = 0.0f; explosion.angleMax = 360.0f;
    explosion.drag = 3.0f;
    explosion.colorOverLife = {
        {0.0f, {255, 255, 200, 255}},
        {0.2f, {255, 200, 50, 255}},
        {0.5f, {255, 100, 0, 200}},
        {1.0f, {100, 50, 50, 0}}
    };
    explosion.blendMode = ParticleBlend::Additive;
    RegisterEmitter(explosion);
    
    // Dust emitter
    EmitterConfig dust;
    dust.id = "dust";
    dust.name = "Dust";
    dust.emissionRate = 10.0f;
    dust.shape = EmitterShape::Box;
    dust.shapeWidth = 30.0f;
    dust.shapeHeight = 10.0f;
    dust.particleShape = ParticleShape::Circle;
    dust.sizeMin = 2.0f; dust.sizeMax = 5.0f;
    dust.lifetimeMin = 0.5f; dust.lifetimeMax = 1.5f;
    dust.speedMin = 5.0f; dust.speedMax = 20.0f;
    dust.angleMin = 230.0f; dust.angleMax = 310.0f;
    dust.startColorMin = {150, 130, 100, 100}; dust.startColorMax = {180, 160, 130, 150};
    dust.endColorMin = {150, 130, 100, 0}; dust.endColorMax = {180, 160, 130, 0};
    RegisterEmitter(dust);
    
    // Magic emitter
    EmitterConfig magic;
    magic.id = "magic";
    magic.name = "Magic";
    magic.emissionRate = 25.0f;
    magic.shape = EmitterShape::Circle;
    magic.shapeWidth = 30.0f;
    magic.particleShape = ParticleShape::Star;
    magic.sizeMin = 3.0f; magic.sizeMax = 8.0f;
    magic.sizeEndMin = 1.0f; magic.sizeEndMax = 3.0f;
    magic.lifetimeMin = 0.5f; magic.lifetimeMax = 1.0f;
    magic.speedMin = 10.0f; magic.speedMax = 30.0f;
    magic.angleMin = 0.0f; magic.angleMax = 360.0f;
    magic.rotationSpeedMin = -90.0f; magic.rotationSpeedMax = 90.0f;
    magic.startColorMin = {150, 100, 255, 255}; magic.startColorMax = {200, 150, 255, 255};
    magic.endColorMin = {100, 50, 200, 0}; magic.endColorMax = {150, 100, 255, 0};
    magic.blendMode = ParticleBlend::Additive;
    magic.modifiers = {VelocityModifier::Vortex};
    magic.vortexStrength = 100.0f;
    RegisterEmitter(magic);
    
    // Rain emitter
    EmitterConfig rain;
    rain.id = "rain";
    rain.name = "Rain";
    rain.emissionRate = 100.0f;
    rain.shape = EmitterShape::Line;
    rain.shapeWidth = 800.0f;
    rain.particleShape = ParticleShape::Spark;
    rain.sizeMin = 8.0f; rain.sizeMax = 15.0f;
    rain.lifetimeMin = 0.5f; rain.lifetimeMax = 1.0f;
    rain.speedMin = 300.0f; rain.speedMax = 400.0f;
    rain.angleMin = 95.0f; rain.angleMax = 100.0f;
    rain.startColorMin = {150, 180, 220, 150}; rain.startColorMax = {180, 200, 240, 200};
    rain.endColorMin = {150, 180, 220, 0}; rain.endColorMax = {180, 200, 240, 0};
    RegisterEmitter(rain);
    
    // Snow emitter
    EmitterConfig snow;
    snow.id = "snow";
    snow.name = "Snow";
    snow.emissionRate = 30.0f;
    snow.shape = EmitterShape::Line;
    snow.shapeWidth = 800.0f;
    snow.particleShape = ParticleShape::Circle;
    snow.sizeMin = 3.0f; snow.sizeMax = 6.0f;
    snow.lifetimeMin = 3.0f; snow.lifetimeMax = 5.0f;
    snow.speedMin = 30.0f; snow.speedMax = 60.0f;
    snow.angleMin = 85.0f; snow.angleMax = 95.0f;
    snow.startColorMin = {230, 240, 255, 200}; snow.startColorMax = {255, 255, 255, 255};
    snow.endColorMin = {230, 240, 255, 0}; snow.endColorMax = {255, 255, 255, 0};
    snow.modifiers = {VelocityModifier::Turbulence};
    snow.turbulenceScale = 0.02f;
    snow.turbulenceSpeed = 2.0f;
    RegisterEmitter(snow);
    
    // Gold coin emitter
    EmitterConfig gold;
    gold.id = "gold";
    gold.name = "Gold Coins";
    gold.continuous = false;
    gold.burstCount = 10;
    gold.particleShape = ParticleShape::Circle;
    gold.sizeMin = 5.0f; gold.sizeMax = 8.0f;
    gold.lifetimeMin = 0.8f; gold.lifetimeMax = 1.2f;
    gold.speedMin = 80.0f; gold.speedMax = 150.0f;
    gold.angleMin = 220.0f; gold.angleMax = 320.0f;
    gold.gravityY = 300.0f;
    gold.startColorMin = {255, 200, 50, 255}; gold.startColorMax = {255, 220, 100, 255};
    gold.endColorMin = {255, 200, 50, 0}; gold.endColorMax = {255, 220, 100, 0};
    gold.bounce = 0.5f;
    RegisterEmitter(gold);
    
    // Level up emitter
    EmitterConfig levelUp;
    levelUp.id = "levelup";
    levelUp.name = "Level Up";
    levelUp.emissionRate = 40.0f;
    levelUp.duration = 1.5f;
    levelUp.shape = EmitterShape::Circle;
    levelUp.shapeWidth = 50.0f;
    levelUp.particleShape = ParticleShape::Star;
    levelUp.sizeMin = 5.0f; levelUp.sizeMax = 12.0f;
    levelUp.sizeEndMin = 2.0f; levelUp.sizeEndMax = 5.0f;
    levelUp.lifetimeMin = 0.8f; levelUp.lifetimeMax = 1.5f;
    levelUp.speedMin = 40.0f; levelUp.speedMax = 80.0f;
    levelUp.angleMin = 250.0f; levelUp.angleMax = 290.0f;
    levelUp.rotationSpeedMin = -180.0f; levelUp.rotationSpeedMax = 180.0f;
    levelUp.colorOverLife = {
        {0.0f, {255, 255, 150, 255}},
        {0.3f, {255, 220, 100, 255}},
        {0.7f, {255, 180, 50, 200}},
        {1.0f, {255, 150, 0, 0}}
    };
    levelUp.blendMode = ParticleBlend::Additive;
    RegisterEmitter(levelUp);
    
    // Boss aura emitter
    EmitterConfig bossAura;
    bossAura.id = "boss_aura";
    bossAura.name = "Boss Aura";
    bossAura.emissionRate = 15.0f;
    bossAura.shape = EmitterShape::Circle;
    bossAura.shapeWidth = 60.0f;
    bossAura.particleShape = ParticleShape::Smoke;
    bossAura.sizeMin = 15.0f; bossAura.sizeMax = 25.0f;
    bossAura.sizeEndMin = 30.0f; bossAura.sizeEndMax = 50.0f;
    bossAura.lifetimeMin = 1.0f; bossAura.lifetimeMax = 2.0f;
    bossAura.speedMin = 5.0f; bossAura.speedMax = 15.0f;
    bossAura.angleMin = 250.0f; bossAura.angleMax = 290.0f;
    bossAura.startColorMin = {80, 0, 80, 100}; bossAura.startColorMax = {120, 0, 120, 150};
    bossAura.endColorMin = {50, 0, 50, 0}; bossAura.endColorMax = {80, 0, 80, 0};
    bossAura.modifiers = {VelocityModifier::Vortex};
    bossAura.vortexStrength = 30.0f;
    RegisterEmitter(bossAura);
}

void AdvancedParticleSystem::RegisterAllEffects() {
    // Fireball effect
    RegisterEffect({
        "fireball", "Fireball Impact",
        {"fire", "smoke", "spark"},
        2.0f, false
    });
    
    // Ice spell effect
    RegisterEffect({
        "ice_spell", "Ice Spell",
        {"ice", "magic"},
        1.5f, false
    });
    
    // Heal effect
    RegisterEffect({
        "heal_effect", "Healing Effect",
        {"heal", "magic"},
        1.5f, false
    });
    
    // Death effect
    RegisterEffect({
        "death", "Unit Death",
        {"blood", "dust"},
        1.0f, false
    });
    
    // Boss death effect
    RegisterEffect({
        "boss_death", "Boss Death",
        {"explosion", "smoke", "spark"},
        3.0f, false
    });
    
    // Level up effect
    RegisterEffect({
        "levelup_effect", "Level Up",
        {"levelup", "magic"},
        2.0f, false
    });
}

} // namespace DDD
