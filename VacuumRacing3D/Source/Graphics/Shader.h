// -----------------------------------------------------------------------------
//  Shader.h - GLSL program wrapper with cached uniform lookups.
// -----------------------------------------------------------------------------
#pragma once

#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

#include "../Core/GL.h"

namespace vr {

class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    /// Compiles from source strings. Returns false and logs on failure.
    bool compile(const std::string& vertexSrc, const std::string& fragmentSrc,
                 const std::string& debugName = "shader");

    /// Loads both stages from disk (paths resolved by ResourceManager).
    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);

    void bind() const;
    static void unbind();

    bool   valid() const { return m_program != 0; }
    GLuint id() const { return m_program; }
    void   destroy();

    void setInt(const char* name, int v);
    void setFloat(const char* name, float v);
    void setVec2(const char* name, const glm::vec2& v);
    void setVec3(const char* name, const glm::vec3& v);
    void setVec4(const char* name, const glm::vec4& v);
    void setMat4(const char* name, const glm::mat4& v);

private:
    GLint location(const char* name);

    GLuint      m_program = 0;
    std::string m_name;
    std::unordered_map<std::string, GLint> m_uniforms;
};

} // namespace vr
