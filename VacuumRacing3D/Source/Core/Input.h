// -----------------------------------------------------------------------------
//  Input.h - edge aware keyboard/mouse state, refreshed once per frame.
// -----------------------------------------------------------------------------
#pragma once

#include <array>

#include <glm/glm.hpp>

namespace vr {

class Input {
public:
    static constexpr int kMaxKeys    = 512;
    static constexpr int kMaxButtons = 8;

    void beginFrame();

    // Called by the GLFW callbacks.
    void onKey(int key, int action);
    void onMouseButton(int button, int action);
    void onCursorPos(double x, double y);
    void onScroll(double dy);
    void onChar(unsigned int codepoint);

    bool keyDown(int key) const;
    bool keyPressed(int key) const;   ///< went down this frame
    bool keyReleased(int key) const;

    bool mouseDown(int button) const;
    bool mousePressed(int button) const;
    bool mouseReleased(int button) const;

    glm::vec2 mousePosition() const { return m_mousePos; }
    glm::vec2 mouseDelta() const { return m_mousePos - m_prevMousePos; }
    float     scrollDelta() const { return m_scroll; }

    /// Axis helper: returns -1, 0 or +1 from two keys.
    float axis(int negativeKey, int positiveKey) const;

private:
    std::array<bool, kMaxKeys>       m_keys{};
    std::array<bool, kMaxKeys>       m_prevKeys{};
    std::array<bool, kMaxButtons>    m_buttons{};
    std::array<bool, kMaxButtons>    m_prevButtons{};
    glm::vec2                        m_mousePos{0.0f};
    glm::vec2                        m_prevMousePos{0.0f};
    float                            m_scroll = 0.0f;
};

} // namespace vr
