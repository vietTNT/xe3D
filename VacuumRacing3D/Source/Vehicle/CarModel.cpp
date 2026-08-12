#include "CarModel.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include "../Graphics/Renderer.h"
#include "../Managers/ResourceManager.h"
#include "../Utilities/MathUtils.h"

namespace vr {
namespace {

// ---------------------------------------------------------------- body profile
// Stations run from the rear bumper (-) to the nose (+).
struct Station {
    float z;        ///< longitudinal position
    float halfW;    ///< maximum half width
    float bottom;   ///< underbody height
    float top;      ///< roof/deck height
    float tumble;   ///< how much the section narrows toward the top
    float lower;    ///< how much the section narrows toward the floor
};

const Station kStations[] = {
    {-2.310f, 0.62f, 0.320f, 0.845f, 0.30f, 0.34f},
    {-2.180f, 0.83f, 0.285f, 0.905f, 0.28f, 0.30f},
    {-2.000f, 0.945f, 0.255f, 0.955f, 0.26f, 0.26f},
    {-1.780f, 0.990f, 0.235f, 1.020f, 0.25f, 0.24f},
    {-1.520f, 1.000f, 0.225f, 1.115f, 0.30f, 0.24f},
    {-1.240f, 0.998f, 0.220f, 1.215f, 0.38f, 0.24f},
    {-0.940f, 0.992f, 0.218f, 1.292f, 0.44f, 0.24f},
    {-0.600f, 0.988f, 0.216f, 1.330f, 0.46f, 0.24f},
    {-0.250f, 0.986f, 0.215f, 1.338f, 0.46f, 0.24f},
    { 0.100f, 0.985f, 0.215f, 1.322f, 0.45f, 0.24f},
    { 0.420f, 0.982f, 0.218f, 1.258f, 0.43f, 0.25f},
    { 0.700f, 0.978f, 0.222f, 1.140f, 0.40f, 0.26f},
    { 0.950f, 0.968f, 0.228f, 1.010f, 0.30f, 0.27f},
    { 1.200f, 0.955f, 0.235f, 0.960f, 0.24f, 0.28f},
    { 1.480f, 0.940f, 0.243f, 0.930f, 0.22f, 0.29f},
    { 1.740f, 0.918f, 0.252f, 0.902f, 0.22f, 0.30f},
    { 1.960f, 0.885f, 0.262f, 0.848f, 0.22f, 0.32f},
    { 2.130f, 0.838f, 0.272f, 0.796f, 0.24f, 0.34f},
    { 2.250f, 0.752f, 0.288f, 0.742f, 0.26f, 0.36f},
    { 2.310f, 0.575f, 0.320f, 0.672f, 0.30f, 0.42f},
};
constexpr int kStationCount = (int)(sizeof(kStations) / sizeof(kStations[0]));

/// Beltline: the shoulder line where paint ends and glazing begins.
float beltHeight(float z) {
    if (z > 0.95f) return 1.02f;
    if (z < -1.60f) return 1.00f;
    return math::lerpf(0.985f, 1.045f, math::smoothstepf(-1.60f, 0.20f, z));
}

bool inCabin(float z) { return z > -1.62f && z < 0.98f; }

/// Superellipse with tumblehome, returns a point of the section outline.
glm::vec3 sectionPoint(const Station& st, float angle, float widthScale = 1.0f,
                       float heightScale = 1.0f) {
    const float cx = std::cos(angle);
    const float cy = std::sin(angle);
    
    // Tăng thông số nx, ny để ép khung xe vuông vức như NASCAR
    const float nx = 8.0f;   
    const float ny = 6.0f;   
    
    const float sx = (cx >= 0.0f ? 1.0f : -1.0f) * std::pow(std::fabs(cx), 2.0f / nx);
    const float sy = (cy >= 0.0f ? 1.0f : -1.0f) * std::pow(std::fabs(cy), 2.0f / ny);

    const float yc     = (st.top + st.bottom) * 0.5f;
    const float halfH  = (st.top - st.bottom) * 0.5f;
    const float y      = yc + halfH * sy * heightScale;
    const float hNorm  = math::saturate((sy + 1.0f) * 0.5f);

    float w = 1.0f;
    if (hNorm > 0.42f) {
        const float t = (hNorm - 0.42f) / 0.58f;
        w -= st.tumble * t * t;
    } else {
        const float t = (0.42f - hNorm) / 0.42f;
        w -= st.lower * t * t * 0.85f;
    }
    const float x = st.halfW * sx * w * widthScale;
    return glm::vec3(x, y, st.z);
}

} // namespace

// ================================================================== body shell
void CarModel::buildBodyShell(int quality) {
    const int   segments = (quality >= 2) ? 40 : (quality == 1 ? 32 : 24);
    MeshBuilder paint, glass;
    MeshBuilder& trim = m_trimAccum;   // details are appended later

    std::vector<std::vector<glm::vec3>> rings((size_t)kStationCount);
    for (int i = 0; i < kStationCount; ++i) {
        rings[(size_t)i].resize((size_t)segments);
        for (int k = 0; k < segments; ++k) {
            const float a = math::kTwoPi * (float)k / (float)segments;
            rings[(size_t)i][(size_t)k] = sectionPoint(kStations[i], a);
        }
    }

    auto normalAt = [&](int i, int k) {
        const int ip = std::max(i - 1, 0);
        const int in = std::min(i + 1, kStationCount - 1);
        const int kp = (k - 1 + segments) % segments;
        const int kn = (k + 1) % segments;
        const glm::vec3 du = rings[(size_t)in][(size_t)k] - rings[(size_t)ip][(size_t)k];
        const glm::vec3 dv = rings[(size_t)i][(size_t)kn] - rings[(size_t)i][(size_t)kp];
        glm::vec3 n = glm::cross(dv, du);
        const float len = glm::length(n);
        return len > 1e-6f ? n / len : glm::vec3(0.0f, 1.0f, 0.0f);
    };

    for (int i = 0; i + 1 < kStationCount; ++i) {
        for (int k = 0; k < segments; ++k) {
            const int k1 = (k + 1) % segments;
            const glm::vec3 p0 = rings[(size_t)i][(size_t)k];
            const glm::vec3 p1 = rings[(size_t)i][(size_t)k1];
            const glm::vec3 p2 = rings[(size_t)i + 1][(size_t)k1];
            const glm::vec3 p3 = rings[(size_t)i + 1][(size_t)k];

            const glm::vec3 n0 = normalAt(i, k);
            const glm::vec3 n1 = normalAt(i, k1);
            const glm::vec3 n2 = normalAt(i + 1, k1);
            const glm::vec3 n3 = normalAt(i + 1, k);

            const glm::vec3 centre = (p0 + p1 + p2 + p3) * 0.25f;
            const glm::vec3 nAvg   = glm::normalize(n0 + n1 + n2 + n3);
            const float     belt   = beltHeight(centre.z);
            const bool      cabin  = inCabin(centre.z);
            const bool      above  = centre.y > belt;
            
            const bool      roofish = nAvg.y > 0.82f && std::fabs(centre.x) < 0.30f;

            const float ax = std::fabs(centre.x);
            const bool aPillar = centre.z > 0.52f && centre.z < 0.86f && ax > 0.40f;
            const bool cPillar = centre.z < -1.10f && centre.z > -1.58f && ax > 0.40f;
            const bool bPillar = std::fabs(centre.z + 0.28f) < 0.07f && ax > 0.52f;

            MeshBuilder* target = &paint;
            glm::vec3    color(1.0f);
            if (cabin && above && !roofish && !aPillar && !cPillar && !bPillar) {
                target = &glass;
            } else if (above && !cabin && nAvg.y < -0.4f) {
                target = &trim;   
                color  = glm::vec3(0.10f);
            } else if (nAvg.y < -0.55f) {
                target = &trim;   
                color  = glm::vec3(0.08f);
            }
            glm::vec2 uv0, uv1, uv2, uv3;
            if (target == &paint) {
                float u0 = (float)k / (float)segments;
                float u1 = (float)(k + 1) / (float)segments;
                float z0 = (kStations[i].z + 2.31f) / 4.62f;
                float z1 = (kStations[i + 1].z + 2.31f) / 4.62f;
                if (roofish) {
                    uv0 = glm::vec2(0.32f + 0.36f * u0, 0.72f + 0.24f * z0);
                    uv1 = glm::vec2(0.32f + 0.36f * u1, 0.72f + 0.24f * z0);
                    uv2 = glm::vec2(0.32f + 0.36f * u1, 0.72f + 0.24f * z1);
                    uv3 = glm::vec2(0.32f + 0.36f * u0, 0.72f + 0.24f * z1);
                } else if (centre.z > 0.8f) {
                    uv0 = glm::vec2(0.25f + 0.50f * u0, 0.55f + 0.13f * z0);
                    uv1 = glm::vec2(0.25f + 0.50f * u1, 0.55f + 0.13f * z0);
                    uv2 = glm::vec2(0.25f + 0.50f * u1, 0.55f + 0.13f * z1);
                    uv3 = glm::vec2(0.25f + 0.50f * u0, 0.55f + 0.13f * z1);
                } else if (centre.x < -0.3f) {
                    uv0 = glm::vec2(0.12f + 0.30f * z0, 0.12f + 0.30f * (1.0f - u0));
                    uv1 = glm::vec2(0.12f + 0.30f * z0, 0.12f + 0.30f * (1.0f - u1));
                    uv2 = glm::vec2(0.12f + 0.30f * z1, 0.12f + 0.30f * (1.0f - u1));
                    uv3 = glm::vec2(0.12f + 0.30f * z1, 0.12f + 0.30f * (1.0f - u0));
                } else if (centre.x > 0.3f) {
                    uv0 = glm::vec2(0.58f + 0.30f * z0, 0.12f + 0.30f * u0);
                    uv1 = glm::vec2(0.58f + 0.30f * z0, 0.12f + 0.30f * u1);
                    uv2 = glm::vec2(0.58f + 0.30f * z1, 0.12f + 0.30f * u1);
                    uv3 = glm::vec2(0.58f + 0.30f * z1, 0.12f + 0.30f * u0);
                } else {
                    float v0 = (p0.y - 0.20f) / 1.15f;
                    float v1 = (p1.y - 0.20f) / 1.15f;
                    float v2 = (p2.y - 0.20f) / 1.15f;
                    float v3 = (p3.y - 0.20f) / 1.15f;
                    uv0 = glm::vec2(u0, v0);
                    uv1 = glm::vec2(u1, v1);
                    uv2 = glm::vec2(u1, v2);
                    uv3 = glm::vec2(u0, v3);
                }
            } else {
                const float uScale = 0.35f;
                uv0 = glm::vec2((float)k / (float)segments, kStations[i].z * uScale);
                uv1 = glm::vec2((float)(k + 1) / (float)segments, kStations[i].z * uScale);
                uv2 = glm::vec2((float)(k + 1) / (float)segments, kStations[i + 1].z * uScale);
                uv3 = glm::vec2((float)k / (float)segments, kStations[i + 1].z * uScale);
            }
            target->addQuadN(p0, p1, p2, p3, n0, n1, n2, n3, uv0, uv1, uv2, uv3, color);
        }
    }

    {
        std::vector<glm::vec3> front(rings.back());
        std::vector<glm::vec3> rear(rings.front());
        glm::vec3 cf(0.0f), cr(0.0f);
        for (const glm::vec3& p : front) cf += p;
        for (const glm::vec3& p : rear) cr += p;
        cf /= (float)front.size();
        cr /= (float)rear.size();
        paint.addFan(front, cf, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f));
        std::reverse(rear.begin(), rear.end());
        paint.addFan(rear, cr, glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f));
    }

    const float archR = kWheelRadius + 0.115f;
    for (int i = 0; i < 4; ++i) {
        const bool  frontAxle = (i < 2);
        const float zc = frontAxle ? kWheelBase * 0.5f : -kWheelBase * 0.5f;
        const float xs = (i % 2 == 0) ? -1.0f : 1.0f;
        const float xc = xs * kTrackWidth * 0.5f;
        const float yc = kWheelRadius;   
        m_wheelOffsets[i] = glm::vec3(xc, yc, zc);

        const int   arcSteps = 18;
        const float inX  = xs * (kTrackWidth * 0.5f - kWheelWidth * 0.5f - 0.06f);
        const float outX = xs * (kTrackWidth * 0.5f + kWheelWidth * 0.5f + 0.02f);

        for (int s = 0; s < arcSteps; ++s) {
            const float a0 = math::kPi * (float)s / (float)arcSteps;
            const float a1 = math::kPi * (float)(s + 1) / (float)arcSteps;
            const glm::vec3 q0(inX, yc + std::sin(a0) * archR, zc - std::cos(a0) * archR);
            const glm::vec3 q1(inX, yc + std::sin(a1) * archR, zc - std::cos(a1) * archR);
            const glm::vec3 q2(outX, yc + std::sin(a1) * archR, zc - std::cos(a1) * archR);
            const glm::vec3 q3(outX, yc + std::sin(a0) * archR, zc - std::cos(a0) * archR);
            if (xs > 0.0f) {
                trim.addQuad(q0, q1, q2, q3, glm::vec3(0.055f));
            } else {
                trim.addQuad(q3, q2, q1, q0, glm::vec3(0.055f));
            }
        }
        for (int s = 0; s < arcSteps; ++s) {
            const float a0 = math::kPi * (float)s / (float)arcSteps;
            const float a1 = math::kPi * (float)(s + 1) / (float)arcSteps;
            const float r0 = archR;
            const float r1 = archR + 0.028f;
            const glm::vec3 q0(outX, yc + std::sin(a0) * r0, zc - std::cos(a0) * r0);
            const glm::vec3 q1(outX, yc + std::sin(a1) * r0, zc - std::cos(a1) * r0);
            const glm::vec3 q2(outX, yc + std::sin(a1) * r1, zc - std::cos(a1) * r1);
            const glm::vec3 q3(outX, yc + std::sin(a0) * r1, zc - std::cos(a0) * r1);
            if (xs > 0.0f) {
                trim.addQuad(q3, q2, q1, q0, glm::vec3(0.09f));
            } else {
                trim.addQuad(q0, q1, q2, q3, glm::vec3(0.09f));
            }
        }
    }

    paint.build(m_body);
    glass.build(m_glass);
}

