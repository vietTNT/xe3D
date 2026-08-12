#include "Input.h"

namespace vr {

void Input::beginFrame() {
    m_prevKeys     = m_keys;
    m_prevButtons  = m_buttons;
    m_prevMousePos = m_mousePos;
    m_scroll       = 0.0f;
}

void Input::onKey(int key, int action) {
    if (key < 0 || key >= kMaxKeys) return;
    if (action == 0) {
        m_keys[(size_t)key] = false;      // GLFW_RELEASE
    } else if (action == 1) {
        m_keys[(size_t)key] = true;       // GLFW_PRESS
    }
    // action == 2 (repeat) keeps the current state.
}

void Input::onMouseButton(int button, int action) {
    if (button < 0 || button >= kMaxButtons) return;
    m_buttons[(size_t)button] = (action != 0);
}

void Input::onCursorPos(double x, double y) { m_mousePos = glm::vec2((float)x, (float)y); }

void Input::onScroll(double dy) { m_scroll += (float)dy; }

void Input::onChar(unsigned int) {}

bool Input::keyDown(int key) const {
    return key >= 0 && key < kMaxKeys && m_keys[(size_t)key];
}
bool Input::keyPressed(int key) const {
    return key >= 0 && key < kMaxKeys && m_keys[(size_t)key] && !m_prevKeys[(size_t)key];
}
bool Input::keyReleased(int key) const {
    return key >= 0 && key < kMaxKeys && !m_keys[(size_t)key] && m_prevKeys[(size_t)key];
}
bool Input::mouseDown(int b) const {
    return b >= 0 && b < kMaxButtons && m_buttons[(size_t)b];
}
bool Input::mousePressed(int b) const {
    return b >= 0 && b < kMaxButtons && m_buttons[(size_t)b] && !m_prevButtons[(size_t)b];
}
bool Input::mouseReleased(int b) const {
    return b >= 0 && b < kMaxButtons && !m_buttons[(size_t)b] && m_prevButtons[(size_t)b];
}

float Input::axis(int negativeKey, int positiveKey) const {
    float v = 0.0f;
    if (keyDown(negativeKey)) v -= 1.0f;
    if (keyDown(positiveKey)) v += 1.0f;
    return v;
}

} // namespace vr
