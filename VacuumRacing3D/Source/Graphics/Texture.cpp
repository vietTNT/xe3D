#include "Texture.h"

#include <cmath>
#include <cstring>

#include <cstdio>
#include <string>
#include <vector>

#include "../Utilities/MathUtils.h"
#include "../Utilities/Noise.h"
#include "../Utilities/Random.h"

namespace vr {
namespace {

inline unsigned char toByte(float v) {
    const int i = (int)(math::saturate(v) * 255.0f + 0.5f);
    return (unsigned char)i;
}

inline void put(std::vector<unsigned char>& buf, int idx, const glm::vec3& c, float a = 1.0f) {
    buf[(size_t)idx * 4 + 0] = toByte(c.r);
    buf[(size_t)idx * 4 + 1] = toByte(c.g);
    buf[(size_t)idx * 4 + 2] = toByte(c.b);
    buf[(size_t)idx * 4 + 3] = toByte(a);
}

} // namespace

// ========================================================================= Texture
Texture::~Texture() { destroy(); }

Texture::Texture(Texture&& o) noexcept : m_id(o.m_id), m_width(o.m_width), m_height(o.m_height) {
    o.m_id = 0;
}

Texture& Texture::operator=(Texture&& o) noexcept {
    if (this != &o) {
        destroy();
        m_id     = o.m_id;
        m_width  = o.m_width;
        m_height = o.m_height;
        o.m_id   = 0;
    }
    return *this;
}

void Texture::destroy() {
    if (m_id && glDeleteTextures) glDeleteTextures(1, &m_id);
    m_id = 0;
}

void Texture::createRGBA(int width, int height, const unsigned char* pixels, bool mipmaps,
                         bool repeat, bool anisotropic) {
    destroy();
    m_width  = width;
    m_height = height;
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    const GLint wrap = repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (mipmaps) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        if (anisotropic && glTexParameterf && glGetFloatv) {
            // EXT_texture_filter_anisotropic is not core in 3.3: probe it once
            // instead of blindly setting an enum some drivers reject.
            static float maxAniso = -1.0f;
            if (maxAniso < 0.0f) {
                maxAniso = 0.0f;
                glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
                while (glGetError() != GL_NO_ERROR) {}   // clear if unsupported
            }
            if (maxAniso > 1.0f) {
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                                maxAniso < 8.0f ? maxAniso : 8.0f);
            }
        }
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::createR8(int width, int height, const unsigned char* pixels, bool mipmaps,
                       bool repeat) {
    destroy();
    m_width  = width;
    m_height = height;
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
    const GLint wrap = repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps ? GL_LINEAR_MIPMAP_LINEAR
                                                                  : GL_LINEAR);
    if (mipmaps) glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::createColorTarget(int width, int height) {
    destroy();
    m_width  = width;
    m_height = height;
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::createDepthTarget(int width, int height) {
    destroy();
    m_width  = width;
    m_height = height;
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT,
                 GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const GLfloat border[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::bind(int unit) const {
    glActiveTexture((GLenum)(GL_TEXTURE0 + unit));
    glBindTexture(GL_TEXTURE_2D, m_id);
}

bool Texture::loadBMP(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) {
        return false;
    }

    unsigned char fileHeader[14];
    if (std::fread(fileHeader, 1, 14, f) != 14) {
        std::fclose(f);
        return false;
    }
    if (fileHeader[0] != 'B' || fileHeader[1] != 'M') {
        std::fclose(f);
        return false;
    }
    unsigned int dataOffset = *(unsigned int*)&fileHeader[10];

    unsigned int dibSize = 0;
    if (std::fread(&dibSize, 4, 1, f) != 1) {
        std::fclose(f);
        return false;
    }
    // Read common DIB header fields (BITMAPINFOHEADER)
    int width = 0, height = 0;
    unsigned short planes = 0, bpp = 0;
    unsigned int compression = 0;
    if (dibSize >= 40) {
        if (std::fread(&width, 4, 1, f) != 1 || std::fread(&height, 4, 1, f) != 1 ||
            std::fread(&planes, 2, 1, f) != 1 || std::fread(&bpp, 2, 1, f) != 1 ||
            std::fread(&compression, 4, 1, f) != 1) {
            std::fclose(f);
            return false;
        }
        // Seek to pixel data offset
        std::fseek(f, dataOffset, SEEK_SET);
    } else {
        std::fclose(f);
        return false;
    }

    if (compression != 0) {
        std::fclose(f);
        return false;
    }
    if (bpp != 24 && bpp != 32) {
        std::fclose(f);
        return false;
    }

    const bool topDown = (height < 0);
    if (topDown) height = -height;
    const int rowSize = ((bpp * width + 31) / 32) * 4;
    std::vector<unsigned char> rows((size_t)rowSize * (size_t)height);
    if (std::fread(rows.data(), 1, rows.size(), f) != rows.size()) {
        // Some BMPs may have padding or different ordering; attempt to read by rows
        std::fseek(f, dataOffset, SEEK_SET);
        for (int y = 0; y < height; ++y) {
            if (std::fread(rows.data() + (size_t)rowSize * y, 1, rowSize, f) != (size_t)rowSize) {
                std::fclose(f);
                return false;
            }
        }
    }
    std::fclose(f);

    std::vector<unsigned char> rgba((size_t)width * (size_t)height * 4);
    for (int y = 0; y < height; ++y) {
        int srcY = topDown ? y : (height - 1 - y);
        unsigned char* src = rows.data() + (size_t)srcY * rowSize;
        for (int x = 0; x < width; ++x) {
            unsigned char b = src[x * (bpp / 8) + 0];
            unsigned char g = src[x * (bpp / 8) + 1];
            unsigned char r = src[x * (bpp / 8) + 2];
            unsigned char a = 255;
            if (bpp == 32) a = src[x * 4 + 3];
            const size_t dstIdx = ((size_t)y * (size_t)width + (size_t)x) * 4;
            rgba[dstIdx + 0] = r;
            rgba[dstIdx + 1] = g;
            rgba[dstIdx + 2] = b;
            rgba[dstIdx + 3] = a;
        }
    }

    createRGBA(width, height, rgba.data(), true, false, false);
    return true;
}

// ====================================================================== generators
namespace procedural {

std::vector<unsigned char> asphalt(int size) {
    std::vector<unsigned char> buf((size_t)size * size * 4, 255);
    Random rng(20240614u);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float fx = (float)x / (float)size;
            const float fy = (float)y / (float)size;
            // Coarse tarmac blotches + fine aggregate grain.
            const float coarse = noise::fbm2D(fx * 8.0f, fy * 8.0f, 8, 4);
            const float grain  = noise::fbm2D(fx * 96.0f, fy * 96.0f, 96, 3);
            const float cell   = noise::worley2D(fx * 48.0f, fy * 48.0f, 48);
            float       v      = 0.15f + 0.05f * coarse + 0.09f * grain + 0.05f * (1.0f - cell);
            // Subtle long cracks.
            const float crack = noise::fbm2D(fx * 5.0f + 12.3f, fy * 5.0f, 5, 3);
            if (crack > 0.74f && crack < 0.755f) v *= 0.55f;
            glm::vec3 c(v * 1.02f, v * 1.0f, v * 1.04f);
            put(buf, y * size + x, c);
        }
    }
    return buf;
}

std::vector<unsigned char> grass(int size) {
    std::vector<unsigned char> buf((size_t)size * size * 4, 255);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float fx = (float)x / (float)size;
            const float fy = (float)y / (float)size;
            const float blades = noise::fbm2D(fx * 140.0f, fy * 140.0f, 140, 2);
            const float patch  = noise::fbm2D(fx * 6.0f, fy * 6.0f, 6, 4);
            const float mow    = 0.5f + 0.5f * std::sin(fy * math::kTwoPi * 6.0f);
            glm::vec3   dark(0.11f, 0.26f, 0.08f);
            glm::vec3   light(0.30f, 0.52f, 0.17f);
            glm::vec3   c = glm::mix(dark, light, math::saturate(0.35f * blades + 0.5f * patch +
                                                                 0.15f * mow));
            put(buf, y * size + x, c);
        }
    }
    return buf;
}

