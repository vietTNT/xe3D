#include "GL.h"

#include <cstdio>
#include <cstring>

#define X(ret, name, params) PFN_##name name = nullptr;
VR_GL_FUNCTIONS
#undef X

namespace vr {
namespace {
int g_missing = 0;
}

bool loadOpenGL(GLProcLoader loader) {
    if (!loader) return false;
    g_missing = 0;

#define X(ret, name, params)                                     \
    name = (PFN_##name)loader(#name);                            \
    if (!name) {                                                 \
        std::fprintf(stderr, "[GL] missing entry point: %s\n", #name); \
        ++g_missing;                                             \
    }
    VR_GL_FUNCTIONS
#undef X

    return g_missing == 0;
}

int missingOpenGLFunctions() { return g_missing; }

void checkGLError(const char* tag) {
    if (!glGetError) return;
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::fprintf(stderr, "[GL] error 0x%04X at %s\n", (unsigned)err, tag ? tag : "?");
    }
}

} // namespace vr
