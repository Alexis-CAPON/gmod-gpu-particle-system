#pragma once

#include "../particle_data.h"
#include <vector>
#include <memory>
#include <random>

namespace GPUParticles {

/**
 * @brief Individual particle data
 */
struct Particle {
    Vector3 position;
    Vector3 velocity;
    Color color;
    float size;
    float rotation;
    float lifetime;      // Total lifetime
    float age;          // Current age
    bool alive;

    Particle()
        : position(0, 0, 0)
        , velocity(0, 0, 0)
        , color(1, 1, 1, 1)
        , size(1.0f)
        , rotation(0.0f)
        , lifetime(1.0f)
        , age(0.0f)
        , alive(false)
    {}
};

/**
 * @brief CPU-based particle simulator
 *
 * Simulates particle physics on the CPU each frame.
 * Supports emission, forces, color/size curves, and all Unity modules.
 */
class CPUParticleSimulator {
public:
    CPUParticleSimulator();
    ~CPUParticleSimulator();

    /**
     * @brief Initialize simulator with particle system data
     * @param data Particle system configuration
     * @return True if successful
     */
    bool Initialize(const ParticleSystemData& data);

    /**
     * @brief Update simulation
     * @param deltaTime Time since last frame
     */
    void Update(float deltaTime);

    /**
     * @brief Get all alive particles
     */
    const std::vector<Particle>& GetParticles() const { return m_particles; }

    /**
     * @brief Get count of alive particles
     */
    int GetAliveCount() const { return m_aliveCount; }

    /**
     * @brief Check if system is initialized
     */
    bool IsInitialized() const { return m_initialized; }

    /**
     * @brief Get last error
     */
    const std::string& GetLastError() const { return m_lastError; }

    /**
     * @brief Reset simulation
     */
    void Reset();

private:
    // Initialization
    void InitializeParticlePool();

    // Simulation steps
    void EmitParticles(float deltaTime);
    void UpdateParticles(float deltaTime);
    void ApplyForces(Particle& p, float deltaTime);
    void UpdateColorOverLifetime(Particle& p);
    void UpdateSizeOverLifetime(Particle& p);
    void UpdateRotationOverLifetime(Particle& p, float deltaTime);

    // Particle spawning
    void SpawnParticle();
    Vector3 GetEmissionPosition();
    Vector3 GetEmissionVelocity();

    // Utility
    float EvaluateMinMaxCurve(const MinMaxCurve& curve, float time) const;
    float EvaluateCurve(const AnimationCurve& curve, float time) const;
    Color EvaluateGradient(const Gradient& gradient, float time) const;
    float RandomRange(float min, float max) const;

    // Data
    ParticleSystemData m_data;
    std::vector<Particle> m_particles;
    int m_aliveCount;
    float m_emissionAccumulator;
    float m_systemTime;
    bool m_initialized;
    std::string m_lastError;

    // Random number generation
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_dist;
};

} // namespace GPUParticles
