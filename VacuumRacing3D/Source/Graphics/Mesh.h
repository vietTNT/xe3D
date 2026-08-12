// -----------------------------------------------------------------------------
//  Mesh.h - interleaved vertex buffer + MeshBuilder, the procedural geometry
//  toolkit every object in the game is modelled with (car, track, scenery).
// -----------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "../Core/GL.h"

namespace vr {

struct Vertex {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec2 uv{0.0f};
    glm::vec3 color{1.0f};
};

class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void upload(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices,
                bool dynamic = false);
    /// Re-uploads a dynamic mesh (particles, skid marks) without reallocating when possible.
    void updateDynamic(const std::vector<Vertex>& vertices,
                       const std::vector<std::uint32_t>& indices);

    void draw() const;
    void destroy();

    bool  valid() const { return m_vao != 0 && m_indexCount > 0; }
    int   indexCount() const { return (int)m_indexCount; }
    int   triangleCount() const { return (int)m_indexCount / 3; }

    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
    glm::vec3 boundsCenter{0.0f};
    float     boundsRadius = 0.0f;
    GLenum    primitive    = GL_TRIANGLES;

private:
    GLuint  m_vao        = 0;
    GLuint  m_vbo        = 0;
    GLuint  m_ebo        = 0;
    GLsizei m_indexCount = 0;
    size_t  m_vboBytes   = 0;
    size_t  m_eboBytes   = 0;
    bool    m_dynamic    = false;
};

// -----------------------------------------------------------------------------
//  MeshBuilder
// -----------------------------------------------------------------------------
class MeshBuilder {
public:
    std::vector<Vertex>        vertices;
    std::vector<std::uint32_t> indices;

    void clear();
    void reserve(size_t vertexCount, size_t indexCount);
    bool empty() const { return indices.empty(); }

    std::uint32_t addVertex(const Vertex& v);
    void          addTriangle(std::uint32_t a, std::uint32_t b, std::uint32_t c);

    /// Adds a quad (counter clockwise) with a flat normal derived from the corners.
    void addQuad(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2,
                 const glm::vec3& p3, const glm::vec3& color, float uvScale = 1.0f);
    void addQuadUV(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2,
                   const glm::vec3& p3, const glm::vec2& uv0, const glm::vec2& uv1,
                   const glm::vec2& uv2, const glm::vec2& uv3, const glm::vec3& color);
    /// Quad with explicit smooth normals (used by lofted surfaces).
    void addQuadN(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2,
                  const glm::vec3& p3, const glm::vec3& n0, const glm::vec3& n1,
                  const glm::vec3& n2, const glm::vec3& n3, const glm::vec2& uv0,
                  const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3,
                  const glm::vec3& color);

    void addBox(const glm::vec3& center, const glm::vec3& halfSize, const glm::vec3& color,
                float uvScale = 1.0f);
    /// Unit cube (-0.5..0.5) transformed by `xf`; the workhorse for scenery props.
    void addTransformedBox(const glm::mat4& xf, const glm::vec3& color, float uvScale = 1.0f);
    void addCylinder(const glm::vec3& base, const glm::vec3& axis, float radiusBottom,
                     float radiusTop, int segments, const glm::vec3& color, bool capBottom = true,
                     bool capTop = true);
    void addSphere(const glm::vec3& center, float radius, int rings, int segments,
                   const glm::vec3& color);
    /// Squashed sphere, handy for tree canopies and rounded shells.
    void addEllipsoid(const glm::vec3& center, const glm::vec3& radii, int rings, int segments,
                      const glm::vec3& color);
    void addCone(const glm::vec3& base, float radius, float height, int segments,
                 const glm::vec3& color);
    /// Connects two closed rings of equal size into a tube segment (smooth normals).
    void addRingBridge(const std::vector<glm::vec3>& ringA, const std::vector<glm::vec3>& ringB,
                       const glm::vec3& color, float vA, float vB, bool flip = false);
    void addFan(const std::vector<glm::vec3>& ring, const glm::vec3& center,
                const glm::vec3& normal, const glm::vec3& color);

    /// Appends another builder, optionally transformed.
    void append(const MeshBuilder& other);
    void append(const MeshBuilder& other, const glm::mat4& xf);

    void transform(const glm::mat4& xf);
    void setColor(const glm::vec3& color);
    void scaleUV(float s);
    /// Averages face normals into per-vertex normals (welding by position).
    void recomputeSmoothNormals(float weldEpsilon = 1e-4f);

    void      computeBounds(glm::vec3& outMin, glm::vec3& outMax) const;
    void      build(Mesh& mesh, bool dynamic = false) const;
    glm::vec3 centroid() const;

private:
    static glm::vec3 faceNormal(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c);
};

} // namespace vr
