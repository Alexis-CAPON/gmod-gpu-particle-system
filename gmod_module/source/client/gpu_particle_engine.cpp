#include "gpu_particle_engine.h"
#include "opengl_includes.h"
#include <iostream>
#include <cstring>
#include <random>

namespace GPUParticles {

GPUParticleEngine::GPUParticleEngine()
    : m_initialized(false)
    , m_maxParticles(0)
    , m_aliveCount(0)
    , m_deadCount(0)
    , m_particleBuffer(0)
    , m_aliveIndexBuffer(0)
    , m_deadIndexBuffer(0)
    , m_aliveCountBuffer(0)
    , m_deadCountBuffer(0)
    , m_emissionRequestBuffer(0)
    , m_sizeOverLifetimeTexture(0)
    , m_colorOverLifetimeTexture(0)
    , m_accumulatedEmission(0.0f)
    , m_systemTime(0.0f)
{
}

GPUParticleEngine::~GPUParticleEngine() {
    Shutdown();
}

bool GPUParticleEngine::Initialize(const ParticleSystemData& particleSystemData) {
    if (m_initialized) {
        Shutdown();
    }

    std::cout << "[GPUParticleEngine] Initializing particle system: " << particleSystemData.name << std::endl;

    m_systemData = particleSystemData;
    m_maxParticles = particleSystemData.main.maxParticles;

    // Create GPU buffers
    if (!CreateBuffers()) {
        m_lastError = "Failed to create GPU buffers";
        return false;
    }

    // Load compute shaders
    if (!LoadShaders()) {
        m_lastError = "Failed to load compute shaders";
        CleanupBuffers();
        return false;
    }

    // Upload curves and gradients
    if (!UploadCurvesAndGradients()) {
        m_lastError = "Failed to upload curves/gradients";
        CleanupBuffers();
        return false;
    }

    // Initialize burst tracking
    for (size_t i = 0; i < m_systemData.emission.bursts.size(); ++i) {
        m_burstsFired.push_back(false);
    }

    m_initialized = true;
    std::cout << "[GPUParticleEngine] Initialization successful" << std::endl;

    return true;
}

void GPUParticleEngine::Shutdown() {
    if (!m_initialized) {
        return;
    }

    std::cout << "[GPUParticleEngine] Shutting down..." << std::endl;

    CleanupBuffers();
    CleanupTextures();

    m_updateShader.reset();
    m_emitShader.reset();

    m_initialized = false;
}

bool GPUParticleEngine::CreateBuffers() {
    std::cout << "[GPUParticleEngine] Creating buffers for " << m_maxParticles << " particles" << std::endl;

    // 1. Create particle buffer
    GL_CHECK(glGenBuffers(1, &m_particleBuffer));
    GL_CHECK(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_particleBuffer));
    GL_CHECK(glBufferData(GL_SHADER_STORAGE_BUFFER,
                          m_maxParticles * sizeof(GPUParticle),
                          nullptr,
                          GL_DYNAMIC_DRAW));

    // 2. Create alive index buffer
    GL_CHECK(glGenBuffers(1, &m_aliveIndexBuffer));
    GL_CHECK(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_aliveIndexBuffer));
    GL_CHECK(glBufferData(GL_SHADER_STORAGE_BUFFER,
                          m_maxParticles * sizeof(unsigned int),
                          nullptr,
                          GL_DYNAMIC_DRAW));

    // 3. Create dead index buffer
    GL_CHECK(glGenBuffers(1, &m_deadIndexBuffer));
    GL_CHECK(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_deadIndexBuffer));
    GL_CHECK(glBufferData(GL_SHADER_STORAGE_BUFFER,
                          m_maxParticles * sizeof(unsigned int),
                          nullptr,
                          GL_DYNAMIC_DRAW));

    // 4. Create counter buffers
    GL_CHECK(glGenBuffers(1, &m_aliveCountBuffer));
    GL_CHECK(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_aliveCountBuffer));
    unsigned int initialCount = 0;
    GL_CHECK(glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned int) * 4, &initialCount, GL_DYNAMIC_DRAW));

    GL_CHECK(glGenBuffers(1, &m_deadCountBuffer));
    GL_CHECK(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_deadCountBuffer));
    unsigned int initialDeadCount = m_maxParticles;
    GL_CHECK(glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned int) * 4, &initialDeadCount, GL_DYNAMIC_DRAW));

    // 5. Create emission request buffer
    GL_CHECK(glGenBuffers(1, &m_emissionRequestBuffer));
    GL_CHECK(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_emissionRequestBuffer));
    GL_CHECK(glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned int) * 4, &initialCount, GL_DYNAMIC_DRAW));

    GL_CHECK(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));

    // Initialize dead list (all particles start dead)
    InitializeDeadList();

    m_deadCount = m_maxParticles;

    std::cout << "[GPUParticleEngine] Buffers created successfully" << std::endl;
    return true;
}