std::vector<unsigned char> concrete(int size) {
    std::vector<unsigned char> buf((size_t)size * size * 4, 255);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float fx = (float)x / (float)size;
            const float fy = (float)y / (float)size;
            const float n  = noise::fbm2D(fx * 20.0f, fy * 20.0f, 20, 4);
            const float stain = noise::fbm2D(fx * 3.0f, fy * 3.0f, 3, 3);
            float       v  = 0.52f + 0.10f * n - 0.09f * stain;
            // Panel seams every quarter of the tile.
            const float sx = std::fabs(std::fmod(fx * 4.0f, 1.0f) - 0.5f);
            const float sy = std::fabs(std::fmod(fy * 4.0f, 1.0f) - 0.5f);
            if (sx > 0.487f || sy > 0.487f) v *= 0.78f;
            put(buf, y * size + x, glm::vec3(v, v * 0.99f, v * 0.96f));
        }
    }
    return buf;
}

std::vector<unsigned char> brushedMetal(int size) {
    std::vector<unsigned char> buf((size_t)size * size * 4, 255);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float fx = (float)x / (float)size;
            const float fy = (float)y / (float)size;
            const float streak = noise::fbm2D(fx * 220.0f, fy * 4.0f, 220, 2);
            const float v      = 0.62f + 0.13f * streak;
            put(buf, y * size + x, glm::vec3(v, v * 1.01f, v * 1.05f));
        }
    }
    return buf;
}

