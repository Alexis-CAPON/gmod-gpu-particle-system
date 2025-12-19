#pragma once

#include <string>
#include <vector>
#include <array>

namespace GPUParticles {

// ============================================================================
// Core Data Types
// ============================================================================

struct Vector3 {
    float x, y, z;
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct Vector4 {
    float x, y, z, w;
    Vector4() : x(0), y(0), z(0), w(0) {}
    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
};

struct Color {
    float r, g, b, a;
    Color() : r(1), g(1), b(1), a(1) {}
    Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
};

// ============================================================================
// Curve Types
// ============================================================================

enum class CurveMode {
    Constant,
    Curve,
    TwoConstants,
    TwoCurves,
    RandomBetweenTwoConstants,
    RandomBetweenTwoCurves
};

struct Keyframe {
    float time;
    float value;
    float inTangent;
    float outTangent;

    Keyframe() : time(0), value(0), inTangent(0), outTangent(0) {}
    Keyframe(float t, float v) : time(t), value(v), inTangent(0), outTangent(0) {}
};

struct AnimationCurve {
    std::vector<Keyframe> keys;

    // Evaluate curve at time t (0-1)
    float Evaluate(float t) const;
};

struct MinMaxCurve {
    CurveMode mode;
    float constant;
    float constantMin;
    float constantMax;
    AnimationCurve curve;
    AnimationCurve curveMin;
    AnimationCurve curveMax;
    float multiplier;

    MinMaxCurve() : mode(CurveMode::Constant), constant(0), constantMin(0),
                    constantMax(0), multiplier(1.0f) {}

    float Evaluate(float t, float randomValue) const;
};

// ============================================================================
// Gradient
// ============================================================================

struct GradientColorKey {
    Color color;
    float time;

    GradientColorKey() : time(0) {}
    GradientColorKey(const Color& c, float t) : color(c), time(t) {}
};

struct GradientAlphaKey {
    float alpha;
    float time;

    GradientAlphaKey() : alpha(1.0f), time(0) {}
    GradientAlphaKey(float a, float t) : alpha(a), time(t) {}
};

struct Gradient {
    std::vector<GradientColorKey> colorKeys;
    std::vector<GradientAlphaKey> alphaKeys;

    Color Evaluate(float t) const;
};

// ============================================================================
// Emission Module
// ============================================================================

struct Burst {
    float time;
    int minCount;
    int maxCount;
    int cycles;
    float repeatInterval;

    Burst() : time(0), minCount(0), maxCount(0), cycles(1), repeatInterval(0) {}
};

struct EmissionModule {
    bool enabled;
    MinMaxCurve rateOverTime;
    MinMaxCurve rateOverDistance;
    std::vector<Burst> bursts;

    EmissionModule() : enabled(true) {}
};

// ============================================================================
// Shape Module
// ============================================================================

enum class ParticleSystemShapeType {
    Sphere,
    Hemisphere,
    Cone,
    Box,
    Circle,
    Edge,
    Rectangle
};

enum class ParticleSystemShapeMultiModeValue {
    Random,
    Loop,
    PingPong
};

struct ShapeModule {
    bool enabled;
    ParticleSystemShapeType shapeType;
    float angle;
    float radius;
    float radiusThickness;
    float arc;
    Vector3 boxScale;
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    bool alignToDirection;
    float randomDirectionAmount;
    float sphericalDirectionAmount;
    ParticleSystemShapeMultiModeValue arcMode;

    ShapeModule() : enabled(true), shapeType(ParticleSystemShapeType::Cone),
                    angle(25.0f), radius(1.0f), radiusThickness(1.0f),
                    arc(360.0f), alignToDirection(false),
                    randomDirectionAmount(0), sphericalDirectionAmount(0),
                    arcMode(ParticleSystemShapeMultiModeValue::Random) {}
};

// ============================================================================
// Velocity Over Lifetime Module
// ============================================================================

enum class ParticleSystemSimulationSpace {
    Local,
    World,
    Custom
};

struct VelocityOverLifetimeModule {
    bool enabled;
    MinMaxCurve x;
    MinMaxCurve y;
    MinMaxCurve z;
    ParticleSystemSimulationSpace space;

