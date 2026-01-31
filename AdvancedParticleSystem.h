#pragma once

#include "../Utils/Math.h"
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <cmath>

namespace DDD {

/**
 * Particle shape types
 */
enum class ParticleShape {
    Point,
    Circle,
    Square,
    Triangle,
    Star,
    Ring,
    Spark,
    Smoke,
    Custom
};

/**
 * Emitter shape types
 */
enum class EmitterShape {
    Point,          // Single point
    Circle,         // Circle outline
    Disc,           // Filled circle
    Rectangle,      // Rectangle outline
    Box,            // Filled rectangle
    Line,           // Line between two points
    Cone            // Cone/wedge shape
};

/**
 * Particle blend modes
 */
enum class ParticleBlend {
    Normal,
    Additive,
    Multiply,
    Screen
};

/**
 * Color over lifetime keyframe
 */
struct ColorKey {
    float time;     // 0.0 to 1.0
    SDL_Color color;
};

/**
 * Velocity modifier types
 */
enum class VelocityModifier {
    None,
    Gravity,
    Wind,
    Vortex,
    Attract,
    Repel,
    Turbulence,
    Orbit
};

/**
 * Individual particle data
 */
struct AdvancedParticle {
    float x, y;
    float velX, velY;
    float accX, accY;
    float rotation;
    float rotationSpeed;
    float size;
    float startSize;
    float endSize;
    SDL_Color color;
    SDL_Color startColor;
    SDL_Color endColor;
    float alpha;
    float lifetime;
    float maxLifetime;
    float delay;
    ParticleShape shape;
    int spriteFrame;
    bool alive;
    std::vector<std::pair<float, float>> trail;
    int maxTrailLength;
};

/**
 * Particle emitter configuration
 */
struct EmitterConfig {
    std::string id;
    std::string name;
    
    // Emission
    float emissionRate = 10.0f;
    int burstCount = 0;
    float burstInterval = 0.0f;
    bool continuous = true;
    float duration = -1.0f;
    
    // Emitter shape
    EmitterShape shape = EmitterShape::Point;
    float shapeWidth = 0.0f;
    float shapeHeight = 0.0f;
    float coneAngle = 45.0f;
    
    // Particle properties
    ParticleShape particleShape = ParticleShape::Circle;
    float sizeMin = 5.0f, sizeMax = 10.0f;
    float sizeEndMin = 0.0f, sizeEndMax = 0.0f;
    float lifetimeMin = 1.0f, lifetimeMax = 2.0f;
    float speedMin = 50.0f, speedMax = 100.0f;
    float angleMin = 0.0f, angleMax = 360.0f;
    float rotationMin = 0.0f, rotationMax = 0.0f;
    float rotationSpeedMin = 0.0f, rotationSpeedMax = 0.0f;
    
    // Colors
    std::vector<ColorKey> colorOverLife;
    SDL_Color startColorMin = {255, 255, 255, 255};
    SDL_Color startColorMax = {255, 255, 255, 255};
    SDL_Color endColorMin = {255, 255, 255, 0};
    SDL_Color endColorMax = {255, 255, 255, 0};
    
    // Physics
    float gravityX = 0.0f, gravityY = 0.0f;
    float drag = 0.0f;
    float bounce = 0.0f;
    
    // Modifiers
    std::vector<VelocityModifier> modifiers;
    float vortexStrength = 0.0f;
    float attractPointX = 0.0f, attractPointY = 0.0f;
    float attractStrength = 0.0f;
    float turbulenceScale = 1.0f;
    float turbulenceSpeed = 1.0f;
    
    // Rendering
    ParticleBlend blendMode = ParticleBlend::Additive;
    bool trailEnabled = false;
    int trailLength = 5;
    float trailFadeRate = 0.8f;
    
    // Sub-emitters
    std::string onDeathEmitter;
    std::string onCollisionEmitter;
    
    // Limits
    int maxParticles = 500;
};

/**
 * Active emitter instance
 */
struct EmitterInstance {
    int instanceId;
    std::string configId;
    float x, y;
    float rotation;
    float scale;
    std::vector<AdvancedParticle> particles;
    float emissionAccum;
    float burstTimer;
    float lifetime;
    bool active;
    bool paused;
    std::function<void(const AdvancedParticle&)> onParticleDeath;
};

/**
 * Preset effect definition
 */
struct ParticleEffect {
    std::string id;
    std::string name;
    std::vector<std::string> emitterIds;
    float duration;
    bool looping;
};

/**
 * AdvancedParticleSystem - complex particle effects
 */
class AdvancedParticleSystem {
public:
    static AdvancedParticleSystem& Instance();
    
    void Initialize();
    void Shutdown();
    void Update(float deltaTime);
    void Render(SDL_Renderer* renderer);
    
    void RegisterEmitter(const EmitterConfig& config);
    const EmitterConfig* GetEmitterConfig(const std::string& id) const;
    void RegisterEffect(const ParticleEffect& effect);
    const ParticleEffect* GetEffect(const std::string& id) const;
    
    int SpawnEmitter(const std::string& configId, float x, float y);
    int SpawnEmitterRotated(const std::string& configId, float x, float y, float rotation);
    int SpawnEffect(const std::string& effectId, float x, float y);
    void SpawnBurst(const std::string& configId, float x, float y, int count);
    
    void StopEmitter(int instanceId);
    void PauseEmitter(int instanceId);
    void ResumeEmitter(int instanceId);
    void SetEmitterPosition(int instanceId, float x, float y);
    void SetEmitterScale(int instanceId, float scale);
    
    void StopAll();
    void PauseAll();
    void ResumeAll();
    void Clear();
    
    int GetActiveParticleCount() const;
    int GetActiveEmitterCount() const;
    bool IsEmitterActive(int instanceId) const;
    
    void SetDebugDraw(bool enabled) { m_debugDraw = enabled; }
    
private:
    AdvancedParticleSystem() = default;
    
    void RegisterAllEmitters();
    void RegisterAllEffects();
    
    void UpdateEmitter(EmitterInstance& instance, float deltaTime);
    void UpdateParticle(AdvancedParticle& particle, const EmitterConfig& config, float deltaTime);
    void EmitParticle(EmitterInstance& instance, const EmitterConfig& config);
    
    void RenderParticle(SDL_Renderer* renderer, const AdvancedParticle& particle, const EmitterConfig& config);
    void RenderShape(SDL_Renderer* renderer, ParticleShape shape, float x, float y, float size, float rotation, const SDL_Color& color);
    
    SDL_Color LerpColor(const SDL_Color& a, const SDL_Color& b, float t) const;
    SDL_Color GetColorAtTime(const std::vector<ColorKey>& keys, float t) const;
    float GetRandomInRange(float min, float max) const;
    SDL_Color GetRandomColor(const SDL_Color& min, const SDL_Color& max) const;
    void ApplyModifiers(AdvancedParticle& particle, const EmitterConfig& config, float deltaTime);
    
    std::unordered_map<std::string, EmitterConfig> m_emitterConfigs;
    std::unordered_map<std::string, ParticleEffect> m_effects;
    std::vector<EmitterInstance> m_emitters;
    int m_nextInstanceId = 0;
    bool m_debugDraw = false;
    float m_globalTimeScale = 1.0f;
    bool m_initialized = false;
};

} // namespace DDD
