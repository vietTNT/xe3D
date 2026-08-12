// -----------------------------------------------------------------------------
//  Window.h - GLFW window + OpenGL 3.3 core context.
// -----------------------------------------------------------------------------
#pragma once

#include <string>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Input.h"

namespace vr {

struct WindowConfig {
    int         width      = 1600;
    int         height     = 900;
    bool        fullscreen = false;
    bool        vsync      = true;
    int         msaa       = 0;   ///< scene is rendered off-screen, FXAA handles edges
    std::string title      = "Dua Xe May Hut Bui - Vacuum Racing 3D";
};

class Window {
public:
    ~Window();

    bool create(const WindowConfig& config);
    void destroy();

    bool shouldClose() const;
    void requestClose();
    void pollEvents();
    void swapBuffers();

    void setFullscreen(bool enabled);
    bool fullscreen() const { return m_fullscreen; }
    void setVSync(bool enabled);
    void setSize(int width, int height);
    void setCursorVisible(bool visible);
    void setTitle(const std::string& title);

    int   width() const { return m_width; }
    int   height() const { return m_height; }
    int   framebufferWidth() const { return m_fbWidth; }
    int   framebufferHeight() const { return m_fbHeight; }
    float aspect() const {
        return m_fbHeight > 0 ? (float)m_fbWidth / (float)m_fbHeight : 1.7778f;
    }
    /// True for exactly one frame after the framebuffer changed size.
    bool consumeResizeFlag();

    Input&       input() { return m_input; }
    const Input& input() const { return m_input; }
    GLFWwindow*  handle() const { return m_window; }

    double time() const;

private:
    static void keyCallback(GLFWwindow*, int, int, int, int);
    static void mouseButtonCallback(GLFWwindow*, int, int, int);
    static void cursorPosCallback(GLFWwindow*, double, double);
    static void scrollCallback(GLFWwindow*, double, double);
    static void framebufferSizeCallback(GLFWwindow*, int, int);
    static void charCallback(GLFWwindow*, unsigned int);

    GLFWwindow* m_window     = nullptr;
    Input       m_input;
    int         m_width      = 0;
    int         m_height     = 0;
    int         m_fbWidth    = 0;
    int         m_fbHeight   = 0;
    bool        m_fullscreen = false;
    bool        m_resized    = false;
    // Saved windowed placement so we can restore it when leaving fullscreen.
    int         m_savedX = 100, m_savedY = 100, m_savedW = 1600, m_savedH = 900;
};

} // namespace vr
