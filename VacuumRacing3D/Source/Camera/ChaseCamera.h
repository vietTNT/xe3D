// -----------------------------------------------------------------------------
//  ChaseCamera.h - smooth third person camera with speed zoom, collision
//  avoidance against the barriers and impact shake.
// -----------------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>

#include "../Graphics/Renderer.h"

namespace vr {

class Track;
class Vehicle;

enum class CameraMode { Chase = 0, Close = 1, Bumper = 2, Count = 3 };

class ChaseCamera {
public:
    void reset(const Vehicle& target);
    void update(float dt, const Vehicle& target, const Track& track);
    /// Slow orbit used by the main menu and the car colour picker.
    void updateOrbit(float dt, const glm::vec3& centre, float radius, float height);

    CameraView view(float aspect) const;

    void setMode(CameraMode mode) { m_mode = mode; }
    void cycleMode();
    CameraMode mode() const { return m_mode; }

    void addShake(float amount) { m_shake = glm::min(m_shake + amount, 1.4f); }
    float speedBlur() const { return m_speedBlur; }

    float fieldOfView = 62.0f;

private:
    glm::vec3  m_position{0.0f, 5.0f, -10.0f};
    glm::vec3  m_lookAt{0.0f};
    glm::vec3  m_up{0.0f, 1.0f, 0.0f};
    float      m_shake      = 0.0f;
    float      m_shakeTime  = 0.0f;
    float      m_fov        = 62.0f;
    float      m_speedBlur  = 0.0f;
    float      m_orbitAngle = 0.0f;
    CameraMode m_mode       = CameraMode::Chase;
};

} // namespace vr
