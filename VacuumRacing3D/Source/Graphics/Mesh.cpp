#include "Mesh.h"

#include <cmath>
#include <cstring>
#include <map>

#include <glm/gtc/matrix_inverse.hpp>

#include "../Utilities/MathUtils.h"

namespace vr {

// ============================================================================ Mesh
Mesh::~Mesh() { destroy(); }

Mesh::Mesh(Mesh&& o) noexcept
    : boundsMin(o.boundsMin), boundsMax(o.boundsMax), boundsCenter(o.boundsCenter),
      boundsRadius(o.boundsRadius), primitive(o.primitive), m_vao(o.m_vao), m_vbo(o.m_vbo),
      m_ebo(o.m_ebo), m_indexCount(o.m_indexCount), m_vboBytes(o.m_vboBytes),
      m_eboBytes(o.m_eboBytes), m_dynamic(o.m_dynamic) {
    o.m_vao = o.m_vbo = o.m_ebo = 0;
    o.m_indexCount              = 0;
}

Mesh& Mesh::operator=(Mesh&& o) noexcept {
    if (this != &o) {
        destroy();
        boundsMin    = o.boundsMin;
        boundsMax    = o.boundsMax;
        boundsCenter = o.boundsCenter;
        boundsRadius = o.boundsRadius;
        primitive    = o.primitive;
        m_vao        = o.m_vao;
        m_vbo        = o.m_vbo;
        m_ebo        = o.m_ebo;
        m_indexCount = o.m_indexCount;
        m_vboBytes   = o.m_vboBytes;
        m_eboBytes   = o.m_eboBytes;
        m_dynamic    = o.m_dynamic;
        o.m_vao = o.m_vbo = o.m_ebo = 0;
        o.m_indexCount              = 0;
    }
    return *this;
}

void Mesh::destroy() {
    if (!glDeleteBuffers) return;
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    m_vao = m_vbo = m_ebo = 0;
    m_indexCount          = 0;
    m_vboBytes = m_eboBytes = 0;
}

void Mesh::upload(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices,
                  bool dynamic) {
    if (vertices.empty() || indices.empty()) {
        m_indexCount = 0;
        return;
    }
    m_dynamic = dynamic;
    if (!m_vao) {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ebo);
    }
    const GLenum usage = dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    m_vboBytes = vertices.size() * sizeof(Vertex);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)m_vboBytes, vertices.data(), usage);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    m_eboBytes = indices.size() * sizeof(std::uint32_t);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)m_eboBytes, indices.data(), usage);

    const GLsizei stride = (GLsizei)sizeof(Vertex);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (const void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (const void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (const void*)offsetof(Vertex, color));

    glBindVertexArray(0);
    m_indexCount = (GLsizei)indices.size();

    // Bounds (used for frustum culling).
    glm::vec3 mn(1e18f), mx(-1e18f);
    for (const Vertex& v : vertices) {
        mn = glm::min(mn, v.position);
        mx = glm::max(mx, v.position);
    }
    boundsMin    = mn;
    boundsMax    = mx;
    boundsCenter = (mn + mx) * 0.5f;
    boundsRadius = glm::length(mx - boundsCenter);
}

void Mesh::updateDynamic(const std::vector<Vertex>& vertices,
                         const std::vector<std::uint32_t>& indices) {
    if (vertices.empty() || indices.empty()) {
        m_indexCount = 0;
        return;
    }
    if (!m_vao) {
        upload(vertices, indices, true);
        return;
    }
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    const size_t vBytes = vertices.size() * sizeof(Vertex);
    if (vBytes > m_vboBytes) {
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)vBytes, vertices.data(), GL_DYNAMIC_DRAW);
        m_vboBytes = vBytes;
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)vBytes, vertices.data());
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    const size_t iBytes = indices.size() * sizeof(std::uint32_t);
    if (iBytes > m_eboBytes) {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)iBytes, indices.data(), GL_DYNAMIC_DRAW);
        m_eboBytes = iBytes;
    } else {
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, (GLsizeiptr)iBytes, indices.data());
    }
    glBindVertexArray(0);
    m_indexCount = (GLsizei)indices.size();
}

