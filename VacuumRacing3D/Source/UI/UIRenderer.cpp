#include "UIRenderer.h"

#include <cmath>
#include <cstdio>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include "../Core/Paths.h"
#include "../Graphics/FontData.h"
#include "../Utilities/MathUtils.h"

namespace vr {

bool UIRenderer::init() {
    if (!m_shader.loadFromFiles(paths::resolve("Shaders/ui.vert"),
                                paths::resolve("Shaders/ui.frag"))) {
        std::fprintf(stderr, "[UI] failed to load the UI shader\n");
        return false;
    }
    const std::vector<unsigned char> px = procedural::white(2);
    m_white.createRGBA(2, 2, px.data(), false, false, false);
    buildFontAtlas();
    m_verts.reserve(4096);
    m_indices.reserve(6144);
    return true;
}

void UIRenderer::shutdown() {
    m_shader.destroy();
    m_mesh.destroy();
    m_white.destroy();
    m_font.destroy();
}

void UIRenderer::buildFontAtlas() {
    using namespace font;
    const int cols  = kCharCount;
    const int width = cols * kGlyphWidth;
    std::vector<unsigned char> atlas((size_t)width * (size_t)kGlyphHeight, 0);

    for (int c = 0; c < kCharCount; ++c) {
        int minX = kGlyphWidth, maxX = -1;
        for (int y = 0; y < kGlyphHeight; ++y) {
            for (int b = 0; b < kBytesPerRow; ++b) {
                const unsigned char bits =
                    kGlyphBits[(size_t)((c * kGlyphHeight + y) * kBytesPerRow + b)];
                for (int i = 0; i < 8; ++i) {
                    if (bits & (1u << (7 - i))) {
                        const int x = b * 8 + i;
                        atlas[(size_t)y * (size_t)width + (size_t)(c * kGlyphWidth + x)] = 255;
                        if (x < minX) minX = x;
                        if (x > maxX) maxX = x;
                    }
                }
            }
        }
        if (maxX < minX) {   // blank glyph (space)
            minX = 4;
            maxX = 10;
        }
        m_glyphLeft[c]  = (float)minX;
        m_glyphRight[c] = (float)(maxX + 1);
    }
    m_font.createR8(width, kGlyphHeight, atlas.data(), false, false);
}

void UIRenderer::begin(int screenWidth, int screenHeight) {
    m_width  = screenWidth > 0 ? screenWidth : 1;
    m_height = screenHeight > 0 ? screenHeight : 1;
    m_projection = glm::ortho(0.0f, (float)m_width, (float)m_height, 0.0f, -1.0f, 1.0f);

    m_verts.clear();
    m_indices.clear();
    m_mode    = Mode::Solid;
    m_texture = nullptr;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_shader.bind();
    m_shader.setMat4("uProjection", m_projection);
    m_shader.setInt("uTexture", 0);
}

void UIRenderer::end() {
    flush();
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void UIRenderer::setState(Mode mode, const Texture* texture) {
    if (mode != m_mode || texture != m_texture) {
        flush();
        m_mode    = mode;
        m_texture = texture;
    }
}

void UIRenderer::flush() {
    if (m_indices.empty()) return;
    m_shader.bind();
    m_shader.setMat4("uProjection", m_projection);
    m_shader.setFloat("uMode", (float)(int)m_mode);
    const Texture* tex = m_texture ? m_texture : &m_white;
    tex->bind(0);
    m_mesh.updateDynamic(m_verts, m_indices);
    m_mesh.draw();
    m_verts.clear();
    m_indices.clear();
}

void UIRenderer::pushQuad(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2,
                          const glm::vec2& p3, const glm::vec2& uv0, const glm::vec2& uv1,
                          const glm::vec3& color, float alpha) {
    const std::uint32_t base = (std::uint32_t)m_verts.size();
    const glm::vec3     nrm(alpha, 0.0f, 0.0f);
    m_verts.push_back({glm::vec3(p0, 0.0f), nrm, glm::vec2(uv0.x, uv0.y), color});
    m_verts.push_back({glm::vec3(p1, 0.0f), nrm, glm::vec2(uv1.x, uv0.y), color});
    m_verts.push_back({glm::vec3(p2, 0.0f), nrm, glm::vec2(uv1.x, uv1.y), color});
    m_verts.push_back({glm::vec3(p3, 0.0f), nrm, glm::vec2(uv0.x, uv1.y), color});
    m_indices.push_back(base + 0);
    m_indices.push_back(base + 1);
    m_indices.push_back(base + 2);
    m_indices.push_back(base + 0);
    m_indices.push_back(base + 2);
    m_indices.push_back(base + 3);
}

void UIRenderer::rect(float x, float y, float w, float h, const glm::vec3& color, float alpha) {
    if (alpha <= 0.001f) return;
    setState(Mode::Solid, nullptr);
    pushQuad(glm::vec2(x, y), glm::vec2(x + w, y), glm::vec2(x + w, y + h), glm::vec2(x, y + h),
             glm::vec2(0.0f), glm::vec2(1.0f), color, alpha);
}

void UIRenderer::rectOutline(float x, float y, float w, float h, float t, const glm::vec3& color,
                             float alpha) {
    rect(x, y, w, t, color, alpha);
    rect(x, y + h - t, w, t, color, alpha);
    rect(x, y + t, t, h - 2.0f * t, color, alpha);
    rect(x + w - t, y + t, t, h - 2.0f * t, color, alpha);
}

void UIRenderer::line(const glm::vec2& a, const glm::vec2& b, float thickness,
                      const glm::vec3& color, float alpha) {
    const glm::vec2 d = b - a;
    const float     len = glm::length(d);
    if (len < 1e-4f) return;
    const glm::vec2 n = glm::vec2(-d.y, d.x) / len * (thickness * 0.5f);
    setState(Mode::Solid, nullptr);
    pushQuad(a - n, b - n, b + n, a + n, glm::vec2(0.0f), glm::vec2(1.0f), color, alpha);
}

void UIRenderer::texturedRect(const Texture& texture, float x, float y, float w, float h,
                              const glm::vec3& tint, float alpha) {
    setState(Mode::Textured, &texture);
    pushQuad(glm::vec2(x, y), glm::vec2(x + w, y), glm::vec2(x + w, y + h), glm::vec2(x, y + h),
             glm::vec2(0.0f), glm::vec2(1.0f), tint, alpha);
}

void UIRenderer::disc(const glm::vec2& c, float radius, const glm::vec3& color, float alpha,
                      int segments) {
    setState(Mode::Solid, nullptr);
    if (segments < 4) segments = 4;
    const std::uint32_t base = (std::uint32_t)m_verts.size();
    const glm::vec3     nrm(alpha, 0.0f, 0.0f);
    m_verts.push_back({glm::vec3(c, 0.0f), nrm, glm::vec2(0.5f), color});
    for (int i = 0; i <= segments; ++i) {
        const float a = math::kTwoPi * (float)i / (float)segments;
        m_verts.push_back({glm::vec3(c.x + std::cos(a) * radius, c.y + std::sin(a) * radius, 0.0f),
                           nrm, glm::vec2(0.5f), color});
    }
    for (int i = 0; i < segments; ++i) {
        m_indices.push_back(base);
        m_indices.push_back(base + 1 + (std::uint32_t)i);
        m_indices.push_back(base + 2 + (std::uint32_t)i);
    }
}

// ========================================================================== text
float UIRenderer::textWidth(const std::string& s, float pixelHeight) const {
    const float scale = pixelHeight / (float)font::kGlyphHeight;
    float       w     = 0.0f;
    for (char ch : s) {
        const int c = (int)(unsigned char)ch - font::kFirstChar;
        if (c < 0 || c >= font::kCharCount) {
            w += pixelHeight * 0.4f;
            continue;
        }
        w += (m_glyphRight[c] - m_glyphLeft[c] + 2.6f) * scale;
    }
    return w;
}

void UIRenderer::text(const std::string& s, float x, float y, float pixelHeight,
                      const glm::vec3& color, float alpha, TextAlign align) {
    if (alpha <= 0.003f || s.empty()) return;
    const float scale = pixelHeight / (float)font::kGlyphHeight;
    const float total = textWidth(s, pixelHeight);
    float       penX  = x;
    if (align == TextAlign::Center) penX -= total * 0.5f;
    if (align == TextAlign::Right) penX -= total;

    setState(Mode::Glyph, &m_font);
    const float atlasW = (float)(font::kCharCount * font::kGlyphWidth);

    for (char ch : s) {
        const int c = (int)(unsigned char)ch - font::kFirstChar;
        if (c < 0 || c >= font::kCharCount) {
            penX += pixelHeight * 0.4f;
            continue;
        }
        const float l = m_glyphLeft[c];
        const float r = m_glyphRight[c];
        const float w = (r - l) * scale;

        const float u0 = ((float)(c * font::kGlyphWidth) + l) / atlasW;
        const float u1 = ((float)(c * font::kGlyphWidth) + r) / atlasW;

        pushQuad(glm::vec2(penX, y), glm::vec2(penX + w, y), glm::vec2(penX + w, y + pixelHeight),
                 glm::vec2(penX, y + pixelHeight), glm::vec2(u0, 0.0f), glm::vec2(u1, 1.0f), color,
                 alpha);
        penX += w + 2.6f * scale;
    }
}

void UIRenderer::textShadow(const std::string& s, float x, float y, float pixelHeight,
                            const glm::vec3& color, float alpha, TextAlign align) {
    const float o = std::max(1.5f, pixelHeight * 0.055f);
    text(s, x + o, y + o, pixelHeight, glm::vec3(0.0f), alpha * 0.65f, align);
    text(s, x, y, pixelHeight, color, alpha, align);
}

} // namespace vr