std::vector<unsigned char> carbonFibre(int size) {
    std::vector<unsigned char> buf((size_t)size * size * 4, 255);
    const int cell = size / 16;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const int cx = (x / cell) % 2;
            const int cy = (y / cell) % 2;
            const bool warp = (cx == cy);
            const float local = warp ? (float)(x % cell) / (float)cell : (float)(y % cell) / (float)cell;
            const float shade = 0.06f + 0.16f * std::sin(local * math::kPi);
            const float speck = noise::value2D((float)x * 0.7f, (float)y * 0.7f, size) * 0.05f;
            put(buf, y * size + x, glm::vec3(shade + speck));
        }
    }
    return buf;
}

std::vector<unsigned char> gravel(int size) {
    std::vector<unsigned char> buf((size_t)size * size * 4, 255);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float fx = (float)x / (float)size;
            const float fy = (float)y / (float)size;
            const float w  = noise::worley2D(fx * 40.0f, fy * 40.0f, 40);
            const float n  = noise::fbm2D(fx * 30.0f, fy * 30.0f, 30, 3);
            const float v  = 0.42f + 0.28f * w + 0.10f * n;
            put(buf, y * size + x, glm::vec3(v * 0.95f, v * 0.87f, v * 0.74f));
        }
    }
    return buf;
}

std::vector<unsigned char> smokePuff(int size) {
    std::vector<unsigned char> buf((size_t)size * size * 4, 0);
    const float c = (float)size * 0.5f;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float dx = ((float)x - c) / c;
            const float dy = ((float)y - c) / c;
            const float r  = std::sqrt(dx * dx + dy * dy);
            const float fx = (float)x / (float)size;
            const float fy = (float)y / (float)size;
            const float n  = noise::fbm2D(fx * 8.0f, fy * 8.0f, 8, 4);
            float a = math::saturate(1.0f - r) ;
            a = a * a * (0.55f + 0.65f * n);
            put(buf, y * size + x, glm::vec3(1.0f), math::saturate(a));
        }
    }
    return buf;
}

std::vector<unsigned char> softGlow(int size) {
    std::vector<unsigned char> buf((size_t)size * size * 4, 0);
    const float c = (float)size * 0.5f;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float dx = ((float)x - c) / c;
            const float dy = ((float)y - c) / c;
            const float r  = std::sqrt(dx * dx + dy * dy);
            const float a  = std::pow(math::saturate(1.0f - r), 2.4f);
            put(buf, y * size + x, glm::vec3(1.0f), a);
        }
    }
    return buf;
}

