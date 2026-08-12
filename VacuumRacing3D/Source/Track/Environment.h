// -----------------------------------------------------------------------------
//  Environment.h - everything around the circuit: grandstands, pit buildings,
//  vegetation, lighting rigs, sponsor boards, LED screens, marshal posts and
//  the distant hills that close the horizon.
//
//  Objects are merged into a handful of large meshes (one per material) so the
//  whole scenery costs about a dozen draw calls.
// -----------------------------------------------------------------------------
#pragma once

#include <vector>

#include "../Graphics/Material.h"
#include "../Graphics/Mesh.h"

namespace vr {

class Renderer;
class ResourceManager;
class Track;

class Environment {
public:
    bool build(const Track& track, const ResourceManager& resources, int qualityLevel = 2);
    void collect(Renderer& renderer) const;
    int  triangleCount() const;

private:
    struct Part {
        Mesh     mesh;
        Material material;
    };

    Part& newPart(const Material& material);

    void buildVegetation(const Track& track, int quality);
    void buildGrandstands(const Track& track, const ResourceManager& res);
    void buildLighting(const Track& track, const ResourceManager& res);
    void buildSignage(const Track& track, const ResourceManager& res);
    void buildProps(const Track& track, const ResourceManager& res);
    void buildHorizon(const Track& track);

    std::vector<Part> m_parts;
};

} // namespace vr
