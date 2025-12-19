#include "cpu_particle_simulator.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace GPUParticles {

CPUParticleSimulator::CPUParticleSimulator()
    : m_aliveCount(0)
    , m_emissionAccumulator(0.0f)
    , m_systemTime(0.0f)
    , m_initialized(false)
    , m_rng(std::random_device{}())
    , m_dist(0.0f, 1.0f)
{
}

CPUParticleSimulator::~CPUParticleSimulator() {
}

bool CPUParticleSimulator::Initialize(const ParticleSystemData& data) {
    std::cout << "[CPUParticleSimulator] Initializing with max particles: "
              << data.main.maxParticles << std::endl;

    m_data = data;

    // Validate data
    if (m_data.main.maxParticles <= 0) {
        m_lastError = "Invalid max particles count";
        return false;
    }

    if (m_data.main.maxParticles > 100000) {
        std::cout << "[CPUParticleSimulator] Warning: High particle count ("
                  << m_data.main.maxParticles << ") may impact performance" << std::endl;
    }

    // Initialize particle pool
    InitializeParticlePool();

    m_initialized = true;
    m_systemTime = 0.0f;
    m_emissionAccumulator = 0.0f;

    std::cout << "[CPUParticleSimulator] Initialization successful!" << std::endl;
    return true;
}

void CPUParticleSimulator::InitializeParticlePool() {
    m_particles.resize(m_data.main.maxParticles);
    m_aliveCount = 0;

    // Initialize all particles as dead
    for (auto& p : m_particles) {
        p.alive = false;
    }
}

void CPUParticleSimulator::Update(float deltaTime) {
    if (!m_initialized) {
        return;
    }

    // Clamp delta time to prevent huge jumps
    deltaTime = std::min(deltaTime, 0.1f);

    m_systemTime += deltaTime;

    // Check duration and looping
    if (m_systemTime >= m_data.main.duration) {
        if (m_data.main.looping) {
            // Reset time for looping systems
            m_systemTime = fmod(m_systemTime, m_data.main.duration);
        } else {
            // Non-looping: stop emitting but keep updating existing particles
            UpdateParticles(deltaTime);
            return;
        }
    }

    // Emit new particles
    if (m_data.emission.enabled) {
        EmitParticles(deltaTime);
    }

    // Update existing particles
    UpdateParticles(deltaTime);
}

void CPUParticleSimulator::EmitParticles(float deltaTime) {
    // Calculate emission rate
    float emissionRate = EvaluateMinMaxCurve(m_data.emission.rateOverTime, m_systemTime);

    // Accumulate particles to emit
    m_emissionAccumulator += emissionRate * deltaTime;

    // Emit integer number of particles
    int particlesToEmit = static_cast<int>(m_emissionAccumulator);
    m_emissionAccumulator -= particlesToEmit;

    // Process bursts
    for (const auto& burst : m_data.emission.bursts) {
        // Check if burst should trigger at this time
        float timeSinceBurst = m_systemTime - burst.time;
        if (timeSinceBurst >= 0.0f && timeSinceBurst < deltaTime) {
            // Use random count between min and max
            int count = burst.minCount;
            if (burst.maxCount > burst.minCount) {
                count = burst.minCount + (rand() % (burst.maxCount - burst.minCount + 1));
            }
            particlesToEmit += count;
        }
    }

    // Spawn particles
    for (int i = 0; i < particlesToEmit && m_aliveCount < m_data.main.maxParticles; ++i) {
        SpawnParticle();
    }
}

void CPUParticleSimulator::SpawnParticle() {
    // Find dead particle slot
    for (auto& p : m_particles) {
        if (!p.alive) {
            // Initialize particle
            p.alive = true;
            p.age = 0.0f;
            p.lifetime = EvaluateMinMaxCurve(m_data.main.startLifetime, m_systemTime);
            p.position = GetEmissionPosition();
            p.velocity = GetEmissionVelocity();
            p.size = EvaluateMinMaxCurve(m_data.main.startSize, m_systemTime);
            p.rotation = EvaluateMinMaxCurve(m_data.main.startRotation, m_systemTime);
            p.color = m_data.main.startColor;

            m_aliveCount++;
            return;
        }
    }
}

