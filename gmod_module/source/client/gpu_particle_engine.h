#pragma once

#include "../particle_data.h"
#include "compute_shader.h"
#include <memory>
#include <vector>

namespace GPUParticles {

/**
 * @brief GPU-side particle representation (matches GLSL struct)
 */
struct GPUParticle {
    float position[3];
    float velocity[3];
    float startPosition[3];
    float lifetime;
    float age;
    float size;
    float startSize;
    float rotation[3];
    float rotationSpeed;
    float color[4];
    float startColor[4];
    float randomSeed;
    int alive;
    int _padding[3];  // Alignment
};

/**
 * @brief Manages GPU particle simulation
 *
 * This class handles:
 * - GPU buffer management
 * - Compute shader execution
 * - Particle emission
 * - Particle updates (physics, forces, curves)
 * - Curve/gradient texture uploads
 */
class GPUParticleEngine {
public:
    GPUParticleEngine();
    ~GPUParticleEngine();

    /**
     * @brief Initialize the particle engine
     * @param particleSystemData Particle system configuration
     * @return true if successful
     */
    bool Initialize(const ParticleSystemData& particleSystemData);

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    /**
     * @brief Update particles (run compute shaders)
     * @param deltaTime Time step in seconds
     */
    void Update(float deltaTime);

    /**
     * @brief Emit particles
     * @param count Number of particles to emit
     */
    void Emit(int count);

    /**
     * @brief Get particle buffer ID for rendering
     */
    unsigned int GetParticleBufferID() const { return m_particleBuffer; }

    /**
     * @brief Get alive particle count
     */
    int GetAliveCount() const { return m_aliveCount; }

    /**
     * @brief Get maximum particle capacity
     */
    int GetMaxParticles() const { return m_maxParticles; }

    /**
     * @brief Get particle system data
     */
    const ParticleSystemData& GetSystemData() const { return m_systemData; }

    /**
     * @brief Check if engine is initialized
     */
    bool IsInitialized() const { return m_initialized; }

    /**
     * @brief Get last error message
     */
    const std::string& GetLastError() const { return m_lastError; }

    /**
     * @brief Reset all particles
     */
    void Reset();

private:
    bool m_initialized;
    std::string m_lastError;

    // Particle system configuration
    ParticleSystemData m_systemData;
    int m_maxParticles;
    int m_aliveCount;
    int m_deadCount;

    // GPU buffers
    unsigned int m_particleBuffer;       // Main particle data
    unsigned int m_aliveIndexBuffer;     // Indices of alive particles
    unsigned int m_deadIndexBuffer;      // Indices of dead particles
    unsigned int m_aliveCountBuffer;     // Counter for alive particles
    unsigned int m_deadCountBuffer;      // Counter for dead particles
    unsigned int m_emissionRequestBuffer; // Emission requests

    // Compute shaders
    std::unique_ptr<ComputeShader> m_updateShader;
    std::unique_ptr<ComputeShader> m_emitShader;

    // Curve/gradient textures
    unsigned int m_sizeOverLifetimeTexture;
    unsigned int m_colorOverLifetimeTexture;

    // Emission tracking
    float m_accumulatedEmission;
    float m_systemTime;
    std::vector<bool> m_burstsFired;

    // Initialization helpers
    bool CreateBuffers();
    bool LoadShaders();
    bool UploadCurvesAndGradients();

    // Buffer management
    void InitializeDeadList();
    void UpdateAliveCount();
    void UpdateDeadCount();

    // Curve/gradient upload
    unsigned int CreateCurveTexture(const MinMaxCurve& curve, int resolution = 256);
    unsigned int CreateGradientTexture(const Gradient& gradient, int resolution = 256);

    // Cleanup
    void CleanupBuffers();
    void CleanupTextures();
};

} // namespace GPUParticles