// ======================================================================== aero
void CarModel::buildAero() {
    MeshBuilder carbon;

    // Cản trước phẳng sát đất (Front Air Dam)
    carbon.addBox(glm::vec3(0.0f, 0.22f, 2.2f), glm::vec3(0.85f, 0.10f, 0.15f), glm::vec3(0.8f, 0.8f, 0.8f)); 
    // Lưới tản nhiệt dưới
    carbon.addBox(glm::vec3(0.0f, 0.20f, 2.32f), glm::vec3(0.6f, 0.06f, 0.05f), glm::vec3(0.05f));

    // Lườn xe (Side skirts) dài
    for (int s = -1; s <= 1; s += 2) {
        carbon.addBox(glm::vec3((float)s * 0.85f, 0.22f, 0.0f), glm::vec3(0.02f, 0.10f, 1.8f), glm::vec3(0.1f));
    }

    // Cánh gió dạng tấm dựng đứng ở cốp sau (NASCAR Blade Spoiler màu đỏ/đen)
    carbon.addBox(glm::vec3(0.0f, 1.08f, -2.25f), glm::vec3(0.85f, 0.12f, 0.02f), glm::vec3(0.85f, 0.12f, 0.10f)); 
    carbon.addBox(glm::vec3(-0.65f, 1.08f, -2.2f), glm::vec3(0.02f, 0.12f, 0.06f), glm::vec3(0.1f));
    carbon.addBox(glm::vec3( 0.65f, 1.08f, -2.2f), glm::vec3(0.02f, 0.12f, 0.06f), glm::vec3(0.1f));

    carbon.build(m_carbon);
}

