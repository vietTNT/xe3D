#include "Framebuffer.h"

#include <cstdio>

namespace vr {

// ===================================================================== SceneTarget
SceneTarget::~SceneTarget() { destroy(); }

bool SceneTarget::create(int width, int height) {
    destroy();
    if (width <= 0 || height <= 0) return false;
    m_width  = width;
    m_height = height;

    m_color.createColorTarget(width, height);

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_color.id(), 0);

    glGenRenderbuffers(1, &m_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth);

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::fprintf(stderr, "[SceneTarget] incomplete framebuffer (0x%04X)\n", (unsigned)status);
        destroy();
        return false;
    }
    return true;
}

void SceneTarget::destroy() {
    if (m_depth && glDeleteRenderbuffers) glDeleteRenderbuffers(1, &m_depth);
    if (m_fbo && glDeleteFramebuffers) glDeleteFramebuffers(1, &m_fbo);
    m_depth = 0;
    m_fbo   = 0;
    m_color.destroy();
}

void SceneTarget::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
}

void SceneTarget::bindDefault(int width, int height) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
}

// ==================================================================== ShadowTarget
ShadowTarget::~ShadowTarget() { destroy(); }

bool ShadowTarget::create(int resolution) {
    destroy();
    if (resolution <= 0) return false;
    m_resolution = resolution;
    m_depth.createDepthTarget(resolution, resolution);

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depth.id(), 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::fprintf(stderr, "[ShadowTarget] incomplete framebuffer (0x%04X)\n", (unsigned)status);
        destroy();
        return false;
    }
    return true;
}

void ShadowTarget::destroy() {
    if (m_fbo && glDeleteFramebuffers) glDeleteFramebuffers(1, &m_fbo);
    m_fbo = 0;
    m_depth.destroy();
}

void ShadowTarget::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_resolution, m_resolution);
}

} // namespace vr
