using System;
using System.Collections.Generic;
using UnityEngine;

namespace GModParticleExporter
{
    // ============================================================================
    // Serializable Data Structures
    // These mirror the C++ structures and will be serialized to JSON
    // ============================================================================

    [Serializable]
    public class Vector3Data
    {
        public float x, y, z;

        public Vector3Data(Vector3 v)
        {
            x = v.x; y = v.y; z = v.z;
        }
    }

    [Serializable]
    public class ColorData
    {
        public float r, g, b, a;

        public ColorData(Color c)
        {
            r = c.r; g = c.g; b = c.b; a = c.a;
        }
    }

    [Serializable]
    public class KeyframeData
    {
        public float time;
        public float value;
        public float inTangent;
        public float outTangent;

        public KeyframeData(Keyframe kf)
        {
            time = kf.time;
            value = kf.value;
            inTangent = kf.inTangent;
            outTangent = kf.outTangent;
        }
    }

    [Serializable]
    public class AnimationCurveData
    {
        public List<KeyframeData> keys = new List<KeyframeData>();

        public AnimationCurveData(AnimationCurve curve)
        {
            if (curve != null)
            {
                foreach (var key in curve.keys)
                {
                    keys.Add(new KeyframeData(key));
                }
            }
        }
    }

    [Serializable]
    public class MinMaxCurveData
    {
        public string mode;
        public float constant;
        public float constantMin;
        public float constantMax;
        public AnimationCurveData curve;
        public AnimationCurveData curveMin;
        public AnimationCurveData curveMax;
        public float multiplier;

        public MinMaxCurveData(ParticleSystem.MinMaxCurve minMaxCurve)
        {
            mode = minMaxCurve.mode.ToString();
            constant = minMaxCurve.constant;
            constantMin = minMaxCurve.constantMin;
            constantMax = minMaxCurve.constantMax;
            multiplier = minMaxCurve.curveMultiplier;

            if (minMaxCurve.curve != null && minMaxCurve.curve.keys.Length > 0)
            {
                curve = new AnimationCurveData(minMaxCurve.curve);
            }

            if (minMaxCurve.curveMin != null && minMaxCurve.curveMin.keys.Length > 0)
            {
                curveMin = new AnimationCurveData(minMaxCurve.curveMin);
            }

            if (minMaxCurve.curveMax != null && minMaxCurve.curveMax.keys.Length > 0)
            {
                curveMax = new AnimationCurveData(minMaxCurve.curveMax);
            }
        }
    }

    [Serializable]
    public class GradientColorKeyData
    {
        public ColorData color;
        public float time;

        public GradientColorKeyData(GradientColorKey key)
        {
            color = new ColorData(key.color);
            time = key.time;
        }
    }

    [Serializable]
    public class GradientAlphaKeyData
    {
        public float alpha;
        public float time;

        public GradientAlphaKeyData(GradientAlphaKey key)
        {
            alpha = key.alpha;
            time = key.time;
        }
    }

    [Serializable]
    public class GradientData
    {
        public List<GradientColorKeyData> colorKeys = new List<GradientColorKeyData>();
        public List<GradientAlphaKeyData> alphaKeys = new List<GradientAlphaKeyData>();

        public GradientData(Gradient gradient)
        {
            if (gradient != null)
            {
                foreach (var key in gradient.colorKeys)
                {
                    colorKeys.Add(new GradientColorKeyData(key));
                }

                foreach (var key in gradient.alphaKeys)
                {
                    alphaKeys.Add(new GradientAlphaKeyData(key));
                }
            }
        }
    }

    [Serializable]
    public class BurstData
    {
        public float time;
        public int minCount;
        public int maxCount;
        public int cycles;
        public float repeatInterval;

        public BurstData(ParticleSystem.Burst burst)
        {
            time = burst.time;
            minCount = (int)burst.minCount;
            maxCount = (int)burst.maxCount;
            cycles = burst.cycleCount;
            repeatInterval = burst.repeatInterval;
        }
    }

    // ============================================================================
    // Module Data Classes
    // ============================================================================

    [Serializable]
    public class MainModuleData
    {
        public float duration;
        public bool looping;
        public bool prewarm;
        public MinMaxCurveData startDelay;
        public MinMaxCurveData startLifetime;
        public MinMaxCurveData startSpeed;
        public MinMaxCurveData startSize;
        public bool startSize3D;
        public MinMaxCurveData startSizeX;
        public MinMaxCurveData startSizeY;
        public MinMaxCurveData startSizeZ;
        public MinMaxCurveData startRotation;
        public bool startRotation3D;
        public MinMaxCurveData startRotationX;
        public MinMaxCurveData startRotationY;
        public MinMaxCurveData startRotationZ;
        public ColorData startColor;
        public MinMaxCurveData gravityModifier;
        public string simulationSpace;
        public float simulationSpeed;
        public bool playOnAwake;
        public int maxParticles;
    }