// ====================================================================== lights
void CarModel::buildLights() {
    MeshBuilder head, tail, rev;

    for (int s = -1; s <= 1; s += 2) {
        const float x = (float)s * 0.60f;
        glm::mat4 xf = glm::translate(glm::mat4(1.0f), glm::vec3(x, 0.845f, 2.115f)) *
                       glm::rotate(glm::mat4(1.0f), (float)s * -0.16f, glm::vec3(0.0f, 0.0f, 1.0f)) *
                       glm::rotate(glm::mat4(1.0f), 0.10f, glm::vec3(1.0f, 0.0f, 0.0f)) *
                       glm::scale(glm::mat4(1.0f), glm::vec3(0.38f, 0.065f, 0.09f));
        head.addTransformedBox(xf, glm::vec3(1.0f));
        glm::mat4 drl = glm::translate(glm::mat4(1.0f), glm::vec3(x - (float)s * 0.02f, 0.745f, 2.10f)) *
                        glm::rotate(glm::mat4(1.0f), (float)s * -0.35f, glm::vec3(0.0f, 0.0f, 1.0f)) *
                        glm::scale(glm::mat4(1.0f), glm::vec3(0.30f, 0.045f, 0.08f));
        head.addTransformedBox(drl, glm::vec3(1.0f));
    }

    tail.addBox(glm::vec3(0.0f, 0.790f, -2.255f), glm::vec3(0.60f, 0.028f, 0.030f),
                glm::vec3(1.0f));
    for (int s = -1; s <= 1; s += 2) {
        tail.addBox(glm::vec3((float)s * 0.56f, 0.790f, -2.250f),
                    glm::vec3(0.17f, 0.048f, 0.035f), glm::vec3(1.0f));
    }
    for (int s = -1; s <= 1; s += 2) {
        rev.addBox(glm::vec3((float)s * 0.34f, 0.545f, -2.240f),
                   glm::vec3(0.085f, 0.028f, 0.030f), glm::vec3(1.0f));
    }

    head.build(m_headlights);
    tail.build(m_taillights);
    rev.build(m_reverseLights);
}

