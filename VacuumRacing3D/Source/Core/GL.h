// -----------------------------------------------------------------------------
//  GL.h - Minimal, self contained OpenGL 3.3 Core function loader.
//
//  The project deliberately avoids shipping a generated GLAD file: everything
//  the renderer needs is declared here through a single X-macro list, which
//  keeps the dependency footprint at GLFW + GLM only.
//
//  Include this header instead of <GL/gl.h>.  Never include a system GL header.
// -----------------------------------------------------------------------------
#pragma once

#include <cstddef>

#if defined(_WIN32)
#define VR_GLAPI __stdcall
#else
#define VR_GLAPI
#endif

// ---------------------------------------------------------------- GL typedefs
typedef unsigned int  GLenum;
typedef unsigned char GLboolean;
typedef unsigned int  GLbitfield;
typedef signed char   GLbyte;
typedef short         GLshort;
typedef int           GLint;
typedef int           GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int  GLuint;
typedef float         GLfloat;
typedef float         GLclampf;
typedef double        GLdouble;
typedef char          GLchar;
typedef void          GLvoid;
typedef std::ptrdiff_t GLintptr;
typedef std::ptrdiff_t GLsizeiptr;

// ---------------------------------------------------------------- GL constants
#define GL_FALSE                          0
#define GL_TRUE                           1
#define GL_ZERO                           0
#define GL_ONE                            1
#define GL_NONE                           0
#define GL_NO_ERROR                       0

#define GL_POINTS                         0x0000
#define GL_LINES                          0x0001
#define GL_LINE_STRIP                     0x0003
#define GL_TRIANGLES                      0x0004
#define GL_TRIANGLE_STRIP                 0x0005
#define GL_TRIANGLE_FAN                   0x0006

#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_STENCIL_BUFFER_BIT             0x00000400
#define GL_COLOR_BUFFER_BIT               0x00004000

#define GL_NEVER                          0x0200
#define GL_LESS                           0x0201
#define GL_EQUAL                          0x0202
#define GL_LEQUAL                         0x0203
#define GL_GREATER                        0x0204
#define GL_ALWAYS                         0x0207

#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_DST_ALPHA                      0x0304
#define GL_ONE_MINUS_DST_ALPHA            0x0305

#define GL_FRONT                          0x0404
#define GL_BACK                           0x0405
#define GL_FRONT_AND_BACK                 0x0408
#define GL_CW                             0x0900
#define GL_CCW                            0x0901

#define GL_CULL_FACE                      0x0B44
#define GL_DEPTH_TEST                     0x0B71
#define GL_BLEND                          0x0BE2
#define GL_SCISSOR_TEST                   0x0C11
#define GL_UNPACK_ALIGNMENT               0x0CF5
#define GL_TEXTURE_2D                     0x0DE1
#define GL_MULTISAMPLE                    0x809D
#define GL_POLYGON_OFFSET_FILL            0x8037
#define GL_LINE_SMOOTH                    0x0B20

#define GL_BYTE                           0x1400
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_SHORT                          0x1402
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_INT                            0x1404
#define GL_UNSIGNED_INT                   0x1405
#define GL_FLOAT                          0x1406

#define GL_VENDOR                         0x1F00
#define GL_RENDERER                       0x1F01
#define GL_VERSION                        0x1F02

#define GL_DEPTH_COMPONENT                0x1902
#define GL_RED                            0x1903
#define GL_RGB                            0x1907
#define GL_RGBA                           0x1908

#define GL_NEAREST                        0x2600
#define GL_LINEAR                         0x2601
#define GL_NEAREST_MIPMAP_NEAREST         0x2700
#define GL_LINEAR_MIPMAP_NEAREST          0x2701
#define GL_NEAREST_MIPMAP_LINEAR          0x2702
#define GL_LINEAR_MIPMAP_LINEAR           0x2703
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_TEXTURE_WRAP_S                 0x2802
#define GL_TEXTURE_WRAP_T                 0x2803
#define GL_TEXTURE_BORDER_COLOR           0x1004
#define GL_REPEAT                         0x2901
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_CLAMP_TO_BORDER                0x812D
#define GL_MIRRORED_REPEAT                0x8370
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