    [Serializable]
    public class EmissionModuleData
    {
        public bool enabled;
        public MinMaxCurveData rateOverTime;
        public MinMaxCurveData rateOverDistance;
        public List<BurstData> bursts = new List<BurstData>();
    }

    [Serializable]
    public class ShapeModuleData
    {
        public bool enabled;
        public string shapeType;
        public float angle;
        public float radius;
        public float radiusThickness;
        public float arc;
        public Vector3Data boxScale;
        public Vector3Data position;
        public Vector3Data rotation;
        public Vector3Data scale;
        public bool alignToDirection;
        public float randomDirectionAmount;
        public float sphericalDirectionAmount;
    }

    [Serializable]
    public class VelocityOverLifetimeModuleData
    {
        public bool enabled;
        public MinMaxCurveData x;
        public MinMaxCurveData y;
        public MinMaxCurveData z;
        public string space;
    }

    [Serializable]
    public class LimitVelocityOverLifetimeModuleData
    {
        public bool enabled;
        public MinMaxCurveData limit;
        public float dampen;
        public bool separateAxes;
        public MinMaxCurveData limitX;
        public MinMaxCurveData limitY;
        public MinMaxCurveData limitZ;
    }

    [Serializable]
    public class ForceOverLifetimeModuleData
    {
        public bool enabled;
        public MinMaxCurveData x;
        public MinMaxCurveData y;
        public MinMaxCurveData z;
        public string space;
        public bool randomized;
    }

    [Serializable]
    public class ColorOverLifetimeModuleData
    {
        public bool enabled;
        public GradientData gradient;
    }

    [Serializable]
    public class SizeOverLifetimeModuleData
    {
        public bool enabled;
        public MinMaxCurveData size;
        public bool separateAxes;
        public MinMaxCurveData x;
        public MinMaxCurveData y;
        public MinMaxCurveData z;
    }

    [Serializable]
    public class RotationOverLifetimeModuleData
    {
        public bool enabled;
        public MinMaxCurveData x;
        public MinMaxCurveData y;
        public MinMaxCurveData z;
        public bool separateAxes;
    }

    [Serializable]
    public class NoiseModuleData
    {
        public bool enabled;
        public MinMaxCurveData strength;
        public float frequency;
        public float scrollSpeed;
        public bool damping;
        public int octaves;
        public float octaveMultiplier;
        public float octaveScale;
        public int quality;
        public bool separateAxes;
        public MinMaxCurveData strengthX;
        public MinMaxCurveData strengthY;
        public MinMaxCurveData strengthZ;
    }

    [Serializable]
    public class CollisionModuleData
    {
        public bool enabled;
        public string type;
        public string mode;
        public MinMaxCurveData dampen;
        public MinMaxCurveData bounce;
        public MinMaxCurveData lifetimeLoss;
        public float minKillSpeed;
        public float maxKillSpeed;
        public float radiusScale;
        public bool collidesWithDynamic;
        public int maxCollisionShapes;
    }

    [Serializable]
    public class TextureSheetAnimationModuleData
    {
        public bool enabled;
        public int numTilesX;
        public int numTilesY;
        public string animationType;
        public string mode;
        public MinMaxCurveData frameOverTime;
        public MinMaxCurveData startFrame;
        public int cycleCount;
        public int rowIndex;
    }

    [Serializable]
    public class RendererModuleData
    {
        public string renderMode;
        public string sortMode;
        public float minParticleSize;
        public float maxParticleSize;
        public string material;
        public string texture;
        public Vector3Data pivot;
        public bool flip;
        public Vector3Data velocityScale;
        public float lengthScale;
        public float normalDirection;
        public int sortingOrder;
    }

    [Serializable]
    public class SubEmitterData
    {
        public string type;
        public string name;
    }

    [Serializable]
    public class MetadataData
    {
        public string name;
        public string version = "1.0";
        public string exportDate;
        public string exporter = "Unity GMod Exporter";
    }

    // ============================================================================
    // Root Particle System Data
    // ============================================================================

    [Serializable]
    public class ParticleSystemExportData
    {
        public MetadataData metadata;
        public MainModuleData system;
        public EmissionModuleData emission;
        public ShapeModuleData shape;
        public VelocityOverLifetimeModuleData velocityOverLifetime;
        public LimitVelocityOverLifetimeModuleData limitVelocityOverLifetime;
        public ForceOverLifetimeModuleData forceOverLifetime;
        public ColorOverLifetimeModuleData colorOverLifetime;
        public SizeOverLifetimeModuleData sizeOverLifetime;
        public RotationOverLifetimeModuleData rotationOverLifetime;
        public NoiseModuleData noise;
        public CollisionModuleData collision;
        public TextureSheetAnimationModuleData textureSheetAnimation;
        public RendererModuleData renderer;
        public List<SubEmitterData> subEmitters = new List<SubEmitterData>();
    }
}
