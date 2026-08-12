// -----------------------------------------------------------------------------
//  Framebuffer.h - render targets for the shadow pass and the post-process pass.
// -----------------------------------------------------------------------------
#pragma once

#include "Texture.h"

namespace vr {

/// Colour + depth render target used as the scene buffer before post processing.
class SceneTarget {
public:
    ~SceneTarget();
    bool create(int width, int height);
    void destroy();
    void bind() const;
    static void bindDefault(int width, int height);

    const Texture& colorTexture() const { return m_color; }
    int  width() const { return m_width; }
    int  height() const { return m_height; }
    bool valid() const { return m_fbo != 0; }

private:
    GLuint  m_fbo   = 0;
    GLuint  m_depth = 0;
    Texture m_color;
    int     m_width  = 0;
    int     m_height = 0;
};

/// Depth only target for directional shadow mapping.
class ShadowTarget {
public:
    ~ShadowTarget();
    bool create(int resolution);
    void destroy();
    void bind() const;

    const Texture& depthTexture() const { return m_depth; }
    int  resolution() const { return m_resolution; }
    bool valid() const { return m_fbo != 0; }

private:
    GLuint  m_fbo        = 0;
    Texture m_depth;
    int     m_resolution = 0;
};

} // namespace vr
