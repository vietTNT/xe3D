// -----------------------------------------------------------------------------
//  AIController.h - waypoint racing AI.
//
//  Each opponent follows the pre-computed racing line with a speed-dependent
//  look-ahead, brakes for the corner ahead using the same physics the player
//  feels, avoids cars in front, and makes small, believable mistakes.
//  The AI never gets extra grip or power: it drives the same Vehicle model.
// -----------------------------------------------------------------------------
#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "../Utilities/Random.h"
#include "../Vehicle/Vehicle.h"

namespace vr {

class Track;

class AIController {
public:
    void init(int index, float skill, unsigned seed);

    /// Produces the driving input for this frame.
    VehicleInput update(float dt, const Vehicle& self, const Track& track,
                        const std::vector<Vehicle>& field);

    float skill() const { return m_skill; }

private:
    float m_skill        = 0.9f;   ///< 0.8 .. 1.0
    float m_lineBias     = 0.0f;   ///< personal offset from the ideal line
    float m_avoidOffset  = 0.0f;   ///< smoothed overtaking offset
    float m_mistakeTimer = 6.0f;
    float m_mistakeLeft  = 0.0f;
    float m_mistakeSteer = 0.0f;
    float m_stuckTimer   = 0.0f;
    int   m_index        = 0;
    Random m_rng;
};

} // namespace vr
