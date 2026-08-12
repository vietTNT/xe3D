#include "Track.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <glm/gtc/matrix_transform.hpp>

#include "../Graphics/Renderer.h"
#include "../Managers/ResourceManager.h"
#include "../Utilities/MathUtils.h"
#include "../Utilities/Random.h"
#include "CircuitData.h"

namespace vr {
namespace {

constexpr float kSampleSpacing = 3.0f;

/// Gentle, perfectly periodic elevation profile so the loop always closes.
float elevationAt(float t) {   // t = distance / length, in [0,1)
    const float a = math::kTwoPi * t;
    return 5.2f * std::sin(a + 0.40f) + 2.8f * std::sin(2.0f * a + 2.10f) +
           1.5f * std::sin(3.0f * a + 4.05f) - 1.0f;
}

glm::vec2 catmullRom(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2,
                     const glm::vec2& p3, float t) {
    const float t2 = t * t;
    const float t3 = t2 * t;
    return 0.5f * ((2.0f * p1) + (-p0 + p2) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

glm::vec3 rotateAroundAxis(const glm::vec3& v, const glm::vec3& axis, float angle) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    return v * c + glm::cross(axis, v) * s + axis * glm::dot(axis, v) * (1.0f - c);
}

} // namespace

// ============================================================== sample generation
void Track::generateSamples() {
    const int n = circuit::kControlPointCount;
    std::vector<glm::vec2> ctrl((size_t)n);
    for (int i = 0; i < n; ++i) {
        ctrl[(size_t)i] = glm::vec2(circuit::kControlPoints[i * 2 + 0],
                                    circuit::kControlPoints[i * 2 + 1]) *
                          circuit::kCircuitScale;
    }

    // 1. Dense Catmull-Rom evaluation of the closed control polygon.
    std::vector<glm::vec2> dense;
    dense.reserve((size_t)n * 24);
    const int stepsPerSegment = 24;
    for (int i = 0; i < n; ++i) {
        const glm::vec2& p0 = ctrl[(size_t)((i - 1 + n) % n)];
        const glm::vec2& p1 = ctrl[(size_t)i];
        const glm::vec2& p2 = ctrl[(size_t)((i + 1) % n)];
        const glm::vec2& p3 = ctrl[(size_t)((i + 2) % n)];
        for (int s = 0; s < stepsPerSegment; ++s) {
            dense.push_back(catmullRom(p0, p1, p2, p3, (float)s / (float)stepsPerSegment));
        }
    }

    // 2. Arc length parameterisation.
    std::vector<float> cum(dense.size() + 1, 0.0f);
    for (size_t i = 0; i < dense.size(); ++i) {
        const glm::vec2& a = dense[i];
        const glm::vec2& b = dense[(i + 1) % dense.size()];
        cum[i + 1] = cum[i] + glm::length(b - a);
    }
    m_length  = cum.back();
    m_spacing = kSampleSpacing;

    const int sampleCount = std::max(64, (int)std::round(m_length / m_spacing));
    m_spacing = m_length / (float)sampleCount;
    m_samples.assign((size_t)sampleCount, TrackSample{});

    size_t cursor = 0;
    for (int i = 0; i < sampleCount; ++i) {
        const float target = (float)i * m_spacing;
        while (cursor + 1 < cum.size() - 1 && cum[cursor + 1] < target) ++cursor;
        const float segLen = std::max(cum[cursor + 1] - cum[cursor], 1e-5f);
        const float t      = math::saturate((target - cum[cursor]) / segLen);
        const glm::vec2 a  = dense[cursor];
        const glm::vec2 b  = dense[(cursor + 1) % dense.size()];
        const glm::vec2 p  = a + (b - a) * t;

        TrackSample& s = m_samples[(size_t)i];
        s.distance = target;
        s.position = glm::vec3(p.x, elevationAt(target / m_length), p.y);
    }

    // 3. Frames, curvature and banking.
    const int count = (int)m_samples.size();
    for (int i = 0; i < count; ++i) {
        TrackSample&       s    = m_samples[(size_t)i];
        const glm::vec3&   prev = m_samples[(size_t)((i - 1 + count) % count)].position;
        const glm::vec3&   next = m_samples[(size_t)((i + 1) % count)].position;
        s.forward = glm::normalize(next - prev);
        s.right   = glm::normalize(glm::cross(s.forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        s.up      = glm::normalize(glm::cross(s.right, s.forward));
    }
    for (int i = 0; i < count; ++i) {
        TrackSample&       s  = m_samples[(size_t)i];
        const glm::vec3&   f0 = m_samples[(size_t)((i - 1 + count) % count)].forward;
        const glm::vec3&   f1 = m_samples[(size_t)((i + 1) % count)].forward;
        const float turn = -glm::dot(glm::cross(f0, f1), glm::vec3(0.0f, 1.0f, 0.0f));
        s.curvature = turn / (2.0f * m_spacing);
    }
    // Light smoothing so curbs and the racing line do not react to noise.
    for (int pass = 0; pass < 3; ++pass) {
        std::vector<float> tmp((size_t)count);
        for (int i = 0; i < count; ++i) {
            const float a = m_samples[(size_t)((i - 1 + count) % count)].curvature;
            const float b = m_samples[(size_t)i].curvature;
            const float c = m_samples[(size_t)((i + 1) % count)].curvature;
            tmp[(size_t)i] = (a + 2.0f * b + c) * 0.25f;
        }
        for (int i = 0; i < count; ++i) m_samples[(size_t)i].curvature = tmp[(size_t)i];
    }
    for (int i = 0; i < count; ++i) {
        TrackSample& s = m_samples[(size_t)i];
        s.bank  = math::clampf(s.curvature * 170.0f, -0.075f, 0.075f);
        s.right = glm::normalize(rotateAroundAxis(s.right, s.forward, s.bank));
        s.up    = glm::normalize(glm::cross(s.right, s.forward));
    }

    m_centroid = glm::vec3(0.0f);
    m_minElevation = 1e9f;
    m_maxElevation = -1e9f;
    for (const TrackSample& s : m_samples) {
        m_centroid += s.position;
        m_minElevation = std::min(m_minElevation, s.position.y);
        m_maxElevation = std::max(m_maxElevation, s.position.y);
    }
    m_centroid /= (float)count;
    // The surrounding countryside sits below the lowest dip of the circuit. If it
    // were left at y = 0 the flat ground would rise above the camera in every
    // valley and blot out the sky.
    m_groundLevel = m_minElevation - 2.5f;
    std::printf("[Track] elevation %.1f m to %.1f m, terrain datum %.1f m\n", m_minElevation,
                m_maxElevation, m_groundLevel);

    std::printf("[Track] %d samples, lap length %.1f m, spacing %.2f m\n", count, m_length,
                m_spacing);
}

// =================================================================== racing line
void Track::computeRacingLine() {
    const int   count     = (int)m_samples.size();
    const float maxOffset = kHalfWidth - 2.4f;

    std::vector<float> offset((size_t)count, 0.0f);
    for (int i = 0; i < count; ++i) {
        const float k = m_samples[(size_t)i].curvature;
        // Aim at the apex: positive curvature bends toward +right, so hug +right.
        offset[(size_t)i] = math::clampf(k * 900.0f, -1.0f, 1.0f) * maxOffset;
    }
    // Heavy smoothing turns the apex targets into a real out-in-out trajectory.
    for (int pass = 0; pass < 90; ++pass) {
        std::vector<float> tmp((size_t)count);
        for (int i = 0; i < count; ++i) {
            const float a = offset[(size_t)((i - 2 + count) % count)];
            const float b = offset[(size_t)((i - 1 + count) % count)];
            const float c = offset[(size_t)i];
            const float d = offset[(size_t)((i + 1) % count)];
            const float e = offset[(size_t)((i + 2) % count)];
            tmp[(size_t)i] = (a + 2.0f * b + 3.0f * c + 2.0f * d + e) / 9.0f;
        }
        offset.swap(tmp);
    }
    for (int i = 0; i < count; ++i) {
        m_samples[(size_t)i].lineOffset = math::clampf(offset[(size_t)i], -maxOffset, maxOffset);
    }

    // Curvature of the racing line itself -> reference speed.
    for (int i = 0; i < count; ++i) {
        const int   i0 = (i - 4 + count) % count;
        const int   i1 = (i + 4) % count;
        const glm::vec3 p0 = surfacePoint(m_samples[(size_t)i0].distance,
                                          m_samples[(size_t)i0].lineOffset);
        const glm::vec3 p1 = surfacePoint(m_samples[(size_t)i].distance,
                                          m_samples[(size_t)i].lineOffset);
        const glm::vec3 p2 = surfacePoint(m_samples[(size_t)i1].distance,
                                          m_samples[(size_t)i1].lineOffset);
        const glm::vec3 a = p1 - p0;
        const glm::vec3 b = p2 - p1;
        const float la = glm::length(a), lb = glm::length(b);
        float k = 0.0f;
        if (la > 1e-3f && lb > 1e-3f) {
            const float cross = std::fabs(a.x * b.z - a.z * b.x);
            const float chord = glm::length(p2 - p0);
            if (chord > 1e-3f) k = 2.0f * cross / (la * lb * chord);
        }
        const float latAccel = 15.5f;   // m/s^2 the arcade model can sustain
        const float vmax     = 92.0f;   // ~330 km/h ceiling
        m_samples[(size_t)i].lineSpeed =
            math::clampf(k > 1e-5f ? std::sqrt(latAccel / k) : vmax, 11.0f, vmax);
    }
    // Backward pass: slow down early enough for the next corner.
    const float brakeDecel = 13.0f;
    for (int pass = 0; pass < 3; ++pass) {
        for (int i = count - 1; i >= 0; --i) {
            const int next = (i + 1) % count;
            const float v  = std::sqrt(m_samples[(size_t)next].lineSpeed *
                                           m_samples[(size_t)next].lineSpeed +
                                       2.0f * brakeDecel * m_spacing);
            m_samples[(size_t)i].lineSpeed = std::min(m_samples[(size_t)i].lineSpeed, v);
        }
    }
}

void Track::debugRacingLine() const {
    float mn = 1e9f, mx = -1e9f, sum = 0.0f, kmax = 0.0f;
    for (const TrackSample& s : m_samples) {
        mn = std::min(mn, s.lineSpeed);
        mx = std::max(mx, s.lineSpeed);
        sum += s.lineSpeed;
        kmax = std::max(kmax, std::fabs(s.curvature));
    }
    std::printf("[Track] racing line speed  min %.1f  avg %.1f  max %.1f km/h | tightest radius %.0f m\n",
                mn * 3.6f, sum / (float)m_samples.size() * 3.6f, mx * 3.6f,
                kmax > 1e-6f ? 1.0f / kmax : 0.0f);
}

void Track::findMainStraight() {
    const int count = (int)m_samples.size();
    int bestStart = 0, bestLen = 0, curStart = 0, curLen = 0;
    for (int i = 0; i < count * 2; ++i) {
        const int idx = i % count;
        if (std::fabs(m_samples[(size_t)idx].curvature) < 0.0016f) {
            if (curLen == 0) curStart = idx;
            ++curLen;
            if (curLen > bestLen) {
                bestLen   = curLen;
                bestStart = curStart;
            }
        } else {
            curLen = 0;
        }
        if (i == count - 1 && bestLen >= count) break;
    }
    bestLen = std::min(bestLen, count);

    m_straightBegin = m_samples[(size_t)bestStart].distance;
    m_straightEnd   = m_straightBegin + (float)bestLen * m_spacing;
    // Start line sits three quarters down the straight, leaving room for the grid.
    m_startLine = std::fmod(m_straightBegin + (float)bestLen * m_spacing * 0.80f, m_length);

    const TrackSample s = sampleAt(m_startLine);
    m_insideSign = glm::dot(m_centroid - s.position, s.right) > 0.0f ? 1.0f : -1.0f;

    std::printf("[Track] main straight %.0f m, start/finish at %.0f m, infield on %s\n",
                (float)bestLen * m_spacing, m_startLine, m_insideSign > 0 ? "+right" : "-right");
}

// ======================================================================= queries
int Track::indexAt(float distance) const {
    if (m_samples.empty()) return 0;
    float d = std::fmod(distance, m_length);
    if (d < 0.0f) d += m_length;
    int idx = (int)(d / m_spacing);
    return std::min(idx, (int)m_samples.size() - 1);
}

TrackSample Track::sampleAt(float distance) const {
    if (m_samples.empty()) return TrackSample{};
    float d = std::fmod(distance, m_length);
    if (d < 0.0f) d += m_length;

    const int count = (int)m_samples.size();
    const int i0    = std::min((int)(d / m_spacing), count - 1);
    const int i1    = (i0 + 1) % count;
    const float t   = math::saturate((d - m_samples[(size_t)i0].distance) / m_spacing);

    const TrackSample& a = m_samples[(size_t)i0];
    const TrackSample& b = m_samples[(size_t)i1];

    TrackSample out;
    out.position   = glm::mix(a.position, b.position, t);
    out.forward    = glm::normalize(glm::mix(a.forward, b.forward, t));
    out.right      = glm::normalize(glm::mix(a.right, b.right, t));
    out.up         = glm::normalize(glm::cross(out.right, out.forward));
    out.distance   = d;
    out.curvature  = math::lerpf(a.curvature, b.curvature, t);
    out.bank       = math::lerpf(a.bank, b.bank, t);
    out.lineOffset = math::lerpf(a.lineOffset, b.lineOffset, t);
    out.lineSpeed  = math::lerpf(a.lineSpeed, b.lineSpeed, t);
    return out;
}

glm::vec3 Track::surfacePoint(float distance, float lateral) const {
    const TrackSample s = sampleAt(distance);
    return s.position + s.right * lateral;
}

glm::vec3 Track::racingLinePoint(float distance) const {
    const TrackSample s = sampleAt(distance);
    return s.position + s.right * s.lineOffset;
}

float Track::racingLineSpeed(float distance) const { return sampleAt(distance).lineSpeed; }

TrackQuery Track::locate(const glm::vec3& position, int hint) const {
    TrackQuery q;
    if (m_samples.empty()) return q;
    const int count = (int)m_samples.size();

    int best = 0;
    if (hint >= 0) {
        // Local search around the previous result (cars move a few metres/frame).
        float bestD2 = 1e18f;
        const int window = 90;
        for (int o = -window; o <= window; ++o) {
            const int i = ((hint + o) % count + count) % count;
            const glm::vec3 d = position - m_samples[(size_t)i].position;
            const float d2 = d.x * d.x + d.z * d.z;
            if (d2 < bestD2) {
                bestD2 = d2;
                best   = i;
            }
        }
    } else {
        float bestD2 = 1e18f;
        for (int i = 0; i < count; i += 8) {
            const glm::vec3 d = position - m_samples[(size_t)i].position;
            const float d2 = d.x * d.x + d.z * d.z;
            if (d2 < bestD2) {
                bestD2 = d2;
                best   = i;
            }
        }
        for (int o = -10; o <= 10; ++o) {
            const int i = ((best + o) % count + count) % count;
            const glm::vec3 d = position - m_samples[(size_t)i].position;
            const float d2 = d.x * d.x + d.z * d.z;
            if (d2 < bestD2) {
                bestD2 = d2;
                best   = i;
            }
        }
    }

    const TrackSample& s = m_samples[(size_t)best];
    const glm::vec3    delta = position - s.position;
    const float along = glm::dot(delta, s.forward);

    q.index    = best;
    q.distance = std::fmod(s.distance + along + m_length, m_length);

    // Re-project onto the interpolated frame at that arc length. Snapping to the
    // nearest 3 m sample would step the road height, which reads as the car
    // twitching up and down; interpolating keeps the surface perfectly smooth.
    const TrackSample interp = sampleAt(q.distance);
    q.lateral  = glm::dot(position - interp.position, interp.right);
    q.forward  = interp.forward;
    q.normal   = interp.up;
    q.onAsphalt = std::fabs(q.lateral) <= kHalfWidth + kCurbWidth;

    // Height: banked asphalt near the centre line, flattening out on the grass.
    const float bankY = interp.position.y + std::sin(interp.bank) * q.lateral;
    const float edge  = kWallOffset;
    if (std::fabs(q.lateral) <= edge) {
        q.surfaceY = bankY;
    } else {
        const float t = math::saturate((std::fabs(q.lateral) - edge) / 45.0f);
        // Use the track's ground level (m_groundLevel) as the fall-back height
        // so the collision surface matches the distant ground/grass geometry.
        q.surfaceY = math::lerpf(bankY, m_groundLevel, t);
    }
    return q;
}

float Track::checkpointDistance(int index) const {
    const int i = ((index % kCheckpoints) + kCheckpoints) % kCheckpoints;
    return std::fmod(m_startLine + m_length * (float)i / (float)kCheckpoints, m_length);
}

glm::vec3 Track::gridPosition(int slot) const {
    // Two staggered columns behind the start line, F1 style.
    const int   row  = slot / 2;
    const int   col  = slot % 2;
    const float back = 12.0f + (float)row * 9.0f + (float)col * 4.5f;
    const float lat  = (col == 0 ? -1.0f : 1.0f) * 3.1f;
    const float d    = std::fmod(m_startLine - back + m_length * 2.0f, m_length);
    const TrackSample s = sampleAt(d);
    return s.position + s.right * lat + s.up * 0.05f;
}

float Track::gridYaw(int slot) const {
    const int   row  = slot / 2;
    const int   col  = slot % 2;
    const float back = 12.0f + (float)row * 9.0f + (float)col * 4.5f;
    const float d    = std::fmod(m_startLine - back + m_length * 2.0f, m_length);
    const glm::vec3 f = sampleAt(d).forward;
    return std::atan2(f.x, f.z);
}

void Track::buildMinimap() {
    m_minimap.clear();
    glm::vec2 mn(1e18f), mx(-1e18f);
    for (size_t i = 0; i < m_samples.size(); i += 4) {
        const glm::vec2 p(m_samples[i].position.x, m_samples[i].position.z);
        m_minimap.push_back(p);
        mn = glm::min(mn, p);
        mx = glm::max(mx, p);
    }
    m_minimapMin = mn;
    m_minimapMax = mx;
}

TrackPart& Track::newPart(const Material& material) {
    m_parts.emplace_back();
    m_parts.back().material = material;
    return m_parts.back();
}

void Track::collect(Renderer& renderer) const {
    const glm::mat4 identity(1.0f);
    for (const TrackPart& part : m_parts) {
        renderer.submit(part.mesh, identity, part.material);
    }
}


// ================================================================== geometry build
namespace {

struct ProfilePoint {
    float     lateral;
    float     height;
    glm::vec3 color;
    float     u;
};

} // namespace

void Track::buildAsphalt(const ResourceManager& res) {
    Material mat;
    mat.albedo         = glm::vec3(1.0f);
    mat.roughness      = 0.86f;
    mat.metallic       = 0.0f;
    mat.reflectivity   = 0.35f;
    mat.texture        = &res.asphalt();
    mat.useVertexColor = true;
    mat.castShadow     = false;   // the road is flat, shadowing itself costs fill rate

    TrackPart&   part = newPart(mat);
    MeshBuilder  mb;
    const int    count = (int)m_samples.size();
    const int    lanes = 8;
    mb.reserve((size_t)count * (size_t)(lanes + 1), (size_t)count * (size_t)lanes * 6);

    for (int i = 0; i < count; ++i) {
        const TrackSample& s = m_samples[(size_t)i];
        for (int j = 0; j <= lanes; ++j) {
            const float lat = math::lerpf(-kHalfWidth, kHalfWidth, (float)j / (float)lanes);
            // Rubbered-in racing line: darker asphalt where the cars run.
            const float dLine = std::fabs(lat - s.lineOffset);
            const float rubber = 1.0f - 0.30f * math::smoothstepf(3.4f, 0.6f, dLine);
            const float edge   = 1.0f - 0.10f * math::smoothstepf(kHalfWidth - 2.5f,
                                                                  kHalfWidth, std::fabs(lat));
            Vertex v;
            v.position = s.position + s.right * lat;
            v.normal   = s.up;
            v.uv       = glm::vec2(lat * 0.075f, s.distance * 0.075f);
            v.color    = glm::vec3(rubber * edge);
            mb.addVertex(v);
        }
    }
    const int stride = lanes + 1;
    for (int i = 0; i < count; ++i) {
        const int i1 = (i + 1) % count;
        for (int j = 0; j < lanes; ++j) {
            const std::uint32_t a = (std::uint32_t)(i * stride + j);
            const std::uint32_t b = (std::uint32_t)(i * stride + j + 1);
            const std::uint32_t c = (std::uint32_t)(i1 * stride + j + 1);
            const std::uint32_t d = (std::uint32_t)(i1 * stride + j);
            mb.addTriangle(a, b, c);
            mb.addTriangle(a, c, d);
        }
    }
    mb.build(part.mesh);
}

void Track::buildMarkings() {
    Material mat;
    mat.albedo         = glm::vec3(1.0f);
    mat.roughness      = 0.45f;
    mat.metallic       = 0.0f;
    mat.useVertexColor = true;
    mat.castShadow     = false;

    TrackPart&  part = newPart(mat);
    MeshBuilder mb;
    const int   count = (int)m_samples.size();

    auto stripe = [&](float latInner, float latOuter, float lift, const glm::vec3& color) {
        for (int i = 0; i < count; ++i) {
            const TrackSample& s0 = m_samples[(size_t)i];
            const TrackSample& s1 = m_samples[(size_t)((i + 1) % count)];
            const glm::vec3 a = s0.position + s0.right * latInner + s0.up * lift;
            const glm::vec3 b = s0.position + s0.right * latOuter + s0.up * lift;
            const glm::vec3 c = s1.position + s1.right * latOuter + s1.up * lift;
            const glm::vec3 d = s1.position + s1.right * latInner + s1.up * lift;
            mb.addQuadN(a, b, c, d, s0.up, s0.up, s1.up, s1.up, glm::vec2(0.0f),
                        glm::vec2(1.0f, 0.0f), glm::vec2(1.0f), glm::vec2(0.0f, 1.0f), color);
        }
    };

    const glm::vec3 white(0.94f);
    stripe(kHalfWidth - 0.32f, kHalfWidth - 0.06f, 0.016f, white);
    stripe(-kHalfWidth + 0.06f, -kHalfWidth + 0.32f, 0.016f, white);
    mb.build(part.mesh);
}

void Track::buildCurbs() {
    Material mat;
    mat.albedo         = glm::vec3(1.0f);
    mat.roughness      = 0.62f;
    mat.metallic       = 0.0f;
    mat.useVertexColor = true;

    TrackPart&  part  = newPart(mat);
    MeshBuilder mb;
    const int   count = (int)m_samples.size();

    // Corner mask: curbs appear where the circuit actually turns, with a margin.
    std::vector<float> mask((size_t)count, 0.0f);
    for (int i = 0; i < count; ++i) {
        mask[(size_t)i] = math::smoothstepf(0.0022f, 0.0075f,
                                            std::fabs(m_samples[(size_t)i].curvature));
    }
    for (int pass = 0; pass < 8; ++pass) {
        std::vector<float> tmp((size_t)count);
        for (int i = 0; i < count; ++i) {
            const float a = mask[(size_t)((i - 1 + count) % count)];
            const float b = mask[(size_t)i];
            const float c = mask[(size_t)((i + 1) % count)];
            tmp[(size_t)i] = std::max(b, std::max(a, c) * 0.97f);
        }
        mask.swap(tmp);
    }

    for (int side = -1; side <= 1; side += 2) {
        for (int i = 0; i < count; ++i) {
            const int i1 = (i + 1) % count;
            const float m0 = mask[(size_t)i];
            const float m1 = mask[(size_t)i1];
            if (m0 < 0.02f && m1 < 0.02f) continue;

            const TrackSample& s0 = m_samples[(size_t)i];
            const TrackSample& s1 = m_samples[(size_t)i1];

            const bool  red = (((int)(s0.distance / 2.6f)) % 2) == 0;
            glm::vec3   col = red ? glm::vec3(0.74f, 0.10f, 0.11f) : glm::vec3(0.90f, 0.90f, 0.88f);
            // Scuffed rubber marks on the curb faces.
            col *= 0.86f + 0.14f * std::fabs(std::sin(s0.distance * 0.7f));

            const float h0 = 0.055f + 0.075f * m0;
            const float h1 = 0.055f + 0.075f * m1;
            const float innerLat = (float)side * kHalfWidth;
            const float outerLat = (float)side * (kHalfWidth + kCurbWidth);

            const glm::vec3 i0p = s0.position + s0.right * innerLat + s0.up * 0.02f;
            const glm::vec3 i1p = s1.position + s1.right * innerLat + s1.up * 0.02f;
            const glm::vec3 o0p = s0.position + s0.right * outerLat + s0.up * h0;
            const glm::vec3 o1p = s1.position + s1.right * outerLat + s1.up * h1;

            if (side > 0) {
                mb.addQuadN(i0p, o0p, o1p, i1p, s0.up, s0.up, s1.up, s1.up, glm::vec2(0.0f),
                            glm::vec2(1.0f, 0.0f), glm::vec2(1.0f), glm::vec2(0.0f, 1.0f), col);
            } else {
                mb.addQuadN(o0p, i0p, i1p, o1p, s0.up, s0.up, s1.up, s1.up, glm::vec2(0.0f),
                            glm::vec2(1.0f, 0.0f), glm::vec2(1.0f), glm::vec2(0.0f, 1.0f), col);
            }

            // Outer vertical face so the curb reads as a raised concrete block.
            const glm::vec3 b0 = s0.position + s0.right * outerLat;
            const glm::vec3 b1 = s1.position + s1.right * outerLat;
            const glm::vec3 nrm = s0.right * (float)side;
            if (side > 0) {
                mb.addQuadN(o0p, b0, b1, o1p, nrm, nrm, nrm, nrm, glm::vec2(0.0f),
                            glm::vec2(1.0f, 0.0f), glm::vec2(1.0f), glm::vec2(0.0f, 1.0f),
                            col * 0.8f);
            } else {
                mb.addQuadN(b0, o0p, o1p, b1, nrm, nrm, nrm, nrm, glm::vec2(0.0f),
                            glm::vec2(1.0f, 0.0f), glm::vec2(1.0f), glm::vec2(0.0f, 1.0f),
                            col * 0.8f);
            }
        }
    }
    mb.build(part.mesh);
}

// Continuous red/white curbs on both sides for the whole circuit.
void Track::buildContinuousCurbs(const ResourceManager& res) {
    Material mat;
    mat.albedo         = glm::vec3(1.0f);
    mat.roughness      = 0.62f;
    mat.metallic       = 0.0f;
    mat.useVertexColor = true;

    TrackPart&  part  = newPart(mat);
    MeshBuilder mb;
    const int   count = (int)m_samples.size();

    for (int side = -1; side <= 1; side += 2) {
        for (int i = 0; i < count; ++i) {
            const int i1 = (i + 1) % count;
            const TrackSample& s0 = m_samples[(size_t)i];
            const TrackSample& s1 = m_samples[(size_t)i1];

            const float innerLat = (float)side * kHalfWidth;
            const float outerLat = (float)side * (kHalfWidth + kCurbWidth);
            const glm::vec3 i0p = s0.position + s0.right * innerLat + s0.up * 0.02f;
            const glm::vec3 i1p = s1.position + s1.right * innerLat + s1.up * 0.02f;
            const glm::vec3 o0p = s0.position + s0.right * outerLat + s0.up * 0.06f;
            const glm::vec3 o1p = s1.position + s1.right * outerLat + s1.up * 0.06f;

            // Alternate red/white along the track length using distance bands.
            const bool red = (((int)(s0.distance / 2.6f)) % 2) == 0;
            const glm::vec3 colA = red ? glm::vec3(0.74f, 0.10f, 0.11f) : glm::vec3(0.90f, 0.90f, 0.88f);
            const glm::vec3 colB = red ? glm::vec3(0.90f, 0.90f, 0.88f) : glm::vec3(0.74f, 0.10f, 0.11f);

            // Top face (painted curb)
            if (side > 0) {
                mb.addQuad(i0p, o0p, o1p, i1p, colA);
            } else {
                mb.addQuad(o0p, i0p, i1p, o1p, colA);
            }
            // Outer vertical face
            const glm::vec3 b0 = s0.position + s0.right * outerLat;
            const glm::vec3 b1 = s1.position + s1.right * outerLat;
            const glm::vec3 nrm = s0.right * (float)side;
            if (side > 0) {
                mb.addQuadN(o0p, b0, b1, o1p, nrm, nrm, nrm, nrm, glm::vec2(0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f), glm::vec2(0.0f, 1.0f), colB * 0.9f);
            } else {
                mb.addQuadN(b0, o0p, o1p, b1, nrm, nrm, nrm, nrm, glm::vec2(0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f), glm::vec2(0.0f, 1.0f), colB * 0.9f);
            }
        }
    }
    mb.build(part.mesh);
}

// Build a solid skirt under the track between left and right wall offsets, down to the
// ground level. This closes gaps beneath the road and ensures continuous collision.
void Track::buildUnderTrack(const ResourceManager& res) {
    Material mat;
    mat.albedo         = glm::vec3(0.12f, 0.12f, 0.12f);
    mat.roughness      = 0.95f;
    mat.texture        = &res.grass();
    mat.useVertexColor = false;
    mat.castShadow     = false;

    TrackPart& part = newPart(mat);
    MeshBuilder mb;
    const int count = (int)m_samples.size();

    for (int i = 0; i < count; ++i) {
        const int i1 = (i + 1) % count;
        const TrackSample& s0 = m_samples[(size_t)i];
        const TrackSample& s1 = m_samples[(size_t)i1];

        const glm::vec3 lTop0 = s0.position + s0.right * -kWallOffset;
        const glm::vec3 lTop1 = s1.position + s1.right * -kWallOffset;
        const glm::vec3 rTop0 = s0.position + s0.right * kWallOffset;
        const glm::vec3 rTop1 = s1.position + s1.right * kWallOffset;

        glm::vec3 lBot0 = lTop0; lBot0.y = m_groundLevel - 0.06f;
        glm::vec3 lBot1 = lTop1; lBot1.y = m_groundLevel - 0.06f;
        glm::vec3 rBot0 = rTop0; rBot0.y = m_groundLevel - 0.06f;
        glm::vec3 rBot1 = rTop1; rBot1.y = m_groundLevel - 0.06f;

        // Left vertical face
        mb.addQuadN(lTop0, lTop1, lBot1, lBot0, s0.right * -1.0f, s1.right * -1.0f, s1.right * -1.0f, s0.right * -1.0f,
                    glm::vec2(0.0f), glm::vec2(1.0f), glm::vec2(1.0f), glm::vec2(0.0f), glm::vec3(0.6f));

        // Right vertical face
        mb.addQuadN(rTop1, rTop0, rBot0, rBot1, s0.right, s1.right, s1.right, s0.right,
                    glm::vec2(0.0f), glm::vec2(1.0f), glm::vec2(1.0f), glm::vec2(0.0f), glm::vec3(0.6f));

        // Bottom face closing between left and right bottom points
        mb.addQuadUV(lBot0, rBot0, rBot1, lBot1, glm::vec2(0.0f), glm::vec2(1.0f,0.0f), glm::vec2(1.0f), glm::vec2(0.0f,1.0f), glm::vec3(0.52f));
    }
    mb.build(part.mesh);
}

void Track::buildRunoffAndGrass(const ResourceManager& res) {
    const int count = (int)m_samples.size();

    // ---- asphalt run-off strip between the curbs and the barriers
    {
        Material mat;
        mat.albedo         = glm::vec3(0.62f, 0.62f, 0.64f);
        mat.roughness      = 0.92f;
        mat.texture        = &res.asphalt();
        mat.useVertexColor = true;
        mat.castShadow     = false;

        TrackPart&  part = newPart(mat);
        MeshBuilder mb;
        for (int side = -1; side <= 1; side += 2) {
            for (int i = 0; i < count; ++i) {
                const int i1 = (i + 1) % count;
                const TrackSample& s0 = m_samples[(size_t)i];
                const TrackSample& s1 = m_samples[(size_t)i1];
                const float innerLat = (float)side * (kHalfWidth + kCurbWidth);
                const float outerLat = (float)side * (kWallOffset - 0.25f);

                const glm::vec3 a = s0.position + s0.right * innerLat;
                const glm::vec3 b = s0.position + s0.right * outerLat;
                const glm::vec3 c = s1.position + s1.right * outerLat;
                const glm::vec3 d = s1.position + s1.right * innerLat;
                const glm::vec2 uvA(innerLat * 0.06f, s0.distance * 0.06f);
                const glm::vec2 uvB(outerLat * 0.06f, s0.distance * 0.06f);
                const glm::vec2 uvC(outerLat * 0.06f, s1.distance * 0.06f);
                const glm::vec2 uvD(innerLat * 0.06f, s1.distance * 0.06f);
                const glm::vec3 col(1.0f);
                if (side > 0) {
                    mb.addQuadN(a, b, c, d, s0.up, s0.up, s1.up, s1.up, uvA, uvB, uvC, uvD, col);
                } else {
                    mb.addQuadN(b, a, d, c, s0.up, s0.up, s1.up, s1.up, uvB, uvA, uvD, uvC, col);
                }
            }
        }
        mb.build(part.mesh);
    }

    // ---- grass apron following the circuit, fading down to the ground plane
    {
        Material mat;
        mat.albedo         = glm::vec3(1.0f);
        mat.roughness      = 0.96f;
        mat.reflectivity   = 0.12f;
        mat.texture        = &res.grass();
        mat.useVertexColor = true;
        mat.castShadow     = false;

        TrackPart&  part = newPart(mat);
        MeshBuilder mb;
        const int   step  = 2;
        const float bands[5] = {0.0f, 12.0f, 32.0f, 62.0f, 105.0f};

        for (int side = -1; side <= 1; side += 2) {
            for (int i = 0; i < count; i += step) {
                const int i1 = (i + step) % count;
                const TrackSample& s0 = m_samples[(size_t)i];
                const TrackSample& s1 = m_samples[(size_t)i1];
                for (int b = 0; b + 1 < 5; ++b) {
                    const float o0 = kWallOffset + 0.5f + bands[b];
                    const float o1 = kWallOffset + 0.5f + bands[b + 1];
                    const float f0 = math::saturate(1.0f - bands[b] / 105.0f);
                    const float f1 = math::saturate(1.0f - bands[b + 1] / 105.0f);

                    auto pnt = [&](const TrackSample& s, float lat, float fade) {
                        glm::vec3 p = s.position + s.right * ((float)side * lat);
                        p.y = math::lerpf(m_groundLevel, s.position.y, fade) - 0.06f;
                        return p;
                    };
                    const glm::vec3 p0 = pnt(s0, o0, f0);
                    const glm::vec3 p1 = pnt(s0, o1, f1);
                    const glm::vec3 p2 = pnt(s1, o1, f1);
                    const glm::vec3 p3 = pnt(s1, o0, f0);

                    const glm::vec2 uv0(o0 * 0.05f, s0.distance * 0.05f);
                    const glm::vec2 uv1(o1 * 0.05f, s0.distance * 0.05f);
                    const glm::vec2 uv2(o1 * 0.05f, s1.distance * 0.05f);
                    const glm::vec2 uv3(o0 * 0.05f, s1.distance * 0.05f);
                    const glm::vec3 col(0.92f + 0.08f * std::sin(s0.distance * 0.11f));
                    if (side > 0) {
                        mb.addQuadUV(p0, p1, p2, p3, uv0, uv1, uv2, uv3, col);
                    } else {
                        mb.addQuadUV(p1, p0, p3, p2, uv1, uv0, uv3, uv2, col);
                    }
                }
            }
        }
        mb.build(part.mesh);
    }

    // ---- distant ground plane
    {
        Material mat;
        mat.albedo         = glm::vec3(0.85f);
        mat.roughness      = 0.98f;
        mat.reflectivity   = 0.08f;
        mat.texture        = &res.grass();
        mat.useVertexColor = false;
        mat.castShadow     = false;

        TrackPart&  part = newPart(mat);
        MeshBuilder mb;
        const float ext = 6500.0f;   // must extend past the horizon ridges
        const glm::vec3 c(m_centroid.x, m_groundLevel - 0.5f, m_centroid.z);
        // Counter clockwise seen from above, so the normal points at the sky.
        // (Wound the other way the plane was invisible from the cockpit and only
        // showed up as a green ceiling when the car dropped into a dip.)
        mb.addQuadUV(c + glm::vec3(-ext, 0.0f, -ext), c + glm::vec3(-ext, 0.0f, ext),
                     c + glm::vec3(ext, 0.0f, ext), c + glm::vec3(ext, 0.0f, -ext),
                     glm::vec2(0.0f), glm::vec2(0.0f, 300.0f), glm::vec2(300.0f),
                     glm::vec2(300.0f, 0.0f), glm::vec3(1.0f));
        mb.build(part.mesh);
    }
}

void Track::buildBarriers(const ResourceManager& res) {
    const int count = (int)m_samples.size();
    const int step  = 2;

    // ---- concrete safety wall
    {
        Material mat;
        mat.albedo         = glm::vec3(1.0f);
        mat.roughness      = 0.8f;
        mat.texture        = &res.concrete();
        mat.useVertexColor = true;
        TrackPart&  part = newPart(mat);
        MeshBuilder mb;

        const float wallH = 1.12f;
        const float thick = 0.5f;

        for (int side = -1; side <= 1; side += 2) {
            for (int i = 0; i < count; i += step) {
                const int i1 = (i + step) % count;
                const TrackSample& s0 = m_samples[(size_t)i];
                const TrackSample& s1 = m_samples[(size_t)i1];
                const float inner = (float)side * kWallOffset;
                const float outer = (float)side * (kWallOffset + thick);

                // Colour bands: white concrete with periodic red/blue advertising.
                const int   band = (int)(s0.distance / 24.0f) % 3;
                glm::vec3   col  = glm::vec3(0.88f);
                if (band == 1) col = glm::vec3(0.80f, 0.18f, 0.16f);
                if (band == 2) col = glm::vec3(0.16f, 0.28f, 0.66f);

                auto P = [&](const TrackSample& s, float lat, float h) {
                    return s.position + s.right * lat + s.up * h;
                };
                const glm::vec3 n = s0.right * (float)-side;
                // inner face
                if (side > 0) {
                    mb.addQuadN(P(s1, inner, 0.0f), P(s1, inner, wallH), P(s0, inner, wallH),
                                P(s0, inner, 0.0f), n, n, n, n, glm::vec2(0.0f),
                                glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f),
                                col);
                } else {
                    mb.addQuadN(P(s0, inner, 0.0f), P(s0, inner, wallH), P(s1, inner, wallH),
                                P(s1, inner, 0.0f), n, n, n, n, glm::vec2(0.0f),
                                glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f),
                                col);
                }
                // top face
                const glm::vec3 up0 = s0.up;
                if (side > 0) {
                    mb.addQuadN(P(s0, inner, wallH), P(s0, outer, wallH), P(s1, outer, wallH),
                                P(s1, inner, wallH), up0, up0, up0, up0, glm::vec2(0.0f),
                                glm::vec2(1.0f, 0.0f), glm::vec2(1.0f), glm::vec2(0.0f, 1.0f),
                                glm::vec3(0.78f));
                } else {
                    mb.addQuadN(P(s0, outer, wallH), P(s0, inner, wallH), P(s1, inner, wallH),
                                P(s1, outer, wallH), up0, up0, up0, up0, glm::vec2(0.0f),
                                glm::vec2(1.0f, 0.0f), glm::vec2(1.0f), glm::vec2(0.0f, 1.0f),
                                glm::vec3(0.78f));
                }
            }
        }
        mb.build(part.mesh);
    }

    // ---- steel guardrail on top of the wall
    {
        Material mat;
        mat.albedo         = glm::vec3(0.72f, 0.74f, 0.78f);
        mat.metallic       = 0.95f;
        mat.roughness      = 0.34f;
        mat.texture        = &res.metal();
        mat.useVertexColor = false;
        TrackPart&  part = newPart(mat);
        MeshBuilder mb;

        for (int side = -1; side <= 1; side += 2) {
            for (int i = 0; i < count; i += step) {
                const int i1 = (i + step) % count;
                const TrackSample& s0 = m_samples[(size_t)i];
                const TrackSample& s1 = m_samples[(size_t)i1];
                const float lat = (float)side * (kWallOffset + 0.08f);
                auto P = [&](const TrackSample& s, float h) {
                    return s.position + s.right * lat + s.up * h;
                };
                const glm::vec3 n = s0.right * (float)-side;
                const glm::vec3 col(1.0f);
                if (side > 0) {
                    mb.addQuadN(P(s1, 1.12f), P(s1, 1.46f), P(s0, 1.46f), P(s0, 1.12f), n, n, n, n,
                                glm::vec2(0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(1.0f),
                                glm::vec2(1.0f, 0.0f), col);
                } else {
                    mb.addQuadN(P(s0, 1.12f), P(s0, 1.46f), P(s1, 1.46f), P(s1, 1.12f), n, n, n, n,
                                glm::vec2(0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(1.0f),
                                glm::vec2(1.0f, 0.0f), col);
                }
            }
        }
        mb.build(part.mesh);
    }

    // ---- debris fence: posts + horizontal rails (skipped on low quality)
    if (m_quality >= 1) {
        Material mat = Material::metal(glm::vec3(0.32f, 0.34f, 0.36f), 0.55f);
        TrackPart&  part = newPart(mat);
        MeshBuilder mb;
        const int postStep = std::max(2, (int)std::round(9.0f / m_spacing));

        for (int side = -1; side <= 1; side += 2) {
            for (int i = 0; i < count; i += postStep) {
                const TrackSample& s = m_samples[(size_t)i];
                const float lat = (float)side * (kWallOffset + 0.55f);
                const glm::vec3 base = s.position + s.right * lat + s.up * 1.1f;
                const glm::mat4 xf = glm::translate(glm::mat4(1.0f), base + glm::vec3(0.0f, 1.6f, 0.0f)) *
                                     glm::scale(glm::mat4(1.0f), glm::vec3(0.09f, 3.2f, 0.09f));
                mb.addTransformedBox(xf, glm::vec3(0.34f));
            }
            for (int i = 0; i < count; i += step * 2) {
                const int i1 = (i + step * 2) % count;
                const TrackSample& s0 = m_samples[(size_t)i];
                const TrackSample& s1 = m_samples[(size_t)i1];
                const float lat = (float)side * (kWallOffset + 0.55f);
                for (int r = 0; r < 3; ++r) {
                    const float h = 1.5f + (float)r * 0.85f;
                    const glm::vec3 a = s0.position + s0.right * lat + s0.up * h;
                    const glm::vec3 b = s1.position + s1.right * lat + s1.up * h;
                    const glm::vec3 dir = b - a;
                    if (glm::length(dir) < 0.01f) continue;
                    const glm::vec3 mid = (a + b) * 0.5f;
                    const glm::mat4 xf =
                        glm::translate(glm::mat4(1.0f), mid) *
                        glm::mat4(glm::mat3(glm::normalize(dir),
                                            glm::vec3(0.0f, 1.0f, 0.0f),
                                            glm::normalize(glm::cross(glm::normalize(dir),
                                                                      glm::vec3(0.0f, 1.0f, 0.0f))))) *
                        glm::scale(glm::mat4(1.0f), glm::vec3(glm::length(dir), 0.05f, 0.05f));
                    mb.addTransformedBox(xf, glm::vec3(0.3f));
                }
            }
        }
        mb.build(part.mesh);
    }
}


void Track::buildStartArea(const ResourceManager& res) {
    const TrackSample start = sampleAt(m_startLine);

    // ---- painted start/finish line + grid boxes
    {
        Material mat;
        mat.albedo         = glm::vec3(1.0f);
        mat.roughness      = 0.42f;
        mat.useVertexColor = true;
        mat.castShadow     = false;
        TrackPart&  part = newPart(mat);
        MeshBuilder mb;

        // Chequered band, 0.55 m squares across the full width.
        const int   cols  = 26;
        const float depth = 1.10f;
        for (int c = 0; c < cols; ++c) {
            const float l0 = math::lerpf(-kHalfWidth, kHalfWidth, (float)c / (float)cols);
            const float l1 = math::lerpf(-kHalfWidth, kHalfWidth, (float)(c + 1) / (float)cols);
            for (int r = 0; r < 2; ++r) {
                const float d0 = m_startLine - depth * 0.5f + depth * 0.5f * (float)r;
                const float d1 = d0 + depth * 0.5f;
                const glm::vec3 col = ((c + r) % 2) ? glm::vec3(0.95f) : glm::vec3(0.06f);
                const glm::vec3 a = surfacePoint(d0, l0) + start.up * 0.018f;
                const glm::vec3 b = surfacePoint(d0, l1) + start.up * 0.018f;
                const glm::vec3 cc = surfacePoint(d1, l1) + start.up * 0.018f;
                const glm::vec3 d = surfacePoint(d1, l0) + start.up * 0.018f;
                mb.addQuad(a, b, cc, d, col);
            }
        }

        // Grid boxes: an open rectangle painted for every starting slot.
        for (int slot = 0; slot < kGridSlots; ++slot) {
            const int   row  = slot / 2;
            const int   col  = slot % 2;
            const float back = 12.0f + (float)row * 9.0f + (float)col * 4.5f;
            const float lat  = (col == 0 ? -1.0f : 1.0f) * 3.1f;
            const float d0   = m_startLine - back - 2.6f;
            const float d1   = m_startLine - back + 2.6f;
            const float w    = 1.35f;
            auto line = [&](float la, float lb, float da, float db) {
                const glm::vec3 p0 = surfacePoint(da, la) + start.up * 0.019f;
                const glm::vec3 p1 = surfacePoint(da, lb) + start.up * 0.019f;
                const glm::vec3 p2 = surfacePoint(db, lb) + start.up * 0.019f;
                const glm::vec3 p3 = surfacePoint(db, la) + start.up * 0.019f;
                mb.addQuad(p0, p1, p2, p3, glm::vec3(0.92f));
            };
            line(lat - w, lat - w + 0.14f, d0, d1);
            line(lat + w - 0.14f, lat + w, d0, d1);
            line(lat - w, lat + w, d0, d0 + 0.14f);
        }
        mb.build(part.mesh);
    }

    // ---- start line structures, all kept BESIDE the circuit
    // Nothing is allowed to span over the racing surface: the track stays fully
    // open to the sky, so the sun, the shadows and the camera are never blocked.
    {
        Material mat;
        mat.albedo         = glm::vec3(1.0f);
        mat.metallic       = 0.6f;
        mat.roughness      = 0.42f;
        mat.texture        = &res.metal();
        mat.useVertexColor = true;
        TrackPart&  part = newPart(mat);
        MeshBuilder mb;

        const float d = m_startLine + 1.5f;

        // Light panel post on the infield side.
        {
            const float lat  = m_insideSign * (kHalfWidth + 4.6f);
            const glm::vec3 base = surfacePoint(d, lat);
            const TrackSample s = sampleAt(d);
            const glm::mat4 basis(glm::vec4(s.right, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
                                  glm::vec4(s.forward, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
            mb.addBox(base + glm::vec3(0.0f, 2.05f, 0.0f), glm::vec3(0.22f, 2.05f, 0.22f),
                      glm::vec3(0.58f, 0.59f, 0.62f));
            mb.addBox(base + glm::vec3(0.0f, 0.2f, 0.0f), glm::vec3(0.75f, 0.2f, 0.75f),
                      glm::vec3(0.55f));
            // Dark backing panel the five lights are mounted on.
            mb.addTransformedBox(
                glm::translate(glm::mat4(1.0f), base + glm::vec3(0.0f, 3.35f, 0.0f)) * basis *
                    glm::scale(glm::mat4(1.0f), glm::vec3(4.1f, 0.95f, 0.28f)),
                glm::vec3(0.10f, 0.10f, 0.12f));
        }

        // START / FINISH board on the opposite side.
        {
            const float lat  = -m_insideSign * (kHalfWidth + 3.4f);
            const glm::vec3 base = surfacePoint(d, lat);
            const TrackSample s = sampleAt(d);
            const glm::mat4 basis(glm::vec4(s.right, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
                                  glm::vec4(s.forward, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
            for (int k = -1; k <= 1; k += 2) {
                mb.addBox(base + glm::vec3(0.0f, 1.5f, 0.0f) +
                              s.forward * ((float)k * 1.6f),
                          glm::vec3(0.16f, 1.5f, 0.16f), glm::vec3(0.55f));
            }
            mb.addTransformedBox(
                glm::translate(glm::mat4(1.0f), base + glm::vec3(0.0f, 3.35f, 0.0f)) * basis *
                    glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.5f, 3.6f)),
                glm::vec3(0.84f, 0.14f, 0.11f));
        }
        mb.build(part.mesh);
    }

    // ---- five start lights, mounted on the side panel facing the grid
    {
        const float d = m_startLine + 1.5f;
        for (int i = 0; i < 5; ++i) {
            const float lat = m_insideSign * (kHalfWidth + 3.05f + (float)i * 0.78f);
            const glm::vec3 c = surfacePoint(d, lat) + glm::vec3(0.0f, 3.35f, 0.0f) -
                                sampleAt(d).forward * (m_insideSign * 0.16f);
            MeshBuilder mb;
            mb.addSphere(c, 0.30f, 10, 14, glm::vec3(1.0f));
            mb.build(m_startLights[(size_t)i]);
        }
    }
}

void Track::buildPitLane(const ResourceManager& res) {
    const float d0 = m_straightBegin + 25.0f;
    const float d1 = m_straightEnd - 25.0f;
    if (d1 - d0 < 120.0f) return;

    const float laneCentre = m_insideSign * (kWallOffset + 11.0f);
    const float laneHalf   = 6.0f;
    const int   steps      = std::max(8, (int)((d1 - d0) / 6.0f));

    // ---- pit lane surface + markings
    {
        Material mat;
        mat.albedo         = glm::vec3(0.80f);
        mat.roughness      = 0.88f;
        mat.texture        = &res.concrete();
        mat.useVertexColor = true;
        mat.castShadow     = false;
        TrackPart&  part = newPart(mat);
        MeshBuilder mb;

        for (int i = 0; i < steps; ++i) {
            const float a = math::lerpf(d0, d1, (float)i / (float)steps);
            const float b = math::lerpf(d0, d1, (float)(i + 1) / (float)steps);
            // Tapered entry/exit so the lane visually merges with the circuit.
            auto width = [&](float t) {
                const float fade = math::smoothstepf(0.0f, 0.08f, t) *
                                   math::smoothstepf(1.0f, 0.92f, t);
                return laneHalf * (0.35f + 0.65f * fade);
            };
            const float ta = (float)i / (float)steps;
            const float tb = (float)(i + 1) / (float)steps;
            const glm::vec3 p0 = surfacePoint(a, laneCentre - width(ta)) + glm::vec3(0, 0.02f, 0);
            const glm::vec3 p1 = surfacePoint(a, laneCentre + width(ta)) + glm::vec3(0, 0.02f, 0);
            const glm::vec3 p2 = surfacePoint(b, laneCentre + width(tb)) + glm::vec3(0, 0.02f, 0);
            const glm::vec3 p3 = surfacePoint(b, laneCentre - width(tb)) + glm::vec3(0, 0.02f, 0);
            const glm::vec3 col(0.95f);
            if (m_insideSign > 0.0f) {
                mb.addQuadUV(p0, p1, p2, p3, glm::vec2(0.0f), glm::vec2(2.0f, 0.0f),
                             glm::vec2(2.0f, 3.0f), glm::vec2(0.0f, 3.0f), col);
            } else {
                mb.addQuadUV(p1, p0, p3, p2, glm::vec2(0.0f), glm::vec2(2.0f, 0.0f),
                             glm::vec2(2.0f, 3.0f), glm::vec2(0.0f, 3.0f), col);
            }
            // Dashed white guide line down the middle of the lane.
            if (i % 2 == 0) {
                const glm::vec3 q0 = surfacePoint(a, laneCentre - 0.1f) + glm::vec3(0, 0.03f, 0);
                const glm::vec3 q1 = surfacePoint(a, laneCentre + 0.1f) + glm::vec3(0, 0.03f, 0);
                const glm::vec3 q2 = surfacePoint(b, laneCentre + 0.1f) + glm::vec3(0, 0.03f, 0);
                const glm::vec3 q3 = surfacePoint(b, laneCentre - 0.1f) + glm::vec3(0, 0.03f, 0);
                mb.addQuad(q0, q1, q2, q3, glm::vec3(1.0f));
            }
        }
        mb.build(part.mesh);
    }

    // ---- pit wall + garages
    {
        Material mat;
        mat.albedo         = glm::vec3(1.0f);
        mat.roughness      = 0.72f;
        mat.texture        = &res.concrete();
        mat.useVertexColor = true;
        TrackPart&  part = newPart(mat);
        MeshBuilder mb;

        const float wallLat = m_insideSign * (kWallOffset + 3.4f);
        for (int i = 0; i < steps; ++i) {
            const float a = math::lerpf(d0, d1, (float)i / (float)steps);
            const float b = math::lerpf(d0, d1, (float)(i + 1) / (float)steps);
            const glm::vec3 p0 = surfacePoint(a, wallLat);
            const glm::vec3 p1 = surfacePoint(b, wallLat);
            const glm::vec3 mid = (p0 + p1) * 0.5f + glm::vec3(0.0f, 0.55f, 0.0f);
            const glm::vec3 dir = p1 - p0;
            if (glm::length(dir) < 0.01f) continue;
            const glm::vec3 f = glm::normalize(dir);
            const glm::vec3 s = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), f));
            const glm::mat4 basis(glm::vec4(f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
                                  glm::vec4(s, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
            const glm::vec3 col = (i % 3 == 0) ? glm::vec3(0.86f, 0.20f, 0.16f) : glm::vec3(0.9f);
            mb.addTransformedBox(glm::translate(glm::mat4(1.0f), mid) * basis *
                                     glm::scale(glm::mat4(1.0f),
                                                glm::vec3(glm::length(dir) * 1.02f, 1.1f, 0.4f)),
                                 col);
        }

        // Garages behind the lane.
        const float garageLat = m_insideSign * (kWallOffset + 19.5f);
        const int   garages   = std::max(3, (int)((d1 - d0) / 13.0f));
        for (int g = 0; g < garages; ++g) {
            const float t = ((float)g + 0.5f) / (float)garages;
            const float dd = math::lerpf(d0 + 6.0f, d1 - 6.0f, t);
            const TrackSample s = sampleAt(dd);
            const glm::vec3   c = surfacePoint(dd, garageLat);
            const glm::vec3   f = s.forward;
            const glm::vec3   r = s.right;
            const glm::mat4 basis(glm::vec4(f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
                                  glm::vec4(r, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
            mb.addTransformedBox(glm::translate(glm::mat4(1.0f), c + glm::vec3(0.0f, 3.0f, 0.0f)) *
                                     basis * glm::scale(glm::mat4(1.0f), glm::vec3(11.0f, 6.0f, 12.0f)),
                                 glm::vec3(0.84f, 0.85f, 0.87f));
            // Roll-up door facing the pit lane.
            const glm::vec3 doorC = c - r * m_insideSign * 6.1f + glm::vec3(0.0f, 2.1f, 0.0f);
            mb.addTransformedBox(glm::translate(glm::mat4(1.0f), doorC) * basis *
                                     glm::scale(glm::mat4(1.0f), glm::vec3(7.5f, 4.2f, 0.25f)),
                                 glm::vec3(0.12f, 0.13f, 0.15f));
            // Team colour banner above the door.
            const glm::vec3 bannerC = c - r * m_insideSign * 6.2f + glm::vec3(0.0f, 5.0f, 0.0f);
            const glm::vec3 teamCol[6] = {{0.85f, 0.13f, 0.12f}, {0.10f, 0.30f, 0.72f},
                                          {0.95f, 0.75f, 0.08f}, {0.08f, 0.55f, 0.35f},
                                          {0.90f, 0.42f, 0.06f}, {0.35f, 0.12f, 0.62f}};
            mb.addTransformedBox(glm::translate(glm::mat4(1.0f), bannerC) * basis *
                                     glm::scale(glm::mat4(1.0f), glm::vec3(9.5f, 1.2f, 0.2f)),
                                 teamCol[(size_t)(g % 6)]);
        }
        mb.build(part.mesh);
    }
}

void Track::collectStartLights(Renderer& renderer, int litCount, bool greenPhase) const {
    for (int i = 0; i < 5; ++i) {
        if (!m_startLights[(size_t)i].valid()) continue;
        Material mat;
        const bool on = greenPhase || i < litCount;
        if (greenPhase) {
            mat = Material::emissiveLight(glm::vec3(0.10f, 1.0f, 0.25f), 3.4f);
        } else if (on) {
            mat = Material::emissiveLight(glm::vec3(1.0f, 0.08f, 0.05f), 3.0f);
        } else {
            mat = Material::plastic(glm::vec3(0.09f, 0.03f, 0.03f));
        }
        mat.castShadow = false;
        renderer.submit(m_startLights[(size_t)i], glm::mat4(1.0f), mat);
    }
}

bool Track::build(const ResourceManager& resources, int qualityLevel) {
    m_quality = qualityLevel;
    m_parts.clear();
    m_parts.reserve(24);

    generateSamples();
    if (m_samples.empty()) return false;
    computeRacingLine();
    findMainStraight();
    debugRacingLine();

    buildAsphalt(resources);
    buildMarkings();
    buildCurbs();
    // Ensure continuous curbs along the whole circuit (both sides)
    buildContinuousCurbs(resources);
    buildRunoffAndGrass(resources);
    buildBarriers(resources);
    buildStartArea(resources);
    buildPitLane(resources);
    // Solid under-track skirt to close any gaps and provide continuous
    // collision geometry matching the visible road and surrounding ground.
    buildUnderTrack(resources);
    buildMinimap();

    int tris = 0;
    for (const TrackPart& p : m_parts) tris += p.mesh.triangleCount();
    std::printf("[Track] geometry ready: %d meshes, %d triangles\n", (int)m_parts.size(), tris);
    return true;
}

} // namespace vr
