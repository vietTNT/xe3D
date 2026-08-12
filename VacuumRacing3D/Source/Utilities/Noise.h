// -----------------------------------------------------------------------------
//  Noise.h - value noise + fBm used by the procedural texture generator.
// -----------------------------------------------------------------------------
#pragma once

namespace vr {
namespace noise {

/// Tileable 2D value noise in [0,1]. `period` must be a power of two for tiling.
float value2D(float x, float y, int period);

/// Fractal Brownian motion built from value2D, result in [0,1].
float fbm2D(float x, float y, int period, int octaves, float lacunarity = 2.0f,
            float gain = 0.5f);

/// Tileable Worley/cellular noise in [0,1] (distance to nearest feature point).
float worley2D(float x, float y, int period);

} // namespace noise
} // namespace vr