void GPUParticleEngine::InitializeDeadList() {
    // Create list of all particle indices (all start dead)
    std::vector<unsigned int> deadIndices(m_maxParticles);
    for (int i = 0; i < m_maxParticles; ++i) {
        deadIndices[i] = i;
    }

    // Upload to GPU
    GL_CHECK(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_deadIndexBuffer));
    GL_CHECK(glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                             deadIndices.size() * sizeof(unsigned int),
                             deadIndices.data()));
    GL_CHECK(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));

    m_deadCount = m_maxParticles;
    m_aliveCount = 0;
}

bool GPUParticleEngine::LoadShaders() {
    std::cout << "[GPUParticleEngine] Loading compute shaders..." << std::endl;

    // Build paths relative to garrysmod directory
    // GMod modules run from garrysmod/lua/bin/, so we need to go up to find shaders
    std::string updateShaderPath = "../particles/shaders/particle_update.comp";
    std::string emitShaderPath = "../particles/shaders/particle_emit.comp";

    // Load update shader
    m_updateShader = std::make_unique<ComputeShader>();
    if (!m_updateShader->LoadFromFile(updateShaderPath)) {
        m_lastError = "Failed to load update shader: " + m_updateShader->GetLastError();
        std::cerr << "[GPUParticleEngine] Tried path: " << updateShaderPath << std::endl;
        return false;
    }

    // Load emission shader
    m_emitShader = std::make_unique<ComputeShader>();
    if (!m_emitShader->LoadFromFile(emitShaderPath)) {
        m_lastError = "Failed to load emission shader: " + m_emitShader->GetLastError();
        std::cerr << "[GPUParticleEngine] Tried path: " << emitShaderPath << std::endl;
        return false;
    }

    std::cout << "[GPUParticleEngine] Shaders loaded successfully" << std::endl;
    return true;
}

bool GPUParticleEngine::UploadCurvesAndGradients() {
    std::cout << "[GPUParticleEngine] Uploading curves and gradients..." << std::endl;

    // Upload size over lifetime curve
    if (m_systemData.sizeOverLifetime.enabled) {
        m_sizeOverLifetimeTexture = CreateCurveTexture(m_systemData.sizeOverLifetime.size);
    }

    // Upload color over lifetime gradient
    if (m_systemData.colorOverLifetime.enabled) {
        m_colorOverLifetimeTexture = CreateGradientTexture(m_systemData.colorOverLifetime.gradient);
    }

    std::cout << "[GPUParticleEngine] Curves/gradients uploaded" << std::endl;
    return true;
}

unsigned int GPUParticleEngine::CreateCurveTexture(const MinMaxCurve& curve, int resolution) {
    // Sample curve at regular intervals
    std::vector<float> samples(resolution);
    for (int i = 0; i < resolution; ++i) {
        float t = static_cast<float>(i) / (resolution - 1);
        samples[i] = curve.Evaluate(t, 0.5f); // Use 0.5 for random value
    }

    // Create 1D texture
    unsigned int texture;
    GL_CHECK(glGenTextures(1, &texture));
    GL_CHECK(glBindTexture(GL_TEXTURE_1D, texture));
    GL_CHECK(glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, resolution, 0, GL_RED, GL_FLOAT, samples.data()));
    GL_CHECK(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glBindTexture(GL_TEXTURE_1D, 0));

    return texture;
}

unsigned int GPUParticleEngine::CreateGradientTexture(const Gradient& gradient, int resolution) {
    // Sample gradient at regular intervals
    std::vector<float> samples(resolution * 4); // RGBA
    for (int i = 0; i < resolution; ++i) {
        float t = static_cast<float>(i) / (resolution - 1);
        Color color = gradient.Evaluate(t);
        samples[i * 4 + 0] = color.r;
        samples[i * 4 + 1] = color.g;
        samples[i * 4 + 2] = color.b;
        samples[i * 4 + 3] = color.a;
    }

    // Create 1D texture
    unsigned int texture;
    GL_CHECK(glGenTextures(1, &texture));
    GL_CHECK(glBindTexture(GL_TEXTURE_1D, texture));
    GL_CHECK(glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, resolution, 0, GL_RGBA, GL_FLOAT, samples.data()));
    GL_CHECK(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glBindTexture(GL_TEXTURE_1D, 0));

    return texture;
}

