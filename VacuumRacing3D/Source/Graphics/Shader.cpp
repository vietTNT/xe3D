#include "Shader.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

namespace vr {
namespace {

GLuint compileStage(GLenum type, const std::string& src, const char* label,
                    const std::string& name) {
    const GLuint shader = glCreateShader(type);
    const char*  ptr    = src.c_str();
    const GLint  len    = (GLint)src.size();
    glShaderSource(shader, 1, &ptr, &len);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log((size_t)(logLen > 1 ? logLen : 1));
        glGetShaderInfoLog(shader, logLen, nullptr, log.data());
        std::fprintf(stderr, "[Shader] %s (%s) compile failed:\n%s\n", name.c_str(), label,
                     log.data());
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

} // namespace

Shader::~Shader() { destroy(); }

Shader::Shader(Shader&& other) noexcept
    : m_program(other.m_program), m_name(std::move(other.m_name)),
      m_uniforms(std::move(other.m_uniforms)) {
    other.m_program = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        destroy();
        m_program       = other.m_program;
        m_name          = std::move(other.m_name);
        m_uniforms      = std::move(other.m_uniforms);
        other.m_program = 0;
    }
    return *this;
}

void Shader::destroy() {
    if (m_program && glDeleteProgram) {
        glDeleteProgram(m_program);
    }
    m_program = 0;
    m_uniforms.clear();
}

bool Shader::compile(const std::string& vertexSrc, const std::string& fragmentSrc,
                     const std::string& debugName) {
    destroy();
    m_name = debugName;

    const GLuint vs = compileStage(GL_VERTEX_SHADER, vertexSrc, "vertex", m_name);
    if (!vs) return false;
    const GLuint fs = compileStage(GL_FRAGMENT_SHADER, fragmentSrc, "fragment", m_name);
    if (!fs) {
        glDeleteShader(vs);
        return false;
    }

    const GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    glDeleteShader(vs);
    glDeleteShader(fs);

    if (ok != GL_TRUE) {
        GLint logLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log((size_t)(logLen > 1 ? logLen : 1));
        glGetProgramInfoLog(prog, logLen, nullptr, log.data());
        std::fprintf(stderr, "[Shader] %s link failed:\n%s\n", m_name.c_str(), log.data());
        glDeleteProgram(prog);
        return false;
    }

    m_program = prog;
    return true;
}

bool Shader::loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    auto read = [](const std::string& path, std::string& out) -> bool {
        std::ifstream f(path, std::ios::in | std::ios::binary);
        if (!f) {
            std::fprintf(stderr, "[Shader] cannot open %s\n", path.c_str());
            return false;
        }
        std::ostringstream ss;
        ss << f.rdbuf();
        out = ss.str();
        return true;
    };

    std::string vs, fs;
    if (!read(vertexPath, vs) || !read(fragmentPath, fs)) return false;
    return compile(vs, fs, vertexPath);
}

void Shader::bind() const {
    if (m_program) glUseProgram(m_program);
}

void Shader::unbind() { glUseProgram(0); }

GLint Shader::location(const char* name) {
    auto it = m_uniforms.find(name);
    if (it != m_uniforms.end()) return it->second;
    const GLint loc = glGetUniformLocation(m_program, name);
    m_uniforms.emplace(name, loc);
    return loc;
}

void Shader::setInt(const char* name, int v) {
    const GLint l = location(name);
    if (l >= 0) glUniform1i(l, v);
}
void Shader::setFloat(const char* name, float v) {
    const GLint l = location(name);
    if (l >= 0) glUniform1f(l, v);
}
void Shader::setVec2(const char* name, const glm::vec2& v) {
    const GLint l = location(name);
    if (l >= 0) glUniform2f(l, v.x, v.y);
}
void Shader::setVec3(const char* name, const glm::vec3& v) {
    const GLint l = location(name);
    if (l >= 0) glUniform3f(l, v.x, v.y, v.z);
}
void Shader::setVec4(const char* name, const glm::vec4& v) {
    const GLint l = location(name);
    if (l >= 0) glUniform4f(l, v.x, v.y, v.z, v.w);
}
void Shader::setMat4(const char* name, const glm::mat4& v) {
    const GLint l = location(name);
    if (l >= 0) glUniformMatrix4fv(l, 1, GL_FALSE, &v[0][0]);
}

} // namespace vr