Vector3 CPUParticleSimulator::GetEmissionPosition() {
    if (!m_data.shape.enabled) {
        return Vector3(0, 0, 0);
    }

    Vector3 pos(0, 0, 0);

    switch (m_data.shape.shapeType) {
        case ParticleSystemShapeType::Cone: {
            // Emit from cone
            float angle = m_data.shape.angle * (3.14159f / 180.0f);
            float radius = m_data.shape.radius * RandomRange(0, 1);
            float theta = RandomRange(0, 2 * 3.14159f);

            pos.x = radius * std::cos(theta);
            pos.y = radius * std::sin(theta);
            pos.z = 0;
            break;
        }

        case ParticleSystemShapeType::Sphere: {
            // Emit from sphere surface or volume
            float theta = RandomRange(0, 2 * 3.14159f);
            float phi = RandomRange(0, 3.14159f);
            float r = m_data.shape.radius;

            pos.x = r * std::sin(phi) * std::cos(theta);
            pos.y = r * std::sin(phi) * std::sin(theta);
            pos.z = r * std::cos(phi);
            break;
        }

        case ParticleSystemShapeType::Box: {
            // Emit from box volume
            pos.x = RandomRange(-0.5f, 0.5f) * m_data.shape.scale.x;
            pos.y = RandomRange(-0.5f, 0.5f) * m_data.shape.scale.y;
            pos.z = RandomRange(-0.5f, 0.5f) * m_data.shape.scale.z;
            break;
        }

        default:
            break;
    }

    // Apply shape position offset
    pos.x += m_data.shape.position.x;
    pos.y += m_data.shape.position.y;
    pos.z += m_data.shape.position.z;

    return pos;
}

Vector3 CPUParticleSimulator::GetEmissionVelocity() {
    float speed = EvaluateMinMaxCurve(m_data.main.startSpeed, m_systemTime);

    Vector3 direction(0, 0, 1); // Default forward

    if (m_data.shape.enabled) {
        switch (m_data.shape.shapeType) {
            case ParticleSystemShapeType::Cone: {
                // Cone direction
                float angle = m_data.shape.angle * (3.14159f / 180.0f);
                float theta = RandomRange(0, 2 * 3.14159f);
                float phi = RandomRange(0, angle);

                direction.x = std::sin(phi) * std::cos(theta);
                direction.y = std::sin(phi) * std::sin(theta);
                direction.z = std::cos(phi);
                break;
            }

            case ParticleSystemShapeType::Sphere: {
                // Radial direction
                float theta = RandomRange(0, 2 * 3.14159f);
                float phi = RandomRange(0, 3.14159f);

                direction.x = std::sin(phi) * std::cos(theta);
                direction.y = std::sin(phi) * std::sin(theta);
                direction.z = std::cos(phi);
                break;
            }

            default:
                break;
        }
    }

    return Vector3(direction.x * speed, direction.y * speed, direction.z * speed);
}

void CPUParticleSimulator::UpdateParticles(float deltaTime) {
    m_aliveCount = 0;

    for (auto& p : m_particles) {
        if (!p.alive) {
            continue;
        }

        // Update age
        p.age += deltaTime;

        // Check if particle died
        if (p.age >= p.lifetime) {
            p.alive = false;
            continue;
        }

        // Apply forces
        ApplyForces(p, deltaTime);

        // Update position
        p.position.x += p.velocity.x * deltaTime;
        p.position.y += p.velocity.y * deltaTime;
        p.position.z += p.velocity.z * deltaTime;

        // Apply lifetime modules
        UpdateColorOverLifetime(p);
        UpdateSizeOverLifetime(p);
        UpdateRotationOverLifetime(p, deltaTime);

        m_aliveCount++;
    }
}

void CPUParticleSimulator::ApplyForces(Particle& p, float deltaTime) {
    // Gravity
    float gravityMod = EvaluateMinMaxCurve(m_data.main.gravityModifier, 0);
    p.velocity.z -= 9.81f * gravityMod * deltaTime;

    // Force over lifetime
    if (m_data.forceOverLifetime.enabled) {
        float t = p.age / p.lifetime;
        p.velocity.x += EvaluateMinMaxCurve(m_data.forceOverLifetime.x, t) * deltaTime;
        p.velocity.y += EvaluateMinMaxCurve(m_data.forceOverLifetime.y, t) * deltaTime;
        p.velocity.z += EvaluateMinMaxCurve(m_data.forceOverLifetime.z, t) * deltaTime;
    }

    // Velocity over lifetime
    if (m_data.velocityOverLifetime.enabled) {
        float t = p.age / p.lifetime;

        if (m_data.velocityOverLifetime.space == ParticleSystemSimulationSpace::Local) {
            p.velocity.x = EvaluateMinMaxCurve(m_data.velocityOverLifetime.x, t);
            p.velocity.y = EvaluateMinMaxCurve(m_data.velocityOverLifetime.y, t);
            p.velocity.z = EvaluateMinMaxCurve(m_data.velocityOverLifetime.z, t);
        }
    }
}

void CPUParticleSimulator::UpdateColorOverLifetime(Particle& p) {
    if (!m_data.colorOverLifetime.enabled) {
        return;
    }

    float t = p.age / p.lifetime;
    p.color = EvaluateGradient(m_data.colorOverLifetime.gradient, t);
}

