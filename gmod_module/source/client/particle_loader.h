#pragma once

#include "../particle_data.h"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

namespace GPUParticles {

/**
 * @brief Loads particle system data from .gpart JSON files
 */
class ParticleLoader {
public:
    ParticleLoader() = default;
    ~ParticleLoader() = default;

    /**
     * @brief Load a particle system from a .gpart file
     * @param filepath Path to the .gpart file
     * @return Particle system data, or nullptr on failure
     */
    std::unique_ptr<ParticleSystemData> LoadFromFile(const std::string& filepath);

    /**
     * @brief Load a particle system from a JSON string
     * @param jsonString JSON string containing particle data
     * @return Particle system data, or nullptr on failure
     */
    std::unique_ptr<ParticleSystemData> LoadFromString(const std::string& jsonString);

    /**
     * @brief Get the last error message
     */
    const std::string& GetLastError() const { return m_lastError; }

private:
    std::string m_lastError;

    // Helper parsing functions
    Vector3 ParseVector3(const nlohmann::json& j);
    Vector4 ParseVector4(const nlohmann::json& j);
    Color ParseColor(const nlohmann::json& j);
    Keyframe ParseKeyframe(const nlohmann::json& j);
    AnimationCurve ParseAnimationCurve(const nlohmann::json& j);
    MinMaxCurve ParseMinMaxCurve(const nlohmann::json& j);
    Gradient ParseGradient(const nlohmann::json& j);
    GradientColorKey ParseGradientColorKey(const nlohmann::json& j);
    GradientAlphaKey ParseGradientAlphaKey(const nlohmann::json& j);
    Burst ParseBurst(const nlohmann::json& j);

    // Module parsing functions
    void ParseMainModule(MainModule& module, const nlohmann::json& j);
    void ParseEmissionModule(EmissionModule& module, const nlohmann::json& j);
    void ParseShapeModule(ShapeModule& module, const nlohmann::json& j);
    void ParseVelocityOverLifetimeModule(VelocityOverLifetimeModule& module, const nlohmann::json& j);
    void ParseLimitVelocityOverLifetimeModule(LimitVelocityOverLifetimeModule& module, const nlohmann::json& j);
    void ParseForceOverLifetimeModule(ForceOverLifetimeModule& module, const nlohmann::json& j);
    void ParseColorOverLifetimeModule(ColorOverLifetimeModule& module, const nlohmann::json& j);
    void ParseSizeOverLifetimeModule(SizeOverLifetimeModule& module, const nlohmann::json& j);
    void ParseRotationOverLifetimeModule(RotationOverLifetimeModule& module, const nlohmann::json& j);
    void ParseNoiseModule(NoiseModule& module, const nlohmann::json& j);
    void ParseCollisionModule(CollisionModule& module, const nlohmann::json& j);
    void ParseTextureSheetAnimationModule(TextureSheetAnimationModule& module, const nlohmann::json& j);
    void ParseRendererModule(RendererModule& module, const nlohmann::json& j);
    void ParseSubEmitters(std::vector<SubEmitter>& subEmitters, const nlohmann::json& j);

    // Enum parsing helpers
    CurveMode ParseCurveMode(const std::string& str);
    ParticleSystemShapeType ParseShapeType(const std::string& str);
    ParticleSystemSimulationSpace ParseSimulationSpace(const std::string& str);
    ParticleSystemRenderMode ParseRenderMode(const std::string& str);
    ParticleSystemSortMode ParseSortMode(const std::string& str);
    ParticleSystemCollisionType ParseCollisionType(const std::string& str);
    ParticleSystemAnimationType ParseAnimationType(const std::string& str);
    ParticleSystemSubEmitterType ParseSubEmitterType(const std::string& str);
};

} // namespace GPUParticles
