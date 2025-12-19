#include "particle_renderer.h"
#include "opengl_includes.h"
#include <iostream>
#include <cmath>

namespace GPUParticles {

ParticleRenderer::ParticleRenderer()
    : m_initialized(false)
    , m_vao(0)
    , m_quadVBO(0)
    , m_quadEBO(0)
    , m_defaultTexture(0)
{
}

ParticleRenderer::~ParticleRenderer() {
    Shutdown();
}

bool ParticleRenderer::Initialize() {
    if (m_initialized) {
        return true;
    }

    std::cout << "[ParticleRenderer] Initializing..." << std::endl;

    // Load shaders
    if (!LoadShaders()) {
        m_lastError = "Failed to load rendering shaders";
        return false;
    }

    // Create quad geometry
    if (!CreateQuadGeometry()) {
        m_lastError = "Failed to create quad geometry";
        return false;
    }

    // Create default texture
    m_defaultTexture = CreateDefaultTexture();

    m_initialized = true;
    std::cout << "[ParticleRenderer] Initialization successful" << std::endl;

    return true;
}

void ParticleRenderer::Shutdown() {
    if (!m_initialized) {
        return;
    }

    std::cout << "[ParticleRenderer] Shutting down..." << std::endl;

    CleanupGeometry();
    CleanupTextures();

    m_particleShader.reset();

    m_initialized = false;
}

bool ParticleRenderer::LoadShaders() {
    std::cout << "[ParticleRenderer] Loading rendering shaders..." << std::endl;

    m_particleShader = std::make_unique<ShaderProgram>();
    if (!m_particleShader->LoadFromFiles("shaders/particle.vert", "shaders/particle.frag")) {
        m_lastError = "Failed to load particle shaders: " + m_particleShader->GetLastError();
        return false;
    }

    std::cout << "[ParticleRenderer] Shaders loaded successfully" << std::endl;
    return true;
}

bool ParticleRenderer::CreateQuadGeometry() {
    std::cout << "[ParticleRenderer] Creating quad geometry..." << std::endl;

    // Quad vertices (-0.5 to 0.5) with texture coordinates
    float quadVertices[] = {
        // Position         // TexCoord
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f
    };

    unsigned int quadIndices[] = {
        0, 1, 2,
        2, 3, 0
    };

    GL_CHECK(glGenVertexArrays(1, &m_vao));
    GL_CHECK(glGenBuffers(1, &m_quadVBO));
    GL_CHECK(glGenBuffers(1, &m_quadEBO));

    GL_CHECK(glBindVertexArray(m_vao));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW));

    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_quadEBO));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW));

    // Position attribute
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0));
    GL_CHECK(glEnableVertexAttribArray(0));

    // TexCoord attribute
    GL_CHECK(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))));
    GL_CHECK(glEnableVertexAttribArray(1));

    GL_CHECK(glBindVertexArray(0));

    std::cout << "[ParticleRenderer] Quad geometry created" << std::endl;
    return true;
}

unsigned int ParticleRenderer::CreateDefaultTexture() {
    // Create simple white circular gradient texture
    const int size = 64;
    std::vector<unsigned char> data(size * size * 4);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = (x / static_cast<float>(size) - 0.5f) * 2.0f;
            float dy = (y / static_cast<float>(size) - 0.5f) * 2.0f;
            float dist = std::sqrt(dx * dx + dy * dy);
            float alpha = std::max(0.0f, 1.0f - dist);

            int idx = (y * size + x) * 4;
            data[idx + 0] = 255;  // R
            data[idx + 1] = 255;  // G
            data[idx + 2] = 255;  // B
            data[idx + 3] = static_cast<unsigned char>(alpha * 255);  // A
        }
    }

    unsigned int texture;
    GL_CHECK(glGenTextures(1, &texture));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture));
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data()));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));

    return texture;
}