#define GL_R8                             0x8229
#define GL_RGB8                           0x8051
#define GL_RGBA8                          0x8058
#define GL_SRGB8                          0x8C41
#define GL_SRGB8_ALPHA8                   0x8C43
#define GL_DEPTH_COMPONENT16              0x81A5
#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_DEPTH_COMPONENT32F             0x8CAC
#define GL_RGB16F                         0x881B
#define GL_RGBA16F                        0x881A
#define GL_HALF_FLOAT                     0x140B

#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STREAM_DRAW                    0x88E0
#define GL_STATIC_DRAW                    0x88E4
#define GL_DYNAMIC_DRAW                   0x88E8

#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_INFO_LOG_LENGTH                0x8B84

#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE1                       0x84C1
#define GL_TEXTURE2                       0x84C2
#define GL_TEXTURE3                       0x84C3

#define GL_FRAMEBUFFER                    0x8D40
#define GL_RENDERBUFFER                   0x8D41
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_DEPTH_ATTACHMENT               0x8D00
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5
#define GL_MAX_SAMPLES                    0x8D57

// --------------------------------------------------------- function pointer set
#define VR_GL_FUNCTIONS                                                            \
    X(void,   glClear,                  (GLbitfield mask))                         \
    X(void,   glClearColor,             (GLfloat, GLfloat, GLfloat, GLfloat))      \
    X(void,   glEnable,                 (GLenum))                                  \
    X(void,   glDisable,                (GLenum))                                  \
    X(void,   glViewport,               (GLint, GLint, GLsizei, GLsizei))          \
    X(void,   glScissor,                (GLint, GLint, GLsizei, GLsizei))          \
    X(void,   glDepthFunc,              (GLenum))                                  \
    X(void,   glDepthMask,              (GLboolean))                               \
    X(void,   glBlendFunc,              (GLenum, GLenum))                          \
    X(void,   glBlendFuncSeparate,      (GLenum, GLenum, GLenum, GLenum))          \
    X(void,   glCullFace,               (GLenum))                                  \
    X(void,   glFrontFace,              (GLenum))                                  \
    X(void,   glPolygonOffset,          (GLfloat, GLfloat))                        \
    X(void,   glPixelStorei,            (GLenum, GLint))                           \
    X(void,   glGetIntegerv,            (GLenum, GLint*))                          \
    X(void,   glGetFloatv,              (GLenum, GLfloat*))                        \
    X(const GLubyte*, glGetString,      (GLenum))                                  \
    X(GLenum, glGetError,               (void))                                    \
    X(void,   glDrawArrays,             (GLenum, GLint, GLsizei))                  \
    X(void,   glDrawElements,           (GLenum, GLsizei, GLenum, const void*))    \
    X(void,   glGenBuffers,             (GLsizei, GLuint*))                        \
    X(void,   glBindBuffer,             (GLenum, GLuint))                          \
    X(void,   glBufferData,             (GLenum, GLsizeiptr, const void*, GLenum)) \
    X(void,   glBufferSubData,          (GLenum, GLintptr, GLsizeiptr, const void*)) \
    X(void,   glDeleteBuffers,          (GLsizei, const GLuint*))                  \
    X(void,   glGenVertexArrays,        (GLsizei, GLuint*))                        \
    X(void,   glBindVertexArray,        (GLuint))                                  \
    X(void,   glDeleteVertexArrays,     (GLsizei, const GLuint*))                  \
    X(void,   glEnableVertexAttribArray,(GLuint))                                  \
    X(void,   glVertexAttribPointer,    (GLuint, GLint, GLenum, GLboolean, GLsizei, const void*)) \
    X(GLuint, glCreateShader,           (GLenum))                                  \
    X(void,   glShaderSource,           (GLuint, GLsizei, const GLchar* const*, const GLint*)) \
    X(void,   glCompileShader,          (GLuint))                                  \
    X(void,   glGetShaderiv,            (GLuint, GLenum, GLint*))                  \
    X(void,   glGetShaderInfoLog,       (GLuint, GLsizei, GLsizei*, GLchar*))      \
    X(void,   glDeleteShader,           (GLuint))                                  \
    X(GLuint, glCreateProgram,          (void))                                    \
    X(void,   glAttachShader,           (GLuint, GLuint))                          \
    X(void,   glLinkProgram,            (GLuint))                                  \
    X(void,   glGetProgramiv,           (GLuint, GLenum, GLint*))                  \
    X(void,   glGetProgramInfoLog,      (GLuint, GLsizei, GLsizei*, GLchar*))      \
    X(void,   glDeleteProgram,          (GLuint))                                  \
    X(void,   glUseProgram,             (GLuint))                                  \
    X(GLint,  glGetUniformLocation,     (GLuint, const GLchar*))                   \
    X(void,   glUniform1i,              (GLint, GLint))                            \
    X(void,   glUniform1f,              (GLint, GLfloat))                          \
    X(void,   glUniform2f,              (GLint, GLfloat, GLfloat))                 \
    X(void,   glUniform3f,              (GLint, GLfloat, GLfloat, GLfloat))        \
    X(void,   glUniform4f,              (GLint, GLfloat, GLfloat, GLfloat, GLfloat)) \
    X(void,   glUniform3fv,             (GLint, GLsizei, const GLfloat*))          \
    X(void,   glUniformMatrix4fv,       (GLint, GLsizei, GLboolean, const GLfloat*)) \
    X(void,   glGenTextures,            (GLsizei, GLuint*))                        \
    X(void,   glBindTexture,            (GLenum, GLuint))                          \
    X(void,   glTexImage2D,             (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*)) \
    X(void,   glTexSubImage2D,          (GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void*)) \
    X(void,   glTexParameteri,          (GLenum, GLenum, GLint))                   \
    X(void,   glTexParameterf,          (GLenum, GLenum, GLfloat))                 \
    X(void,   glTexParameterfv,         (GLenum, GLenum, const GLfloat*))          \
    X(void,   glGenerateMipmap,         (GLenum))                                  \
    X(void,   glActiveTexture,          (GLenum))                                  \
    X(void,   glDeleteTextures,         (GLsizei, const GLuint*))                  \
    X(void,   glGenFramebuffers,        (GLsizei, GLuint*))                        \
    X(void,   glBindFramebuffer,        (GLenum, GLuint))                          \
    X(void,   glFramebufferTexture2D,   (GLenum, GLenum, GLenum, GLuint, GLint))   \
    X(GLenum, glCheckFramebufferStatus, (GLenum))                                  \
    X(void,   glDeleteFramebuffers,     (GLsizei, const GLuint*))                  \
    X(void,   glGenRenderbuffers,       (GLsizei, GLuint*))                        \
    X(void,   glBindRenderbuffer,       (GLenum, GLuint))                          \
    X(void,   glRenderbufferStorage,    (GLenum, GLenum, GLsizei, GLsizei))        \
    X(void,   glFramebufferRenderbuffer,(GLenum, GLenum, GLenum, GLuint))          \
    X(void,   glDeleteRenderbuffers,    (GLsizei, const GLuint*))                  \
    X(void,   glDrawBuffer,             (GLenum))                                  \
    X(void,   glReadBuffer,             (GLenum))                                  \
    X(void,   glFinish,                 (void))                                    \
    X(void,   glReadPixels,             (GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*))

#define X(ret, name, params)                       \
    typedef ret(VR_GLAPI* PFN_##name) params;      \
    extern PFN_##name name;
VR_GL_FUNCTIONS
#undef X

namespace vr {

/// Signature returned by glfwGetProcAddress.
typedef void (*GLProc)(void);
typedef GLProc (*GLProcLoader)(const char*);

/// Resolves every entry point in VR_GL_FUNCTIONS. Returns false if any is missing.
bool loadOpenGL(GLProcLoader loader);

/// Number of entry points that failed to resolve during the last loadOpenGL call.
int  missingOpenGLFunctions();

/// Logs any pending GL error together with a tag. Compiled out in release builds.
void checkGLError(const char* tag);

} // namespace vr

#if defined(VR_DEBUG_GL)
#define VR_GL_CHECK(tag) ::vr::checkGLError(tag)
#else
#define VR_GL_CHECK(tag) ((void)0)
#endif
