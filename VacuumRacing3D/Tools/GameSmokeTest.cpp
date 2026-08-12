// -----------------------------------------------------------------------------
//  GameSmokeTest.cpp - runs the real Game state machine with no window.
//
//  This file provides its own implementations of the handful of GLFW entry
//  points Window.cpp uses, backed by an EGL off-screen context. That means the
//  actual Game class - staged loading, main menu, colour picker, race start,
//  racing frames, pause - is exercised end to end, which a pure gameplay test
//  such as HeadlessSim cannot reach.
//
//  It exists because a crash slipped through once: the loading screen indexed
//  into the car array before it was populated. A test that never runs Game.cpp
//  cannot catch that class of bug.
//
//  Build (Linux, needs EGL; do NOT link glfw):
//     g++ -std=c++17 Tools/GameSmokeTest.cpp <all Source/*.cpp> -ISource -lEGL
// -----------------------------------------------------------------------------
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <cstdio>
#include <cstring>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Game.h"

// =============================================================== fake GLFW layer
struct GLFWwindow {
    int  width      = 640;
    int  height     = 360;
    bool shouldClose = false;
    void* user       = nullptr;

    GLFWkeyfun         onKey    = nullptr;
    GLFWmousebuttonfun onButton = nullptr;
    GLFWcursorposfun   onCursor = nullptr;
    GLFWscrollfun      onScroll = nullptr;
    GLFWframebuffersizefun onResize = nullptr;
    GLFWcharfun        onChar   = nullptr;
};

namespace {

GLFWwindow  g_window;
double      g_time      = 0.0;
long        g_pollCount = 0;
EGLDisplay  g_display   = EGL_NO_DISPLAY;
EGLContext  g_context   = EGL_NO_CONTEXT;

/// A scripted key event: fire `key` at frame `frame` (action 1 = press).
struct ScriptedKey {
    long frame;
    int  key;
    int  action;
};
std::vector<ScriptedKey> g_script;
size_t                   g_scriptCursor = 0;

bool initEGL() {
    auto getPlatformDisplay =
        (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
    if (getPlatformDisplay) {
        g_display = getPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY,
                                       nullptr);
    }
    if (g_display == EGL_NO_DISPLAY) g_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_display == EGL_NO_DISPLAY) return false;

    EGLint major = 0, minor = 0;
    if (!eglInitialize(g_display, &major, &minor)) return false;
    eglBindAPI(EGL_OPENGL_API);

    EGLConfig config = (EGLConfig)0;   // EGL_NO_CONFIG_KHR
    {
        const EGLint attribs[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT, EGL_NONE};
        EGLint       n = 0;
        EGLConfig    picked;
        if (eglChooseConfig(g_display, attribs, &picked, 1, &n) && n > 0) config = picked;
    }
    const EGLint ctxAttribs[] = {EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 3,
                                 EGL_CONTEXT_OPENGL_PROFILE_MASK,
                                 EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT, EGL_NONE};
    g_context = eglCreateContext(g_display, config, EGL_NO_CONTEXT, ctxAttribs);
    if (g_context == EGL_NO_CONTEXT) return false;
    return eglMakeCurrent(g_display, EGL_NO_SURFACE, EGL_NO_SURFACE, g_context);
}

} // namespace

// ---- the subset of GLFW that Window.cpp touches -----------------------------
int  glfwInit(void) { return initEGL() ? GLFW_TRUE : GLFW_FALSE; }
void glfwTerminate(void) {}
GLFWerrorfun glfwSetErrorCallback(GLFWerrorfun) { return nullptr; }
void glfwWindowHint(int, int) {}
GLFWmonitor* glfwGetPrimaryMonitor(void) { return nullptr; }
const GLFWvidmode* glfwGetVideoMode(GLFWmonitor*) { return nullptr; }

GLFWwindow* glfwCreateWindow(int w, int h, const char*, GLFWmonitor*, GLFWwindow*) {
    g_window.width  = w > 0 ? w : 640;
    g_window.height = h > 0 ? h : 360;
    return &g_window;
}
void glfwDestroyWindow(GLFWwindow*) {}
void glfwMakeContextCurrent(GLFWwindow*) {}
void glfwSwapInterval(int) {}
void glfwSwapBuffers(GLFWwindow*) {}
int  glfwWindowShouldClose(GLFWwindow* w) { return w && w->shouldClose ? 1 : 0; }
void glfwSetWindowShouldClose(GLFWwindow* w, int v) { if (w) w->shouldClose = (v != 0); }

GLFWglproc glfwGetProcAddress(const char* name) { return (GLFWglproc)eglGetProcAddress(name); }

