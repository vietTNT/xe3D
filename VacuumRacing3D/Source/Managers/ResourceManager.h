// -----------------------------------------------------------------------------
//  ResourceManager.h - owns every GPU texture in the game. All of them are
//  generated procedurally at start-up, so the build has no binary asset files.
// -----------------------------------------------------------------------------
#pragma once

#include <array>

#include "../Graphics/Texture.h"

namespace vr {

class ResourceManager {
public:
    bool loadAll(int qualityLevel = 2);
    void unloadAll();

    const Texture& asphalt() const { return m_asphalt; }
    const Texture& grass() const { return m_grass; }
    const Texture& concrete() const { return m_concrete; }
    const Texture& metal() const { return m_metal; }
    const Texture& carbon() const { return m_carbon; }
    const Texture& gravel() const { return m_gravel; }
    const Texture& smoke() const { return m_smoke; }
    const Texture& glow() const { return m_glow; }
    const Texture& sponsor(int index) const {
        return m_sponsors[(size_t)(index % (int)m_sponsors.size())];
    }
    int sponsorCount() const { return (int)m_sponsors.size(); }
    const Texture& splash() const { return m_splash; }

private:
    Texture m_asphalt;
    Texture m_grass;
    Texture m_concrete;
    Texture m_metal;
    Texture m_carbon;
    Texture m_gravel;
    Texture m_smoke;
    Texture m_glow;
    std::array<Texture, 6> m_sponsors;
    Texture m_splash;
};

} // namespace vr