void Mesh::draw() const {
    if (!m_vao || m_indexCount == 0) return;
    glBindVertexArray(m_vao);
    glDrawElements(primitive, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

// ===================================================================== MeshBuilder
void MeshBuilder::clear() {
    vertices.clear();
    indices.clear();
}

void MeshBuilder::reserve(size_t vertexCount, size_t indexCount) {
    vertices.reserve(vertices.size() + vertexCount);
    indices.reserve(indices.size() + indexCount);
}

std::uint32_t MeshBuilder::addVertex(const Vertex& v) {
    vertices.push_back(v);
    return (std::uint32_t)(vertices.size() - 1);
}

void MeshBuilder::addTriangle(std::uint32_t a, std::uint32_t b, std::uint32_t c) {
    indices.push_back(a);
    indices.push_back(b);
    indices.push_back(c);
}

glm::vec3 MeshBuilder::faceNormal(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    const glm::vec3 n = glm::cross(b - a, c - a);
    const float     l = glm::length(n);
    return l > 1e-9f ? n / l : glm::vec3(0.0f, 1.0f, 0.0f);
}

void MeshBuilder::addQuad(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2,
                          const glm::vec3& p3, const glm::vec3& color, float uvScale) {
    const float su = glm::length(p1 - p0) * uvScale;
    const float sv = glm::length(p3 - p0) * uvScale;
    addQuadUV(p0, p1, p2, p3, glm::vec2(0.0f, 0.0f), glm::vec2(su, 0.0f), glm::vec2(su, sv),
              glm::vec2(0.0f, sv), color);
}

void MeshBuilder::addQuadUV(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2,
                            const glm::vec3& p3, const glm::vec2& uv0, const glm::vec2& uv1,
                            const glm::vec2& uv2, const glm::vec2& uv3, const glm::vec3& color) {
    const glm::vec3 n = faceNormal(p0, p1, p2);
    addQuadN(p0, p1, p2, p3, n, n, n, n, uv0, uv1, uv2, uv3, color);
}

void MeshBuilder::addQuadN(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2,
                           const glm::vec3& p3, const glm::vec3& n0, const glm::vec3& n1,
                           const glm::vec3& n2, const glm::vec3& n3, const glm::vec2& uv0,
                           const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3,
                           const glm::vec3& color) {
    const std::uint32_t base = (std::uint32_t)vertices.size();
    vertices.push_back({p0, n0, uv0, color});
    vertices.push_back({p1, n1, uv1, color});
    vertices.push_back({p2, n2, uv2, color});
    vertices.push_back({p3, n3, uv3, color});
    addTriangle(base + 0, base + 1, base + 2);
    addTriangle(base + 0, base + 2, base + 3);
}

void MeshBuilder::addBox(const glm::vec3& c, const glm::vec3& h, const glm::vec3& col,
                         float uvScale) {
    const glm::vec3 p000 = c + glm::vec3(-h.x, -h.y, -h.z);
    const glm::vec3 p100 = c + glm::vec3(h.x, -h.y, -h.z);
    const glm::vec3 p110 = c + glm::vec3(h.x, h.y, -h.z);
    const glm::vec3 p010 = c + glm::vec3(-h.x, h.y, -h.z);
    const glm::vec3 p001 = c + glm::vec3(-h.x, -h.y, h.z);
    const glm::vec3 p101 = c + glm::vec3(h.x, -h.y, h.z);
    const glm::vec3 p111 = c + glm::vec3(h.x, h.y, h.z);
    const glm::vec3 p011 = c + glm::vec3(-h.x, h.y, h.z);

    addQuad(p001, p101, p111, p011, col, uvScale); // +Z
    addQuad(p100, p000, p010, p110, col, uvScale); // -Z
    addQuad(p101, p100, p110, p111, col, uvScale); // +X
    addQuad(p000, p001, p011, p010, col, uvScale); // -X
    addQuad(p011, p111, p110, p010, col, uvScale); // +Y
    addQuad(p000, p100, p101, p001, col, uvScale); // -Y
}

void MeshBuilder::addTransformedBox(const glm::mat4& xf, const glm::vec3& color, float uvScale) {
    MeshBuilder tmp;
    tmp.addBox(glm::vec3(0.0f), glm::vec3(0.5f), color, uvScale);
    append(tmp, xf);
}

void MeshBuilder::addCylinder(const glm::vec3& base, const glm::vec3& axis, float radiusBottom,
                              float radiusTop, int segments, const glm::vec3& color, bool capBottom,
                              bool capTop) {
    if (segments < 3) segments = 3;
    const float     height = glm::length(axis);
    if (height < 1e-6f) return;
    const glm::vec3 up     = axis / height;
    glm::vec3       ref    = std::fabs(up.y) > 0.95f ? glm::vec3(1.0f, 0.0f, 0.0f)
                                                     : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 right  = glm::normalize(glm::cross(ref, up));
    const glm::vec3 fwd    = glm::cross(up, right);

    std::vector<glm::vec3> ringB(segments + 1), ringT(segments + 1);
    for (int i = 0; i <= segments; ++i) {
        const float a = (float)i / (float)segments * math::kTwoPi;
        const glm::vec3 dir = right * std::cos(a) + fwd * std::sin(a);
        ringB[(size_t)i] = base + dir * radiusBottom;
        ringT[(size_t)i] = base + axis + dir * radiusTop;
    }
    for (int i = 0; i < segments; ++i) {
        const glm::vec3& b0 = ringB[(size_t)i];
        const glm::vec3& b1 = ringB[(size_t)i + 1];
        const glm::vec3& t0 = ringT[(size_t)i];
        const glm::vec3& t1 = ringT[(size_t)i + 1];
        glm::vec3        n0 = glm::normalize(b0 - (base + up * glm::dot(b0 - base, up)));
        glm::vec3        n1 = glm::normalize(b1 - (base + up * glm::dot(b1 - base, up)));
        const float      u0 = (float)i / (float)segments;
        const float      u1 = (float)(i + 1) / (float)segments;
        addQuadN(b0, b1, t1, t0, n0, n1, n1, n0, glm::vec2(u0, 0.0f), glm::vec2(u1, 0.0f),
                 glm::vec2(u1, 1.0f), glm::vec2(u0, 1.0f), color);
    }
    if (capBottom) {
        std::vector<glm::vec3> ring(ringB.begin(), ringB.end() - 1);
        std::reverse(ring.begin(), ring.end());
        addFan(ring, base, -up, color);
    }
    if (capTop) {
        std::vector<glm::vec3> ring(ringT.begin(), ringT.end() - 1);
        addFan(ring, base + axis, up, color);
    }
}

void MeshBuilder::addFan(const std::vector<glm::vec3>& ring, const glm::vec3& center,
                         const glm::vec3& normal, const glm::vec3& color) {
    if (ring.size() < 3) return;
    const std::uint32_t c = addVertex({center, normal, glm::vec2(0.5f), color});
    const std::uint32_t base = (std::uint32_t)vertices.size();
    for (size_t i = 0; i < ring.size(); ++i) {
        const float a = (float)i / (float)ring.size() * math::kTwoPi;
        vertices.push_back({ring[i], normal,
                            glm::vec2(0.5f + 0.5f * std::cos(a), 0.5f + 0.5f * std::sin(a)), color});
    }
    for (size_t i = 0; i < ring.size(); ++i) {
        const std::uint32_t i0 = base + (std::uint32_t)i;
        const std::uint32_t i1 = base + (std::uint32_t)((i + 1) % ring.size());
        addTriangle(c, i0, i1);
    }
}

void MeshBuilder::addSphere(const glm::vec3& center, float radius, int rings, int segments,
                            const glm::vec3& color) {
    addEllipsoid(center, glm::vec3(radius), rings, segments, color);
}

void MeshBuilder::addEllipsoid(const glm::vec3& center, const glm::vec3& radii, int rings,
                               int segments, const glm::vec3& color) {
    if (rings < 2) rings = 2;
    if (segments < 3) segments = 3;
    const std::uint32_t base = (std::uint32_t)vertices.size();
    for (int y = 0; y <= rings; ++y) {
        const float v   = (float)y / (float)rings;
        const float phi = v * math::kPi;
        for (int x = 0; x <= segments; ++x) {
            const float u     = (float)x / (float)segments;
            const float theta = u * math::kTwoPi;
            const glm::vec3 unit(std::sin(phi) * std::cos(theta), std::cos(phi),
                                 std::sin(phi) * std::sin(theta));
            const glm::vec3 pos = center + unit * radii;
            const glm::vec3 nrm = glm::normalize(unit / glm::max(radii, glm::vec3(1e-4f)));
            vertices.push_back({pos, nrm, glm::vec2(u, v), color});
        }
    }
    const int stride = segments + 1;
    for (int y = 0; y < rings; ++y) {
        for (int x = 0; x < segments; ++x) {
            const std::uint32_t i0 = base + (std::uint32_t)(y * stride + x);
            const std::uint32_t i1 = i0 + 1;
            const std::uint32_t i2 = i0 + (std::uint32_t)stride;
            const std::uint32_t i3 = i2 + 1;
            addTriangle(i0, i2, i1);
            addTriangle(i1, i2, i3);
        }
    }
}

void MeshBuilder::addCone(const glm::vec3& base, float radius, float height, int segments,
                          const glm::vec3& color) {
    addCylinder(base, glm::vec3(0.0f, height, 0.0f), radius, radius * 0.05f, segments, color, true,
                false);
}

void MeshBuilder::addRingBridge(const std::vector<glm::vec3>& ringA,
                                const std::vector<glm::vec3>& ringB, const glm::vec3& color,
                                float vA, float vB, bool flip) {
    const size_t n = ringA.size();
    if (n < 3 || ringB.size() != n) return;

    glm::vec3 ca(0.0f), cb(0.0f);
    for (size_t i = 0; i < n; ++i) {
        ca += ringA[i];
        cb += ringB[i];
    }
    ca /= (float)n;
    cb /= (float)n;

    const std::uint32_t base = (std::uint32_t)vertices.size();
    for (size_t i = 0; i < n; ++i) {
        const float u = (float)i / (float)(n - 1);
        vertices.push_back({ringA[i], glm::normalize(ringA[i] - ca), glm::vec2(u, vA), color});
    }
    for (size_t i = 0; i < n; ++i) {
        const float u = (float)i / (float)(n - 1);
        vertices.push_back({ringB[i], glm::normalize(ringB[i] - cb), glm::vec2(u, vB), color});
    }
    for (size_t i = 0; i + 1 < n; ++i) {
        const std::uint32_t a0 = base + (std::uint32_t)i;
        const std::uint32_t a1 = base + (std::uint32_t)(i + 1);
        const std::uint32_t b0 = base + (std::uint32_t)(n + i);
        const std::uint32_t b1 = base + (std::uint32_t)(n + i + 1);
        if (flip) {
            addTriangle(a0, b0, a1);
            addTriangle(a1, b0, b1);
        } else {
            addTriangle(a0, a1, b0);
            addTriangle(a1, b1, b0);
        }
    }
}

void MeshBuilder::append(const MeshBuilder& other) {
    const std::uint32_t base = (std::uint32_t)vertices.size();
    vertices.insert(vertices.end(), other.vertices.begin(), other.vertices.end());
    indices.reserve(indices.size() + other.indices.size());
    for (std::uint32_t i : other.indices) indices.push_back(base + i);
}

void MeshBuilder::append(const MeshBuilder& other, const glm::mat4& xf) {
    const glm::mat3     nrm  = glm::mat3(glm::inverseTranspose(xf));
    const std::uint32_t base = (std::uint32_t)vertices.size();
    vertices.reserve(vertices.size() + other.vertices.size());
    for (const Vertex& v : other.vertices) {
        Vertex nv = v;
        nv.position = glm::vec3(xf * glm::vec4(v.position, 1.0f));
        nv.normal   = glm::normalize(nrm * v.normal);
        vertices.push_back(nv);
    }
    indices.reserve(indices.size() + other.indices.size());
    for (std::uint32_t i : other.indices) indices.push_back(base + i);
}

void MeshBuilder::transform(const glm::mat4& xf) {
    const glm::mat3 nrm = glm::mat3(glm::inverseTranspose(xf));
    for (Vertex& v : vertices) {
        v.position = glm::vec3(xf * glm::vec4(v.position, 1.0f));
        v.normal   = glm::normalize(nrm * v.normal);
    }
}

void MeshBuilder::setColor(const glm::vec3& color) {
    for (Vertex& v : vertices) v.color = color;
}

void MeshBuilder::scaleUV(float s) {
    for (Vertex& v : vertices) v.uv *= s;
}

void MeshBuilder::recomputeSmoothNormals(float weldEpsilon) {
    const float inv = 1.0f / glm::max(weldEpsilon, 1e-6f);
    std::map<std::tuple<int, int, int>, glm::vec3> accum;

    auto key = [inv](const glm::vec3& p) {
        return std::make_tuple((int)std::lround(p.x * inv), (int)std::lround(p.y * inv),
                               (int)std::lround(p.z * inv));
    };

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        Vertex& a = vertices[indices[i + 0]];
        Vertex& b = vertices[indices[i + 1]];
        Vertex& c = vertices[indices[i + 2]];
        const glm::vec3 n = glm::cross(b.position - a.position, c.position - a.position);
        accum[key(a.position)] += n;
        accum[key(b.position)] += n;
        accum[key(c.position)] += n;
    }
    for (Vertex& v : vertices) {
        auto it = accum.find(key(v.position));
        if (it != accum.end() && glm::length(it->second) > 1e-9f) {
            v.normal = glm::normalize(it->second);
        }
    }
}

void MeshBuilder::computeBounds(glm::vec3& outMin, glm::vec3& outMax) const {
    outMin = glm::vec3(1e18f);
    outMax = glm::vec3(-1e18f);
    for (const Vertex& v : vertices) {
        outMin = glm::min(outMin, v.position);
        outMax = glm::max(outMax, v.position);
    }
    if (vertices.empty()) {
        outMin = outMax = glm::vec3(0.0f);
    }
}

glm::vec3 MeshBuilder::centroid() const {
    if (vertices.empty()) return glm::vec3(0.0f);
    glm::vec3 c(0.0f);
    for (const Vertex& v : vertices) c += v.position;
    return c / (float)vertices.size();
}

void MeshBuilder::build(Mesh& mesh, bool dynamic) const { mesh.upload(vertices, indices, dynamic); }

} // namespace vr
