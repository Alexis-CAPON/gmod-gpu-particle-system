#include "particle_loader.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

using json = nlohmann::json;

namespace GPUParticles {

// ============================================================================
// Main Loading Functions
// ============================================================================

std::unique_ptr<ParticleSystemData> ParticleLoader::LoadFromFile(const std::string& filepath) {
    try {
        std::cout << "[ParticleLoader] Attempting to open: " << filepath << std::endl;

        std::ifstream file(filepath);
        if (!file.is_open()) {
            m_lastError = "Failed to open file: " + filepath;
            std::cerr << "[ParticleLoader] File does not exist or cannot be opened" << std::endl;
            return nullptr;
        }

        std::cout << "[ParticleLoader] File opened successfully, parsing JSON..." << std::endl;

        json j;
        file >> j;

        std::cout << "[ParticleLoader] JSON parsed successfully" << std::endl;

        return LoadFromString(j.dump());
    }
    catch (const std::exception& e) {
        m_lastError = std::string("JSON parse error: ") + e.what();
        std::cerr << "[ParticleLoader] Exception: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<ParticleSystemData> ParticleLoader::LoadFromString(const std::string& jsonString) {
    try {
        json j = json::parse(jsonString);

        auto data = std::make_unique<ParticleSystemData>();

        // Parse metadata
        if (j.contains("metadata")) {
            data->name = j["metadata"].value("name", "Unnamed");
            data->version = j["metadata"].value("version", "1.0");
        }

        // Parse main system
        if (j.contains("system")) {
            ParseMainModule(data->main, j["system"]);
        }

        // Parse modules
        if (j.contains("emission")) {
            ParseEmissionModule(data->emission, j["emission"]);
        }

        if (j.contains("shape")) {
            ParseShapeModule(data->shape, j["shape"]);
        }

        if (j.contains("velocityOverLifetime")) {
            ParseVelocityOverLifetimeModule(data->velocityOverLifetime, j["velocityOverLifetime"]);
        }

        if (j.contains("limitVelocityOverLifetime")) {
            ParseLimitVelocityOverLifetimeModule(data->limitVelocityOverLifetime, j["limitVelocityOverLifetime"]);
        }

        if (j.contains("forceOverLifetime")) {
            ParseForceOverLifetimeModule(data->forceOverLifetime, j["forceOverLifetime"]);
        }

        if (j.contains("colorOverLifetime")) {
            ParseColorOverLifetimeModule(data->colorOverLifetime, j["colorOverLifetime"]);
        }

        if (j.contains("sizeOverLifetime")) {
            ParseSizeOverLifetimeModule(data->sizeOverLifetime, j["sizeOverLifetime"]);
        }

        if (j.contains("rotationOverLifetime")) {
            ParseRotationOverLifetimeModule(data->rotationOverLifetime, j["rotationOverLifetime"]);
        }

        if (j.contains("noise")) {
            ParseNoiseModule(data->noise, j["noise"]);
        }

        if (j.contains("collision")) {
            ParseCollisionModule(data->collision, j["collision"]);
        }

        if (j.contains("textureSheetAnimation")) {
            ParseTextureSheetAnimationModule(data->textureSheetAnimation, j["textureSheetAnimation"]);
        }

        if (j.contains("renderer")) {
            ParseRendererModule(data->renderer, j["renderer"]);
        }

        if (j.contains("subEmitters")) {
            ParseSubEmitters(data->subEmitters, j["subEmitters"]);
        }

        m_lastError.clear();
        return data;
    }
    catch (const std::exception& e) {
        m_lastError = std::string("Parse error: ") + e.what();
        return nullptr;
    }
}

// ============================================================================
// Basic Type Parsers
// ============================================================================

Vector3 ParticleLoader::ParseVector3(const json& j) {
    return Vector3(
        j.value("x", 0.0f),
        j.value("y", 0.0f),
        j.value("z", 0.0f)
    );
}

Vector4 ParticleLoader::ParseVector4(const json& j) {
    return Vector4(
        j.value("x", 0.0f),
        j.value("y", 0.0f),
        j.value("z", 0.0f),
        j.value("w", 0.0f)
    );
}

Color ParticleLoader::ParseColor(const json& j) {
    return Color(
        j.value("r", 1.0f),
        j.value("g", 1.0f),
        j.value("b", 1.0f),
        j.value("a", 1.0f)
    );
}

Keyframe ParticleLoader::ParseKeyframe(const json& j) {
    Keyframe kf;
    kf.time = j.value("time", 0.0f);
    kf.value = j.value("value", 0.0f);
    kf.inTangent = j.value("inTangent", 0.0f);
    kf.outTangent = j.value("outTangent", 0.0f);
    return kf;
}

AnimationCurve ParticleLoader::ParseAnimationCurve(const json& j) {
    AnimationCurve curve;
    if (j.contains("keys") && j["keys"].is_array()) {
        for (const auto& keyJson : j["keys"]) {
            curve.keys.push_back(ParseKeyframe(keyJson));
        }
    }
    return curve;
}

MinMaxCurve ParticleLoader::ParseMinMaxCurve(const json& j) {
    MinMaxCurve curve;

    if (j.contains("mode")) {
        curve.mode = ParseCurveMode(j["mode"].get<std::string>());
    }

    curve.constant = j.value("constant", 0.0f);
    curve.constantMin = j.value("constantMin", 0.0f);
    curve.constantMax = j.value("constantMax", 0.0f);
    curve.multiplier = j.value("multiplier", 1.0f);

    if (j.contains("curve")) {
        curve.curve = ParseAnimationCurve(j["curve"]);
    }

    if (j.contains("curveMin")) {
        curve.curveMin = ParseAnimationCurve(j["curveMin"]);
    }

    if (j.contains("curveMax")) {
        curve.curveMax = ParseAnimationCurve(j["curveMax"]);
    }

    return curve;
}

GradientColorKey ParticleLoader::ParseGradientColorKey(const json& j) {
    GradientColorKey key;
    key.color = ParseColor(j["color"]);
    key.time = j.value("time", 0.0f);
    return key;
}

GradientAlphaKey ParticleLoader::ParseGradientAlphaKey(const json& j) {
    GradientAlphaKey key;
    key.alpha = j.value("alpha", 1.0f);
    key.time = j.value("time", 0.0f);
    return key;
}

Gradient ParticleLoader::ParseGradient(const json& j) {
    Gradient gradient;

    if (j.contains("colorKeys") && j["colorKeys"].is_array()) {
        for (const auto& keyJson : j["colorKeys"]) {
            gradient.colorKeys.push_back(ParseGradientColorKey(keyJson));
        }
    }

    if (j.contains("alphaKeys") && j["alphaKeys"].is_array()) {
        for (const auto& keyJson : j["alphaKeys"]) {
            gradient.alphaKeys.push_back(ParseGradientAlphaKey(keyJson));
        }
    }

    return gradient;
}

Burst ParticleLoader::ParseBurst(const json& j) {
    Burst burst;
    burst.time = j.value("time", 0.0f);
    burst.minCount = j.value("minCount", 0);
    burst.maxCount = j.value("maxCount", 0);
    burst.cycles = j.value("cycles", 1);
    burst.repeatInterval = j.value("repeatInterval", 0.0f);
    return burst;
}

// ============================================================================
// Module Parsers
// ============================================================================

void ParticleLoader::ParseMainModule(MainModule& module, const json& j) {
    module.duration = j.value("duration", 5.0f);
    module.looping = j.value("looping", true);
    module.prewarm = j.value("prewarm", false);
    module.simulationSpeed = j.value("simulationSpeed", 1.0f);
    module.playOnAwake = j.value("playOnAwake", true);
    module.maxParticles = j.value("maxParticles", 1000);
    module.startSize3D = j.value("startSize3D", false);
    module.startRotation3D = j.value("startRotation3D", false);

    if (j.contains("startDelay")) {
        module.startDelay = ParseMinMaxCurve(j["startDelay"]);
    }

    if (j.contains("startLifetime")) {
        module.startLifetime = ParseMinMaxCurve(j["startLifetime"]);
    }

    if (j.contains("startSpeed")) {
        module.startSpeed = ParseMinMaxCurve(j["startSpeed"]);
    }

    if (j.contains("startSize")) {
        module.startSize = ParseMinMaxCurve(j["startSize"]);
    }

    if (j.contains("startRotation")) {
        module.startRotation = ParseMinMaxCurve(j["startRotation"]);
    }

    if (j.contains("startColor")) {
        module.startColor = ParseColor(j["startColor"]);
    }

    if (j.contains("gravityModifier")) {
        module.gravityModifier = ParseMinMaxCurve(j["gravityModifier"]);
    }

    if (j.contains("simulationSpace")) {
        module.simulationSpace = ParseSimulationSpace(j["simulationSpace"].get<std::string>());
    }
}

void ParticleLoader::ParseEmissionModule(EmissionModule& module, const json& j) {
    module.enabled = j.value("enabled", true);

    if (j.contains("rateOverTime")) {
        module.rateOverTime = ParseMinMaxCurve(j["rateOverTime"]);
    }

    if (j.contains("rateOverDistance")) {
        module.rateOverDistance = ParseMinMaxCurve(j["rateOverDistance"]);
    }

    if (j.contains("bursts") && j["bursts"].is_array()) {
        for (const auto& burstJson : j["bursts"]) {
            module.bursts.push_back(ParseBurst(burstJson));
        }
    }
}

void ParticleLoader::ParseShapeModule(ShapeModule& module, const json& j) {
    module.enabled = j.value("enabled", true);

    if (j.contains("shapeType")) {
        module.shapeType = ParseShapeType(j["shapeType"].get<std::string>());
    }

    module.angle = j.value("angle", 25.0f);
    module.radius = j.value("radius", 1.0f);
    module.radiusThickness = j.value("radiusThickness", 1.0f);
    module.arc = j.value("arc", 360.0f);
    module.alignToDirection = j.value("alignToDirection", false);
    module.randomDirectionAmount = j.value("randomDirectionAmount", 0.0f);
    module.sphericalDirectionAmount = j.value("sphericalDirectionAmount", 0.0f);

    if (j.contains("boxScale")) {
        module.boxScale = ParseVector3(j["boxScale"]);
    }

    if (j.contains("position")) {
        module.position = ParseVector3(j["position"]);
    }

    if (j.contains("rotation")) {
        module.rotation = ParseVector3(j["rotation"]);
    }

    if (j.contains("scale")) {
        module.scale = ParseVector3(j["scale"]);
    }
}

void ParticleLoader::ParseVelocityOverLifetimeModule(VelocityOverLifetimeModule& module, const json& j) {
    module.enabled = j.value("enabled", false);

    if (j.contains("x")) {
        module.x = ParseMinMaxCurve(j["x"]);
    }

    if (j.contains("y")) {
        module.y = ParseMinMaxCurve(j["y"]);
    }

    if (j.contains("z")) {
        module.z = ParseMinMaxCurve(j["z"]);
    }

    if (j.contains("space")) {
        module.space = ParseSimulationSpace(j["space"].get<std::string>());
    }
}

void ParticleLoader::ParseLimitVelocityOverLifetimeModule(LimitVelocityOverLifetimeModule& module, const json& j) {
    module.enabled = j.value("enabled", false);
    module.dampen = j.value("dampen", 0.5f);
    module.separateAxes = j.value("separateAxes", false);

    if (j.contains("limit")) {
        module.limit = ParseMinMaxCurve(j["limit"]);
    }

    if (j.contains("limitX")) {
        module.limitX = ParseMinMaxCurve(j["limitX"]);
    }

    if (j.contains("limitY")) {
        module.limitY = ParseMinMaxCurve(j["limitY"]);
    }

    if (j.contains("limitZ")) {
        module.limitZ = ParseMinMaxCurve(j["limitZ"]);
    }
}

void ParticleLoader::ParseForceOverLifetimeModule(ForceOverLifetimeModule& module, const json& j) {
    module.enabled = j.value("enabled", false);
    module.randomized = j.value("randomized", false);

    if (j.contains("x")) {
        module.x = ParseMinMaxCurve(j["x"]);
    }

    if (j.contains("y")) {
        module.y = ParseMinMaxCurve(j["y"]);
    }

    if (j.contains("z")) {
        module.z = ParseMinMaxCurve(j["z"]);
    }

    if (j.contains("space")) {
        module.space = ParseSimulationSpace(j["space"].get<std::string>());
    }
}

void ParticleLoader::ParseColorOverLifetimeModule(ColorOverLifetimeModule& module, const json& j) {
    module.enabled = j.value("enabled", false);

    if (j.contains("gradient")) {
        module.gradient = ParseGradient(j["gradient"]);
    }
}

void ParticleLoader::ParseSizeOverLifetimeModule(SizeOverLifetimeModule& module, const json& j) {
    module.enabled = j.value("enabled", false);
    module.separateAxes = j.value("separateAxes", false);

    if (j.contains("size")) {
        module.size = ParseMinMaxCurve(j["size"]);
    }

    if (j.contains("x")) {
        module.x = ParseMinMaxCurve(j["x"]);
    }

    if (j.contains("y")) {
        module.y = ParseMinMaxCurve(j["y"]);
    }

    if (j.contains("z")) {
        module.z = ParseMinMaxCurve(j["z"]);
    }
}

void ParticleLoader::ParseRotationOverLifetimeModule(RotationOverLifetimeModule& module, const json& j) {
    module.enabled = j.value("enabled", false);
    module.separateAxes = j.value("separateAxes", false);

    if (j.contains("x")) {
        module.x = ParseMinMaxCurve(j["x"]);
    }

    if (j.contains("y")) {
        module.y = ParseMinMaxCurve(j["y"]);
    }

    if (j.contains("z")) {
        module.z = ParseMinMaxCurve(j["z"]);
    }
}

void ParticleLoader::ParseNoiseModule(NoiseModule& module, const json& j) {
    module.enabled = j.value("enabled", false);
    module.frequency = j.value("frequency", 0.5f);
    module.scrollSpeed = j.value("scrollSpeed", 0.0f);
    module.damping = j.value("damping", true);
    module.octaves = j.value("octaves", 1);
    module.octaveMultiplier = j.value("octaveMultiplier", 0.5f);
    module.octaveScale = j.value("octaveScale", 2.0f);
    module.quality = j.value("quality", 1);
    module.separateAxes = j.value("separateAxes", false);

    if (j.contains("strength")) {
        module.strength = ParseMinMaxCurve(j["strength"]);
    }

    if (j.contains("strengthX")) {
        module.strengthX = ParseMinMaxCurve(j["strengthX"]);
    }

    if (j.contains("strengthY")) {
        module.strengthY = ParseMinMaxCurve(j["strengthY"]);
    }

    if (j.contains("strengthZ")) {
        module.strengthZ = ParseMinMaxCurve(j["strengthZ"]);
    }
}

void ParticleLoader::ParseCollisionModule(CollisionModule& module, const json& j) {
    module.enabled = j.value("enabled", false);
    module.minKillSpeed = j.value("minKillSpeed", 0.0f);
    module.maxKillSpeed = j.value("maxKillSpeed", 10000.0f);
    module.radiusScale = j.value("radiusScale", 1.0f);
    module.collidesWithDynamic = j.value("collidesWithDynamic", true);
    module.maxCollisionShapes = j.value("maxCollisionShapes", 256);

    if (j.contains("type")) {
        module.type = ParseCollisionType(j["type"].get<std::string>());
    }

    if (j.contains("dampen")) {
        module.dampen = ParseMinMaxCurve(j["dampen"]);
    }

    if (j.contains("bounce")) {
        module.bounce = ParseMinMaxCurve(j["bounce"]);
    }

    if (j.contains("lifetimeLoss")) {
        module.lifetimeLoss = ParseMinMaxCurve(j["lifetimeLoss"]);
    }
}

void ParticleLoader::ParseTextureSheetAnimationModule(TextureSheetAnimationModule& module, const json& j) {
    module.enabled = j.value("enabled", false);
    module.numTilesX = j.value("numTilesX", 1);
    module.numTilesY = j.value("numTilesY", 1);
    module.cycleCount = j.value("cycleCount", 1);
    module.rowIndex = j.value("rowIndex", 0);

    if (j.contains("animationType")) {
        module.animationType = ParseAnimationType(j["animationType"].get<std::string>());
    }

    if (j.contains("frameOverTime")) {
        module.frameOverTime = ParseMinMaxCurve(j["frameOverTime"]);
    }

    if (j.contains("startFrame")) {
        module.startFrame = ParseMinMaxCurve(j["startFrame"]);
    }
}

void ParticleLoader::ParseRendererModule(RendererModule& module, const json& j) {
    module.minParticleSize = j.value("minParticleSize", 0.0f);
    module.maxParticleSize = j.value("maxParticleSize", 0.5f);
    module.material = j.value("material", "");
    module.texture = j.value("texture", "");
    module.flip = j.value("flip", false);
    module.lengthScale = j.value("lengthScale", 2.0f);
    module.normalDirection = j.value("normalDirection", 1.0f);
    module.sortingOrder = j.value("sortingOrder", 0);

    if (j.contains("renderMode")) {
        module.renderMode = ParseRenderMode(j["renderMode"].get<std::string>());
    }

    if (j.contains("sortMode")) {
        module.sortMode = ParseSortMode(j["sortMode"].get<std::string>());
    }

    if (j.contains("pivot")) {
        module.pivot = ParseVector3(j["pivot"]);
    }

    if (j.contains("velocityScale")) {
        module.velocityScale = ParseVector3(j["velocityScale"]);
    }
}

void ParticleLoader::ParseSubEmitters(std::vector<SubEmitter>& subEmitters, const json& j) {
    if (j.is_array()) {
        for (const auto& subEmitterJson : j) {
            SubEmitter subEmitter;
            subEmitter.subEmitterName = subEmitterJson.value("name", "");

            if (subEmitterJson.contains("type")) {
                subEmitter.type = ParseSubEmitterType(subEmitterJson["type"].get<std::string>());
            }

            subEmitters.push_back(subEmitter);
        }
    }
}

// ============================================================================
// Enum Parsers
// ============================================================================

CurveMode ParticleLoader::ParseCurveMode(const std::string& str) {
    if (str == "Curve") return CurveMode::Curve;
    if (str == "TwoConstants") return CurveMode::TwoConstants;
    if (str == "TwoCurves") return CurveMode::TwoCurves;
    if (str == "RandomBetweenTwoConstants") return CurveMode::RandomBetweenTwoConstants;
    if (str == "RandomBetweenTwoCurves") return CurveMode::RandomBetweenTwoCurves;
    return CurveMode::Constant;
}

ParticleSystemShapeType ParticleLoader::ParseShapeType(const std::string& str) {
    if (str == "Sphere") return ParticleSystemShapeType::Sphere;
    if (str == "Hemisphere") return ParticleSystemShapeType::Hemisphere;
    if (str == "Cone") return ParticleSystemShapeType::Cone;
    if (str == "Box") return ParticleSystemShapeType::Box;
    if (str == "Circle") return ParticleSystemShapeType::Circle;
    if (str == "Edge") return ParticleSystemShapeType::Edge;
    if (str == "Rectangle") return ParticleSystemShapeType::Rectangle;
    return ParticleSystemShapeType::Cone;
}

ParticleSystemSimulationSpace ParticleLoader::ParseSimulationSpace(const std::string& str) {
    if (str == "Local") return ParticleSystemSimulationSpace::Local;
    if (str == "World") return ParticleSystemSimulationSpace::World;
    if (str == "Custom") return ParticleSystemSimulationSpace::Custom;
    return ParticleSystemSimulationSpace::World;
}

ParticleSystemRenderMode ParticleLoader::ParseRenderMode(const std::string& str) {
    if (str == "Billboard") return ParticleSystemRenderMode::Billboard;
    if (str == "Stretch") return ParticleSystemRenderMode::Stretch;
    if (str == "HorizontalBillboard") return ParticleSystemRenderMode::HorizontalBillboard;
    if (str == "VerticalBillboard") return ParticleSystemRenderMode::VerticalBillboard;
    if (str == "Mesh") return ParticleSystemRenderMode::Mesh;
    return ParticleSystemRenderMode::Billboard;
}

ParticleSystemSortMode ParticleLoader::ParseSortMode(const std::string& str) {
    if (str == "None") return ParticleSystemSortMode::None;
    if (str == "Distance") return ParticleSystemSortMode::Distance;
    if (str == "OldestInFront") return ParticleSystemSortMode::OldestInFront;
    if (str == "YoungestInFront") return ParticleSystemSortMode::YoungestInFront;
    return ParticleSystemSortMode::None;
}

ParticleSystemCollisionType ParticleLoader::ParseCollisionType(const std::string& str) {
    if (str == "Planes") return ParticleSystemCollisionType::Planes;
    if (str == "World") return ParticleSystemCollisionType::World;
    return ParticleSystemCollisionType::World;
}

ParticleSystemAnimationType ParticleLoader::ParseAnimationType(const std::string& str) {
    if (str == "WholeSheet") return ParticleSystemAnimationType::WholeSheet;
    if (str == "SingleRow") return ParticleSystemAnimationType::SingleRow;
    return ParticleSystemAnimationType::WholeSheet;
}

ParticleSystemSubEmitterType ParticleLoader::ParseSubEmitterType(const std::string& str) {
    if (str == "Birth") return ParticleSystemSubEmitterType::Birth;
    if (str == "Collision") return ParticleSystemSubEmitterType::Collision;
    if (str == "Death") return ParticleSystemSubEmitterType::Death;
    return ParticleSystemSubEmitterType::Birth;
}

} // namespace GPUParticles