std::vector<unsigned char> sponsorBoard(int size, const glm::vec3& base, int style) {
    std::vector<unsigned char> buf((size_t)size * size * 4, 255);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float fx = (float)x / (float)size;
            const float fy = (float)y / (float)size;
            glm::vec3   c  = base;
            switch (style % 4) {
                case 0: { // diagonal speed stripes
                    const float s = std::fmod(fx * 6.0f + fy * 2.0f, 1.0f);
                    c = s < 0.5f ? base : glm::mix(base, glm::vec3(1.0f), 0.75f);
                    break;
                }
                case 1: { // chunky wordmark bar
                    const bool bar = fy > 0.32f && fy < 0.68f && fx > 0.08f && fx < 0.92f;
                    c = bar ? glm::vec3(0.96f) : base;
                    if (bar) {
                        const float g = std::fmod(fx * 9.0f, 1.0f);
                        if (g < 0.55f) c = base * 0.35f + glm::vec3(0.05f);
                    }
                    break;
                }
                case 2: { // split block
                    c = fx < 0.5f ? base : glm::vec3(0.08f);
                    if (fy > 0.44f && fy < 0.56f) c = glm::vec3(0.95f, 0.78f, 0.10f);
                    break;
                }
                default: { // checkered edge
                    const int cx = (int)(fx * 12.0f);
                    const int cy = (int)(fy * 4.0f);
                    const bool edge = fy < 0.18f || fy > 0.82f;
                    c = edge ? (((cx + cy) % 2) ? glm::vec3(0.95f) : glm::vec3(0.06f)) : base;
                    break;
                }
            }
            put(buf, y * size + x, c);
        }
    }
    return buf;
}

std::vector<unsigned char> white(int size) {
    return std::vector<unsigned char>((size_t)size * size * 4, 255);
}

