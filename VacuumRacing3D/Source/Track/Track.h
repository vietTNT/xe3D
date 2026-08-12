// -----------------------------------------------------------------------------
//  Track.h - the racing circuit: spline sampling, geometry generation, surface
//  queries, racing line, checkpoints, starting grid and the pit lane.
// -----------------------------------------------------------------------------
#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "../Graphics/Material.h"
#include "../Graphics/Mesh.h"

namespace vr {

class Renderer;
class ResourceManager;

/// One arc-length sample of the circuit centre line.
struct TrackSample {
    glm::vec3 position{0.0f};   ///< centre line point (world space)
    glm::vec3 forward{0.0f, 0.0f, 1.0f};
    glm::vec3 right{1.0f, 0.0f, 0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    float     distance  = 0.0f; ///< arc length from the start of the loop
    float     curvature = 0.0f; ///< signed: positive curves toward +right
    float     bank      = 0.0f; ///< banking angle in radians
    float     lineOffset = 0.0f;///< racing line lateral offset
    float     lineSpeed  = 0.0f;///< suggested speed on the racing line (m/s)
};

/// Result of projecting a world position onto the circuit.
struct TrackQuery {
    int       index      = 0;      ///< nearest sample index
    float     distance   = 0.0f;   ///< arc length at the projection
    float     lateral    = 0.0f;   ///< signed offset along sample.right
    float     surfaceY   = 0.0f;   ///< ground height under the query point
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec3 forward{0.0f, 0.0f, 1.0f};
    bool      onAsphalt  = true;
};

struct TrackPart {
    Mesh     mesh;
    Material material;
};

class Track {
public:
    static constexpr float kHalfWidth   = 7.2f;   ///< 14.4 m racing surface
    static constexpr float kCurbWidth   = 1.25f;
    static constexpr float kRunoff      = 9.5f;
    static constexpr float kWallOffset  = kHalfWidth + kCurbWidth + kRunoff;
    static constexpr int   kCheckpoints = 24;
    static constexpr int   kGridSlots   = 8;

    bool build(const ResourceManager& resources, int qualityLevel = 2);
    void collect(Renderer& renderer) const;
    /// Draws the five start lights with the emissive state of the countdown.
    void collectStartLights(Renderer& renderer, int litCount, bool greenPhase) const;

    // ---------------------------------------------------------------- queries
    const std::vector<TrackSample>& samples() const { return m_samples; }
    float length() const { return m_length; }
    int   sampleCount() const { return (int)m_samples.size(); }
    float sampleSpacing() const { return m_spacing; }

    /// Interpolated sample at an arbitrary arc length (wraps automatically).
    TrackSample sampleAt(float distance) const;
    int         indexAt(float distance) const;
    /// Projects a world position onto the circuit. `hint` speeds the search up.
    TrackQuery  locate(const glm::vec3& position, int hint = -1) const;
    /// Surface point for a given arc length + lateral offset (includes banking).
    glm::vec3   surfacePoint(float distance, float lateral) const;

    /// Racing line helpers used by the AI.
    glm::vec3 racingLinePoint(float distance) const;
    float     racingLineSpeed(float distance) const;

    // ------------------------------------------------------------- race setup
    float     startLineDistance() const { return m_startLine; }
    glm::vec3 gridPosition(int slot) const;
    float     gridYaw(int slot) const;
    float     checkpointDistance(int index) const;

    const std::vector<glm::vec2>& minimapPoints() const { return m_minimap; }
    glm::vec2                     minimapMin() const { return m_minimapMin; }
    glm::vec2                     minimapMax() const { return m_minimapMax; }
    glm::vec3                     centroid() const { return m_centroid; }
    /// Height of the surrounding terrain. Always below the lowest point of the
    /// circuit, so the ground can never end up above the camera.
    float groundLevel() const { return m_groundLevel; }
    float lowestPoint() const { return m_minElevation; }
    float highestPoint() const { return m_maxElevation; }
    /// +1 or -1: which side of the track the infield (and the pit lane) is on.
    float insideSign() const { return m_insideSign; }

private:
    void generateSamples();
    void computeRacingLine();
    void findMainStraight();
    void buildAsphalt(const ResourceManager& res);
    void buildMarkings();
    void buildCurbs();
    void buildRunoffAndGrass(const ResourceManager& res);
    void buildBarriers(const ResourceManager& res);
    void buildStartArea(const ResourceManager& res);
    void buildPitLane(const ResourceManager& res);
    void buildContinuousCurbs(const ResourceManager& res);
    void buildUnderTrack(const ResourceManager& res);
    void buildMinimap();
    void debugRacingLine() const;

    TrackPart& newPart(const Material& material);

    std::vector<TrackSample> m_samples;
    std::vector<TrackPart>   m_parts;
    std::vector<glm::vec2>   m_minimap;
    Mesh                     m_startLights[5];

    float     m_length     = 0.0f;
    float     m_spacing    = 3.0f;
    float     m_startLine  = 0.0f;
    float     m_straightBegin = 0.0f;
    float     m_straightEnd   = 0.0f;
    float     m_insideSign = 1.0f;
    float     m_minElevation = 0.0f;
    float     m_maxElevation = 0.0f;
    float     m_groundLevel  = -3.0f;
    glm::vec3 m_centroid{0.0f};
    glm::vec2 m_minimapMin{0.0f};
    glm::vec2 m_minimapMax{0.0f};
    int       m_quality = 2;
};

} // namespace vr
