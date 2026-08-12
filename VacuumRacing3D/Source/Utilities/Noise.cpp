#include "Noise.h"

#include <cmath>

namespace vr {
namespace noise {
namespace {

inline int wrapi(int v, int period) {
    v %= period;
    return v < 0 ? v + period : v;
}

inline float hash2(int x, int y) {
    unsigned int h = (unsigned int)(x * 374761393) + (unsigned int)(y * 668265263);
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= (h >> 16);
    return (float)(h & 0x00FFFFFFu) / (float)0x01000000u;
}

inline float fade(float t) { return t * t * (3.0f - 2.0f * t); }

} // namespace

float value2D(float x, float y, int period) {
    if (period < 1) period = 1;
    const int   xi = (int)std::floor(x);
    const int   yi = (int)std::floor(y);
    const float xf = x - (float)xi;
    const float yf = y - (float)yi;

    const float v00 = hash2(wrapi(xi, period), wrapi(yi, period));
    const float v10 = hash2(wrapi(xi + 1, period), wrapi(yi, period));
    const float v01 = hash2(wrapi(xi, period), wrapi(yi + 1, period));
    const float v11 = hash2(wrapi(xi + 1, period), wrapi(yi + 1, period));

    const float u = fade(xf);
    const float v = fade(yf);
    const float a = v00 + (v10 - v00) * u;
    const float b = v01 + (v11 - v01) * u;
    return a + (b - a) * v;
}

float fbm2D(float x, float y, int period, int octaves, float lacunarity, float gain) {
    float sum = 0.0f, amp = 1.0f, norm = 0.0f, freq = 1.0f;
    int   p = period;
    for (int i = 0; i < octaves; ++i) {
        sum += amp * value2D(x * freq, y * freq, p);
        norm += amp;
        amp *= gain;
        freq *= lacunarity;
        p = (int)(p * lacunarity);
        if (p < 1) p = 1;
    }
    return norm > 0.0f ? sum / norm : 0.0f;
}

float worley2D(float x, float y, int period) {
    if (period < 1) period = 1;
    const int xi = (int)std::floor(x);
    const int yi = (int)std::floor(y);
    float best = 1e9f;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            const int cx = xi + dx;
            const int cy = yi + dy;
            const float fx = (float)cx + hash2(wrapi(cx, period), wrapi(cy, period));
            const float fy = (float)cy + hash2(wrapi(cy, period) + 91, wrapi(cx, period) + 17);
            const float ddx = fx - x;
            const float ddy = fy - y;
            const float d2 = ddx * ddx + ddy * ddy;
            if (d2 < best) best = d2;
        }
    }
    const float d = std::sqrt(best);
    return d > 1.0f ? 1.0f : d;
}

} // namespace noise
} // namespace vr
