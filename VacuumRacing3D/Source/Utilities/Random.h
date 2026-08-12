// -----------------------------------------------------------------------------
//  Random.h - deterministic, dependency free RNG (xorshift128).
//  Deterministic seeding keeps AI behaviour and scenery layout reproducible.
// -----------------------------------------------------------------------------
#pragma once

#include <cstdint>

#include <glm/glm.hpp>

namespace vr {

class Random {
public:
    explicit Random(std::uint32_t seed = 0x1337BEEFu) { reseed(seed); }

    void reseed(std::uint32_t seed) {
        if (seed == 0) seed = 0x9E3779B9u;
        m_s[0] = seed;
        m_s[1] = seed ^ 0x9E3779B9u;
        m_s[2] = seed * 2654435761u + 1u;
        m_s[3] = seed ^ 0xC2B2AE35u;
        for (int i = 0; i < 16; ++i) nextUInt();
    }

    std::uint32_t nextUInt() {
        std::uint32_t t = m_s[3];
        const std::uint32_t s = m_s[0];
        m_s[3] = m_s[2];
        m_s[2] = m_s[1];
        m_s[1] = s;
        t ^= t << 11;
        t ^= t >> 8;
        m_s[0] = t ^ s ^ (s >> 19);
        return m_s[0];
    }

    /// Uniform in [0,1).
    float next() { return (float)(nextUInt() >> 8) * (1.0f / 16777216.0f); }
    float range(float lo, float hi) { return lo + (hi - lo) * next(); }
    int   rangeInt(int lo, int hi) {
        if (hi <= lo) return lo;
        return lo + (int)(nextUInt() % (std::uint32_t)(hi - lo));
    }
    bool chance(float p) { return next() < p; }

    glm::vec3 insideUnitSphere() {
        for (int i = 0; i < 8; ++i) {
            glm::vec3 p(range(-1.0f, 1.0f), range(-1.0f, 1.0f), range(-1.0f, 1.0f));
            if (glm::dot(p, p) <= 1.0f) return p;
        }
        return glm::vec3(0.0f);
    }

private:
    std::uint32_t m_s[4];
};

} // namespace vr
