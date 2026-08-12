#include "Renderer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Core/Paths.h"
#include "../Utilities/MathUtils.h"

namespace vr {

bool Renderer::init(int framebufferWidth, int framebufferHeight, int qualityLevel) {
    m_width  = std::max(framebufferWidth, 1);
    m_height = std::max(framebufferHeight, 1);
    setQualityLevel(qualityLevel);

    const bool ok =
        m_sceneShader.loadFromFiles(paths::resolve("Shaders/scene.vert"),
                                    paths::resolve("Shaders/scene.frag")) &&
        m_depthShader.loadFromFiles(paths::resolve("Shaders/depth.vert"),
                                    paths::resolve("Shaders/depth.frag")) &&
        m_skyShader.loadFromFiles(paths::resolve("Shaders/sky.vert"),
                                  paths::resolve("Shaders/sky.frag")) &&
        m_postShader.loadFromFiles(paths::resolve("Shaders/post.vert"),
                                   paths::resolve("Shaders/post.frag")) &&
        m_particleShader.loadFromFiles(paths::resolve("Shaders/particle.vert"),
                                       paths::resolve("Shaders/particle.frag"));
    if (!ok) {
        std::fprintf(stderr, "[Renderer] shader loading failed.\n");
        return false;
    }

    const std::vector<unsigned char> px = procedural::white(2);
    m_whiteTexture.createRGBA(2, 2, px.data(), false, true, false);

    if (!m_sceneTarget.create(m_width, m_height)) return false;
    if (!m_shadowTarget.create(m_shadowRes)) return false;

    glGenVertexArrays(1, &m_emptyVAO);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    m_ready = true;
    return true;
}

void Renderer::shutdown() {
    if (m_emptyVAO && glDeleteVertexArrays) glDeleteVertexArrays(1, &m_emptyVAO);
    m_emptyVAO = 0;
    m_sceneTarget.destroy();
    m_shadowTarget.destroy();
    m_whiteTexture.destroy();
    m_sceneShader.destroy();
    m_depthShader.destroy();
    m_skyShader.destroy();
    m_postShader.destroy();
    m_particleShader.destroy();
    m_ready = false;
}

void Renderer::resize(int framebufferWidth, int framebufferHeight) {
    const int w = std::max(framebufferWidth, 1);
    const int h = std::max(framebufferHeight, 1);
    if (w == m_width && h == m_height) return;
    m_width  = w;
    m_height = h;
    if (m_ready) m_sceneTarget.create(w, h);
}

void Renderer::setQualityLevel(int level) {
    m_quality = std::max(0, std::min(2, level));
    const int res = (m_quality == 0) ? 1024 : (m_quality == 1 ? 2048 : 3072);
    if (res != m_shadowRes) {
        m_shadowRes = res;
        if (m_ready) m_shadowTarget.create(m_shadowRes);
    }
}

void Renderer::beginFrame(const CameraView& camera, const DirectionalLight& light,
                          const glm::vec3& shadowFocus) {
    m_camera = camera;
    m_light  = light;
    m_opaque.clear();
    m_transparent.clear();
    m_stats = RenderStats{};
    buildFrustum(m_camera.viewProjection);
    computeLightMatrix(shadowFocus);
}

void Renderer::submit(const Mesh& mesh, const glm::mat4& model, const Material& material) {
    if (!mesh.valid()) return;
    ++m_stats.submitted;

    // Conservative world space bounding sphere.
    const glm::vec3 center = glm::vec3(model * glm::vec4(mesh.boundsCenter, 1.0f));
    const float sx = glm::length(glm::vec3(model[0]));
    const float sy = glm::length(glm::vec3(model[1]));
    const float sz = glm::length(glm::vec3(model[2]));
    const float radius = mesh.boundsRadius * std::max(sx, std::max(sy, sz));

    if (!sphereVisible(center, radius)) return;

    DrawItem item;
    item.mesh      = &mesh;
    item.model     = model;
    item.material  = material;
    item.viewDepth = glm::length(center - m_camera.position);

    if (material.transparent()) {
        m_transparent.push_back(item);
    } else {
        m_opaque.push_back(item);
    }
}

void Renderer::buildFrustum(const glm::mat4& m) {
    // Gribb/Hartmann plane extraction, normalised.
    const glm::vec4 rows[4] = {glm::vec4(m[0][0], m[1][0], m[2][0], m[3][0]),
                               glm::vec4(m[0][1], m[1][1], m[2][1], m[3][1]),
                               glm::vec4(m[0][2], m[1][2], m[2][2], m[3][2]),
                               glm::vec4(m[0][3], m[1][3], m[2][3], m[3][3])};
    m_frustum[0] = rows[3] + rows[0];   // left
    m_frustum[1] = rows[3] - rows[0];   // right
    m_frustum[2] = rows[3] + rows[1];   // bottom
    m_frustum[3] = rows[3] - rows[1];   // top
    m_frustum[4] = rows[3] + rows[2];   // near
    m_frustum[5] = rows[3] - rows[2];   // far
    for (glm::vec4& p : m_frustum) {
        const float len = glm::length(glm::vec3(p));
        if (len > 1e-6f) p /= len;
    }
}

bool Renderer::sphereVisible(const glm::vec3& center, float radius) const {
    for (const glm::vec4& p : m_frustum) {
        if (glm::dot(glm::vec3(p), center) + p.w < -radius) return false;
    }
    return true;
}

void Renderer::computeLightMatrix(const glm::vec3& focus) {
    const float radius = (m_quality == 0) ? 85.0f : (m_quality == 1 ? 110.0f : 135.0f);
    const glm::vec3 dir = glm::normalize(m_light.direction);
    const glm::vec3 eye = focus - dir * (radius * 2.0f);
    const glm::vec3 up  = std::fabs(dir.y) > 0.95f ? glm::vec3(0.0f, 0.0f, 1.0f)
                                                   : glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 view = glm::lookAt(eye, focus, up);

    // Texel snapping removes shadow shimmering while the camera moves.
    const float texelWorld = (radius * 2.0f) / (float)m_shadowRes;
    glm::vec4   focusLS    = view * glm::vec4(focus, 1.0f);
    focusLS.x = std::floor(focusLS.x / texelWorld) * texelWorld;
    focusLS.y = std::floor(focusLS.y / texelWorld) * texelWorld;
    const glm::vec3 snappedFocus = glm::vec3(glm::inverse(view) * focusLS);
    view = glm::lookAt(snappedFocus - dir * (radius * 2.0f), snappedFocus, up);

    const glm::mat4 proj =
        glm::ortho(-radius, radius, -radius, radius, 1.0f, radius * 4.5f);
    m_lightViewProj = proj * view;
}

void Renderer::applyMaterial(Shader& shader, const Material& material) {
    shader.setVec3("uAlbedo", material.albedo);
    shader.setFloat("uMetallic", material.metallic);
    shader.setFloat("uRoughness", material.roughness);
    shader.setVec3("uEmissive", material.emissive);
    shader.setFloat("uAlpha", material.alpha);
    shader.setFloat("uReflectivity", material.reflectivity);
    shader.setFloat("uClearCoat", material.clearCoat);
    shader.setFloat("uUseVertexColor", material.useVertexColor ? 1.0f : 0.0f);
    shader.setFloat("uReceiveShadow", material.receiveShadow ? 1.0f : 0.0f);
    shader.setFloat("uUVScale", material.uvScale);

    if (material.texture && material.texture->valid()) {
        material.texture->bind(0);
        shader.setFloat("uUseTexture", 1.0f);
    } else {
        m_whiteTexture.bind(0);
        shader.setFloat("uUseTexture", 0.0f);
    }
}

void Renderer::drawFullscreenTriangle() {
    glBindVertexArray(m_emptyVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void Renderer::drawScene() {
    if (!m_ready) return;

    // ------------------------------------------------------------ shadow pass
    m_shadowTarget.bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.6f, 3.0f);
    glCullFace(GL_FRONT);

    m_depthShader.bind();
    m_depthShader.setMat4("uLightViewProj", m_lightViewProj);
    for (const DrawItem& item : m_opaque) {
        if (!item.material.castShadow) continue;
        m_depthShader.setMat4("uModel", item.model);
        item.mesh->draw();
        ++m_stats.shadowDrawn;
    }
    glCullFace(GL_BACK);
    glDisable(GL_POLYGON_OFFSET_FILL);

    // ------------------------------------------------------------- scene pass
    m_sceneTarget.bind();
    glClearColor(fogColor.r, fogColor.g, fogColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Sky first, no depth interaction.
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    m_skyShader.bind();
    m_skyShader.setMat4("uInvViewProj", glm::inverse(m_camera.viewProjection));
    m_skyShader.setVec3("uCameraPos", m_camera.position);
    m_skyShader.setVec3("uLightDir", glm::normalize(m_light.direction));
    m_skyShader.setVec3("uLightColor", m_light.color * m_light.intensity * 0.35f);
    m_skyShader.setFloat("uTime", timeSeconds);
    drawFullscreenTriangle();
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    m_sceneShader.bind();
    m_sceneShader.setMat4("uViewProj", m_camera.viewProjection);
    m_sceneShader.setMat4("uLightViewProj", m_lightViewProj);
    m_sceneShader.setVec3("uCameraPos", m_camera.position);
    m_sceneShader.setVec3("uLightDir", glm::normalize(m_light.direction));
    m_sceneShader.setVec3("uLightColor", m_light.color * m_light.intensity);
    m_sceneShader.setVec3("uAmbientSky", m_light.ambientSky);
    m_sceneShader.setVec3("uAmbientGround", m_light.ambientGround);
    m_sceneShader.setVec3("uFogColor", fogColor);
    m_sceneShader.setFloat("uFogDensity", fogDensity);
    m_sceneShader.setInt("uAlbedoMap", 0);
    m_sceneShader.setInt("uShadowMap", 1);
    m_sceneShader.setFloat("uShadowTexel", 1.0f / (float)m_shadowRes);
    m_sceneShader.setFloat("uShadowStrength", m_light.shadowStrength);
    m_shadowTarget.depthTexture().bind(1);

    auto drawList = [&](std::vector<DrawItem>& list, bool blend) {
        for (const DrawItem& item : list) {
            if (item.material.doubleSided) {
                glDisable(GL_CULL_FACE);
            } else {
                glEnable(GL_CULL_FACE);
            }
            const bool wantOffset = std::fabs(item.material.polygonOffset) > 1e-4f;
            if (wantOffset) {
                glEnable(GL_POLYGON_OFFSET_FILL);
                glPolygonOffset(1.0f, item.material.polygonOffset);
            }
            applyMaterial(m_sceneShader, item.material);
            m_sceneShader.setMat4("uModel", item.model);
            m_sceneShader.setMat4("uNormalMatrix", glm::inverseTranspose(item.model));
            item.mesh->draw();
            ++m_stats.drawn;
            m_stats.triangles += item.mesh->triangleCount();
            if (wantOffset) {
                glDisable(GL_POLYGON_OFFSET_FILL);
            }
        }
        (void)blend;
    };

    glDisable(GL_BLEND);
    drawList(m_opaque, false);

    // Transparent objects: far to near, no depth write.
    std::sort(m_transparent.begin(), m_transparent.end(),
              [](const DrawItem& a, const DrawItem& b) { return a.viewDepth > b.viewDepth; });
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    drawList(m_transparent, true);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}

void Renderer::bindSceneTarget() const { m_sceneTarget.bind(); }

void Renderer::endFrame(float speedBlur, float flash) {
    if (!m_ready) return;

    glBindFramebuffer(GL_FRAMEBUFFER, outputFramebuffer);
    glViewport(0, 0, m_width, m_height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_postShader.bind();
    m_sceneTarget.colorTexture().bind(0);
    m_postShader.setInt("uScene", 0);
    m_postShader.setVec2("uTexelSize", glm::vec2(1.0f / (float)m_width, 1.0f / (float)m_height));
    m_postShader.setFloat("uBlurStrength", m_quality == 0 ? 0.0f : math::saturate(speedBlur));
    m_postShader.setFloat("uVignette", vignette);
    m_postShader.setFloat("uExposure", exposure);
    m_postShader.setFloat("uSaturation", saturation);
    m_postShader.setFloat("uFlash", flash);
    m_postShader.setFloat("uEnableFXAA", m_quality == 0 ? 0.0f : 1.0f);
    drawFullscreenTriangle();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

} // namespace vr
