// -----------------------------------------------------------------------------
//  RaceManager.h - race director: start lights, checkpoint validation, lap
//  timing, live standings and the final classification.
// -----------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>

namespace vr {

class Track;
class Vehicle;

enum class RacePhase { Countdown, Green, Finished };

struct ResultRow {
    int         carIndex   = 0;
    int         position   = 1;
    float       totalTime  = 0.0f;
    float       bestLap    = -1.0f;
    float       gap        = 0.0f;
    bool        finished   = false;
    std::string name;
};

class RaceManager {
public:
    void begin(const Track& track, std::vector<Vehicle>& cars, int laps = 3);
    void update(float dt, const Track& track, std::vector<Vehicle>& cars);

    RacePhase phase() const { return m_phase; }
    float     countdown() const { return m_countdown; }
    int       lightsLit() const;
    bool      greenFlag() const { return m_phase != RacePhase::Countdown; }
    /// True on the single frame the lights go out.
    bool      justStarted() const { return m_justStarted; }
    float     raceTime() const { return m_raceTime; }
    int       totalLaps() const { return m_totalLaps; }

    const std::vector<ResultRow>& results() const { return m_results; }
    int  playerPosition() const { return m_playerPosition; }
    bool playerFinished() const { return m_playerFinished; }

    /// Short status line shown in the middle of the HUD (may be empty).
    const std::string& banner() const { return m_banner; }
    float              bannerAlpha() const { return m_bannerTimer; }
    void               setBanner(const std::string& text, float seconds = 2.2f);

private:
    void updateProgress(const Track& track, std::vector<Vehicle>& cars, float dt);
    void updateStandings(std::vector<Vehicle>& cars);
    void buildResults(std::vector<Vehicle>& cars);

    RacePhase          m_phase       = RacePhase::Countdown;
    float              m_countdown   = 4.2f;
    float              m_raceTime    = 0.0f;
    int                m_totalLaps   = 3;
    bool               m_justStarted = false;
    bool               m_playerFinished = false;
    int                m_playerPosition = 1;
    float              m_finishDelay = 0.0f;

    std::vector<float> m_prevRel;
    std::vector<ResultRow> m_results;
    std::string        m_banner;
    float              m_bannerTimer = 0.0f;
};

} // namespace vr