// ===================================================================== details
void CarModel::buildDetails() {
    MeshBuilder& trim = m_trimAccum;   
    MeshBuilder  chrome;

    trim.addBox(glm::vec3(0.0f, 0.480f, 2.275f), glm::vec3(0.44f, 0.105f, 0.045f),
                glm::vec3(0.035f));
    for (int i = 0; i < 3; ++i) {
        chrome.addBox(glm::vec3(0.0f, 0.430f + (float)i * 0.052f, 2.300f),
                      glm::vec3(0.415f, 0.011f, 0.020f), glm::vec3(1.0f));
    }
    
    for (int s = -1; s <= 1; s += 2) {
        trim.addBox(glm::vec3((float)s * 0.585f, 0.430f, 2.215f),
                    glm::vec3(0.115f, 0.070f, 0.045f), glm::vec3(0.035f));
    }
    
    trim.addBox(glm::vec3(0.0f, 0.44f, -2.24f), glm::vec3(0.80f, 0.13f, 0.05f),
                glm::vec3(0.06f));

    for (int s = -1; s <= 1; s += 2) {
        const float x = (float)s * 0.995f; 
        trim.addBox(glm::vec3(x, 0.66f, 0.66f), glm::vec3(0.015f, 0.30f, 0.015f), glm::vec3(0.02f));
        trim.addBox(glm::vec3(x, 0.66f, -0.72f), glm::vec3(0.015f, 0.30f, 0.015f), glm::vec3(0.02f));
        trim.addBox(glm::vec3(x, 0.85f, -0.03f), glm::vec3(0.015f, 0.015f, 0.70f), glm::vec3(0.02f));
        chrome.addBox(glm::vec3(x + (float)s * 0.012f, 0.90f, -0.10f),
                      glm::vec3(0.014f, 0.026f, 0.11f), glm::vec3(1.0f));
    }

    for (int s = -1; s <= 1; s += 2) {
        const float x = (float)s * 0.99f;
        trim.addBox(glm::vec3(x + (float)s * 0.05f, 1.005f, 0.60f),
                    glm::vec3(0.055f, 0.018f, 0.035f), glm::vec3(0.06f));
        glm::mat4 xf = glm::translate(glm::mat4(1.0f),
                                      glm::vec3(x + (float)s * 0.125f, 1.030f, 0.585f)) *
                       glm::rotate(glm::mat4(1.0f), (float)s * 0.12f, glm::vec3(0.0f, 1.0f, 0.0f)) *
                       glm::scale(glm::mat4(1.0f), glm::vec3(0.075f, 0.075f, 0.19f));
        trim.addTransformedBox(xf, glm::vec3(0.07f));
        chrome.addBox(glm::vec3(x + (float)s * 0.155f, 1.030f, 0.585f),
                      glm::vec3(0.006f, 0.055f, 0.075f), glm::vec3(1.0f));
    }

    for (int s = -1; s <= 1; s += 2) {
        chrome.addCylinder(glm::vec3((float)s * 0.42f, 0.40f, -2.22f),
                           glm::vec3(0.0f, 0.0f, -0.10f), 0.055f, 0.062f, 12, glm::vec3(1.0f));
    }
    
    trim.addBox(glm::vec3(0.0f, 1.352f, -0.80f), glm::vec3(0.035f, 0.05f, 0.16f),
                glm::vec3(0.06f));

    // Driver side window safety net (Lưới bảo vệ cửa sổ tài xế)
    MeshBuilder windowNetBuilder;
    windowNetBuilder.addQuad(glm::vec3(-0.955f, 0.98f, 0.50f),
                             glm::vec3(-0.955f, 0.98f, -0.40f),
                             glm::vec3(-0.955f, 1.25f, -0.35f),
                             glm::vec3(-0.955f, 1.25f, 0.40f),
                             glm::vec3(1.0f));
    windowNetBuilder.build(m_windowNet);

    // Dual chrome side exhaust pipes under right rocker panel (NASCAR style)
    MeshBuilder exhaustBuilder;
    exhaustBuilder.addCylinder(glm::vec3(0.96f, 0.28f, -0.45f), glm::vec3(0.12f, 0.0f, 0.0f),
                               0.045f, 0.052f, 12, glm::vec3(1.0f));
    exhaustBuilder.addCylinder(glm::vec3(0.96f, 0.28f, -0.58f), glm::vec3(0.12f, 0.0f, 0.0f),
                               0.045f, 0.052f, 12, glm::vec3(1.0f));
    exhaustBuilder.build(m_sideExhaust);

    trim.build(m_trim);   
    chrome.build(m_chrome);
    m_trimAccum.clear();
}

