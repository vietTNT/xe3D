// -----------------------------------------------------------------------------
//  UIRenderer.h - batched 2D renderer for the HUD and the menus.
//  Text comes from a 1-bit bitmap font compiled into the executable, unpacked
//  into a single-channel atlas at start-up with per-glyph proportional metrics.
// -----------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "../Graphics/Mesh.h"
#include "../Graphics/Shader.h"
#include "../Graphics/Texture.h"

namespace vr {

enum class TextAlign { Left, Center, Right };

class UIRenderer {
public:
    bool init();
    void shutdown();

    void begin(int screenWidth, int screenHeight);
    void end();

    // ------------------------------------------------------------ primitives
    void rect(float x, float y, float w, float h, const glm::vec3& color, float alpha = 1.0f);
    void rectOutline(float x, float y, float w, float h, float thickness,
                     const glm::vec3& color, float alpha = 1.0f);
    void line(const glm::vec2& a, const glm::vec2& b, float thickness, const glm::vec3& color,
              float alpha = 1.0f);
    void texturedRect(const Texture& texture, float x, float y, float w, float h,
                      const glm::vec3& tint = glm::vec3(1.0f), float alpha = 1.0f);
    void disc(const glm::vec2& centre, float radius, const glm::vec3& color, float alpha = 1.0f,
              int segments = 12);

    // ------------------------------------------------------------------ text
    void  text(const std::string& s, float x, float y, float pixelHeight, const glm::vec3& color,
               float alpha = 1.0f, TextAlign align = TextAlign::Left);
    /// Text with a soft drop shadow - keeps the HUD readable over bright sky.
    void  textShadow(const std::string& s, float x, float y, float pixelHeight,
                     const glm::vec3& color, float alpha = 1.0f,
                     TextAlign align = TextAlign::Left);
    float textWidth(const std::string& s, float pixelHeight) const;
    float lineHeight(float pixelHeight) const { return pixelHeight * 1.32f; }

    int screenWidth() const { return m_width; }
    int screenHeight() const { return m_height; }

private:
    enum class Mode { Solid = 0, Textured = 1, Glyph = 2 };

    void setState(Mode mode, const Texture* texture);
    void flush();
    void pushQuad(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2,
                  const glm::vec2& p3, const glm::vec2& uv0, const glm::vec2& uv1,
                  const glm::vec3& color, float alpha);
    void buildFontAtlas();

    Shader  m_shader;
    Mesh    m_mesh;
    Texture m_white;
    Texture m_font;

    std::vector<Vertex>        m_verts;
    std::vector<std::uint32_t> m_indices;

    Mode           m_mode      = Mode::Solid;
    const Texture* m_texture   = nullptr;
    int            m_width     = 1280;
    int            m_height    = 720;
    glm::mat4      m_projection{1.0f};

    // Proportional metrics extracted from the packed bitmap font.
    float m_glyphLeft[95]  = {0.0f};
    float m_glyphRight[95] = {0.0f};
};

} // namespace vr