void GPUParticleEngine::Update(float deltaTime) {
    if (!m_initialized) return;

    m_systemTime += deltaTime;

    // Handle emission
    if (m_systemData.emission.enabled) {
        // Rate over time emission
        float emissionRate = m_systemData.emission.rateOverTime.Evaluate(0.0f, 0.5f);
        m_accumulatedEmission += emissionRate * deltaTime;

        int particlesToEmit = static_cast<int>(m_accumulatedEmission);
        if (particlesToEmit > 0) {
            Emit(particlesToEmit);
            m_accumulatedEmission -= particlesToEmit;
        }

        // Burst emission
        for (size_t i = 0; i < m_systemData.emission.bursts.size(); ++i) {
            const auto& burst = m_systemData.emission.bursts[i];
            if (!m_burstsFired[i] && m_systemTime >= burst.time) {
                int burstCount = (burst.minCount + burst.maxCount) / 2;
                Emit(burstCount);
                m_burstsFired[i] = true;
            }
        }

        // Reset burst tracking if looping
        if (m_systemData.main.looping && m_systemTime >= m_systemData.main.duration) {
            m_systemTime = 0.0f;
            for (size_t i = 0; i < m_burstsFired.size(); ++i) {
                m_burstsFired[i] = false;
            }
        }
    }

    // Run update compute shader
    if (m_updateShader && m_aliveCount > 0) {
        m_updateShader->Bind();

        // Bind buffers
        m_updateShader->BindStorageBuffer(0, m_particleBuffer);
        m_updateShader->BindStorageBuffer(1, m_aliveIndexBuffer);
        m_updateShader->BindStorageBuffer(2, m_aliveCountBuffer);

        // Set uniforms
        m_updateShader->SetFloat("uDeltaTime", deltaTime);
        m_updateShader->SetVec3("uGravity", 0.0f, 0.0f, -9.81f * m_systemData.main.gravityModifier.Evaluate(0.0f, 0.5f));
        m_updateShader->SetFloat("uDrag", 0.1f);
        m_updateShader->SetFloat("uSimulationSpeed", m_systemData.main.simulationSpeed);
        m_updateShader->SetFloat("uTime", m_systemTime);

        // Module flags
        m_updateShader->SetInt("uSizeOverLifetimeEnabled", m_systemData.sizeOverLifetime.enabled ? 1 : 0);
        m_updateShader->SetInt("uColorOverLifetimeEnabled", m_systemData.colorOverLifetime.enabled ? 1 : 0);
        m_updateShader->SetInt("uRotationOverLifetimeEnabled", m_systemData.rotationOverLifetime.enabled ? 1 : 0);
        m_updateShader->SetInt("uNoiseEnabled", m_systemData.noise.enabled ? 1 : 0);

        // Calculate dispatch size (64 particles per work group)
        int numGroups = (m_aliveCount + 63) / 64;
        m_updateShader->Dispatch(numGroups, 1, 1);
        m_updateShader->MemoryBarrier();

        m_updateShader->Unbind();
    }

    // Update alive/dead counts (would read back from GPU in real implementation)
    // For now, just decrement alive count slowly for demonstration
    // m_aliveCount = std::max(0, m_aliveCount - 1);
}