    VelocityOverLifetimeModule() : enabled(false),
                                    space(ParticleSystemSimulationSpace::Local) {}
};

// ============================================================================
// Limit Velocity Over Lifetime Module
// ============================================================================

struct LimitVelocityOverLifetimeModule {
    bool enabled;
    MinMaxCurve limit;
    float dampen;
    bool separateAxes;
    MinMaxCurve limitX;
    MinMaxCurve limitY;
    MinMaxCurve limitZ;

    LimitVelocityOverLifetimeModule() : enabled(false), dampen(0.5f),
                                         separateAxes(false) {}
};

// ============================================================================
// Force Over Lifetime Module
// ============================================================================

struct ForceOverLifetimeModule {
    bool enabled;
    MinMaxCurve x;
    MinMaxCurve y;
    MinMaxCurve z;
    ParticleSystemSimulationSpace space;
    bool randomized;

    ForceOverLifetimeModule() : enabled(false),
                                 space(ParticleSystemSimulationSpace::Local),
                                 randomized(false) {}
};

// ============================================================================
// Color Over Lifetime Module
// ============================================================================

struct ColorOverLifetimeModule {
    bool enabled;
    Gradient gradient;

    ColorOverLifetimeModule() : enabled(false) {}
};

// ============================================================================
// Size Over Lifetime Module
// ============================================================================

struct SizeOverLifetimeModule {
    bool enabled;
    MinMaxCurve size;
    bool separateAxes;
    MinMaxCurve x;
    MinMaxCurve y;
    MinMaxCurve z;

    SizeOverLifetimeModule() : enabled(false), separateAxes(false) {}
};

// ============================================================================
// Rotation Over Lifetime Module
// ============================================================================

struct RotationOverLifetimeModule {
    bool enabled;
    MinMaxCurve x;
    MinMaxCurve y;
    MinMaxCurve z;
    bool separateAxes;

    RotationOverLifetimeModule() : enabled(false), separateAxes(false) {}
};

// ============================================================================
// Noise Module
// ============================================================================

struct NoiseModule {
    bool enabled;
    MinMaxCurve strength;
    float frequency;
    float scrollSpeed;
    bool damping;
    int octaves;
    float octaveMultiplier;
    float octaveScale;
    int quality;
    bool separateAxes;
    MinMaxCurve strengthX;
    MinMaxCurve strengthY;
    MinMaxCurve strengthZ;

    NoiseModule() : enabled(false), frequency(0.5f), scrollSpeed(0),
                    damping(true), octaves(1), octaveMultiplier(0.5f),
                    octaveScale(2.0f), quality(1), separateAxes(false) {}
};

// ============================================================================
// Collision Module
// ============================================================================

enum class ParticleSystemCollisionType {
    Planes,
    World
};

enum class ParticleSystemCollisionMode {
    Collision3D,
    Collision2D
};

struct CollisionModule {
    bool enabled;
    ParticleSystemCollisionType type;
    ParticleSystemCollisionMode mode;
    MinMaxCurve dampen;
    MinMaxCurve bounce;
    MinMaxCurve lifetimeLoss;
    float minKillSpeed;
    float maxKillSpeed;
    float radiusScale;
    bool collidesWithDynamic;
    int maxCollisionShapes;

    CollisionModule() : enabled(false), type(ParticleSystemCollisionType::World),
                        mode(ParticleSystemCollisionMode::Collision3D),
                        minKillSpeed(0), maxKillSpeed(10000), radiusScale(1.0f),
                        collidesWithDynamic(true), maxCollisionShapes(256) {}
};

// ============================================================================
// Texture Sheet Animation Module
// ============================================================================

enum class ParticleSystemAnimationType {
    WholeSheet,
    SingleRow
};

enum class ParticleSystemAnimationMode {
    Grid,
    Sprites
};

struct TextureSheetAnimationModule {
    bool enabled;
    int numTilesX;
    int numTilesY;
    ParticleSystemAnimationType animationType;
    ParticleSystemAnimationMode mode;
    MinMaxCurve frameOverTime;
    MinMaxCurve startFrame;
    int cycleCount;
    int rowIndex;

