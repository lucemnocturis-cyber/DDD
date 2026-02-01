#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <memory>
#include <functional>
#include <string>

namespace DDD {

class Renderer;

/**
 * A single particle
 */
struct Particle {
    float x, y;           // Position
    float vx, vy;         // Velocity
    float ax, ay;         // Acceleration (gravity, etc)
    float life;           // Remaining life (seconds)
    float maxLife;        // Total lifetime
    float size;           // Current size
    float startSize;      // Initial size
    float endSize;        // Final size
    float rotation;       // Rotation angle
    float rotationSpeed;  // Rotation speed
    SDL_Color startColor; // Initial color
    SDL_Color endColor;   // Final color
    bool active = true;
};

/**
 * Particle emitter configuration
 */
struct EmitterConfig {
    // Emission
    float emitRate = 10.0f;       // Particles per second
    int burstCount = 0;           // If > 0, emit this many instantly
    float duration = -1.0f;       // How long to emit (-1 = forever)
    
    // Position variance
    float spawnRadius = 0.0f;     // Random offset from origin
    
    // Velocity
    float minSpeed = 50.0f;
    float maxSpeed = 100.0f;
    float minAngle = 0.0f;        // Degrees
    float maxAngle = 360.0f;
    
    // Acceleration
    float gravityX = 0.0f;
    float gravityY = 0.0f;
    
    // Lifetime
    float minLife = 0.5f;
    float maxLife = 1.0f;
    
    // Size
    float startSize = 8.0f;
    float endSize = 2.0f;
    float sizeVariance = 0.0f;
    
    // Rotation
    float minRotationSpeed = 0.0f;
    float maxRotationSpeed = 0.0f;
    
    // Color
    SDL_Color startColor = {255, 255, 255, 255};
    SDL_Color endColor = {255, 255, 255, 0};
    bool randomColor = false;
    
    // Shape
    enum class Shape { Square, Circle, Star } shape = Shape::Square;
};

/**
 * Particle emitter
 */
class ParticleEmitter {
public:
    ParticleEmitter();
    ~ParticleEmitter() = default;
    
    /**
     * Configure the emitter
     */
    void Configure(const EmitterConfig& config);
    
    /**
     * Set emitter position
     */
    void SetPosition(float x, float y);
    
    /**
     * Start emitting
     */
    void Start();
    
    /**
     * Stop emitting (existing particles continue)
     */
    void Stop();
    
    /**
     * Emit a burst of particles
     */
    void Burst(int count);
    
    /**
     * Update particles
     */
    void Update(float deltaTime);
    
    /**
     * Render particles
     */
    void Render(Renderer& renderer);
    
    /**
     * Check if emitter is active
     */
    bool IsActive() const { return m_emitting || !m_particles.empty(); }
    
    /**
     * Get particle count
     */
    size_t GetParticleCount() const { return m_particles.size(); }
    
private:
    void EmitParticle();
    SDL_Color LerpColor(const SDL_Color& a, const SDL_Color& b, float t);
    
    EmitterConfig m_config;
    std::vector<Particle> m_particles;
    
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_emitTimer = 0.0f;
    float m_durationTimer = 0.0f;
    bool m_emitting = false;
};

/**
 * ParticleSystem - manages all particle effects
 */
class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();
    
    /**
     * Update all emitters
     */
    void Update(float deltaTime);
    
    /**
     * Render all particles
     */
    void Render(Renderer& renderer);
    
    /**
     * Spawn a damage effect at position
     */
    void SpawnDamageEffect(float x, float y, int damage, bool isCritical = false);
    
    /**
     * Spawn a heal effect at position
     */
    void SpawnHealEffect(float x, float y, int amount);
    
    /**
     * Spawn a death explosion at position
     */
    void SpawnDeathEffect(float x, float y, SDL_Color unitColor);
    
    /**
     * Spawn territory capture effect
     */
    void SpawnCaptureEffect(float x, float y, SDL_Color ownerColor);
    
    /**
     * Spawn a generic burst effect
     */
    void SpawnBurst(float x, float y, const EmitterConfig& config);
    
    /**
     * Clear all particles
     */
    void Clear();
    
    /**
     * Get total particle count
     */
    size_t GetTotalParticles() const;
    
private:
    std::vector<std::unique_ptr<ParticleEmitter>> m_emitters;
    
    // Pre-configured effect templates
    EmitterConfig m_damageConfig;
    EmitterConfig m_criticalConfig;
    EmitterConfig m_healConfig;
    EmitterConfig m_deathConfig;
    EmitterConfig m_captureConfig;
};

} // namespace DDD
