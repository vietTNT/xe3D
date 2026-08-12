// -----------------------------------------------------------------------------
//  Texture.h - GPU texture wrapper + fully procedural texture generators.
//  The game ships with zero image files: asphalt, grass, concrete, metal and
//  the particle sprites are all synthesised at load time from noise.
// -----------------------------------------------------------------------------
#pragma once

#include <vector>
#include <string>

#include <glm/glm.hpp>

#include "../Core/GL.h"

namespace vr {

class Texture {
public:
    Texture() = default;
    ~Texture();

    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& o) noexcept;
    Texture& operator=(Texture&& o) noexcept;

    void createRGBA(int width, int height, const unsigned char* pixels, bool mipmaps = true,
                    bool repeat = true, bool anisotropic = true);
    void createR8(int width, int height, const unsigned char* pixels, bool mipmaps = false,
                  bool repeat = false);
    void createColorTarget(int width, int height);
    void createDepthTarget(int width, int height);
    void destroy();

    void bind(int unit) const;

    bool   valid() const { return m_id != 0; }
    GLuint id() const { return m_id; }
    int    width() const { return m_width; }
    int    height() const { return m_height; }

    // Attempts to load an uncompressed 24-bit or 32-bit BMP from disk.
    // Returns true on success and creates an RGBA texture.
    bool loadBMP(const std::string& path);

private:
    GLuint m_id     = 0;
    int    m_width  = 0;
    int    m_height = 0;
};

// Helper: load BMP files (simple, supports uncompressed 24/32-bit BMP)
// Implemented in Texture.cpp
// Returns true if the texture was created successfully.
// Usage: `Texture t; if (t.loadBMP(path)) { /* use t */ }`


// -----------------------------------------------------------------------------
//  Procedural pixel generators - all return tightly packed RGBA8 buffers.
// -----------------------------------------------------------------------------
namespace procedural {

std::vector<unsigned char> asphalt(int size);
std::vector<unsigned char> grass(int size);
std::vector<unsigned char> concrete(int size);
std::vector<unsigned char> brushedMetal(int size);
std::vector<unsigned char> carbonFibre(int size);
std::vector<unsigned char> gravel(int size);
std::vector<unsigned char> smokePuff(int size);
std::vector<unsigned char> softGlow(int size);
std::vector<unsigned char> sponsorBoard(int size, const glm::vec3& base, int style);
std::vector<unsigned char> white(int size);
std::vector<unsigned char> nascarLivery(int size, const glm::vec3& primaryColor = glm::vec3(0.85f, 0.12f, 0.10f), int number = 12);
std::vector<unsigned char> windowNet(int size);

} // namespace procedural
} // namespace vr