void ParticleRenderer::Render(
    const GPUParticleEngine& engine,
    const float* viewMatrix,
    const float* projectionMatrix,
    const float* cameraPos)
{
    if (!m_initialized || !engine.IsInitialized()) {
        return;
    }

    int aliveCount = engine.GetAliveCount();
    if (aliveCount == 0) {
        return;
    }

    // Bind shader
    m_particleShader->Bind();

    // Set up uniforms
    SetupShaderUniforms(engine, viewMatrix, projectionMatrix, cameraPos);

    // Bind VAO
    GL_CHECK(glBindVertexArray(m_vao));

    // Set up OpenGL state for particle rendering
    GL_CHECK(glEnable(GL_BLEND));
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)); // Or GL_ONE for additive
    GL_CHECK(glDepthMask(GL_FALSE)); // Don't write to depth buffer

    // Bind particle buffer as instance data
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, engine.GetParticleBufferID()));

    // Set up instance attributes (position, color, size, rotation, alive)
    // These would be configured based on the GPUParticle struct layout
    // For now, this is a placeholder - full instancing setup would require defining
    // vertex attribute divisors for per-instance data

    // Render instances
    GL_CHECK(glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, aliveCount));

    // Restore OpenGL state
    GL_CHECK(glDepthMask(GL_TRUE));
    GL_CHECK(glDisable(GL_BLEND));

    GL_CHECK(glBindVertexArray(0));
    m_particleShader->Unbind();

    std::cout << "[ParticleRenderer] Rendered " << aliveCount << " particles" << std::endl;
}

void ParticleRenderer::SetupShaderUniforms(
    const GPUParticleEngine& engine,
    const float* viewMatrix,
    const float* projectionMatrix,
    const float* cameraPos)
{
    // Set matrices
    m_particleShader->SetMat4("uViewMatrix", viewMatrix);
    m_particleShader->SetMat4("uProjectionMatrix", projectionMatrix);

    // Extract camera right and up vectors from view matrix
    float cameraRight[3] = { viewMatrix[0], viewMatrix[4], viewMatrix[8] };
    float cameraUp[3] = { viewMatrix[1], viewMatrix[5], viewMatrix[9] };

    m_particleShader->SetVec3("uCameraRight", cameraRight[0], cameraRight[1], cameraRight[2]);
    m_particleShader->SetVec3("uCameraUp", cameraUp[0], cameraUp[1], cameraUp[2]);

    // Bind particle texture (default for now)
    m_particleShader->SetInt("uParticleTexture", 0);
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, m_defaultTexture));

    // Soft particles
    m_particleShader->SetInt("uUseSoftParticles", 0);
    m_particleShader->SetFloat("uSoftParticleDistance", 1.0f);

    // Blend mode (0=Alpha, 1=Additive, 2=Multiply)
    m_particleShader->SetInt("uBlendMode", 0);

    // Texture sheet animation
    const auto& texSheet = engine.GetSystemData().textureSheetAnimation;
    m_particleShader->SetInt("uUseTextureAnimation", texSheet.enabled ? 1 : 0);
    if (texSheet.enabled) {
        m_particleShader->SetInt("uTilesX", texSheet.numTilesX);
        m_particleShader->SetInt("uTilesY", texSheet.numTilesY);
        m_particleShader->SetFloat("uAnimationFrame", 0.0f); // Would be time-based
    }
}

void ParticleRenderer::CleanupGeometry() {
    if (m_vao != 0) {
        GL_CHECK(glDeleteVertexArrays(1, &m_vao));
        m_vao = 0;
    }

    if (m_quadVBO != 0) {
        GL_CHECK(glDeleteBuffers(1, &m_quadVBO));
        m_quadVBO = 0;
    }

    if (m_quadEBO != 0) {
        GL_CHECK(glDeleteBuffers(1, &m_quadEBO));
        m_quadEBO = 0;
    }
}

void ParticleRenderer::CleanupTextures() {
    if (m_defaultTexture != 0) {
        GL_CHECK(glDeleteTextures(1, &m_defaultTexture));
        m_defaultTexture = 0;
    }
}

} // namespace GPUParticles
