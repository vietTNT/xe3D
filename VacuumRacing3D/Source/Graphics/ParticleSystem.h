// -----------------------------------------------------------------------------
//  ParticleSystem.h - CPU billboard particles (dust, tyre smoke) plus a ring
//  buffer of skid marks laid down on the asphalt.
// -----------------------------------------------------------------------------
#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "Mesh.h"

namespace vr {

class Renderer;
class ResourceManager;

class ParticleSystem {
public:
    static constexpr int kMaxParticles = 1400;
    static constexpr int kMaxSkidMarks = 1600;

    bool init(const ResourceManager& resources, int qualityLevel = 2);
    void shutdown();
    void clear();

    void update(float dt);
    /// Builds the billboards and issues the two extra passes (marks + smoke).
    void render(Renderer& renderer);

    void spawnDust(const glm::vec3& position, const glm::vec3& velocity, float amount);
    void spawnSmoke(const glm::vec3& position, const glm::vec3& velocity, float amount);
    void spawnSparks(const glm::vec3& position, const glm::vec3& normal, float amount);
    void addSkidMark(const glm::vec3& position, const glm::vec3& right, const glm::vec3& segment,
                     float intensity);

    int liveParticles() const { return (int)m_particles.size(); }

private:
    struct Particle {
        glm::vec3 position{0.0f};
        glm::vec3 velocity{0.0f};
        glm::vec3 color{1.0f};
        float     life     = 0.0f;
        float     maxLife  = 1.0f;
        float     size     = 1.0f;
        float     growth   = 1.0f;
        float     alpha    = 1.0f;
        float     drag     = 1.2f;
    };
    struct SkidMark {
        glm::vec3 a{0.0f}, b{0.0f}, c{0.0f}, d{0.0f};
        float     life  = 0.0f;
        float     alpha = 0.0f;
    };

    void emit(const glm::vec3& position, const glm::vec3& velocity, float amount,
              const glm::vec3& color, float size, float growth, float life, float alpha,
              float spread, float rise);

    std::vector<Particle> m_particles;
    std::vector<SkidMark> m_skid;
    int  m_skidCursor = 0;
    bool m_skidDirty  = false;

    Mesh m_billboardMesh;
    Mesh m_skidMesh;

    std::vector<Vertex>        m_scratchVerts;
    std::vector<std::uint32_t> m_scratchIndices;

    const ResourceManager* m_res = nullptr;
    unsigned m_rngState = 12345u;
    int      m_quality  = 2;

    float randf();
};

} // namespace vr
