#include "ResourceManager.h"

#include <cstdio>
#include <fstream>
#include "../Core/Paths.h"

namespace vr {

bool ResourceManager::loadAll(int qualityLevel) {
    const int big   = (qualityLevel >= 2) ? 512 : (qualityLevel == 1 ? 256 : 128);
    const int small = (qualityLevel >= 2) ? 256 : 128;
    m_liverySize = big;

    {
        const auto px = procedural::asphalt(big);
        m_asphalt.createRGBA(big, big, px.data());
    }
    {
        const auto px = procedural::grass(big);
        m_grass.createRGBA(big, big, px.data());
    }
    {
        const auto px = procedural::concrete(small);
        m_concrete.createRGBA(small, small, px.data());
    }
    {
        const auto px = procedural::brushedMetal(small);
        m_metal.createRGBA(small, small, px.data());
    }
    {
        const auto px = procedural::carbonFibre(small);
        m_carbon.createRGBA(small, small, px.data());
    }
    {
        const auto px = procedural::gravel(small);
        m_gravel.createRGBA(small, small, px.data());
    }
    {
        const auto px = procedural::smokePuff(128);
        m_smoke.createRGBA(128, 128, px.data(), true, false);
    }
    {
        const auto px = procedural::softGlow(128);
        m_glow.createRGBA(128, 128, px.data(), true, false);
    }

    const glm::vec3 bases[6] = {{0.78f, 0.10f, 0.12f}, {0.08f, 0.24f, 0.62f},
                                {0.94f, 0.72f, 0.06f}, {0.06f, 0.52f, 0.34f},
                                {0.10f, 0.10f, 0.12f}, {0.85f, 0.36f, 0.05f}};
    for (int i = 0; i < 6; ++i) {
        const auto px = procedural::sponsorBoard(small, bases[i], i);
        m_sponsors[(size_t)i].createRGBA(small, small, px.data(), true, true);
    }

    const glm::vec3 carColors[6] = {
        {0.85f, 0.12f, 0.10f}, // #12 Player / Red
        {0.08f, 0.28f, 0.72f}, // #24 AI 1 / Blue
        {0.06f, 0.58f, 0.30f}, // #48 AI 2 / Green
        {0.95f, 0.82f, 0.05f}, // #88 AI 3 / Yellow
        {0.90f, 0.42f, 0.06f}, // #99 AI 4 / Orange
        {0.45f, 0.12f, 0.65f}  // #17 AI 5 / Purple
    };
    const int raceNumbers[6] = {12, 24, 48, 88, 99, 17};

    for (int i = 0; i < 6; ++i) {
        const auto px = procedural::nascarLivery(big, carColors[i], raceNumbers[i]);
        m_nascarLiveries[(size_t)i].createRGBA(big, big, px.data(), true, false);
    }
    {
        const auto px = procedural::windowNet(small);
        m_windowNet.createRGBA(small, small, px.data(), true, true);
    }

    std::printf("[Resources] procedural textures generated (%d px base)\n", big);
    // Try to load an optional splash image from several likely locations so the
    // image is found whether the game is run from the build dir or project root.
    const std::string candidates[] = {
        paths::resolve("Assets/Textures/splash.bmp"),        // e.g. build/Assets/Textures/...
        paths::resolve("../Assets/Textures/splash.bmp"),     // parent of build
        std::string("Assets/Textures/splash.bmp")            // cwd relative
    };
    for (const std::string& p : candidates) {
        std::ifstream f(p.c_str());
        if (f.good() && m_splash.loadBMP(p)) {
            break;
        }
    }
    return true;
}

void ResourceManager::unloadAll() {
    m_asphalt.destroy();
    m_grass.destroy();
    m_concrete.destroy();
    m_metal.destroy();
    m_carbon.destroy();
    m_gravel.destroy();
    m_smoke.destroy();
    m_glow.destroy();
    for (Texture& t : m_sponsors) t.destroy();
    for (Texture& t : m_nascarLiveries) t.destroy();
    m_windowNet.destroy();
}

void ResourceManager::regeneratePlayerLivery(const glm::vec3& color, int number) {
    m_nascarLiveries[0].destroy();
    const auto px = procedural::nascarLivery(m_liverySize, color, number);
    m_nascarLiveries[0].createRGBA(m_liverySize, m_liverySize, px.data(), true, false);
}

} // namespace vr
