#include "AIController.h"

#include <algorithm>
#include <cmath>

#include "../Track/Track.h"
#include "../Utilities/MathUtils.h"

namespace vr {

void AIController::init(int index, float skill, unsigned seed) {
    m_index = index;
    m_skill = math::clampf(skill, 0.72f, 1.0f);
    m_rng.reseed(seed + (unsigned)index * 7919u);
    m_lineBias     = m_rng.range(-1.1f, 1.1f);
    m_mistakeTimer = m_rng.range(8.0f, 22.0f);
    m_avoidOffset  = 0.0f;
}

VehicleInput AIController::update(float dt, const Vehicle& self, const Track& track,
                                  const std::vector<Vehicle>& field) {
    VehicleInput in;

    const glm::vec3 pos   = self.position();
    const glm::vec3 fwd   = self.forward();
    const glm::vec3 rgt   = self.right();
    const float     speed = std::max(self.speed(), 0.0f);
    const float     s     = self.trackDistance();

    // ---------------------------------------------------------- mistakes
    m_mistakeTimer -= dt;
    if (m_mistakeTimer <= 0.0f) {
        m_mistakeTimer = m_rng.range(11.0f, 30.0f) * m_skill;
        m_mistakeLeft  = m_rng.range(0.35f, 0.9f) * (1.15f - m_skill);
        m_mistakeSteer = m_rng.range(-1.0f, 1.0f);
    }
    if (m_mistakeLeft > 0.0f) m_mistakeLeft -= dt;

    // ------------------------------------------------- traffic avoidance
    float avoidTarget = 0.0f;
    float speedCap    = 1e9f;
    for (const Vehicle& other : field) {
        if (other.index() == self.index()) continue;
        glm::vec3 d = other.position() - pos;
        d.y = 0.0f;
        const float ahead = glm::dot(d, fwd);
        const float side  = glm::dot(d, rgt);
        const float dist  = glm::length(d);
        if (ahead < 0.5f || ahead > 26.0f || std::fabs(side) > 4.2f) continue;

        // Pick the side with more room and back off if we are very close.
        const float urgency = math::saturate(1.0f - ahead / 26.0f);
        const float dir     = (side > 0.0f) ? -1.0f : 1.0f;
        avoidTarget += dir * urgency * 3.4f;
        if (dist < 8.0f) {
            speedCap = std::min(speedCap, std::max(other.speed() * 0.94f, 6.0f));
        }
    }
    m_avoidOffset = math::damp(m_avoidOffset, math::clampf(avoidTarget, -4.0f, 4.0f), 2.6f, dt);

    // ------------------------------------------------------ steering target
    const float lookahead = math::clampf(7.0f + speed * 0.62f, 9.0f, 48.0f);
    const TrackSample aim = track.sampleAt(s + lookahead);
    const float maxOffset = Track::kHalfWidth - 1.6f;
    const float offset =
        math::clampf(aim.lineOffset + m_lineBias + m_avoidOffset, -maxOffset, maxOffset);
    glm::vec3 targetPoint = aim.position + aim.right * offset;

    glm::vec3 toTarget = targetPoint - pos;
    toTarget.y = 0.0f;
    if (glm::length(toTarget) > 1e-3f) toTarget = glm::normalize(toTarget);

    const float angle = std::atan2(glm::dot(toTarget, rgt), glm::dot(toTarget, fwd));
    float steer = math::clampf(angle * 2.1f, -1.0f, 1.0f);

    // Counter-steer when the rear steps out so the AI does not spin.
    const float slide = glm::dot(self.velocity(), rgt);
    steer -= math::clampf(slide * 0.055f, -0.45f, 0.45f);

    if (m_mistakeLeft > 0.0f) steer += m_mistakeSteer * 0.16f;
    in.steer = math::clampf(steer, -1.0f, 1.0f);

    // ------------------------------------------------------ speed control
    // Scan ahead and respect the braking distance for every upcoming corner.
    float targetSpeed = track.racingLineSpeed(s + 6.0f);
    const float decel = 12.6f * m_skill;
    for (float d = 10.0f; d < 190.0f; d += 6.0f) {
        const float vCorner = track.racingLineSpeed(s + d);
        const float allowed = std::sqrt(std::max(vCorner * vCorner + 2.0f * decel * d, 0.0f));
        targetSpeed = std::min(targetSpeed, allowed);
    }
    targetSpeed *= math::lerpf(0.82f, 1.0f, m_skill);
    if (m_mistakeLeft > 0.0f) targetSpeed *= 0.86f;
    targetSpeed = std::min(targetSpeed, speedCap);
    if (self.offTrack()) targetSpeed = std::min(targetSpeed, 24.0f);

    const float error = targetSpeed - speed;
    if (error > 1.2f) {
        in.throttle = math::saturate(error * 0.35f);
        in.brake    = 0.0f;
    } else if (error < -1.6f) {
        in.throttle = 0.0f;
        in.brake    = math::saturate(-error * 0.28f);
    } else {
        in.throttle = math::saturate(0.35f + error * 0.2f);
        in.brake    = 0.0f;
    }
    // Ease off mid-corner when already sliding.
    if (self.slipAmount() > 0.55f) in.throttle *= 0.55f;

    // ----------------------------------------------------------- unstick
    if (speed < 1.6f) {
        m_stuckTimer += dt;
    } else {
        m_stuckTimer = 0.0f;
    }
    if (m_stuckTimer > 2.4f) {
        in.throttle = 1.0f;
        in.brake    = 0.0f;
        in.steer    = math::clampf(angle * 2.6f, -1.0f, 1.0f);
        if (m_stuckTimer > 4.5f) m_stuckTimer = 0.0f;
    }
    return in;
}

} // namespace vr
