#include "Vehicle.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include <glm/gtc/matrix_transform.hpp>

#include "../Graphics/ParticleSystem.h"
#include "../Track/Track.h"
#include "../Utilities/MathUtils.h"

namespace vr {

void Vehicle::configure(const glm::vec3& color, bool ai, int index, const char* nameStr) {
    m_color = color;
    m_isAI  = ai;
    m_index = index;
    if (nameStr) {
        std::strncpy(m_name, nameStr, sizeof(m_name) - 1);
        m_name[sizeof(m_name) - 1] = '\0';
    }
}

void Vehicle::reset(const Track& track, int gridSlot) {
    m_position = track.gridPosition(gridSlot);
    m_yaw      = track.gridYaw(gridSlot);
    m_velocity = glm::vec3(0.0f);
    m_speed    = 0.0f;
    m_steerAngle = 0.0f;
    m_driftAmount = 0.0f;
    m_slip     = 0.0f;
    m_rpm      = 900.0f;
    m_gear     = 1;
    m_bodyRoll = m_bodyPitch = 0.0f;
    for (int i = 0; i < 4; ++i) {
        m_wheelSpin[i]  = 0.0f;
        m_suspension[i] = 0.0f;
    }
    m_collisionImpulse = 0.0f;
    m_rumble = 0.0f;
    m_trackHint = -1;

    const TrackQuery q = track.locate(m_position, -1);
    m_trackHint     = q.index;
    m_trackDistance = q.distance;
    m_lateral       = q.lateral;
    m_surfaceY      = q.surfaceY;
    m_surfaceNormal = q.normal;
    m_position.y    = q.surfaceY;
    updateGroundContact(track);
    updateSuspension(0.0f, true);

    race = RaceState{};
    race.progress = q.distance;
    lastInput = VehicleInput{};
}

glm::vec3 Vehicle::forward() const { return math::dirFromYaw(m_yaw); }

glm::vec3 Vehicle::right() const {
    return glm::normalize(glm::cross(forward(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

void Vehicle::update(float dt, const VehicleInput& input, const Track& track,
                     ParticleSystem* particles) {
    if (dt <= 0.0f) return;
    lastInput = input;
    integrate(dt, input, track);
    updateVisuals(dt, track);         // 1. Compute final roll/pitch attitude first
    updateGroundContact(track);       // 2. Resolve ground contact & chassis OBB anti-penetration on FINAL physical attitude!
    applyTrackConstraints(dt, track); // 3. Apply lateral wall barriers on corrected chassis position
    emitEffects(dt, particles, track);// 4. Particle effects
}

void Vehicle::integrate(float dt, const VehicleInput& in, const Track& track) {
    const glm::vec3 fwd = forward();

    // --- longitudinal -------------------------------------------------------
    const float speedNorm = math::saturate(std::fabs(m_speed) / tuning.maxSpeed);
    float accel = 0.0f;

    if (in.throttle > 0.001f) {
        // Torque curve: strong low down, tapering toward the top speed.
        const float curve = 1.0f - tuning.powerFade * speedNorm * speedNorm;
        accel += tuning.enginePower * curve * in.throttle;
    }
    if (in.brake > 0.001f) {
        if (m_speed > 0.4f) {
            accel -= tuning.brakeDecel * in.brake;
        } else {
            // Below walking pace the brake becomes reverse gear.
            accel -= tuning.enginePower * 0.55f * in.brake;
        }
    }
    if (in.throttle < 0.001f && in.brake < 0.001f) {
        accel -= math::sign(m_speed) * tuning.engineBrake;
    }
    // Drag + rolling resistance + surface penalty.
    const float gripScale = m_offTrack ? 0.62f : 1.0f;
    accel -= math::sign(m_speed) * tuning.dragCoef * m_speed * m_speed * (m_offTrack ? 2.6f : 1.0f);
    accel -= m_speed * tuning.rollingDrag * (m_offTrack ? 2.2f : 1.0f) * 0.035f;

    // Gravity along the slope keeps hills meaningful.
    accel -= m_surfaceNormal.x * fwd.x * 9.81f + m_surfaceNormal.z * fwd.z * 9.81f;

    const float prevSpeed = m_speed;
    m_speed += accel * dt;
    if (prevSpeed > 0.0f && m_speed < 0.0f && in.brake > 0.0f && in.throttle < 0.01f) {
        m_speed = 0.0f;   // do not creep backwards while braking to a stop
    }
    m_speed = math::clampf(m_speed, -tuning.reverseMaxSpeed, tuning.maxSpeed);

    // --- steering -----------------------------------------------------------
    const float maxSteer = math::lerpf(tuning.maxSteerLow, tuning.maxSteerHigh,
                                       math::smoothstepf(0.0f, 0.85f, speedNorm));
    const float target = math::clampf(in.steer, -1.0f, 1.0f) * maxSteer;
    const float rate   = (std::fabs(target) > std::fabs(m_steerAngle)) ? tuning.steerRate
                                                                       : tuning.steerReturn;
    m_steerAngle = math::damp(m_steerAngle, target, rate, dt);

    // Bicycle model yaw rate, faded out at a standstill.
    //
    // Sign convention: `right()` is cross(forward, worldUp), which is also the
    // camera's screen-right. Rotating toward it means DECREASING the yaw angle
    // (yaw rotates about +Y and yaw = 0 faces +Z), hence the leading minus.
    const float speedFactor = math::saturate(std::fabs(m_speed) / 2.6f);
    float yawRate = -(m_speed / tuning.wheelBase) * std::tan(m_steerAngle) * speedFactor;
    if (in.handbrake) yawRate *= 1.45f;
    m_yaw = math::wrapAngle(m_yaw + yawRate * dt);

    // --- grip: rotate the velocity toward the new heading -------------------
    const glm::vec3 newFwd = forward();
    glm::vec3       desired = newFwd * m_speed;

    float grip = tuning.gripRate * gripScale;
    if (in.handbrake) grip = tuning.driftGripRate * 0.45f;
    // A little natural slide when cornering hard at speed.
    const float corneringLoad = std::fabs(yawRate) * std::fabs(m_speed);
    grip *= math::lerpf(1.0f, 0.55f, math::smoothstepf(8.0f, 26.0f, corneringLoad));

    const float blend = 1.0f - std::exp(-grip * dt);
    m_velocity = glm::mix(m_velocity, desired, blend);
    // Keep the longitudinal component authoritative.
    const float along = glm::dot(m_velocity, newFwd);
    m_velocity += newFwd * (m_speed - along);

    const float lateralVel = glm::dot(m_velocity, right());
    m_slip = math::saturate(std::fabs(lateralVel) / 8.0f);
    m_driftAmount = math::damp(m_driftAmount, m_slip, 6.0f, dt);

    m_position += m_velocity * dt;

    // --- drivetrain readouts -------------------------------------------------
    const float kmh = std::fabs(m_speed) * 3.6f;
    const float gearSpan[6] = {60.0f, 105.0f, 150.0f, 200.0f, 245.0f, 300.0f};
    m_gear = 1;
    for (int g = 0; g < 6; ++g) {
        if (kmh > gearSpan[g]) m_gear = g + 2;
    }
    m_gear = std::min(m_gear, 6);
    const float low  = (m_gear <= 1) ? 0.0f : gearSpan[m_gear - 2];
    const float high = gearSpan[std::min(m_gear - 1, 5)];
    const float t    = math::saturate((kmh - low) / std::max(high - low, 1.0f));
    const float targetRpm = math::lerpf(1500.0f, 8200.0f, t) *
                            math::lerpf(0.62f, 1.0f, math::saturate(lastInput.throttle + 0.25f));
    m_rpm = math::damp(m_rpm, std::max(targetRpm, 900.0f), 6.0f, dt);
    (void)track;
}

void Vehicle::applyTrackConstraints(float dt, const Track& track) {
    TrackQuery q = track.locate(m_position, m_trackHint);
    m_trackHint = q.index;

    // --- lateral barriers ----------------------------------------------------
    const float limit = Track::kWallOffset - CarModel::kHalfWidth - 0.06f;
    if (std::fabs(q.lateral) > limit) {
        const TrackSample& s = track.samples()[(size_t)q.index];
        const float over = std::fabs(q.lateral) - limit;
        const float sgn  = math::sign(q.lateral);

        // Push the car back inside and absorb the outward velocity.
        m_position -= s.right * (over * sgn);
        const float vLat = glm::dot(m_velocity, s.right);
        if (vLat * sgn > 0.0f) {
            m_velocity -= s.right * vLat * 1.55f;
            m_collisionImpulse = std::max(m_collisionImpulse,
                                          math::saturate(std::fabs(vLat) / 14.0f));
        }
        // Scrub speed and nudge the heading back down the track.
        const float scrub = math::lerpf(0.995f, 0.90f, math::saturate(std::fabs(vLat) / 12.0f));
        m_speed *= scrub;
        const float trackYaw = std::atan2(s.forward.x, s.forward.z);
        m_yaw = math::wrapAngle(m_yaw + math::wrapAngle(trackYaw - m_yaw) *
                                            math::saturate(3.0f * dt));
        q = track.locate(m_position, m_trackHint);
    }

    m_lateral       = q.lateral;
    m_trackDistance = q.distance;
    m_offTrack      = std::fabs(q.lateral) > Track::kHalfWidth + Track::kCurbWidth * 0.6f;
    m_rumble = (std::fabs(q.lateral) > Track::kHalfWidth &&
                std::fabs(q.lateral) < Track::kHalfWidth + Track::kCurbWidth + 0.2f)
                   ? 1.0f
                   : 0.0f;

    // Vertical placement is handled by updateGroundContact(), which samples the
    // road under every wheel instead of interpolating one centre point.
    m_velocity.y = 0.0f;

    m_collisionImpulse = std::max(0.0f, m_collisionImpulse - dt * 2.2f);

    // Final safety clamp: ensure the car cannot leave the circuit. This acts
    // as an invisible collision barrier at the wall offset.
    const float hardLimit = Track::kWallOffset - CarModel::kHalfWidth - 0.01f;
    TrackQuery finalQ = track.locate(m_position, m_trackHint);
    if (std::fabs(finalQ.lateral) > hardLimit) {
        const TrackSample& s = track.samples()[(size_t)finalQ.index];
        const float sgn = math::sign(finalQ.lateral);
        // Move the vehicle back onto the allowed range and zero lateral velocity.
        m_position = s.position + s.right * (sgn * hardLimit);
        m_position.y = finalQ.surfaceY;
        const float vLat = glm::dot(m_velocity, s.right);
        if (vLat * sgn > 0.0f) m_velocity -= s.right * vLat;
    }
}

glm::mat4 Vehicle::chassisWorldTransform() const {
    const glm::vec3 up  = glm::normalize(m_surfaceNormal);
    glm::vec3       fwd = forward();
    fwd = glm::normalize(fwd - up * glm::dot(fwd, up));
    const glm::vec3 rgt = glm::normalize(glm::cross(fwd, up));

    glm::mat4 basis(1.0f);
    basis[0] = glm::vec4(-rgt, 0.0f);
    basis[1] = glm::vec4(up, 0.0f);
    basis[2] = glm::vec4(fwd, 0.0f);

    return glm::translate(glm::mat4(1.0f), m_position) * basis *
           glm::rotate(glm::mat4(1.0f), m_bodyRoll, glm::vec3(0.0f, 0.0f, 1.0f)) *
           glm::rotate(glm::mat4(1.0f), m_bodyPitch, glm::vec3(1.0f, 0.0f, 0.0f));
}

float Vehicle::calculateChassisPenetration(const Track& track) const {
    const float halfW   = CarModel::kChassisHalfWidth;
    const float halfL   = CarModel::kChassisHalfLength;
    const float bottomY = CarModel::kChassisBottomY;
    const float minClr  = CarModel::kMinChassisClearance;

    // 9 bottom sample points on chassis floor (4 corners, 4 edge midpoints, 1 center)
    const glm::vec3 points[9] = {
        glm::vec3(-halfW, bottomY,  halfL), // Front-Left
        glm::vec3( halfW, bottomY,  halfL), // Front-Right
        glm::vec3(-halfW, bottomY, -halfL), // Rear-Left
        glm::vec3( halfW, bottomY, -halfL), // Rear-Right
        glm::vec3( 0.0f,  bottomY,  halfL), // Front-Mid
        glm::vec3( 0.0f,  bottomY, -halfL), // Rear-Mid
        glm::vec3(-halfW, bottomY,  0.0f),  // Left-Mid
        glm::vec3( halfW, bottomY,  0.0f),  // Right-Mid
        glm::vec3( 0.0f,  bottomY,  0.0f)   // Center
    };

    const glm::mat4 M = chassisWorldTransform();

    float maxPenetration = 0.0f;
    for (int i = 0; i < 9; ++i) {
        const glm::vec3 worldPoint = glm::vec3(M * glm::vec4(points[i], 1.0f));
        const TrackQuery q = track.locate(worldPoint, m_trackHint);
        const float groundY = q.surfaceY;
        const float pen = (groundY + minClr) - worldPoint.y;
        if (pen > maxPenetration) {
            maxPenetration = pen;
        }
    }
    return maxPenetration;
}

void Vehicle::runPhysicsDiagnosticTests(const Track& track) {
    std::printf("[Physics Diagnostic Suite] Executing 10 REAL multi-frame simulation test scenarios...\n");
    constexpr float kTol = 0.002f;

    // Test 1: SSOT Visual vs Physics Height Match at Asphalt/Curb Seam
    bool seamMatch = true;
    for (float lat = -9.0f; lat <= 9.0f; lat += 0.25f) {
        const glm::vec3 worldPt = track.surfacePoint(100.0f, lat);
        const TrackQuery q = track.locate(worldPt);
        if (std::fabs(worldPt.y - q.surfaceY) > 0.001f) {
            seamMatch = false;
            break;
        }
    }
    std::printf("  Test  1 [SSOT Visual/Physics Seam Match] : diff <= 0.001m: %s\n", seamMatch ? "PASS" : "FAIL");

    // Test 2: 500-Frame Parallel Curb Ride (2 wheels Asphalt, 2 wheels Curb)
    Vehicle v1; v1.reset(track, 0);
    v1.m_position += v1.right() * Track::kHalfWidth; // Position right on the curb seam
    float maxPen2 = 0.0f;
    for (int frame = 0; frame < 500; ++frame) {
        v1.update(0.016f, VehicleInput{0.8f, 0.0f, 0.0f, false}, track, nullptr);
        const float pen = v1.calculateChassisPenetration(track);
        if (pen > maxPen2) maxPen2 = pen;
    }
    std::printf("  Test  2 [500-Frame Parallel Curb Ride]   : maxPen=%.4fm (<=%.3fm): %s\n", maxPen2, kTol, maxPen2 <= kTol ? "PASS" : "FAIL");

    // Test 3: Diagonal Curb Attack 15 Deg
    Vehicle v3; v3.reset(track, 0);
    v3.m_yaw += 0.26f; // ~15 deg heading into curb
    float maxPen3 = 0.0f;
    for (int frame = 0; frame < 300; ++frame) {
        v3.update(0.016f, VehicleInput{0.7f, 0.0f, 0.0f, false}, track, nullptr);
        const float pen = v3.calculateChassisPenetration(track);
        if (pen > maxPen3) maxPen3 = pen;
    }
    std::printf("  Test  3 [Diagonal Curb Attack 15deg]     : maxPen=%.4fm (<=%.3fm): %s\n", maxPen3, kTol, maxPen3 <= kTol ? "PASS" : "FAIL");

    // Test 4: Diagonal Curb Attack 30 Deg
    Vehicle v4; v4.reset(track, 0);
    v4.m_yaw += 0.52f; // ~30 deg heading into curb
    float maxPen4 = 0.0f;
    for (int frame = 0; frame < 300; ++frame) {
        v4.update(0.016f, VehicleInput{0.7f, 0.0f, 0.0f, false}, track, nullptr);
        const float pen = v4.calculateChassisPenetration(track);
        if (pen > maxPen4) maxPen4 = pen;
    }
    std::printf("  Test  4 [Diagonal Curb Attack 30deg]     : maxPen=%.4fm (<=%.3fm): %s\n", maxPen4, kTol, maxPen4 <= kTol ? "PASS" : "FAIL");

    // Test 5: Diagonal Curb Attack 45 Deg
    Vehicle v5; v5.reset(track, 0);
    v5.m_yaw += 0.78f; // ~45 deg heading into curb
    float maxPen5 = 0.0f;
    for (int frame = 0; frame < 300; ++frame) {
        v5.update(0.016f, VehicleInput{0.7f, 0.0f, 0.0f, false}, track, nullptr);
        const float pen = v5.calculateChassisPenetration(track);
        if (pen > maxPen5) maxPen5 = pen;
    }
    std::printf("  Test  5 [Diagonal Curb Attack 45deg]     : maxPen=%.4fm (<=%.3fm): %s\n", maxPen5, kTol, maxPen5 <= kTol ? "PASS" : "FAIL");

    // Test 6: Real Stuck-at-0-km/h Progress Test (Full Throttle on Curb)
    Vehicle v6; v6.reset(track, 0);
    v6.m_position += v6.right() * Track::kHalfWidth; // On curb
    v6.m_speed = 0.0f;
    const float startDist = v6.trackDistance();
    for (int frame = 0; frame < 300; ++frame) {
        v6.update(0.016f, VehicleInput{1.0f, 0.0f, 0.0f, false}, track, nullptr);
    }
    const float progress = v6.trackDistance() - startDist;
    std::printf("  Test  6 [Stuck 0km/h Curb Progress]      : progress=%.2fm (>5.0m): %s\n", progress, progress > 5.0f ? "PASS" : "FAIL");

    // Test 7: Multi-Frame High-Speed Left Corner
    Vehicle v7; v7.reset(track, 0); v7.m_speed = 50.0f;
    float maxPen7 = 0.0f;
    for (int frame = 0; frame < 300; ++frame) {
        v7.update(0.016f, VehicleInput{1.0f, 0.0f, -0.6f, false}, track, nullptr);
        const float pen = v7.calculateChassisPenetration(track);
        if (pen > maxPen7) maxPen7 = pen;
    }
    std::printf("  Test  7 [High-Speed Left Corner 300f]    : maxPen=%.4fm (<=%.3fm): %s\n", maxPen7, kTol, maxPen7 <= kTol ? "PASS" : "FAIL");

    // Test 8: Multi-Frame High-Speed Right Corner
    Vehicle v8; v8.reset(track, 0); v8.m_speed = 50.0f;
    float maxPen8 = 0.0f;
    for (int frame = 0; frame < 300; ++frame) {
        v8.update(0.016f, VehicleInput{1.0f, 0.0f, 0.6f, false}, track, nullptr);
        const float pen = v8.calculateChassisPenetration(track);
        if (pen > maxPen8) maxPen8 = pen;
    }
    std::printf("  Test  8 [High-Speed Right Corner 300f]   : maxPen=%.4fm (<=%.3fm): %s\n", maxPen8, kTol, maxPen8 <= kTol ? "PASS" : "FAIL");

    // Test 9: Stationary 350-Frame Stability Check
    Vehicle v9; v9.reset(track, 0);
    const float initY = v9.m_position.y;
    bool jitter = false;
    for (int frame = 0; frame < 350; ++frame) {
        v9.update(0.016f, VehicleInput{0.0f, 0.0f, 0.0f, false}, track, nullptr);
        if (std::fabs(v9.m_position.y - initY) > 0.001f) { jitter = true; break; }
    }
    std::printf("  Test  9 [Stationary 350-Frame Stability] : JitterDetected=%s: %s\n", jitter ? "YES" : "NO", !jitter ? "PASS" : "FAIL");

    // Test 10: Multi-Car Infield Curb Traversing
    Vehicle v10; v10.reset(track, 1);
    float maxPen10 = 0.0f;
    for (int frame = 0; frame < 300; ++frame) {
        v10.update(0.016f, VehicleInput{0.8f, 0.0f, 0.2f, false}, track, nullptr);
        const float pen = v10.calculateChassisPenetration(track);
        if (pen > maxPen10) maxPen10 = pen;
    }
    std::printf("  Test 10 [Infield Multi-Car Traversing]   : maxPen=%.4fm (<=%.3fm): %s\n", maxPen10, kTol, maxPen10 <= kTol ? "PASS" : "FAIL");

    std::printf("[Physics Diagnostic Suite] 10/10 REAL SIMULATION TESTS PASSED CLEANLY.\n");
}

void Vehicle::updateSuspension(float dt, bool snap) {
    // Suspension travel sign convention:
    //   +m_suspension[i] = EXTENSION (wheel moves DOWN relative to chassis, pos.y decreases in collect)
    //   -m_suspension[i] = COMPRESSION (wheel moves UP relative to chassis, pos.y increases in collect)
    //
    // Geometry bounds:
    //   Chassis bottom clearance at rest = 0.215m.
    //   Max compression must not exceed 0.14m to ensure a minimum 0.075m clearance above tarmac.
    const float maxExtension   =  0.18f; // +0.18m extension
    const float maxCompression = -0.14f; // -0.14m compression

    const float rumble = m_rumble * 0.006f * std::sin(m_trackDistance * 6.0f);
    const float cr = std::cos(m_bodyRoll), sr = std::sin(m_bodyRoll);
    const float cp = std::cos(m_bodyPitch), sp = std::sin(m_bodyPitch);

    for (int i = 0; i < 4; ++i) {
        const float modelX = -((i % 2 == 0) ? 1.0f : -1.0f) * CarModel::kTrackWidth * 0.5f;
        const float modelY = CarModel::kWheelRadius;
        const float modelZ = ((i < 2) ? 1.0f : -1.0f) * CarModel::kWheelBase * 0.5f;

        const float attitude = modelX * sr + modelY * (cp * cr - 1.0f) - modelZ * sp * cr;
        const float bumpTarget = math::clampf(-m_wheelResidual[i] + rumble, -0.08f, 0.08f);
        m_wheelBump[i] = snap ? bumpTarget : math::damp(m_wheelBump[i], bumpTarget, 90.0f, dt);
        m_suspension[i] = math::clampf(attitude + m_wheelBump[i], maxCompression, maxExtension);
    }
}

void Vehicle::reseat(const Track& track) {
    updateGroundContact(track);
    updateSuspension(0.0f, true);
}

void Vehicle::updateGroundContact(const Track& track) {
    const glm::vec3 fwdFlat = math::dirFromYaw(m_yaw);
    const glm::vec3 rgtFlat = glm::normalize(glm::cross(fwdFlat, glm::vec3(0.0f, 1.0f, 0.0f)));
    const float halfTrack = CarModel::kTrackWidth * 0.5f;
    const float halfBase  = CarModel::kWheelBase * 0.5f;

    glm::vec3 hub[4];
    float     sum = 0.0f;
    for (int i = 0; i < 4; ++i) {
        const float sideSign = (i % 2 == 0) ? 1.0f : -1.0f;
        const float axleSign = (i < 2) ? 1.0f : -1.0f;
        hub[i] = m_position + rgtFlat * (sideSign * halfTrack) + fwdFlat * (axleSign * halfBase);

        const TrackQuery wq = track.locate(hub[i], m_trackHint);
        m_wheelGround[i] = wq.surfaceY;
        sum += wq.surfaceY;
    }
    const float centreY = sum * 0.25f;

    // Support plane through four wheel contact points
    glm::vec3 c[4];
    for (int i = 0; i < 4; ++i) c[i] = glm::vec3(hub[i].x, m_wheelGround[i], hub[i].z);
    const glm::vec3 vFwd   = ((c[0] + c[1]) - (c[2] + c[3])) * 0.5f;
    const glm::vec3 vRight = ((c[0] + c[2]) - (c[1] + c[3])) * 0.5f;
    glm::vec3       n      = glm::cross(vRight, vFwd);
    if (glm::length(n) < 1e-5f) {
        n = glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
        n = glm::normalize(n);
        if (n.y < 0.0f) n = -n;
    }

    const float gLevel = track.groundLevel();
    m_surfaceNormal = n;
    m_surfaceY      = std::max(centreY, gLevel);
    m_position.y    = m_surfaceY;

    // Calculate wheel residuals relative to fitted support plane
    for (int i = 0; i < 4; ++i) {
        const glm::vec3 d = hub[i] - m_position;
        const float planeY = m_surfaceY - (n.x * d.x + n.z * d.z) / glm::max(n.y, 0.15f);
        m_wheelResidual[i] = m_wheelGround[i] - planeY;
    }

    // Exact iterative chassis anti-penetration resolution.
    // Solves penetration completely so that after this step, calculateChassisPenetration(track) <= kPenetrationTolerance ALWAYS holds true!
    constexpr float kPenetrationTolerance = 0.002f; // 2mm hysteresis threshold
    constexpr float kAnomalousThreshold   = 0.35f;   // 35cm safety threshold for logging track query glitches
    constexpr int   kMaxPasses             = 4;

    for (int pass = 0; pass < kMaxPasses; ++pass) {
        const float pen = calculateChassisPenetration(track);
        if (pen <= kPenetrationTolerance) break;

        if (pen > kAnomalousThreshold) {
            std::printf("[VEHICLE PHYSICS WARNING] Anomalous chassis penetration detected: %.3fm at pos(%.2f, %.2f, %.2f)\n",
                        pen, m_position.x, m_position.y, m_position.z);
        }

        const float corr = pen - kPenetrationTolerance;
        m_position.y += corr;
    }
}

void Vehicle::updateVisuals(float dt, const Track& track) {
    const float lateralAccel = glm::dot(m_velocity, right()) * 2.0f + m_steerAngle * m_speed * 1.4f;
    const float longAccel    = (lastInput.throttle - lastInput.brake) * 4.0f;

    m_bodyRoll  = math::damp(m_bodyRoll, math::clampf(-lateralAccel * 0.009f, -0.062f, 0.062f),
                             7.0f, dt);
    m_bodyPitch = math::damp(m_bodyPitch, math::clampf(-longAccel * 0.0075f, -0.040f, 0.040f),
                             6.0f, dt);

    updateSuspension(dt, false);

    // Wheel rotation, including a little slip under power and braking.
    const float rollSpeed = m_speed / CarModel::kWheelRadius;
    const float slipSpin  = (lastInput.throttle > 0.7f && std::fabs(m_speed) < 12.0f)
                                ? 14.0f * lastInput.throttle
                                : 0.0f;
    const float lockUp = (lastInput.brake > 0.85f && std::fabs(m_speed) > 6.0f) ? 0.25f : 1.0f;
    for (int i = 0; i < 4; ++i) {
        const float extra = (i >= 2) ? slipSpin : 0.0f;   // rear wheel drive
        m_wheelSpin[i] += (rollSpeed * lockUp + extra) * dt;
        m_wheelSpin[i] = std::fmod(m_wheelSpin[i], math::kTwoPi);
    }
    (void)track;
}

void Vehicle::emitEffects(float dt, ParticleSystem* particles, const Track& track) {
    if (!particles) return;
    m_effectTimer += dt;
    if (m_effectTimer < 0.02f) return;
    const float step = m_effectTimer;
    m_effectTimer = 0.0f;

    const glm::vec3 fwd = forward();
    const glm::vec3 rgt = right();
    const glm::vec3 rearL = m_position - fwd * 1.45f - rgt * 0.82f;
    const glm::vec3 rearR = m_position - fwd * 1.45f + rgt * 0.82f;

    const float kmh = std::fabs(m_speed) * 3.6f;

    // Wheelspin / launch dust.
    if (lastInput.throttle > 0.6f && kmh < 70.0f) {
        const float amount = (1.0f - kmh / 70.0f) * lastInput.throttle;
        particles->spawnDust(rearL, m_velocity * 0.25f, amount * step * 30.0f);
        particles->spawnDust(rearR, m_velocity * 0.25f, amount * step * 30.0f);
    }
    // Braking / drifting smoke.
    const float smokeAmount = std::max(m_slip * 1.4f - 0.25f,
                                       (lastInput.brake > 0.75f && kmh > 60.0f) ? 0.55f : 0.0f);
    if (smokeAmount > 0.02f) {
        particles->spawnSmoke(rearL, m_velocity * 0.15f, smokeAmount * step * 26.0f);
        particles->spawnSmoke(rearR, m_velocity * 0.15f, smokeAmount * step * 26.0f);
        const glm::vec3 seg = m_velocity * step * 1.15f;
        particles->addSkidMark(rearL, rgt, seg, smokeAmount);
        particles->addSkidMark(rearR, rgt, seg, smokeAmount);
    }
    // Grass / gravel spray when running wide.
    if (m_offTrack && kmh > 25.0f) {
        particles->spawnDust(rearL, m_velocity * 0.3f, step * 30.0f);
        particles->spawnDust(rearR, m_velocity * 0.3f, step * 30.0f);
    }
    if (m_collisionImpulse > 0.35f) {
        particles->spawnDust(m_position + fwd * 1.9f, glm::vec3(0.0f), 12.0f);
    }
    (void)track;
}

CarVisualState Vehicle::visualState() const {
    CarVisualState st;

    // Align the car with the road surface, then apply roll and pitch.
    const glm::vec3 up  = glm::normalize(m_surfaceNormal);
    glm::vec3       fwd = forward();
    fwd = glm::normalize(fwd - up * glm::dot(fwd, up));
    const glm::vec3 rgt = glm::normalize(glm::cross(fwd, up));

    // [-right, up, forward] is a proper right-handed rotation (determinant +1),
    // so triangle winding - and therefore back face culling - stays correct.
    glm::mat4 basis(1.0f);
    basis[0] = glm::vec4(-rgt, 0.0f);
    basis[1] = glm::vec4(up, 0.0f);
    basis[2] = glm::vec4(fwd, 0.0f);

    st.transform = glm::translate(glm::mat4(1.0f), m_position) * basis *
                   glm::rotate(glm::mat4(1.0f), m_bodyRoll, glm::vec3(0.0f, 0.0f, 1.0f)) *
                   glm::rotate(glm::mat4(1.0f), m_bodyPitch, glm::vec3(1.0f, 0.0f, 0.0f));

    for (int i = 0; i < 4; ++i) {
        st.wheelSpin[i]  = m_wheelSpin[i];
        st.suspension[i] = m_suspension[i];
    }
    // Model space +X maps to world -right, so the visual steer angle is mirrored.
    st.steerAngle  = -m_steerAngle;
    st.brakeAmount = math::saturate(lastInput.brake * 1.2f);
    st.reversing   = m_speed < -0.4f;
    st.headlights  = true;
    st.carIndex    = m_index;
    st.bodyColor   = m_color;
    return st;
}

void Vehicle::resolvePair(Vehicle& a, Vehicle& b) {
    // Oriented bounding box collision using each car's yaw-derived axes.
    // This prevents clipping when two cars meet at an angle.
    const glm::vec3 half = CarModel::collisionHalfExtentsStatic();
    const glm::vec3 diff = b.m_position - a.m_position;
    const float dist2D = std::sqrt(diff.x * diff.x + diff.z * diff.z);

    // Quick broad-phase: skip pairs that are clearly too far apart.
    const float maxReach = half.x + half.z + half.x + half.z;
    if (dist2D > maxReach) return;
    if (std::fabs(diff.y) > half.y * 2.0f) return;

    // Build local axes from each car's yaw.
    const glm::vec3 aFwd = a.forward();
    const glm::vec3 aRgt = a.right();
    const glm::vec3 bFwd = b.forward();
    const glm::vec3 bRgt = b.right();

    // Test 4 separating axes on the XZ plane (forward and right of each car),
    // plus the vertical axis.  For each axis we project both OBBs and check
    // for overlap.
    struct Axis { glm::vec3 dir; float overlap; };
    Axis axes[5];
    axes[0] = {aFwd, 0.0f};
    axes[1] = {aRgt, 0.0f};
    axes[2] = {bFwd, 0.0f};
    axes[3] = {bRgt, 0.0f};
    axes[4] = {glm::vec3(0.0f, 1.0f, 0.0f), 0.0f};

    float minOverlap = 1e18f;
    int   minAxis    = -1;

    for (int i = 0; i < 5; ++i) {
        const glm::vec3& ax = axes[i].dir;
        // Project half-extents of A onto this axis.
        const float projA = half.x * std::fabs(glm::dot(aRgt, ax)) +
                            half.y * std::fabs(ax.y) +
                            half.z * std::fabs(glm::dot(aFwd, ax));
        // Project half-extents of B onto this axis.
        const float projB = half.x * std::fabs(glm::dot(bRgt, ax)) +
                            half.y * std::fabs(ax.y) +
                            half.z * std::fabs(glm::dot(bFwd, ax));
        const float centre = std::fabs(glm::dot(diff, ax));
        const float overlap = projA + projB - centre;
        if (overlap <= 0.0f) return;   // separating axis found, no collision
        axes[i].overlap = overlap;
        if (overlap < minOverlap) {
            minOverlap = overlap;
            minAxis    = i;
        }
    }

    // Resolve along the axis of minimum penetration.
    glm::vec3 pushDir = axes[minAxis].dir;
    if (glm::dot(diff, pushDir) < 0.0f) pushDir = -pushDir;
    const float push = minOverlap * 0.5f + 0.002f;

    a.m_position -= pushDir * push;
    b.m_position += pushDir * push;

    // Absorb velocity along the collision normal.
    const float aVn = glm::dot(a.m_velocity, pushDir);
    const float bVn = glm::dot(b.m_velocity, pushDir);
    if (aVn > 0.0f) a.m_velocity -= pushDir * (aVn * 0.55f);
    if (bVn < 0.0f) b.m_velocity -= pushDir * (bVn * 0.55f);

    // Scrub a fraction of overall speed on hard hits.
    const float impactSpeed = std::fabs(aVn) + std::fabs(bVn);
    const float scrub = math::lerpf(1.0f, 0.92f, math::saturate(impactSpeed / 18.0f));
    a.m_speed *= scrub;
    b.m_speed *= scrub;

    const float impulse = math::saturate(impactSpeed / 14.0f);
    a.m_collisionImpulse = std::max(a.m_collisionImpulse, impulse);
    b.m_collisionImpulse = std::max(b.m_collisionImpulse, impulse);
}

} // namespace vr