void CPUParticleSimulator::UpdateSizeOverLifetime(Particle& p) {
    if (!m_data.sizeOverLifetime.enabled) {
        return;
    }

    float t = p.age / p.lifetime;
    float sizeMultiplier = EvaluateMinMaxCurve(m_data.sizeOverLifetime.size, t);

    float startSize = EvaluateMinMaxCurve(m_data.main.startSize, 0);
    p.size = startSize * sizeMultiplier;
}

void CPUParticleSimulator::UpdateRotationOverLifetime(Particle& p, float deltaTime) {
    if (!m_data.rotationOverLifetime.enabled) {
        return;
    }

    float t = p.age / p.lifetime;
    float rotationSpeed = EvaluateMinMaxCurve(m_data.rotationOverLifetime.z, t);

    // Convert degrees to radians
    rotationSpeed *= (3.14159f / 180.0f);

    p.rotation += rotationSpeed * deltaTime;
}

void CPUParticleSimulator::Reset() {
    m_systemTime = 0.0f;
    m_emissionAccumulator = 0.0f;
    m_aliveCount = 0;

    for (auto& p : m_particles) {
        p.alive = false;
    }
}

float CPUParticleSimulator::EvaluateMinMaxCurve(const MinMaxCurve& curve, float time) const {
    switch (curve.mode) {
        case CurveMode::Constant:
            return curve.constant;

        case CurveMode::Curve:
            return EvaluateCurve(curve.curve, time) * curve.multiplier;

        case CurveMode::RandomBetweenTwoConstants:
            return RandomRange(curve.constantMin, curve.constantMax);

        case CurveMode::RandomBetweenTwoCurves:
            // Simplified: just return average
            return (EvaluateCurve(curve.curveMin, time) + EvaluateCurve(curve.curveMax, time)) * 0.5f * curve.multiplier;

        default:
            return curve.constant;
    }
}

float CPUParticleSimulator::EvaluateCurve(const AnimationCurve& curve, float time) const {
    if (curve.keys.empty()) {
        return 0.0f;
    }

    if (curve.keys.size() == 1) {
        return curve.keys[0].value;
    }

    // Find surrounding keyframes
    for (size_t i = 0; i < curve.keys.size() - 1; ++i) {
        if (time >= curve.keys[i].time && time <= curve.keys[i + 1].time) {
            // Linear interpolation (simplified)
            float t = (time - curve.keys[i].time) / (curve.keys[i + 1].time - curve.keys[i].time);
            return curve.keys[i].value + t * (curve.keys[i + 1].value - curve.keys[i].value);
        }
    }

    // Outside range
    if (time < curve.keys[0].time) {
        return curve.keys[0].value;
    }
    return curve.keys.back().value;
}

Color CPUParticleSimulator::EvaluateGradient(const Gradient& gradient, float time) const {
    if (gradient.colorKeys.empty() || gradient.alphaKeys.empty()) {
        return Color(1, 1, 1, 1);
    }

    // Evaluate color
    Color color(1, 1, 1, 1);

    if (gradient.colorKeys.size() == 1) {
        color = gradient.colorKeys[0].color;
    } else {
        for (size_t i = 0; i < gradient.colorKeys.size() - 1; ++i) {
            if (time >= gradient.colorKeys[i].time && time <= gradient.colorKeys[i + 1].time) {
                float t = (time - gradient.colorKeys[i].time) /
                         (gradient.colorKeys[i + 1].time - gradient.colorKeys[i].time);

                color.r = gradient.colorKeys[i].color.r + t * (gradient.colorKeys[i + 1].color.r - gradient.colorKeys[i].color.r);
                color.g = gradient.colorKeys[i].color.g + t * (gradient.colorKeys[i + 1].color.g - gradient.colorKeys[i].color.g);
                color.b = gradient.colorKeys[i].color.b + t * (gradient.colorKeys[i + 1].color.b - gradient.colorKeys[i].color.b);
                break;
            }
        }
    }

    // Evaluate alpha
    if (gradient.alphaKeys.size() == 1) {
        color.a = gradient.alphaKeys[0].alpha;
    } else {
        for (size_t i = 0; i < gradient.alphaKeys.size() - 1; ++i) {
            if (time >= gradient.alphaKeys[i].time && time <= gradient.alphaKeys[i + 1].time) {
                float t = (time - gradient.alphaKeys[i].time) /
                         (gradient.alphaKeys[i + 1].time - gradient.alphaKeys[i].time);

                color.a = gradient.alphaKeys[i].alpha + t * (gradient.alphaKeys[i + 1].alpha - gradient.alphaKeys[i].alpha);
                break;
            }
        }
    }

    return color;
}

float CPUParticleSimulator::RandomRange(float min, float max) const {
    // Since this is const, we can't modify the RNG. Use standard rand() instead.
    return min + (max - min) * (static_cast<float>(rand()) / RAND_MAX);
}

} // namespace GPUParticles