void GPUParticleEngine::Emit(int count) {
    if (!m_initialized || count <= 0) return;

    // Clamp to available dead particles
    count = std::min(count, m_deadCount);
    if (count == 0) return;

    std::cout << "[GPUParticleEngine] Emitting " << count << " particles" << std::endl;

    if (m_emitShader) {
        m_emitShader->Bind();

        // Bind buffers
        m_emitShader->BindStorageBuffer(0, m_particleBuffer);
        m_emitShader->BindStorageBuffer(1, m_deadIndexBuffer);
        m_emitShader->BindStorageBuffer(2, m_deadCountBuffer);
        m_emitShader->BindStorageBuffer(3, m_emissionRequestBuffer);

        // Upload emission request count
        GL_CHECK(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_emissionRequestBuffer));
        GL_CHECK(glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(unsigned int), &count));
        GL_CHECK(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));

        // Set shape uniforms
        m_emitShader->SetVec3("uEmitterPosition", 0.0f, 0.0f, 0.0f);
        m_emitShader->SetInt("uShapeType", static_cast<int>(m_systemData.shape.shapeType));
        m_emitShader->SetFloat("uShapeAngle", m_systemData.shape.angle);
        m_emitShader->SetFloat("uShapeRadius", m_systemData.shape.radius);
        m_emitShader->SetFloat("uShapeRadiusThickness", m_systemData.shape.radiusThickness);
        m_emitShader->SetFloat("uShapeArc", m_systemData.shape.arc);

        // Set particle property ranges
        float lifetimeMin = m_systemData.main.startLifetime.Evaluate(0.0f, 0.0f);
        float lifetimeMax = m_systemData.main.startLifetime.Evaluate(0.0f, 1.0f);
        m_emitShader->SetFloat("uStartLifetimeMin", lifetimeMin);
        m_emitShader->SetFloat("uStartLifetimeMax", lifetimeMax);

        float speedMin = m_systemData.main.startSpeed.Evaluate(0.0f, 0.0f);
        float speedMax = m_systemData.main.startSpeed.Evaluate(0.0f, 1.0f);
        m_emitShader->SetFloat("uStartSpeedMin", speedMin);
        m_emitShader->SetFloat("uStartSpeedMax", speedMax);

        float sizeMin = m_systemData.main.startSize.Evaluate(0.0f, 0.0f);
        float sizeMax = m_systemData.main.startSize.Evaluate(0.0f, 1.0f);
        m_emitShader->SetFloat("uStartSizeMin", sizeMin);
        m_emitShader->SetFloat("uStartSizeMax", sizeMax);

        Color startColor = m_systemData.main.startColor;
        m_emitShader->SetVec4("uStartColor", startColor.r, startColor.g, startColor.b, startColor.a);

        // Random seed
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<unsigned int> dis(0, UINT32_MAX);
        m_emitShader->SetInt("uRandomSeed", dis(gen));
        m_emitShader->SetFloat("uTime", m_systemTime);

        // Dispatch
        int numGroups = (count + 63) / 64;
        m_emitShader->Dispatch(numGroups, 1, 1);
        m_emitShader->MemoryBarrier();

        m_emitShader->Unbind();
    }

    // Update counts
    m_aliveCount += count;
    m_deadCount -= count;
}

void GPUParticleEngine::Reset() {
    m_aliveCount = 0;
    m_deadCount = m_maxParticles;
    m_accumulatedEmission = 0.0f;
    m_systemTime = 0.0f;

    for (size_t i = 0; i < m_burstsFired.size(); ++i) {
        m_burstsFired[i] = false;
    }

    InitializeDeadList();
}

void GPUParticleEngine::CleanupBuffers() {
    if (m_particleBuffer != 0) {
        GL_CHECK(glDeleteBuffers(1, &m_particleBuffer));
        m_particleBuffer = 0;
    }

    if (m_aliveIndexBuffer != 0) {
        GL_CHECK(glDeleteBuffers(1, &m_aliveIndexBuffer));
        m_aliveIndexBuffer = 0;
    }

    if (m_deadIndexBuffer != 0) {
        GL_CHECK(glDeleteBuffers(1, &m_deadIndexBuffer));
        m_deadIndexBuffer = 0;
    }

    if (m_aliveCountBuffer != 0) {
        GL_CHECK(glDeleteBuffers(1, &m_aliveCountBuffer));
        m_aliveCountBuffer = 0;
    }

    if (m_deadCountBuffer != 0) {
        GL_CHECK(glDeleteBuffers(1, &m_deadCountBuffer));
        m_deadCountBuffer = 0;
    }

    if (m_emissionRequestBuffer != 0) {
        GL_CHECK(glDeleteBuffers(1, &m_emissionRequestBuffer));
        m_emissionRequestBuffer = 0;
    }
}

void GPUParticleEngine::CleanupTextures() {
    if (m_sizeOverLifetimeTexture != 0) {
        GL_CHECK(glDeleteTextures(1, &m_sizeOverLifetimeTexture));
        m_sizeOverLifetimeTexture = 0;
    }

    if (m_colorOverLifetimeTexture != 0) {
        GL_CHECK(glDeleteTextures(1, &m_colorOverLifetimeTexture));
        m_colorOverLifetimeTexture = 0;
    }
}

} // namespace GPUParticles
