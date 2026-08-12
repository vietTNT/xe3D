#include "Window.h"

#include <cstdio>

#include "GL.h"

namespace vr {
namespace {
void errorCallback(int code, const char* description) {
    std::fprintf(stderr, "[GLFW] error %d: %s\n", code, description ? description : "?");
}
} // namespace

Window::~Window() { destroy(); }

bool Window::create(const WindowConfig& config) {
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) {
        std::fprintf(stderr, "[Window] glfwInit failed\n");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, config.msaa);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_FALSE);

    GLFWmonitor* monitor = config.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    int          w = config.width;
    int          h = config.height;
    if (monitor) {
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (mode) {
            w = mode->width;
            h = mode->height;
        }
    }

    m_window = glfwCreateWindow(w, h, config.title.c_str(), monitor, nullptr);
    if (!m_window) {
        std::fprintf(stderr, "[Window] failed to create an OpenGL 3.3 core context.\n");
        glfwTerminate();
        return false;
    }
    m_fullscreen = config.fullscreen;

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(config.vsync ? 1 : 0);

    if (!loadOpenGL((GLProcLoader)glfwGetProcAddress)) {
        std::fprintf(stderr, "[Window] %d OpenGL entry points are missing.\n",
                     missingOpenGLFunctions());
        // Continue anyway: some drivers alias a handful of names, the renderer
        // will report a clearer failure if something essential is absent.
    }

    glfwSetWindowUserPointer(m_window, this);
    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetCursorPosCallback(m_window, cursorPosCallback);
    glfwSetScrollCallback(m_window, scrollCallback);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetCharCallback(m_window, charCallback);

    glfwGetWindowSize(m_window, &m_width, &m_height);
    glfwGetFramebufferSize(m_window, &m_fbWidth, &m_fbHeight);
    if (!m_fullscreen) {
        glfwGetWindowPos(m_window, &m_savedX, &m_savedY);
        m_savedW = m_width;
        m_savedH = m_height;
    }

    std::printf("[GL] %s | %s | GLSL ready\n",
                glGetString ? (const char*)glGetString(GL_RENDERER) : "?",
                glGetString ? (const char*)glGetString(GL_VERSION) : "?");
    return true;
}

void Window::destroy() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
        glfwTerminate();
    }
}

bool Window::shouldClose() const { return m_window ? glfwWindowShouldClose(m_window) != 0 : true; }

void Window::requestClose() {
    if (m_window) glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

void Window::pollEvents() {
    m_input.beginFrame();
    glfwPollEvents();
}

void Window::swapBuffers() {
    if (m_window) glfwSwapBuffers(m_window);
}

void Window::setFullscreen(bool enabled) {
    if (!m_window || enabled == m_fullscreen) return;
    if (enabled) {
        glfwGetWindowPos(m_window, &m_savedX, &m_savedY);
        glfwGetWindowSize(m_window, &m_savedW, &m_savedH);
        GLFWmonitor*       monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode    = monitor ? glfwGetVideoMode(monitor) : nullptr;
        if (monitor && mode) {
            glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height,
                                 mode->refreshRate);
        }
    } else {
        glfwSetWindowMonitor(m_window, nullptr, m_savedX, m_savedY, m_savedW, m_savedH, 0);
    }
    m_fullscreen = enabled;
    glfwGetWindowSize(m_window, &m_width, &m_height);
    glfwGetFramebufferSize(m_window, &m_fbWidth, &m_fbHeight);
    m_resized = true;
}

void Window::setVSync(bool enabled) { glfwSwapInterval(enabled ? 1 : 0); }

void Window::setSize(int width, int height) {
    if (!m_window || m_fullscreen) return;
    glfwSetWindowSize(m_window, width, height);
}

void Window::setCursorVisible(bool visible) {
    if (!m_window) return;
    glfwSetInputMode(m_window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
}

void Window::setTitle(const std::string& title) {
    if (m_window) glfwSetWindowTitle(m_window, title.c_str());
}

bool Window::consumeResizeFlag() {
    const bool r = m_resized;
    m_resized    = false;
    return r;
}

double Window::time() const { return glfwGetTime(); }

// ------------------------------------------------------------------- callbacks
Window* self(GLFWwindow* w) { return static_cast<Window*>(glfwGetWindowUserPointer(w)); }

void Window::keyCallback(GLFWwindow* w, int key, int, int action, int) {
    if (Window* s = self(w)) s->m_input.onKey(key, action);
}
void Window::mouseButtonCallback(GLFWwindow* w, int button, int action, int) {
    if (Window* s = self(w)) s->m_input.onMouseButton(button, action);
}
void Window::cursorPosCallback(GLFWwindow* w, double x, double y) {
    if (Window* s = self(w)) s->m_input.onCursorPos(x, y);
}
void Window::scrollCallback(GLFWwindow* w, double, double dy) {
    if (Window* s = self(w)) s->m_input.onScroll(dy);
}
void Window::charCallback(GLFWwindow* w, unsigned int cp) {
    if (Window* s = self(w)) s->m_input.onChar(cp);
}
void Window::framebufferSizeCallback(GLFWwindow* w, int width, int height) {
    Window* s = self(w);
    if (!s) return;
    s->m_fbWidth  = width;
    s->m_fbHeight = height;
    glfwGetWindowSize(w, &s->m_width, &s->m_height);
    s->m_resized = true;
}

} // namespace vr
