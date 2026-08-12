// -----------------------------------------------------------------------------
//  Vehicle.h - arcade driving model.
//
//  The car is a point mass with a yaw rate derived from a bicycle model. Grip is
//  modelled by rotating the velocity vector toward the heading: high grip snaps
//  it instantly (stable), low grip lets the car slide (a controlled drift).
//  Barriers are handled analytically against the circuit's lateral limits, so a
//  car can never leave the track, flip, or tunnel through a wall.
// -----------------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>

#include "CarModel.h"

namespace vr {

class Track;
class ParticleSystem;

struct VehicleInput {
    float throttle  = 0.0f;   ///< 0..1
    float brake     = 0.0f;   ///< 0..1
    float steer     = 0.0f;   ///< -1..1
    bool  handbrake = false;
};

struct VehicleTuning {
    // Tuned so the terminal velocity lands near maxSpeed: the engine curve and
    // the aerodynamic drag cross at roughly 72 m/s (~260 km/h), and 0-100 km/h
    // takes about three seconds.
    float maxSpeed        = 75.0f;   ///< m/s (~270 km/h)
    float reverseMaxSpeed = 12.0f;
    float enginePower     = 11.8f;   ///< m/s^2 at zero speed
    float powerFade       = 0.55f;   ///< how much torque is lost at top speed
    float brakeDecel      = 26.0f;
    float engineBrake     = 2.6f;
    float dragCoef        = 0.00100f;
    float rollingDrag     = 0.30f;
    float maxSteerLow     = 0.62f;   ///< radians at crawling speed
    float maxSteerHigh    = 0.115f;  ///< radians at top speed
    float steerRate       = 5.2f;
    float steerReturn     = 7.5f;
    float gripRate        = 7.4f;    ///< how fast velocity aligns with heading
    float driftGripRate   = 2.6f;
    float wheelBase       = CarModel::kWheelBase;
};

/// Race bookkeeping that both the player and the AI share.
struct RaceState {
    int   lap            = 0;
    int   nextCheckpoint = 1;
    int   position       = 1;
    float lapTime        = 0.0f;
    float bestLap        = -1.0f;
    float totalTime      = 0.0f;
    float lapTimes[3]    = {-1.0f, -1.0f, -1.0f};
    bool  finished       = false;
    float finishTime     = -1.0f;
    float progress       = 0.0f;   ///< lap * length + distance, for the standings
};

class Vehicle {
public:
    void reset(const Track& track, int gridSlot);
    void configure(const glm::vec3& color, bool ai, int index, const char* name);

    void update(float dt, const VehicleInput& input, const Track& track,
                ParticleSystem* particles);
    /// Elastic-ish separation between two cars (called once per pair).
    static void resolvePair(Vehicle& a, Vehicle& b);
    /// Re-plants the car on the road. Must be called after anything that moves a
    /// car sideways outside update() - car to car contact, for example - or the
    /// chassis keeps last frame's height and a wheel dips through the tarmac.
    void reseat(const Track& track);

    /// Physical world transform of the chassis OBB derived strictly from physics state (no render interpolation).
    glm::mat4 chassisWorldTransform() const;
    CarVisualState visualState() const;

    // -------------------------------------------------------------- accessors
    const glm::vec3& position() const { return m_position; }
    const glm::vec3& velocity() const { return m_velocity; }
    glm::vec3        forward() const;
    glm::vec3        right() const;
    float            yaw() const { return m_yaw; }
    float            speed() const { return m_speed; }
    float            speedKmh() const { return m_speed * 3.6f; }
    float            engineRpm() const { return m_rpm; }
    int              gear() const { return m_gear; }
    float            driftAmount() const { return m_driftAmount; }
    float            slipAmount() const { return m_slip; }
    bool             isAI() const { return m_isAI; }
    int              index() const { return m_index; }
    const char*      name() const { return m_name; }
    const glm::vec3& color() const { return m_color; }
    float            trackDistance() const { return m_trackDistance; }
    float            lateralOffset() const { return m_lateral; }
    int              trackHint() const { return m_trackHint; }
    bool             offTrack() const { return m_offTrack; }
    /// Non-zero for a few frames after hitting something; drives shake + audio.
    float            collisionImpulse() const { return m_collisionImpulse; }
    void             clearCollisionImpulse() { m_collisionImpulse = 0.0f; }

    /// Calculates exact chassis OBB bottom penetration depth into track surface (0 = no penetration, >0 = depth in meters).
    float calculateChassisPenetration(const Track& track) const;
    /// Deterministic diagnostic test suite for physics & curb scenarios.
    static void runPhysicsDiagnosticTests(const Track& track);

    RaceState      race;
    VehicleTuning  tuning;
    VehicleInput   lastInput;

private:
    void integrate(float dt, const VehicleInput& input, const Track& track);
    void applyTrackConstraints(float dt, const Track& track);
    /// Samples the road under all four wheels and fits the chassis to it.
    void updateGroundContact(const Track& track);
    /// Solves the per-wheel travel that keeps every tyre on the tarmac.
    void updateSuspension(float dt, bool snap);
    void updateVisuals(float dt, const Track& track);
    void emitEffects(float dt, ParticleSystem* particles, const Track& track);

    glm::vec3 m_position{0.0f};
    glm::vec3 m_velocity{0.0f};
    float     m_yaw          = 0.0f;
    float     m_speed        = 0.0f;   ///< signed, along the heading
    float     m_steerAngle   = 0.0f;
    float     m_driftAmount  = 0.0f;
    float     m_slip         = 0.0f;
    float     m_rpm          = 900.0f;
    int       m_gear         = 1;

    // Visual-only state.
    float m_bodyRoll     = 0.0f;
    float m_bodyPitch    = 0.0f;
    float m_wheelSpin[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float m_suspension[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    // Height of the road under each wheel, and how far that wheel sits from the
    // plane fitted through all four contact points (the bump the suspension
    // has to absorb so the tyre stays glued to the tarmac).
    float m_wheelGround[4]   = {0.0f, 0.0f, 0.0f, 0.0f};
    float m_wheelResidual[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float m_wheelBump[4]     = {0.0f, 0.0f, 0.0f, 0.0f};
    float m_surfaceY     = 0.0f;
    glm::vec3 m_surfaceNormal{0.0f, 1.0f, 0.0f};

    // Track relationship.
    float m_trackDistance = 0.0f;
    float m_lateral       = 0.0f;
    int   m_trackHint     = -1;
    bool  m_offTrack      = false;
    float m_rumble        = 0.0f;
    float m_collisionImpulse = 0.0f;
    float m_effectTimer   = 0.0f;

    glm::vec3 m_color{0.8f, 0.1f, 0.1f};
    bool      m_isAI  = false;
    int       m_index = 0;
    char      m_name[24] = "PLAYER";
};

} // namespace vr
