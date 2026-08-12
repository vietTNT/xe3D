#include "RaceManager.h"

#include <algorithm>
#include <cmath>

#include "../Track/Track.h"
#include "../Utilities/MathUtils.h"
#include "../Vehicle/Vehicle.h"

namespace vr {
namespace {

/// Distance travelled since the start line, always in [0, length).
float relativeDistance(const Track& track, float distance) {
    float rel = distance - track.startLineDistance();
    while (rel < 0.0f) rel += track.length();
    while (rel >= track.length()) rel -= track.length();
    return rel;
}

} // namespace

void RaceManager::begin(const Track& track, std::vector<Vehicle>& cars, int laps) {
    m_phase       = RacePhase::Countdown;
    m_countdown   = 4.2f;
    m_raceTime    = 0.0f;
    m_totalLaps   = std::max(1, laps);
    m_justStarted = false;
    m_playerFinished = false;
    m_playerPosition = 1;
    m_finishDelay = 0.0f;
    m_results.clear();
    m_banner.clear();
    m_bannerTimer = 0.0f;

    m_prevRel.assign(cars.size(), 0.0f);
    for (size_t i = 0; i < cars.size(); ++i) {
        Vehicle& v = cars[i];
        const float rel = relativeDistance(track, v.trackDistance());
        m_prevRel[i] = rel;
        v.race = RaceState{};
        // Cars start behind the line, so the first crossing begins lap 1.
        v.race.lap            = (rel > track.length() * 0.5f) ? -1 : 0;
        v.race.nextCheckpoint = (rel > track.length() * 0.5f) ? Track::kCheckpoints : 1;
        v.race.progress       = (float)v.race.lap * track.length() + rel;
    }
    updateStandings(cars);
}

int RaceManager::lightsLit() const {
    if (m_phase != RacePhase::Countdown) return 0;
    const float elapsed = 4.2f - m_countdown;
    return std::max(0, std::min(5, (int)(elapsed / 0.62f)));
}

void RaceManager::setBanner(const std::string& text, float seconds) {
    m_banner      = text;
    m_bannerTimer = seconds;
}

void RaceManager::update(float dt, const Track& track, std::vector<Vehicle>& cars) {
    m_justStarted = false;
    if (m_bannerTimer > 0.0f) m_bannerTimer = std::max(0.0f, m_bannerTimer - dt);

    if (m_phase == RacePhase::Countdown) {
        m_countdown -= dt;
        if (m_countdown <= 0.0f) {
            m_countdown   = 0.0f;
            m_phase       = RacePhase::Green;
            m_justStarted = true;
            setBanner("GO!", 1.4f);
        }
        // Cars are held on the grid, but we still keep the standings fresh.
        updateStandings(cars);
        return;
    }

    if (m_phase == RacePhase::Green) {
        m_raceTime += dt;
        updateProgress(track, cars, dt);
        updateStandings(cars);

        if (m_playerFinished) {
            m_finishDelay += dt;
            if (m_finishDelay > 1.6f) {
                buildResults(cars);
                m_phase = RacePhase::Finished;
            }
        }
    }
}

void RaceManager::updateProgress(const Track& track, std::vector<Vehicle>& cars, float dt) {
    const float L = track.length();
    for (size_t i = 0; i < cars.size(); ++i) {
        Vehicle& v = cars[i];
        if (v.race.finished) {
            v.race.progress = (float)m_totalLaps * L + 1.0f;
            continue;
        }

        v.race.totalTime += dt;
        v.race.lapTime += dt;

        const float rel  = relativeDistance(track, v.trackDistance());
        const float prev = m_prevRel[i];
        float       delta = rel - prev;
        if (delta > L * 0.5f) delta -= L;        // crossed the line backwards
        if (delta < -L * 0.5f) delta += L;       // crossed the line forwards

        // ---- intermediate checkpoints (must be taken in order)
        if (delta > 0.0f && v.race.nextCheckpoint < Track::kCheckpoints) {
            const float cp = L * (float)v.race.nextCheckpoint / (float)Track::kCheckpoints;
            const bool crossed = (prev < cp && rel >= cp) ||
                                 (rel < prev && (prev < cp || rel >= cp));
            if (crossed) ++v.race.nextCheckpoint;
        }

        // ---- start/finish line
        const bool wrappedForward = (rel < prev - L * 0.5f);
        if (wrappedForward) {
            if (v.race.nextCheckpoint >= Track::kCheckpoints) {
                if (v.race.lap >= 0) {
                    const int slot = std::min(v.race.lap, 2);
                    v.race.lapTimes[slot] = v.race.lapTime;
                    if (v.race.bestLap < 0.0f || v.race.lapTime < v.race.bestLap) {
                        v.race.bestLap = v.race.lapTime;
                        if (!v.isAI()) setBanner("NEW BEST LAP", 2.0f);
                    }
                }
                ++v.race.lap;
                v.race.lapTime        = 0.0f;
                v.race.nextCheckpoint = 1;

                if (v.race.lap >= m_totalLaps) {
                    v.race.finished   = true;
                    v.race.finishTime = v.race.totalTime;
                    if (!v.isAI()) {
                        m_playerFinished = true;
                        setBanner("FINISH", 3.0f);
                    }
                } else if (!v.isAI()) {
                    char buf[48];
                    std::snprintf(buf, sizeof(buf), "LAP %d / %d", v.race.lap + 1, m_totalLaps);
                    setBanner(buf, 1.6f);
                }
            } else {
                // Lap not validated: the car missed checkpoints, so re-arm them.
                v.race.nextCheckpoint = 1;
                if (!v.isAI()) setBanner("LAP NOT COUNTED", 2.0f);
            }
        }

        m_prevRel[i]    = rel;
        v.race.progress = (float)v.race.lap * L + rel;
    }
}

void RaceManager::updateStandings(std::vector<Vehicle>& cars) {
    std::vector<int> order((size_t)cars.size());
    for (size_t i = 0; i < cars.size(); ++i) order[i] = (int)i;

    std::sort(order.begin(), order.end(), [&](int a, int b) {
        const Vehicle& va = cars[(size_t)a];
        const Vehicle& vb = cars[(size_t)b];
        if (va.race.finished != vb.race.finished) return va.race.finished;
        if (va.race.finished && vb.race.finished) return va.race.finishTime < vb.race.finishTime;
        return va.race.progress > vb.race.progress;
    });

    for (size_t p = 0; p < order.size(); ++p) {
        Vehicle& v = cars[(size_t)order[p]];
        v.race.position = (int)p + 1;
        if (!v.isAI()) m_playerPosition = v.race.position;
    }
}

void RaceManager::buildResults(std::vector<Vehicle>& cars) {
    updateStandings(cars);
    m_results.clear();
    m_results.reserve(cars.size());
    for (const Vehicle& v : cars) {
        ResultRow row;
        row.carIndex  = v.index();
        row.position  = v.race.position;
        row.totalTime = v.race.finished ? v.race.finishTime : v.race.totalTime;
        row.bestLap   = v.race.bestLap;
        row.finished  = v.race.finished;
        row.name      = v.name();
        m_results.push_back(row);
    }
    std::sort(m_results.begin(), m_results.end(),
              [](const ResultRow& a, const ResultRow& b) { return a.position < b.position; });
    const float winnerTime = m_results.empty() ? 0.0f : m_results.front().totalTime;
    for (ResultRow& r : m_results) r.gap = r.totalTime - winnerTime;
}

} // namespace vr
