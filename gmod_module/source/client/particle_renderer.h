#pragma once

#include "compute_shader.h"
#include "gpu_particle_engine.h"
#include <memory>

namespace GPUParticles {

/**
 * @brief Renders particles using instanced rendering
 *
 * This class handles:
 * - Loading vertex/fragment shaders
 * - Setting up instanced rendering
 * - Drawing particles as billboards
 * - Texture management
 * - Blend modes and sorting
 */
class ParticleRenderer {
public:
    ParticleRenderer();
    ~ParticleRenderer();

    /**
     * @brief Initialize the renderer
     * @return true if successful
     */
    bool Initialize();

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    /**
     * @brief Render a particle system
     * @param engine GPU particle engine to render
     * @param viewMatrix Camera view matrix (4x4)
     * @param projectionMatrix Camera projection matrix (4x4)
     * @param cameraPos Camera position
     */
    void Render(
        const GPUParticleEngine& engine,
        const float* viewMatrix,
        const float* projectionMatrix,
        const float* cameraPos
    );

    /**
     * @brief Check if renderer is initialized
     */
    bool IsInitialized() const { return m_initialized; }

    /**
     * @brief Get last error message
     */
    const std::string& GetLastError() const { return m_lastError; }

private:
    bool m_initialized;
    std::string m_lastError;

    // Rendering shader
    std::unique_ptr<ShaderProgram> m_particleShader;

    // Vertex Array Object and buffers
    unsigned int m_vao;
    unsigned int m_quadVBO;  // Quad vertices for billboard
    unsigned int m_quadEBO;  // Quad indices

    // Default particle texture
    unsigned int m_defaultTexture;

    // Initialization helpers
    bool LoadShaders();
    bool CreateQuadGeometry();
    unsigned int CreateDefaultTexture();

    // Rendering helpers
    void SetupShaderUniforms(
        const GPUParticleEngine& engine,
        const float* viewMatrix,
        const float* projectionMatrix,
        const float* cameraPos
    );

    // Cleanup
    void CleanupGeometry();
    void CleanupTextures();
};

} // namespace GPUParticles
