// -----------------------------------------------------------------------------
//  MathUtils.h - small numeric helpers shared by physics, AI and rendering.
// -----------------------------------------------------------------------------
#pragma once

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace vr {
namespace math {

constexpr float kPi      = 3.14159265358979323846f;
constexpr float kTwoPi   = 6.28318530717958647692f;
constexpr float kHalfPi  = 1.57079632679489661923f;
constexpr float kDeg2Rad = kPi / 180.0f;
constexpr float kRad2Deg = 180.0f / kPi;

inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float saturate(float v) { return clampf(v, 0.0f, 1.0f); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }

/// Maps v from [inMin,inMax] into [outMin,outMax] and clamps the result.
inline float remap(float v, float inMin, float inMax, float outMin, float outMax) {
    if (std::fabs(inMax - inMin) < 1e-6f) return outMin;
    return outMin + (outMax - outMin) * saturate((v - inMin) / (inMax - inMin));
}

/// Frame rate independent exponential smoothing.
/// `rate` is roughly "how much of the gap is closed per second".
inline float damp(float current, float target, float rate, float dt) {
    return target + (current - target) * std::exp(-rate * dt);
}

inline glm::vec3 damp(const glm::vec3& current, const glm::vec3& target, float rate, float dt) {
    const float k = std::exp(-rate * dt);
    return target + (current - target) * k;
}

inline float moveTowards(float current, float target, float maxDelta) {
    const float d = target - current;
    if (std::fabs(d) <= maxDelta) return target;
    return current + (d > 0.0f ? maxDelta : -maxDelta);
}

/// Wraps an angle into (-pi, pi].
inline float wrapAngle(float a) {
    while (a > kPi) a -= kTwoPi;
    while (a < -kPi) a += kTwoPi;
    return a;
}

inline float smoothstepf(float edge0, float edge1, float x) {
    const float t = saturate((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

inline float sign(float v) { return v < 0.0f ? -1.0f : (v > 0.0f ? 1.0f : 0.0f); }

inline float lengthXZ(const glm::vec3& v) { return std::sqrt(v.x * v.x + v.z * v.z); }

inline glm::vec3 flatten(const glm::vec3& v) { return glm::vec3(v.x, 0.0f, v.z); }

/// Yaw angle (radians) of a direction vector on the XZ plane. +Z is yaw 0.
inline float yawFromDir(const glm::vec3& d) { return std::atan2(d.x, d.z); }

inline glm::vec3 dirFromYaw(float yaw) { return glm::vec3(std::sin(yaw), 0.0f, std::cos(yaw)); }

/// km/h <-> m/s helpers (the HUD works in km/h, the physics in m/s).
inline float msToKmh(float ms) { return ms * 3.6f; }
inline float kmhToMs(float kmh) { return kmh / 3.6f; }

/// Formats seconds as m:ss.mmm into `out` (needs >= 16 chars).
void formatLapTime(float seconds, char* out, int outSize);

} // namespace math
} // namespace vr
