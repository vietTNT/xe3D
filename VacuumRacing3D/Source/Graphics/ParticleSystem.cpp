#include "ParticleSystem.h"

#include <algorithm>
#include <cmath>

#include "../Managers/ResourceManager.h"
#include "../Utilities/MathUtils.h"
#include "Renderer.h"

namespace vr {

float ParticleSystem::randf() {
    m_rngState = m_rngState * 1664525u + 1013904223u;
    return (float)((m_rngState >> 8) & 0xFFFFFF) / (float)0x1000000;
}

bool ParticleSystem::init(const ResourceManager& resources, int qualityLevel) {
    m_res     = &resources;
    m_quality = qualityLevel;
    m_particles.reserve(kMaxParticles);
    m_skid.assign(kMaxSkidMarks, SkidMark{});
    m_skidCursor = 0;
    return true;
}

void ParticleSystem::shutdown() {
    m_billboardMesh.destroy();
    m_skidMesh.destroy();
    m_particles.clear();
    m_skid.clear();
}

void ParticleSystem::clear() {
    m_particles.clear();
    for (SkidMark& s : m_skid) s.life = 0.0f;
    m_skidDirty = true;
}

void ParticleSystem::emit(const glm::vec3& position, const glm::vec3& velocity, float amount,
                          const glm::vec3& color, float size, float growth, float life,
                          float alpha, float spread, float rise) {
    if (m_quality == 0) amount *= 0.45f;
    int count = (int)amount;
    if (randf() < amount - (float)count) ++count;

    for (int i = 0; i < count; ++i) {
        if ((int)m_particles.size() >= kMaxParticles) return;
        Particle p;
        p.position = position + glm::vec3((randf() - 0.5f) * 0.35f, randf() * 0.12f,
                                          (randf() - 0.5f) * 0.35f);
        p.velocity = velocity + glm::vec3((randf() - 0.5f) * spread, rise * (0.4f + randf()),
                                          (randf() - 0.5f) * spread);
        p.color    = color * (0.85f + 0.3f * randf());
        p.maxLife  = life * (0.75f + 0.5f * randf());
        p.life     = p.maxLife;
        p.size     = size * (0.8f + 0.5f * randf());
        p.growth   = growth;
        p.alpha    = alpha;
        m_particles.push_back(p);
    }
}

void ParticleSystem::spawnDust(const glm::vec3& position, const glm::vec3& velocity,
                               float amount) {
    emit(position, velocity, amount, glm::vec3(0.62f, 0.57f, 0.48f), 0.35f, 1.9f, 0.85f, 0.30f,
         1.4f, 1.1f);
}

void ParticleSystem::spawnSmoke(const glm::vec3& position, const glm::vec3& velocity,
                                float amount) {
    emit(position, velocity, amount, glm::vec3(0.80f, 0.80f, 0.82f), 0.45f, 2.6f, 1.35f, 0.26f,
         1.0f, 1.5f);
}

void ParticleSystem::spawnSparks(const glm::vec3& position, const glm::vec3& normal,
                                 float amount) {
    emit(position, normal * 2.5f, amount, glm::vec3(1.0f, 0.72f, 0.25f), 0.09f, 0.6f, 0.42f, 0.9f,
         3.0f, 1.6f);
}

void ParticleSystem::addSkidMark(const glm::vec3& position, const glm::vec3& right,
                                 const glm::vec3& segment, float intensity) {
    if (intensity < 0.06f) return;
    glm::vec3 seg = segment;
    const float segLen = glm::length(seg);
    if (segLen < 0.05f) return;
    if (segLen > 4.0f) seg *= 4.0f / segLen;

    SkidMark& m = m_skid[(size_t)m_skidCursor];
    m_skidCursor = (m_skidCursor + 1) % kMaxSkidMarks;

    const glm::vec3 lift(0.0f, 0.035f, 0.0f);
    const glm::vec3 w = right * 0.13f;
    m.a = position - w + lift;
    m.b = position + w + lift;
    m.c = position + w + seg + lift;
    m.d = position - w + seg + lift;
    m.life  = 26.0f;
    m.alpha = math::saturate(intensity * 0.75f);
    m_skidDirty = true;
}

void ParticleSystem::update(float dt) {
    for (size_t i = 0; i < m_particles.size();) {
        Particle& p = m_particles[i];
        p.life -= dt;
        if (p.life <= 0.0f) {
            m_particles[i] = m_particles.back();
            m_particles.pop_back();
            continue;
        }
        p.velocity *= std::exp(-p.drag * dt);
        p.velocity.y += 0.35f * dt;      // warm air lift
        p.position += p.velocity * dt;
        ++i;
    }

    for (SkidMark& m : m_skid) {
        if (m.life > 0.0f) {
            m.life -= dt;
            if (m.life <= 0.0f) m_skidDirty = true;
        }
    }
}

void ParticleSystem::render(Renderer& renderer) {
    const CameraView& cam = renderer.camera();

    // ---------------------------------------------------------- skid marks
    if (m_skidDirty) {
        m_scratchVerts.clear();
        m_scratchIndices.clear();
        for (const SkidMark& m : m_skid) {
            if (m.life <= 0.0f) continue;
            const float fade = math::saturate(m.life / 6.0f) * m.alpha;
            const std::uint32_t base = (std::uint32_t)m_scratchVerts.size();
            const glm::vec3 col(0.05f, 0.05f, 0.055f);
            const glm::vec3 nrm(fade, 0.0f, 0.0f);   // alpha travels in normal.x
            m_scratchVerts.push_back({m.a, nrm, glm::vec2(0.0f, 0.0f), col});
            m_scratchVerts.push_back({m.b, nrm, glm::vec2(1.0f, 0.0f), col});
            m_scratchVerts.push_back({m.c, nrm, glm::vec2(1.0f, 1.0f), col});
            m_scratchVerts.push_back({m.d, nrm, glm::vec2(0.0f, 1.0f), col});
            m_scratchIndices.push_back(base + 0);
            m_scratchIndices.push_back(base + 1);
            m_scratchIndices.push_back(base + 2);
            m_scratchIndices.push_back(base + 0);
            m_scratchIndices.push_back(base + 2);
            m_scratchIndices.push_back(base + 3);
        }
        m_skidMesh.updateDynamic(m_scratchVerts, m_scratchIndices);
        m_skidDirty = false;
    }

    // ---------------------------------------------------------- billboards
    m_scratchVerts.clear();
    m_scratchIndices.clear();
    const glm::vec3 camRight(cam.view[0][0], cam.view[1][0], cam.view[2][0]);
    const glm::vec3 camUp(cam.view[0][1], cam.view[1][1], cam.view[2][1]);

    for (const Particle& p : m_particles) {
        const float t   = 1.0f - math::saturate(p.life / p.maxLife);
        const float sz  = p.size * (1.0f + p.growth * t);
        const float a   = p.alpha * (1.0f - t) * math::smoothstepf(0.0f, 0.18f, t + 0.18f);
        const glm::vec3 r = camRight * sz;
        const glm::vec3 u = camUp * sz;
        const std::uint32_t base = (std::uint32_t)m_scratchVerts.size();
        const glm::vec3 nrm(a, 0.0f, 0.0f);
        m_scratchVerts.push_back({p.position - r - u, nrm, glm::vec2(0.0f, 0.0f), p.color});
        m_scratchVerts.push_back({p.position + r - u, nrm, glm::vec2(1.0f, 0.0f), p.color});
        m_scratchVerts.push_back({p.position + r + u, nrm, glm::vec2(1.0f, 1.0f), p.color});
        m_scratchVerts.push_back({p.position - r + u, nrm, glm::vec2(0.0f, 1.0f), p.color});
        m_scratchIndices.push_back(base + 0);
        m_scratchIndices.push_back(base + 1);
        m_scratchIndices.push_back(base + 2);
        m_scratchIndices.push_back(base + 0);
        m_scratchIndices.push_back(base + 2);
        m_scratchIndices.push_back(base + 3);
    }
    m_billboardMesh.updateDynamic(m_scratchVerts, m_scratchIndices);

    // ------------------------------------------------------------- draw
    renderer.bindSceneTarget();
    Shader& shader = renderer.particleShader();
    shader.bind();
    shader.setMat4("uViewProj", cam.viewProjection);
    shader.setInt("uSprite", 0);
    shader.setVec3("uFogColor", renderer.fogColor);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    if (m_skidMesh.valid() && m_res) {
        shader.setFloat("uFogAmount", 0.0f);
        m_res->glow().bind(0);
        // A soft round sprite keeps the mark edges from looking like hard quads.
        m_skidMesh.draw();
    }
    if (m_billboardMesh.valid() && m_res) {
        shader.setFloat("uFogAmount", 0.18f);
        m_res->smoke().bind(0);
        m_billboardMesh.draw();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}

} // namespace vr