std::vector<unsigned char> nascarLivery(int size, const glm::vec3& primaryColor, int number) {
    std::vector<unsigned char> buf((size_t)size * size * 4, 255);
    const glm::vec3 yellow(0.98f, 0.85f, 0.05f);  // Race Yellow for number
    const glm::vec3 black(0.05f, 0.05f, 0.05f);   // Black outline

    const int d1 = number / 10;
    const int d2 = number % 10;

    auto drawNum = [&](float dx, float dy, int digit, glm::vec3& col) {
        if (dx < 0.0f || dx > 1.0f || dy < 0.0f || dy > 1.0f) return;
        bool isPixel = false;
        switch (digit) {
            case 1:
                isPixel = (dx >= 0.35f && dx <= 0.65f && dy >= 0.10f && dy <= 0.90f) ||
                          (dx >= 0.20f && dx <= 0.40f && dy >= 0.75f && dy <= 0.90f) ||
                          (dx >= 0.25f && dx <= 0.75f && dy >= 0.10f && dy <= 0.22f);
                break;
            case 2:
                isPixel = (dx >= 0.15f && dx <= 0.85f && dy >= 0.72f && dy <= 0.90f) ||
                          (dx >= 0.65f && dx <= 0.85f && dy >= 0.48f && dy <= 0.75f) ||
                          (dx >= 0.15f && dx <= 0.85f && dy >= 0.38f && dy <= 0.52f) ||
                          (dx >= 0.15f && dx <= 0.35f && dy >= 0.20f && dy <= 0.42f) ||
                          (dx >= 0.15f && dx <= 0.85f && dy >= 0.10f && dy <= 0.24f);
                break;
            case 4:
                isPixel = (dx >= 0.60f && dx <= 0.80f && dy >= 0.10f && dy <= 0.90f) ||
                          (dx >= 0.15f && dx <= 0.35f && dy >= 0.45f && dy <= 0.90f) ||
                          (dx >= 0.15f && dx <= 0.85f && dy >= 0.40f && dy <= 0.55f);
                break;
            case 7:
                isPixel = (dx >= 0.15f && dx <= 0.85f && dy >= 0.72f && dy <= 0.90f) ||
                          (dx >= 0.55f && dx <= 0.80f && dy >= 0.10f && dy <= 0.75f);
                break;
            case 8:
                isPixel = (dx >= 0.15f && dx <= 0.85f && ((dy >= 0.10f && dy <= 0.24f) || (dy >= 0.43f && dy <= 0.57f) || (dy >= 0.74f && dy <= 0.88f))) ||
                          (((dx >= 0.15f && dx <= 0.32f) || (dx >= 0.68f && dx <= 0.85f)) && dy >= 0.10f && dy <= 0.88f);
                break;
            case 9:
                isPixel = (dx >= 0.15f && dx <= 0.85f && ((dy >= 0.45f && dy <= 0.58f) || (dy >= 0.74f && dy <= 0.90f))) ||
                          (dx >= 0.15f && dx <= 0.32f && dy >= 0.45f && dy <= 0.90f) ||
                          (dx >= 0.65f && dx <= 0.85f && dy >= 0.10f && dy <= 0.90f);
                break;
            default: // 0
                isPixel = (dx >= 0.15f && dx <= 0.85f && ((dy >= 0.10f && dy <= 0.24f) || (dy >= 0.74f && dy <= 0.90f))) ||
                          (((dx >= 0.15f && dx <= 0.32f) || (dx >= 0.68f && dx <= 0.85f)) && dy >= 0.10f && dy <= 0.90f);
                break;
        }
        bool isBorder = (dx >= 0.05f && dx <= 0.95f && dy >= 0.04f && dy <= 0.96f);
        if (isPixel) col = yellow;
        else if (isBorder && col != yellow) col = black;
    };

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float fx = (float)x / (float)size;
            const float fy = (float)y / (float)size;

            // Solid clean body paint in the chosen primary color (Red, Blue, Black, White, Yellow, Green)
            glm::vec3 c = primaryColor;

            // 1. Draw Race Number on Roof (Top center: fx: 0.32..0.68, fy: 0.72..0.96)
            if (fx >= 0.32f && fx <= 0.68f && fy >= 0.72f && fy <= 0.96f) {
                float rx = (fx - 0.32f) / 0.36f;
                float ry = (fy - 0.72f) / 0.24f;
                drawNum(rx * 2.0f, ry, d1, c);
                drawNum(rx * 2.0f - 1.0f, ry, d2, c);
            }

            // 2. Draw Race Number on Left Door (fx: 0.12..0.42, fy: 0.12..0.42)
            if (fx >= 0.12f && fx <= 0.42f && fy >= 0.12f && fy <= 0.42f) {
                float dx = (fx - 0.12f) / 0.30f;
                float dy = (fy - 0.12f) / 0.30f;
                drawNum(dx * 2.0f, dy, d1, c);
                drawNum(dx * 2.0f - 1.0f, dy, d2, c);
            }

            // 3. Draw Race Number on Right Door (fx: 0.58..0.88, fy: 0.12..0.42)
            if (fx >= 0.58f && fx <= 0.88f && fy >= 0.12f && fy <= 0.42f) {
                float dx = (fx - 0.58f) / 0.30f;
                float dy = (fy - 0.12f) / 0.30f;
                drawNum(dx * 2.0f, dy, d1, c);
                drawNum(dx * 2.0f - 1.0f, dy, d2, c);
            }

            // 4. Yellow Fuel Cap Ring on left quarter panel (matching reference design sheet)
            if (fx >= 0.12f && fx <= 0.18f && fy >= 0.38f && fy <= 0.48f) {
                float cx = (fx - 0.15f) / 0.03f;
                float cy = (fy - 0.43f) / 0.05f;
                float dist = std::sqrt(cx * cx + cy * cy);
                if (dist <= 0.90f && dist >= 0.60f) {
                    c = yellow; // Yellow outer ring
                } else if (dist < 0.60f) {
                    c = glm::vec3(0.15f, 0.15f, 0.18f); // Fuel cap center
                }
            }

            // 5. Front Grille Mesh (fx: 0.32..0.68, fy: 0.55..0.65)
            if (fx >= 0.32f && fx <= 0.68f && fy >= 0.55f && fy <= 0.65f) {
                float hx = (fx - 0.32f) / 0.36f;
                float hy = (fy - 0.55f) / 0.10f;
                if (hx > 0.05f && hx < 0.95f && hy > 0.10f && hy < 0.90f) {
                    c = black;
                }
            }

            put(buf, y * size + x, c);
        }
    }
    return buf;
}

std::vector<unsigned char> windowNet(int size) {
    std::vector<unsigned char> buf((size_t)size * size * 4, 0);
    const int step = size / 12;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const int cx = x % step;
            const int cy = y % step;
            if (cx <= 3 || cy <= 3) {
                put(buf, y * size + x, glm::vec3(0.06f, 0.06f, 0.06f), 0.95f);
            }
        }
    }
    return buf;
}

} // namespace procedural
} // namespace vr