void  glfwSetWindowUserPointer(GLFWwindow* w, void* p) { if (w) w->user = p; }
void* glfwGetWindowUserPointer(GLFWwindow* w) { return w ? w->user : nullptr; }

GLFWkeyfun         glfwSetKeyCallback(GLFWwindow* w, GLFWkeyfun f) { w->onKey = f; return nullptr; }
GLFWmousebuttonfun glfwSetMouseButtonCallback(GLFWwindow* w, GLFWmousebuttonfun f) {
    w->onButton = f; return nullptr;
}
GLFWcursorposfun   glfwSetCursorPosCallback(GLFWwindow* w, GLFWcursorposfun f) {
    w->onCursor = f; return nullptr;
}
GLFWscrollfun      glfwSetScrollCallback(GLFWwindow* w, GLFWscrollfun f) {
    w->onScroll = f; return nullptr;
}
GLFWframebuffersizefun glfwSetFramebufferSizeCallback(GLFWwindow* w, GLFWframebuffersizefun f) {
    w->onResize = f; return nullptr;
}
GLFWcharfun glfwSetCharCallback(GLFWwindow* w, GLFWcharfun f) { w->onChar = f; return nullptr; }

void glfwGetWindowSize(GLFWwindow* w, int* x, int* y) {
    if (x) *x = w->width;
    if (y) *y = w->height;
}
void glfwGetFramebufferSize(GLFWwindow* w, int* x, int* y) {
    if (x) *x = w->width;
    if (y) *y = w->height;
}
void glfwGetWindowPos(GLFWwindow*, int* x, int* y) {
    if (x) *x = 0;
    if (y) *y = 0;
}
void glfwSetWindowMonitor(GLFWwindow*, GLFWmonitor*, int, int, int, int, int) {}
void glfwSetWindowSize(GLFWwindow*, int, int) {}
void glfwSetWindowTitle(GLFWwindow*, const char*) {}
void glfwSetInputMode(GLFWwindow*, int, int) {}
double glfwGetTime(void) { return g_time; }

void glfwPollEvents(void) {
    ++g_pollCount;
    g_time += 1.0 / 60.0;
    while (g_scriptCursor < g_script.size() && g_script[g_scriptCursor].frame == g_pollCount) {
        const ScriptedKey& e = g_script[g_scriptCursor++];
        if (g_window.onKey) g_window.onKey(&g_window, e.key, 0, e.action, 0);
    }
    if (g_pollCount > 900) g_window.shouldClose = true;   // safety net
}

// ===================================================================== the test
int main(int argc, char** argv) {
    // Frame numbers are poll counts. The staged load takes six frames.
    auto tap = [](long frame, int key) {
        g_script.push_back({frame, key, 1});
        g_script.push_back({frame + 2, key, 0});
    };

    tap(20, GLFW_KEY_ENTER);                       // main menu  -> START RACE
    for (int i = 0; i < 6; ++i) tap(40 + i * 4, GLFW_KEY_DOWN);  // focus the START button
    tap(80, GLFW_KEY_ENTER);                       // colour select -> start the race
    g_script.push_back({100, GLFW_KEY_W, 1});      // hold the throttle
    tap(300, GLFW_KEY_C);                          // change camera
    tap(340, GLFW_KEY_ESCAPE);                     // pause
    tap(380, GLFW_KEY_ESCAPE);                     // resume
    tap(420, GLFW_KEY_R);                          // restart the race
    tap(520, GLFW_KEY_ESCAPE);                     // pause again
    for (int i = 0; i < 2; ++i) tap(540 + i * 6, GLFW_KEY_DOWN);
    tap(570, GLFW_KEY_ENTER);                      // -> MAIN MENU
    tap(620, GLFW_KEY_DOWN);
    tap(640, GLFW_KEY_ENTER);                      // -> SETTINGS
    tap(680, GLFW_KEY_RIGHT);                      // change an option
    tap(700, GLFW_KEY_ESCAPE);                     // back
    tap(760, GLFW_KEY_DOWN);
    tap(770, GLFW_KEY_DOWN);
    tap(780, GLFW_KEY_ENTER);                      // EXIT

    std::printf("=== Game smoke test: driving the real state machine ===\n");

    vr::Game game;
    if (!game.init(argc, argv)) {
        std::printf("FAIL: Game::init returned false\n");
        return 1;
    }
    game.run();
    game.shutdown();

    std::printf("\nsurvived %ld frames through loading, menu, colour select, race, pause,\n"
                "restart, settings and exit without crashing.\n", g_pollCount);
    std::printf("PASSED\n");
    return 0;
}
