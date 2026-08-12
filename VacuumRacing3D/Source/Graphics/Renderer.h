// -----------------------------------------------------------------------------
//  Renderer.h - forward renderer:
//    1. directional shadow pass (fitted ortho frustum, PCF)
//    2. scene pass into an off-screen target: procedural sky, opaque, transparent
//    3. post pass: speed blur, FXAA, filmic tonemap, vignette
//  Culling is done per draw call against the camera frustum.
// -----------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Framebuffer.h"
#include "Material.h"
#include "Mesh.h"
#include "Shader.h"

namespace vr {

struct DirectionalLight {
    glm::vec3 direction{glm::normalize(glm::vec3(-0.42f, -0.62f, -0.36f))};
    glm::vec3 color{1.0f, 0.84f, 0.62f};
    float     intensity     = 3.1f;
    glm::vec3 ambientSky{0.30f, 0.38f, 0.52f};
    glm::vec3 ambientGround{0.16f, 0.14f, 0.11f};
    float     shadowStrength = 0.82f;
};

struct CameraView {
    glm::vec3 position{0.0f};
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};
    glm::mat4 viewProjection{1.0f};
    float     nearPlane = 0.2f;
    float     farPlane  = 1400.0f;
};

struct RenderStats {
    int submitted   = 0;
    int drawn       = 0;
    int shadowDrawn = 0;
    int triangles   = 0;
};

class Renderer {
public:
    bool init(int framebufferWidth, int framebufferHeight, int qualityLevel = 2);
    void shutdown();
    void resize(int framebufferWidth, int framebufferHeight);

    /// 0 = low, 1 = medium, 2 = high. Adjusts shadow resolution and post effects.
    void setQualityLevel(int level);
    int  qualityLevel() const { return m_quality; }

    void beginFrame(const CameraView& camera, const DirectionalLight& light,
                    const glm::vec3& shadowFocus);
    void submit(const Mesh& mesh, const glm::mat4& model, const Material& material);
    /// Shadow + sky + opaque + transparent, all into the off-screen scene target.
    void drawScene();
    /// Re-binds the scene target so particle/decal passes can append to the frame.
    void bindSceneTarget() const;
    /// Resolves the scene target to the back buffer with post processing.
    void endFrame(float speedBlur, float flash);

    Shader&           particleShader() { return m_particleShader; }
    const CameraView& camera() const { return m_camera; }
    const RenderStats& stats() const { return m_stats; }

    /// Framebuffer that endFrame() resolves into. 0 = the window's back buffer;
    /// set it to an FBO to capture screenshots or render off-screen.
    GLuint    outputFramebuffer = 0;

    glm::vec3 fogColor{0.72f, 0.76f, 0.82f};
    float     fogDensity = 0.0016f;
    float     exposure   = 1.05f;
    float     saturation = 1.06f;
    float     vignette   = 0.55f;
    float     timeSeconds = 0.0f;

private:
    struct DrawItem {
        const Mesh* mesh;
        glm::mat4   model;
        Material    material;
        float       viewDepth;
    };

    void buildFrustum(const glm::mat4& viewProj);
    bool sphereVisible(const glm::vec3& center, float radius) const;
    void computeLightMatrix(const glm::vec3& focus);
    void applyMaterial(Shader& shader, const Material& material);
    void drawFullscreenTriangle();

    Shader m_sceneShader;
    Shader m_depthShader;
    Shader m_skyShader;
    Shader m_postShader;
    Shader m_particleShader;

    SceneTarget  m_sceneTarget;
    ShadowTarget m_shadowTarget;
    Texture      m_whiteTexture;

    std::vector<DrawItem> m_opaque;
    std::vector<DrawItem> m_transparent;

    CameraView       m_camera;
    DirectionalLight m_light;
    glm::mat4        m_lightViewProj{1.0f};
    glm::vec4        m_frustum[6]{};

    RenderStats m_stats;
    int         m_width      = 0;
    int         m_height     = 0;
    int         m_quality    = 2;
    int         m_shadowRes  = 2048;
    GLuint      m_emptyVAO   = 0;
    bool        m_ready      = false;
};

} // namespace vr
