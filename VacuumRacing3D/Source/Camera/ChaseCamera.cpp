#include "ChaseCamera.h"

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

#include "../Track/Track.h"
#include "../Utilities/MathUtils.h"
#include "../Vehicle/Vehicle.h"

namespace vr {

void ChaseCamera::reset(const Vehicle& target) {
    const glm::vec3 fwd = target.forward();
    m_position = target.position() - fwd * 8.5f + glm::vec3(0.0f, 3.1f, 0.0f);
    m_lookAt   = target.position() + fwd * 5.0f;
    m_shake    = 0.0f;
    m_speedBlur = 0.0f;
}

void ChaseCamera::cycleMode() {
    m_mode = (CameraMode)(((int)m_mode + 1) % (int)CameraMode::Count);
}

void ChaseCamera::update(float dt, const Vehicle& target, const Track& track) {
    const glm::vec3 carPos = target.position();
    const glm::vec3 fwd    = target.forward();
    const glm::vec3 rgt    = target.right();
    const float     speed  = std::fabs(target.speed());
    const float     norm   = math::saturate(speed / target.tuning.maxSpeed);

    // Distance and height grow with speed so fast sections feel faster.
    float distance = 8.2f;
    float height   = 3.05f;
    float aheadLook = 7.0f;
    if (m_mode == CameraMode::Close) {
        distance = 6.0f;
        height   = 2.35f;
    } else if (m_mode == CameraMode::Bumper) {
        distance = 0.15f;
        height   = 1.28f;
        aheadLook = 12.0f;
    }
    distance += norm * 2.4f;
    height += norm * 0.55f;

    // Slight lag on the yaw axis: the camera swings behind the car in corners.
    glm::vec3 desired = carPos - fwd * distance + glm::vec3(0.0f, height, 0.0f);
    desired -= rgt * glm::dot(target.velocity(), rgt) * 0.055f;

    const float follow = (m_mode == CameraMode::Bumper) ? 30.0f : math::lerpf(6.5f, 11.0f, norm);
    m_position = math::damp(m_position, desired, follow, dt);

    // Keep the camera inside the circuit walls and above the ground.
    const TrackQuery q = track.locate(m_position, target.trackHint());
    const float wall = Track::kWallOffset - 0.6f;
    if (std::fabs(q.lateral) > wall) {
        const TrackSample& s = track.samples()[(size_t)q.index];
        m_position -= s.right * ((std::fabs(q.lateral) - wall) * math::sign(q.lateral));
    }
    const float minY = q.surfaceY + 1.3f;
    if (m_position.y < minY) m_position.y = minY;

    const glm::vec3 lookTarget = carPos + fwd * aheadLook + glm::vec3(0.0f, 1.05f, 0.0f);
    m_lookAt = math::damp(m_lookAt, lookTarget, 12.0f, dt);

    // --- shake -------------------------------------------------------------
    m_shake = glm::max(0.0f, m_shake - dt * 1.9f);
    m_shake = glm::max(m_shake, target.collisionImpulse() * 0.9f);
    m_shake = glm::max(m_shake, norm * norm * 0.10f);
    if (target.offTrack() && speed > 8.0f) m_shake = glm::max(m_shake, 0.16f);
    m_shakeTime += dt * 34.0f;

    const float amp = m_shake * 0.14f;
    const glm::vec3 jitter(std::sin(m_shakeTime * 1.7f) * amp,
                           std::sin(m_shakeTime * 2.3f + 1.1f) * amp,
                           std::sin(m_shakeTime * 1.3f + 2.7f) * amp);
    m_position += jitter;
    m_lookAt += jitter * 0.35f;

    m_fov = math::damp(m_fov, fieldOfView + norm * 9.0f, 4.0f, dt);
    m_speedBlur = math::damp(m_speedBlur, math::smoothstepf(0.42f, 1.0f, norm), 4.0f, dt);
    m_up = glm::vec3(0.0f, 1.0f, 0.0f);
}

void ChaseCamera::updateOrbit(float dt, const glm::vec3& centre, float radius, float height) {
    m_orbitAngle += dt * 0.22f;
    m_position = centre + glm::vec3(std::sin(m_orbitAngle) * radius, height,
                                    std::cos(m_orbitAngle) * radius);
    m_lookAt   = centre + glm::vec3(0.0f, 0.55f, 0.0f);
    m_fov      = math::damp(m_fov, 42.0f, 3.0f, dt);
    m_speedBlur = 0.0f;
    m_shake     = 0.0f;
    m_up        = glm::vec3(0.0f, 1.0f, 0.0f);
}

CameraView ChaseCamera::view(float aspect) const {
    CameraView v;
    v.position       = m_position;
    v.nearPlane      = 0.25f;
    v.farPlane       = 2200.0f;
    v.view           = glm::lookAt(m_position, m_lookAt, m_up);
    v.projection     = glm::perspective(glm::radians(m_fov), aspect, v.nearPlane, v.farPlane);
    v.viewProjection = v.projection * v.view;
    return v;
}

} // namespace vr
