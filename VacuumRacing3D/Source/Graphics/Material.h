// -----------------------------------------------------------------------------
//  Material.h - PBR material description used by every draw call.
// -----------------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>

namespace vr {

class Texture;

struct Material {
    glm::vec3      albedo{0.8f};
    float          metallic     = 0.0f;
    float          roughness    = 0.7f;
    glm::vec3      emissive{0.0f};
    float          alpha        = 1.0f;
    float          reflectivity = 1.0f;   ///< scales the environment reflection term
    float          clearCoat    = 0.0f;   ///< extra glossy layer, used for car paint
    const Texture* texture      = nullptr;
    float          uvScale      = 1.0f;
    bool           useVertexColor = true;
    bool           doubleSided  = false;
    bool           castShadow   = true;
    bool           receiveShadow = true;
    float          polygonOffset = 0.0f;   ///< non-zero enables GL polygon offset (units)

    bool transparent() const { return alpha < 0.999f; }

    // ---------------------------------------------------------------- presets
    static Material carPaint(const glm::vec3& color) {
        Material m;
        // Car paint is a dielectric base under a clear coat, not a metal:
        // keeping metallic low is what lets the colour stay saturated.
        m.albedo       = color;
        m.metallic     = 0.08f;
        m.roughness    = 0.22f;
        m.clearCoat    = 0.55f;
        m.reflectivity = 0.62f;
        m.useVertexColor = false;
        return m;
    }
    static Material glass(const glm::vec3& tint = glm::vec3(0.06f, 0.08f, 0.10f)) {
        Material m;
        m.albedo       = tint;
        m.metallic     = 0.0f;
        m.roughness    = 0.06f;
        m.alpha        = 0.38f;
        m.reflectivity = 1.10f;
        m.castShadow   = false;
        m.useVertexColor = false;
        m.doubleSided  = true;
        return m;
    }
    static Material rubber() {
        Material m;
        m.albedo       = glm::vec3(0.035f);
        m.metallic     = 0.0f;
        m.roughness    = 0.92f;
        m.reflectivity = 0.25f;
        m.useVertexColor = false;
        return m;
    }
    static Material plastic(const glm::vec3& color) {
        Material m;
        m.albedo    = color;
        m.metallic  = 0.0f;
        m.roughness = 0.55f;
        m.useVertexColor = false;
        return m;
    }
    static Material metal(const glm::vec3& color, float roughness = 0.28f) {
        Material m;
        m.albedo    = color;
        m.metallic  = 1.0f;
        m.roughness = roughness;
        m.useVertexColor = false;
        return m;
    }
    static Material chrome() { return metal(glm::vec3(0.85f), 0.13f); }
    static Material carbon() {
        Material m;
        m.albedo       = glm::vec3(0.045f);
        m.metallic     = 0.15f;
        m.roughness    = 0.46f;
        m.reflectivity = 0.40f;
        m.useVertexColor = false;
        return m;
    }
    static Material emissiveLight(const glm::vec3& color, float strength) {
        Material m;
        m.albedo   = color * 0.25f;
        m.emissive = color * strength;
        m.roughness = 0.25f;
        m.metallic  = 0.0f;
        m.useVertexColor = false;
        return m;
    }
    static Material surface(const glm::vec3& color, float roughness) {
        Material m;
        m.albedo    = color;
        m.roughness = roughness;
        m.metallic  = 0.0f;
        return m;
    }
};

} // namespace vr