// ==================================================================== interior
void CarModel::buildInterior() {
    MeshBuilder mb;
    const glm::vec3 dark(0.075f, 0.075f, 0.085f);
    const glm::vec3 seatCol(0.14f, 0.10f, 0.10f);

    mb.addBox(glm::vec3(0.0f, 0.905f, 0.60f), glm::vec3(0.82f, 0.10f, 0.22f), dark);
    mb.addBox(glm::vec3(0.0f, 0.70f, 0.20f), glm::vec3(0.16f, 0.10f, 0.42f), dark);
    mb.addBox(glm::vec3(-0.38f, 0.985f, 0.50f), glm::vec3(0.20f, 0.055f, 0.12f),
              glm::vec3(0.03f));
    mb.addBox(glm::vec3(0.0f, 0.445f, 0.0f), glm::vec3(0.80f, 0.02f, 0.85f), dark);

    for (int s = -1; s <= 1; s += 2) {
        const float x = (float)s * 0.40f;
        mb.addBox(glm::vec3(x, 0.565f, -0.18f), glm::vec3(0.24f, 0.06f, 0.28f), seatCol);
        glm::mat4 back = glm::translate(glm::mat4(1.0f), glm::vec3(x, 0.82f, -0.46f)) *
                         glm::rotate(glm::mat4(1.0f), 0.20f, glm::vec3(1.0f, 0.0f, 0.0f)) *
                         glm::scale(glm::mat4(1.0f), glm::vec3(0.46f, 0.62f, 0.14f));
        mb.addTransformedBox(back, seatCol);
        mb.addBox(glm::vec3(x, 1.135f, -0.545f), glm::vec3(0.15f, 0.10f, 0.08f), seatCol);
    }

    const glm::vec3 hub(-0.40f, 0.855f, 0.40f);
    const int       seg = 18;
    for (int i = 0; i < seg; ++i) {
        const float a = math::kTwoPi * (float)i / (float)seg;
        const glm::vec3 p = hub + glm::vec3(std::cos(a) * 0.175f, std::sin(a) * 0.175f, 0.0f);
        glm::mat4 xf = glm::translate(glm::mat4(1.0f), p) *
                       glm::rotate(glm::mat4(1.0f), a, glm::vec3(0.0f, 0.0f, 1.0f)) *
                       glm::scale(glm::mat4(1.0f), glm::vec3(0.030f, 0.062f, 0.030f));
        mb.addTransformedBox(xf, glm::vec3(0.05f));
    }
    for (int i = 0; i < 3; ++i) {
        const float a = math::kTwoPi * (float)i / 3.0f + 0.5f;
        glm::mat4 xf = glm::translate(glm::mat4(1.0f),
                                      hub + glm::vec3(std::cos(a) * 0.09f, std::sin(a) * 0.09f, 0.0f)) *
                       glm::rotate(glm::mat4(1.0f), a, glm::vec3(0.0f, 0.0f, 1.0f)) *
                       glm::scale(glm::mat4(1.0f), glm::vec3(0.175f, 0.028f, 0.022f));
        mb.addTransformedBox(xf, glm::vec3(0.06f));
    }
    mb.addBox(hub, glm::vec3(0.055f, 0.055f, 0.035f), glm::vec3(0.09f));
    for (int s = -1; s <= 1; s += 2) {
        mb.addBox(glm::vec3((float)s * 0.42f, 1.02f, -0.72f), glm::vec3(0.035f, 0.22f, 0.035f),
                  glm::vec3(0.20f));
    }
    mb.build(m_interior);
}

