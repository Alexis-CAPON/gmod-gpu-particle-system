#include "particle_data.h"
#include <algorithm>
#include <cmath>

namespace GPUParticles {

// ============================================================================
// AnimationCurve Implementation
// ============================================================================

float AnimationCurve::Evaluate(float t) const {
    if (keys.empty()) return 0.0f;
    if (keys.size() == 1) return keys[0].value;

    // Clamp t to valid range
    t = std::max(0.0f, std::min(1.0f, t));

    // Find the two keyframes to interpolate between
    if (t <= keys[0].time) return keys[0].value;
    if (t >= keys[keys.size() - 1].time) return keys[keys.size() - 1].value;

    for (size_t i = 0; i < keys.size() - 1; ++i) {
        if (t >= keys[i].time && t <= keys[i + 1].time) {
            const Keyframe& k0 = keys[i];
            const Keyframe& k1 = keys[i + 1];

            float dt = k1.time - k0.time;
            if (dt < 0.0001f) return k0.value;

            float normalizedT = (t - k0.time) / dt;

            // Hermite interpolation using tangents
            float t2 = normalizedT * normalizedT;
            float t3 = t2 * normalizedT;

            float h00 = 2 * t3 - 3 * t2 + 1;
            float h10 = t3 - 2 * t2 + normalizedT;
            float h01 = -2 * t3 + 3 * t2;
            float h11 = t3 - t2;

            float m0 = k0.outTangent * dt;
            float m1 = k1.inTangent * dt;

            return h00 * k0.value + h10 * m0 + h01 * k1.value + h11 * m1;
        }
    }

    return keys[keys.size() - 1].value;
}

// ============================================================================
// MinMaxCurve Implementation
// ============================================================================

float MinMaxCurve::Evaluate(float t, float randomValue) const {
    float value = 0.0f;

    switch (mode) {
        case CurveMode::Constant:
            value = constant;
            break;

        case CurveMode::Curve:
            value = curve.Evaluate(t);
            break;

        case CurveMode::TwoConstants:
            // Not typically used, treat as constant
            value = constant;
            break;

        case CurveMode::TwoCurves:
            // Blend between two curves based on random value
            value = curveMin.Evaluate(t) * (1.0f - randomValue) +
                   curveMax.Evaluate(t) * randomValue;
            break;

        case CurveMode::RandomBetweenTwoConstants:
            // Interpolate between min and max based on random value
            value = constantMin * (1.0f - randomValue) +
                   constantMax * randomValue;
            break;

        case CurveMode::RandomBetweenTwoCurves:
            // Blend between two curves based on random value
            value = curveMin.Evaluate(t) * (1.0f - randomValue) +
                   curveMax.Evaluate(t) * randomValue;
            break;
    }

    return value * multiplier;
}

// ============================================================================
// Gradient Implementation
// ============================================================================

Color Gradient::Evaluate(float t) const {
    if (colorKeys.empty()) return Color(1, 1, 1, 1);

    t = std::max(0.0f, std::min(1.0f, t));

    // Evaluate color
    Color color;
    if (colorKeys.size() == 1) {
        color = colorKeys[0].color;
    } else {
        if (t <= colorKeys[0].time) {
            color = colorKeys[0].color;
        } else if (t >= colorKeys[colorKeys.size() - 1].time) {
            color = colorKeys[colorKeys.size() - 1].color;
        } else {
            // Find the two color keys to interpolate between
            for (size_t i = 0; i < colorKeys.size() - 1; ++i) {
                if (t >= colorKeys[i].time && t <= colorKeys[i + 1].time) {
                    float normalizedT = (t - colorKeys[i].time) /
                                       (colorKeys[i + 1].time - colorKeys[i].time);

                    color.r = colorKeys[i].color.r * (1.0f - normalizedT) +
                             colorKeys[i + 1].color.r * normalizedT;
                    color.g = colorKeys[i].color.g * (1.0f - normalizedT) +
                             colorKeys[i + 1].color.g * normalizedT;
                    color.b = colorKeys[i].color.b * (1.0f - normalizedT) +
                             colorKeys[i + 1].color.b * normalizedT;
                    break;
                }
            }
        }
    }

    // Evaluate alpha
    if (alphaKeys.empty()) {
        color.a = 1.0f;
    } else if (alphaKeys.size() == 1) {
        color.a = alphaKeys[0].alpha;
    } else {
        if (t <= alphaKeys[0].time) {
            color.a = alphaKeys[0].alpha;
        } else if (t >= alphaKeys[alphaKeys.size() - 1].time) {
            color.a = alphaKeys[alphaKeys.size() - 1].alpha;
        } else {
            // Find the two alpha keys to interpolate between
            for (size_t i = 0; i < alphaKeys.size() - 1; ++i) {
                if (t >= alphaKeys[i].time && t <= alphaKeys[i + 1].time) {
                    float normalizedT = (t - alphaKeys[i].time) /
                                       (alphaKeys[i + 1].time - alphaKeys[i].time);

                    color.a = alphaKeys[i].alpha * (1.0f - normalizedT) +
                             alphaKeys[i + 1].alpha * normalizedT;
                    break;
                }
            }
        }
    }

    return color;
}

} // namespace GPUParticles
