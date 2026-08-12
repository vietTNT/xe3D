#include "Environment.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <glm/gtc/matrix_transform.hpp>

#include "../Graphics/Renderer.h"
#include "../Managers/ResourceManager.h"
#include "../Utilities/MathUtils.h"
#include "../Utilities/Random.h"
#include "Track.h"

namespace vr {
namespace {

/// True when a world position sits too close to ANY part of the circuit.
/// The infield is narrow in places, so an object placed 100 m to one side can
/// easily land on the opposite straight - this is what stops trees, buildings
/// and hills from growing through the track.
bool clearOfTrack(const Track& track, const glm::vec3& p, float minLateral) {
    const TrackQuery q = track.locate(glm::vec3(p.x, 0.0f, p.z), -1);
    return std::fabs(q.lateral) >= minLateral;
}

/// Orthonormal basis aligned with the circuit at a given sample.
glm::mat4 orient(const TrackSample& s) {
    return glm::mat4(glm::vec4(s.right, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
                     glm::vec4(s.forward, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
}

} // namespace

Environment::Part& Environment::newPart(const Material& material) {
    m_parts.emplace_back();
    m_parts.back().material = material;
    return m_parts.back();
}

// =================================================================== vegetation
void Environment::buildVegetation(const Track& track, int quality) {
    Random rng(9021u);

    Material trunkMat = Material::surface(glm::vec3(1.0f), 0.92f);
    trunkMat.useVertexColor = true;
    Material leafMat = Material::surface(glm::vec3(1.0f), 0.88f);
    leafMat.useVertexColor = true;
    leafMat.reflectivity   = 0.18f;

    Part& trunkPart = newPart(trunkMat);
    Part& leafPart  = newPart(leafMat);

    MeshBuilder trunks, leaves;

    const int   rings = (quality >= 2) ? 6 : 4;
    const int   segs  = (quality >= 2) ? 9 : 6;
    const float spacing = (quality >= 2) ? 13.0f : 20.0f;
    const int   count   = (int)(track.length() / spacing);

    for (int i = 0; i < count; ++i) {
        const float d = (float)i * spacing + rng.range(-4.0f, 4.0f);
        const TrackSample s = track.sampleAt(d);

        for (int side = -1; side <= 1; side += 2) {
            if (rng.chance(0.30f)) continue;
            const float lat = (float)side * rng.range(Track::kWallOffset + 16.0f,
                                                      Track::kWallOffset + 105.0f);
            const glm::vec3 base = s.position + s.right * lat;
            const float ground = math::lerpf(s.position.y, track.groundLevel(),
                                             math::saturate((std::fabs(lat) -
                                                             Track::kWallOffset) / 105.0f)) - 0.1f;
            const glm::vec3 p(base.x, ground, base.z);
            // Never let a tree stand on the racing surface of another straight.
            if (!clearOfTrack(track, p, Track::kWallOffset + 6.0f)) continue;

            const int kind = rng.rangeInt(0, 3);
            const float scale = rng.range(0.8f, 1.45f);
            const glm::vec3 leafTint(rng.range(0.10f, 0.26f), rng.range(0.34f, 0.62f),
                                     rng.range(0.09f, 0.22f));

            if (kind == 0) {
                // Tall conifer.
                const float h = 9.5f * scale;
                trunks.addCylinder(p, glm::vec3(0.0f, h * 0.32f, 0.0f), 0.30f * scale,
                                   0.20f * scale, 6, glm::vec3(0.24f, 0.16f, 0.10f));
                for (int t = 0; t < 3; ++t) {
                    const float y = h * (0.26f + 0.20f * (float)t);
                    const float r = (2.6f - 0.65f * (float)t) * scale;
                    leaves.addCone(p + glm::vec3(0.0f, y, 0.0f), r, h * 0.42f, segs, leafTint);
                }
            } else if (kind == 1) {
                // Broadleaf.
                const float h = 7.0f * scale;
                trunks.addCylinder(p, glm::vec3(0.0f, h * 0.55f, 0.0f), 0.32f * scale,
                                   0.24f * scale, 6, glm::vec3(0.28f, 0.19f, 0.12f));
                leaves.addEllipsoid(p + glm::vec3(0.0f, h * 0.82f, 0.0f),
                                    glm::vec3(3.1f, 2.5f, 3.1f) * scale, rings, segs, leafTint);
                leaves.addEllipsoid(p + glm::vec3(1.2f * scale, h * 0.62f, -0.8f * scale),
                                    glm::vec3(1.9f, 1.6f, 1.9f) * scale, rings - 1, segs - 1,
                                    leafTint * 0.9f);
            } else {
                // Bush cluster.
                for (int b = 0; b < 3; ++b) {
                    const glm::vec3 o(rng.range(-1.6f, 1.6f), 0.0f, rng.range(-1.6f, 1.6f));
                    leaves.addEllipsoid(p + o + glm::vec3(0.0f, 0.75f * scale, 0.0f),
                                        glm::vec3(1.3f, 0.95f, 1.3f) * scale, rings - 2, segs - 2,
                                        leafTint * 1.05f);
                }
            }
        }
    }

    // Flower beds inside the run-off area on a few corners.
    for (int i = 0; i < 26; ++i) {
        const float d = rng.range(0.0f, track.length());
        const TrackSample s = track.sampleAt(d);
        const float side = rng.chance(0.5f) ? 1.0f : -1.0f;
        const float lat  = side * rng.range(Track::kWallOffset + 4.0f, Track::kWallOffset + 12.0f);
        const glm::vec3 base = s.position + s.right * lat;
        if (!clearOfTrack(track, base, Track::kWallOffset + 2.0f)) continue;
        for (int f = 0; f < 26; ++f) {
            const glm::vec3 o(rng.range(-3.5f, 3.5f), 0.0f, rng.range(-3.5f, 3.5f));
            const glm::vec3 col = rng.chance(0.5f) ? glm::vec3(0.92f, 0.24f, 0.28f)
                                                   : glm::vec3(0.95f, 0.82f, 0.22f);
            leaves.addEllipsoid(base + o + glm::vec3(0.0f, 0.22f, 0.0f), glm::vec3(0.22f),
                                3, 5, col);
        }
    }

    trunks.build(trunkPart.mesh);
    leaves.build(leafPart.mesh);
}

// ================================================================= grandstands
void Environment::buildGrandstands(const Track& track, const ResourceManager& res) {
    Material concreteMat = Material::surface(glm::vec3(1.0f), 0.85f);
    concreteMat.texture        = &res.concrete();
    concreteMat.useVertexColor = true;
    Material metalMat = Material::metal(glm::vec3(0.70f, 0.72f, 0.75f), 0.40f);
    metalMat.texture        = &res.metal();
    metalMat.useVertexColor = true;
    Material seatMat = Material::plastic(glm::vec3(1.0f));
    seatMat.useVertexColor = true;
    seatMat.roughness      = 0.65f;

    Part& concretePart = newPart(concreteMat);
    Part& metalPart    = newPart(metalMat);
    Part& seatPart     = newPart(seatMat);

    MeshBuilder concrete, metal, seats;
    Random rng(4711u);

    // Grandstands at the start/finish plus a few big braking zones.
    const float positions[5] = {0.02f, 0.20f, 0.42f, 0.63f, 0.84f};
    const float lengths[5]   = {160.0f, 95.0f, 110.0f, 90.0f, 120.0f};

    for (int g = 0; g < 5; ++g) {
        const float dStart = track.startLineDistance() + track.length() * positions[g] - 60.0f;
        const float len    = lengths[g];
        const float side   = (g % 2 == 0) ? -track.insideSign() : track.insideSign();
        const int   bays   = std::max(6, (int)(len / 9.0f));
        const int   rows   = 9;

        for (int b = 0; b < bays; ++b) {
            const float d = dStart + len * ((float)b + 0.5f) / (float)bays;
            const TrackSample s = track.sampleAt(d);
            const glm::mat4 basis = orient(s);
            const float baseLat = side * (Track::kWallOffset + 12.0f);
            const glm::vec3 origin = s.position + s.right * baseLat;
            const float ground = 0.0f;

            // Stepped seating deck.
            for (int r = 0; r < rows; ++r) {
                const float depth = (float)r * 1.25f;
                const float h     = 0.9f + (float)r * 0.95f;
                const glm::vec3 c = origin + s.right * (side * depth) +
                                    glm::vec3(0.0f, ground + h * 0.5f, 0.0f);
                concrete.append(MeshBuilder{}, glm::mat4(1.0f));
                MeshBuilder step;
                step.addBox(glm::vec3(0.0f), glm::vec3(len / (float)bays * 0.5f, h * 0.5f, 0.62f),
                            glm::vec3(0.86f));
                concrete.append(step, glm::translate(glm::mat4(1.0f), c) * basis);

                // Seats: alternating team colours make the stands feel alive.
                const int seatCount = 7;
                for (int k = 0; k < seatCount; ++k) {
                    const float off = ((float)k / (float)(seatCount - 1) - 0.5f) *
                                      (len / (float)bays * 0.86f);
                    glm::vec3 col = ((r + k + b) % 3 == 0) ? glm::vec3(0.82f, 0.16f, 0.14f)
                                    : (((r + k) % 3 == 1) ? glm::vec3(0.92f, 0.92f, 0.94f)
                                                          : glm::vec3(0.13f, 0.26f, 0.62f));
                    if (rng.chance(0.12f)) col = glm::vec3(0.95f, 0.78f, 0.10f);
                    MeshBuilder seat;
                    seat.addBox(glm::vec3(off, 0.0f, 0.0f), glm::vec3(0.28f, 0.16f, 0.22f), col);
                    seats.append(seat, glm::translate(glm::mat4(1.0f),
                                                      c + glm::vec3(0.0f, h * 0.5f + 0.18f, 0.0f)) *
                                           basis);
                }
            }

            // Roof and supporting columns.
            const float roofH = 0.9f + (float)rows * 0.95f + 3.4f;
            MeshBuilder roof;
            roof.addBox(glm::vec3(0.0f, 0.0f, 0.0f),
                        glm::vec3(len / (float)bays * 0.52f, 0.16f, 7.2f),
                        glm::vec3(0.78f, 0.79f, 0.82f));
            // Set back from the track so the roof never overhangs the barriers.
            metal.append(roof, glm::translate(glm::mat4(1.0f),
                                              origin + s.right * (side * 7.5f) +
                                                  glm::vec3(0.0f, roofH, 0.0f)) *
                                   basis);
            for (int c2 = -1; c2 <= 1; c2 += 2) {
                MeshBuilder col;
                col.addBox(glm::vec3(0.0f), glm::vec3(0.22f, roofH * 0.5f, 0.22f),
                           glm::vec3(0.62f));
                metal.append(col, glm::translate(glm::mat4(1.0f),
                                                 origin + s.right * (side * 11.0f) +
                                                     glm::vec3(0.0f, roofH * 0.5f, 0.0f) +
                                                     s.forward * ((float)c2 * len /
                                                                  (float)bays * 0.45f)) *
                                      basis);
            }
        }
    }

    concrete.build(concretePart.mesh);
    metal.build(metalPart.mesh);
    seats.build(seatPart.mesh);
}

// ==================================================================== lighting
void Environment::buildLighting(const Track& track, const ResourceManager& res) {
    Material poleMat = Material::metal(glm::vec3(0.62f, 0.64f, 0.68f), 0.45f);
    poleMat.texture        = &res.metal();
    poleMat.useVertexColor = true;
    Material lampMat = Material::emissiveLight(glm::vec3(1.0f, 0.96f, 0.86f), 0.55f);

    Part& polePart = newPart(poleMat);
    Part& lampPart = newPart(lampMat);

    MeshBuilder poles, lamps;
    const float spacing = 95.0f;
    const int   count   = (int)(track.length() / spacing);

    for (int i = 0; i < count; ++i) {
        const float d = (float)i * spacing;
        const TrackSample s = track.sampleAt(d);
        const float side = (i % 2 == 0) ? 1.0f : -1.0f;
        const glm::vec3 base = s.position + s.right * (side * (Track::kWallOffset + 6.5f));
        const glm::mat4 basis = orient(s);
        const float h = 17.0f;

        poles.addCylinder(base, glm::vec3(0.0f, h, 0.0f), 0.34f, 0.20f, 8, glm::vec3(0.72f));
        MeshBuilder head;
        head.addBox(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(2.6f, 0.18f, 0.5f), glm::vec3(0.5f));
        poles.append(head, glm::translate(glm::mat4(1.0f), base + glm::vec3(0.0f, h, 0.0f)) *
                               basis);
        for (int k = -1; k <= 1; ++k) {
            MeshBuilder lamp;
            lamp.addBox(glm::vec3((float)k * 1.5f, -0.25f, 0.0f), glm::vec3(0.55f, 0.14f, 0.35f),
                        glm::vec3(1.0f));
            lamps.append(lamp, glm::translate(glm::mat4(1.0f), base + glm::vec3(0.0f, h, 0.0f)) *
                                   basis);
        }
    }

    poles.build(polePart.mesh);
    lamps.build(lampPart.mesh);
}

// ===================================================================== signage
void Environment::buildSignage(const Track& track, const ResourceManager& res) {
    // One merged mesh per sponsor texture keeps the draw call count tiny.
    const int boards = res.sponsorCount();
    std::vector<MeshBuilder> builders((size_t)boards);
    std::vector<Part*>       parts((size_t)boards);
    for (int i = 0; i < boards; ++i) {
        Material m = Material::surface(glm::vec3(1.0f), 0.55f);
        m.texture        = &res.sponsor(i);
        m.useVertexColor = false;
        m.doubleSided    = true;
        parts[(size_t)i] = &newPart(m);
    }

    const float spacing = 26.0f;
    const int   count   = (int)(track.length() / spacing);
    for (int i = 0; i < count; ++i) {
        const float d = (float)i * spacing;
        const TrackSample s = track.sampleAt(d);
        for (int side = -1; side <= 1; side += 2) {
            const int idx = (i * 3 + (side > 0 ? 1 : 2)) % boards;
            const glm::vec3 c = s.position + s.right * ((float)side * (Track::kWallOffset + 0.35f)) +
                                glm::vec3(0.0f, 0.62f, 0.0f);
            const glm::vec3 n = s.right * (float)-side;
            const glm::vec3 f = s.forward * (spacing * 0.5f);
            const glm::vec3 up(0.0f, 0.52f, 0.0f);
            MeshBuilder& mb = builders[(size_t)idx];
            if (side > 0) {
                mb.addQuadUV(c - f - up, c + f - up, c + f + up, c - f + up, glm::vec2(0.0f, 1.0f),
                             glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 0.0f),
                             glm::vec3(1.0f));
            } else {
                mb.addQuadUV(c + f - up, c - f - up, c - f + up, c + f + up, glm::vec2(0.0f, 1.0f),
                             glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 0.0f),
                             glm::vec3(1.0f));
            }
            (void)n;
        }
    }
    for (int i = 0; i < boards; ++i) builders[(size_t)i].build(parts[(size_t)i]->mesh);

    // ---- two big LED screens facing the main straight
    Material frameMat = Material::plastic(glm::vec3(0.10f));
    Part&    framePart = newPart(frameMat);
    Material screenMat = Material::emissiveLight(glm::vec3(0.35f, 0.55f, 0.95f), 1.15f);
    Part&    screenPart = newPart(screenMat);
    MeshBuilder frames, screens;

    for (int k = 0; k < 2; ++k) {
        const float d = track.startLineDistance() + (k == 0 ? -95.0f : 130.0f);
        const TrackSample s = track.sampleAt(d);
        const float side = -track.insideSign();
        const glm::vec3 base = s.position + s.right * (side * (Track::kWallOffset + 15.0f));
        const glm::mat4 basis = orient(s);

        MeshBuilder legs;
        for (int c2 = -1; c2 <= 1; c2 += 2) {
            legs.addBox(glm::vec3((float)c2 * 4.2f, 3.4f, 0.0f), glm::vec3(0.3f, 3.4f, 0.3f),
                        glm::vec3(0.35f));
        }
        legs.addBox(glm::vec3(0.0f, 9.6f, 0.0f), glm::vec3(5.2f, 3.2f, 0.45f), glm::vec3(0.12f));
        frames.append(legs, glm::translate(glm::mat4(1.0f), base) * basis);

        MeshBuilder face;
        face.addBox(glm::vec3(0.0f, 9.6f, -side * 0.5f), glm::vec3(4.8f, 2.8f, 0.08f),
                    glm::vec3(1.0f));
        screens.append(face, glm::translate(glm::mat4(1.0f), base) * basis);
    }
    frames.build(framePart.mesh);
    screens.build(screenPart.mesh);
}

// ======================================================================= props
void Environment::buildProps(const Track& track, const ResourceManager& res) {
    Material propMat = Material::surface(glm::vec3(1.0f), 0.7f);
    propMat.useVertexColor = true;
    Part&       part = newPart(propMat);
    MeshBuilder mb;
    Random      rng(2211u);

    // Marshal posts every ~230 m: a small platform with an orange shelter.
    const int posts = (int)(track.length() / 230.0f);
    for (int i = 0; i < posts; ++i) {
        const float d = (float)i * 230.0f + 40.0f;
        const TrackSample s = track.sampleAt(d);
        const float side = (i % 2 == 0) ? -1.0f : 1.0f;
        const glm::vec3 base = s.position + s.right * (side * (Track::kWallOffset + 3.2f));
        const glm::mat4 basis = orient(s);
        MeshBuilder post;
        post.addBox(glm::vec3(0.0f, 1.3f, 0.0f), glm::vec3(1.4f, 1.3f, 1.1f),
                    glm::vec3(0.85f, 0.85f, 0.88f));
        post.addBox(glm::vec3(0.0f, 2.75f, 0.0f), glm::vec3(1.6f, 0.14f, 1.3f),
                    glm::vec3(0.92f, 0.45f, 0.08f));
        mb.append(post, glm::translate(glm::mat4(1.0f), base) * basis);
    }

    // Distance boards before the two heaviest braking zones.
    const float brakingPoints[2] = {0.30f, 0.72f};
    for (int b = 0; b < 2; ++b) {
        for (int m = 3; m >= 1; --m) {
            const float d = track.length() * brakingPoints[b] - (float)m * 50.0f;
            const TrackSample s = track.sampleAt(d);
            const glm::vec3 base = s.position + s.right * (Track::kWallOffset + 1.6f) *
                                                    track.insideSign();
            const glm::mat4 basis = orient(s);
            MeshBuilder board;
            board.addBox(glm::vec3(0.0f, 1.6f, 0.0f), glm::vec3(0.9f, 0.65f, 0.06f),
                         glm::vec3(0.95f));
            for (int k = 0; k < m; ++k) {
                board.addBox(glm::vec3(-0.45f + (float)k * 0.45f, 1.6f, -0.09f),
                             glm::vec3(0.13f, 0.42f, 0.03f), glm::vec3(0.05f));
            }
            board.addBox(glm::vec3(0.0f, 0.55f, 0.0f), glm::vec3(0.09f, 0.55f, 0.09f),
                         glm::vec3(0.35f));
            mb.append(board, glm::translate(glm::mat4(1.0f), base) * basis);
        }
    }

    // Safety cones scattered along the run-off of a few corners.
    for (int i = 0; i < 90; ++i) {
        const float d = rng.range(0.0f, track.length());
        const TrackSample s = track.sampleAt(d);
        const float side = rng.chance(0.5f) ? 1.0f : -1.0f;
        // Behind the barriers: a cone on the run-off would be driven through.
        const float lat  = side * rng.range(Track::kWallOffset + 1.8f, Track::kWallOffset + 7.0f);
        const glm::vec3 base = s.position + s.right * lat;
        if (!clearOfTrack(track, base, Track::kWallOffset + 1.5f)) continue;
        mb.addCone(base, 0.24f, 0.62f, 8, glm::vec3(0.95f, 0.42f, 0.06f));
        mb.addBox(base + glm::vec3(0.0f, 0.03f, 0.0f), glm::vec3(0.30f, 0.03f, 0.30f),
                  glm::vec3(0.9f));
    }

    // Service buildings in the infield.
    for (int i = 0; i < 5; ++i) {
        const float d = track.length() * ((float)i / 5.0f + 0.06f);
        const TrackSample s = track.sampleAt(d);
        const glm::vec3 base = s.position +
                               s.right * (track.insideSign() * rng.range(52.0f, 96.0f));
        const glm::mat4 basis = orient(s);
        MeshBuilder building;
        const float w = rng.range(9.0f, 18.0f);
        const float h = rng.range(4.0f, 8.0f);
        const float dp = rng.range(8.0f, 14.0f);
        // Give the building its own footprint clearance from every straight.
        if (!clearOfTrack(track, base, Track::kWallOffset + 14.0f + w * 0.5f)) continue;
        building.addBox(glm::vec3(0.0f, h * 0.5f, 0.0f), glm::vec3(w * 0.5f, h * 0.5f, dp * 0.5f),
                        glm::vec3(0.82f, 0.83f, 0.85f));
        building.addBox(glm::vec3(0.0f, h + 0.2f, 0.0f), glm::vec3(w * 0.54f, 0.2f, dp * 0.54f),
                        glm::vec3(0.35f, 0.36f, 0.40f));
        for (int wdw = 0; wdw < 4; ++wdw) {
            building.addBox(glm::vec3(-w * 0.32f + (float)wdw * w * 0.21f, h * 0.62f,
                                      dp * 0.5f + 0.02f),
                            glm::vec3(w * 0.07f, 0.6f, 0.04f), glm::vec3(0.18f, 0.24f, 0.32f));
        }
        // Stand the building on the blended terrain rather than at y = 0.
        const float lat = std::fabs(glm::dot(base - s.position, s.right));
        const float groundY = math::lerpf(s.position.y, track.groundLevel(),
                                          math::saturate((lat - Track::kWallOffset) / 105.0f));
        mb.append(building,
                  glm::translate(glm::mat4(1.0f), glm::vec3(base.x, groundY, base.z)) * basis);
    }
    (void)res;
    mb.build(part.mesh);
}

// ===================================================================== horizon
void Environment::buildHorizon(const Track& track) {
    Material mat = Material::surface(glm::vec3(1.0f), 0.95f);
    mat.useVertexColor = true;
    mat.castShadow     = false;
    mat.reflectivity   = 0.1f;
    Part&       part = newPart(mat);
    MeshBuilder mb;
    Random      rng(777u);

    const glm::vec3 c = track.centroid();

    // Hills must be far from the CIRCUIT, not from its centre. The centre can be
    // 1.5 km away from where the player actually drives, so measuring from it put
    // ridges a few hundred metres off the track: they blocked out the sky and the
    // cars appeared to drive into them. For every direction we therefore measure
    // how far the tarmac itself reaches inside that corridor.
    const std::vector<TrackSample>& samples = track.samples();
    const float kClearance = 1500.0f;   // metres of open ground before the hills

    float minDistanceToTrack = 1e9f;

    for (int i = 0; i < 44; ++i) {
        const float a = math::kTwoPi * (float)i / 44.0f + rng.range(-0.04f, 0.04f);
        const glm::vec2 dir(std::cos(a), std::sin(a));

        // Low, wide ridges: a 400 m peak looks like a wall from a kilometre away.
        const float h = rng.range(110.0f, 245.0f);
        const float w = rng.range(320.0f, 720.0f);
        const float corridor = w * 0.6f + 260.0f;

        // Furthest the circuit reaches along this direction, within the corridor.
        float reachAlong = 0.0f;
        for (size_t s = 0; s < samples.size(); s += 3) {
            const glm::vec2 d(samples[s].position.x - c.x, samples[s].position.z - c.z);
            const float proj = glm::dot(d, dir);
            if (proj <= 0.0f) continue;
            const float perp = glm::length(d - dir * proj);
            if (perp < corridor) reachAlong = std::max(reachAlong, proj);
        }

        const float r = reachAlong + w * 0.5f + kClearance + rng.range(0.0f, 900.0f);
        const glm::vec3 p(c.x + dir.x * r, track.groundLevel() - 18.0f, c.z + dir.y * r);

        // Verify against every sample, so nothing can slip through the corridor test.
        float nearest = 1e9f;
        for (size_t s = 0; s < samples.size(); s += 3) {
            const glm::vec2 d(samples[s].position.x - p.x, samples[s].position.z - p.z);
            nearest = std::min(nearest, glm::length(d) - w * 0.5f);
        }
        if (nearest < kClearance * 0.6f) continue;
        minDistanceToTrack = std::min(minDistanceToTrack, nearest);

        // Hazy blue-green so the ridges read as distance, not as objects.
        const glm::vec3 col = glm::mix(glm::vec3(0.34f, 0.42f, 0.38f),
                                       glm::vec3(0.55f, 0.62f, 0.70f), rng.next() * 0.85f);
        mb.addCone(p, w * 0.5f, h, 9, col);
    }

    std::printf("[Environment] nearest hill is %.0f m of clear ground from the circuit\n",
                minDistanceToTrack);
    mb.build(part.mesh);
}

// ======================================================================= build
bool Environment::build(const Track& track, const ResourceManager& resources, int qualityLevel) {
    m_parts.clear();
    m_parts.reserve(32);

    buildVegetation(track, qualityLevel);
    buildGrandstands(track, resources);
    buildLighting(track, resources);
    buildSignage(track, resources);
    buildProps(track, resources);
    buildHorizon(track);

    std::printf("[Environment] %d meshes, %d triangles\n", (int)m_parts.size(), triangleCount());
    return true;
}

int Environment::triangleCount() const {
    int t = 0;
    for (const Part& p : m_parts) t += p.mesh.triangleCount();
    return t;
}

void Environment::collect(Renderer& renderer) const {
    const glm::mat4 identity(1.0f);
    for (const Part& p : m_parts) renderer.submit(p.mesh, identity, p.material);
}

} // namespace vr