// ======================================================================= wheel
void CarModel::buildWheel(int quality) {
    MeshBuilder tyre, rim, brake;
    const float R = kWheelRadius;
    const float W = kWheelWidth;

    // 1. Lốp béo NASCAR đen đặc
    const int seg = 18; 
    for (int i = 0; i < seg; ++i) {
        float a = math::kTwoPi * (float)i / (float)seg;
        glm::mat4 xf = glm::rotate(glm::mat4(1.0f), a, glm::vec3(1.0f, 0.0f, 0.0f)) *
                       glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, R * 0.9f, 0.0f)) *
                       glm::scale(glm::mat4(1.0f), glm::vec3(W, R * 0.22f, R * 0.42f));
        tyre.addTransformedBox(xf, glm::vec3(0.07f)); 
    }

    // Yellow/Gold tire sidewall lettering ring accent (matching reference design sheet)
    const glm::vec3 yellowSidewall(0.95f, 0.82f, 0.10f);
    for (int i = 0; i < seg; ++i) {
        float a = math::kTwoPi * (float)i / (float)seg;
        glm::mat4 xf = glm::rotate(glm::mat4(1.0f), a, glm::vec3(1.0f, 0.0f, 0.0f)) *
                       glm::translate(glm::mat4(1.0f), glm::vec3(W * 0.51f, R * 0.78f, 0.0f)) *
                       glm::scale(glm::mat4(1.0f), glm::vec3(0.015f, R * 0.06f, R * 0.25f));
        tyre.addTransformedBox(xf, yellowSidewall);
    }

    // 2. Mâm thép (Deep-Dish Red Steelies) sơn màu ĐỎ ĐẬM chuẩn NASCAR
    const glm::vec3 redRim(0.85f, 0.12f, 0.18f);
    for (int i = 0; i < seg; ++i) {
        float a = math::kTwoPi * (float)i / (float)seg;
        glm::mat4 xf = glm::rotate(glm::mat4(1.0f), a, glm::vec3(1.0f, 0.0f, 0.0f)) *
                       glm::translate(glm::mat4(1.0f), glm::vec3(W * 0.1f, R * 0.65f, 0.0f)) *
                       glm::scale(glm::mat4(1.0f), glm::vec3(W * 0.8f, R * 0.2f, R * 0.4f));
        rim.addTransformedBox(xf, redRim); 
    }
    
    // 3. Trục mâm dĩa lỗ nhô ra màu đen & nắp center cap
    rim.addBox(glm::vec3(W * 0.38f, 0.0f, 0.0f), glm::vec3(0.06f, 0.14f, 0.14f), glm::vec3(0.12f));
    rim.addBox(glm::vec3(W * 0.46f, 0.0f, 0.0f), glm::vec3(0.04f, 0.07f, 0.07f), glm::vec3(0.85f, 0.85f, 0.88f));

    tyre.build(m_tyre);
    rim.build(m_rim);
    brake.build(m_brake);
}