    TextureSheetAnimationModule() : enabled(false), numTilesX(1), numTilesY(1),
                                     animationType(ParticleSystemAnimationType::WholeSheet),
                                     mode(ParticleSystemAnimationMode::Grid),
                                     cycleCount(1), rowIndex(0) {}
};

// ============================================================================
// Renderer Module
// ============================================================================

enum class ParticleSystemRenderMode {
    Billboard,
    Stretch,
    HorizontalBillboard,
    VerticalBillboard,
    Mesh
};

enum class ParticleSystemSortMode {
    None,
    Distance,
    OldestInFront,
    YoungestInFront
};

struct RendererModule {
    ParticleSystemRenderMode renderMode;
    ParticleSystemSortMode sortMode;
    float minParticleSize;
    float maxParticleSize;
    std::string material;
    std::string texture;
    Vector3 pivot;
    bool flip;
    Vector3 velocityScale;
    float lengthScale;
    float normalDirection;
    int sortingOrder;

    RendererModule() : renderMode(ParticleSystemRenderMode::Billboard),
                       sortMode(ParticleSystemSortMode::None),
                       minParticleSize(0), maxParticleSize(0.5f),
                       flip(false), lengthScale(2.0f), normalDirection(1.0f),
                       sortingOrder(0) {}
};

// ============================================================================
// Sub-Emitters
// ============================================================================

enum class ParticleSystemSubEmitterType {
    Birth,
    Collision,
    Death
};

struct SubEmitter {
    ParticleSystemSubEmitterType type;
    std::string subEmitterName;

    SubEmitter() : type(ParticleSystemSubEmitterType::Birth) {}
};

// ============================================================================
// Main Particle System Data
// ============================================================================

struct MainModule {
    float duration;
    bool looping;
    bool prewarm;
    MinMaxCurve startDelay;
    MinMaxCurve startLifetime;
    MinMaxCurve startSpeed;
    MinMaxCurve startSize;
    bool startSize3D;
    MinMaxCurve startSizeX;
    MinMaxCurve startSizeY;
    MinMaxCurve startSizeZ;
    MinMaxCurve startRotation;
    bool startRotation3D;
    MinMaxCurve startRotationX;
    MinMaxCurve startRotationY;
    MinMaxCurve startRotationZ;
    Color startColor;
    MinMaxCurve gravityModifier;
    ParticleSystemSimulationSpace simulationSpace;
    float simulationSpeed;
    bool playOnAwake;
    int maxParticles;

    MainModule() : duration(5.0f), looping(true), prewarm(false),
                   startSize3D(false), startRotation3D(false),
                   simulationSpeed(1.0f), playOnAwake(true),
                   maxParticles(1000) {}
};

// ============================================================================
// Complete Particle System
// ============================================================================

struct ParticleSystemData {
    std::string name;
    std::string version;

    MainModule main;
    EmissionModule emission;
    ShapeModule shape;
    VelocityOverLifetimeModule velocityOverLifetime;
    LimitVelocityOverLifetimeModule limitVelocityOverLifetime;
    ForceOverLifetimeModule forceOverLifetime;
    ColorOverLifetimeModule colorOverLifetime;
    SizeOverLifetimeModule sizeOverLifetime;
    RotationOverLifetimeModule rotationOverLifetime;
    NoiseModule noise;
    CollisionModule collision;
    TextureSheetAnimationModule textureSheetAnimation;
    RendererModule renderer;

    std::vector<SubEmitter> subEmitters;

    ParticleSystemData() : version("1.0") {}
};

} // namespace GPUParticles