// ======================================================================= build
bool CarModel::build(const ResourceManager& resources, int qualityLevel) {
    m_res = &resources;
    m_trimAccum.clear();

    buildBodyShell(qualityLevel);
    buildAero();
    buildLights();
    buildInterior();
    buildWheel(qualityLevel);
    buildDetails();
    return true;
}

int CarModel::triangleCount() const {
    return m_body.triangleCount() + m_glass.triangleCount() + m_trim.triangleCount() +
           m_chrome.triangleCount() + m_carbon.triangleCount() + m_interior.triangleCount() +
           m_windowNet.triangleCount() + m_sideExhaust.triangleCount() +
           (m_tyre.triangleCount() + m_rim.triangleCount() + m_brake.triangleCount()) * 4;
}

void CarModel::collect(Renderer& renderer, const CarVisualState& st) const {
    const glm::mat4& M = st.transform;

    Material paint = Material::carPaint(glm::vec3(1.0f));
    if (m_res) {
        paint.texture = &m_res->nascarLivery(st.carIndex);
    }
    paint.clearCoat = 0.65f;
    paint.useVertexColor = false;
    renderer.submit(m_body, M, paint);

    if (m_res) {
        Material netMat = Material::surface(glm::vec3(0.1f), 0.8f);
        netMat.texture = &m_res->windowNet();
        netMat.alpha = 0.90f;
        netMat.doubleSided = true;
        netMat.castShadow = false;
        renderer.submit(m_windowNet, M, netMat);
    }

    Material trim = Material::plastic(glm::vec3(0.055f));
    trim.roughness = 0.62f;
    trim.useVertexColor = true;
    trim.polygonOffset  = 1.0f;   
    renderer.submit(m_trim, M, trim);

    Material carbon = Material::carbon();
    if (m_res) {
        carbon.texture = &m_res->carbon();
        carbon.uvScale = 4.0f;
    }
    renderer.submit(m_carbon, M, carbon);

    renderer.submit(m_chrome, M, Material::chrome());
    renderer.submit(m_sideExhaust, M, Material::chrome());

    Material interior = Material::plastic(glm::vec3(0.10f));
    interior.roughness      = 0.78f;
    interior.useVertexColor = true;
    interior.castShadow     = false;
    renderer.submit(m_interior, M, interior);

    Material head = Material::emissiveLight(glm::vec3(0.95f, 0.97f, 1.0f),
                                            st.headlights ? 2.6f : 0.05f);
    renderer.submit(m_headlights, M, head);

    const float tailGlow = 0.55f + st.brakeAmount * 3.6f;
    Material tail = Material::emissiveLight(glm::vec3(1.0f, 0.09f, 0.05f), tailGlow);
    renderer.submit(m_taillights, M, tail);

    Material rev = Material::emissiveLight(glm::vec3(1.0f), st.reversing ? 2.4f : 0.03f);
    renderer.submit(m_reverseLights, M, rev);

    Material tyreMat = Material::rubber();

    Material rimMat = Material::metal(glm::vec3(0.65f, 0.66f, 0.68f), 0.30f);
    rimMat.useVertexColor = true;
    rimMat.clearCoat = 0.25f;

    Material discMat = Material::metal(glm::vec3(0.30f, 0.30f, 0.32f), 0.45f);
    discMat.useVertexColor = false;

    for (int i = 0; i < 4; ++i) {
        const bool front = (i < 2);
        const bool left  = (i % 2 == 0);
        glm::vec3  pos   = m_wheelOffsets[i];
        pos.y -= st.suspension[i];

        glm::mat4 W = M * glm::translate(glm::mat4(1.0f), pos);
        if (front) W = W * glm::rotate(glm::mat4(1.0f), st.steerAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        if (left)  W = W * glm::rotate(glm::mat4(1.0f), math::kPi, glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 spin =
            W * glm::rotate(glm::mat4(1.0f), st.wheelSpin[i], glm::vec3(1.0f, 0.0f, 0.0f));

        renderer.submit(m_tyre, spin, tyreMat);
        renderer.submit(m_rim, spin, rimMat);
        renderer.submit(m_brake, spin, discMat);
    }

    Material glass = Material::glass();
    renderer.submit(m_glass, M, glass);
}

} // namespace vr