// -----------------------------------------------------------------------------
//  CarModel.h - the player/AI sports car, modelled procedurally in code.
//
//  The body is a lofted surface driven by a station profile (a classic car
//  modelling technique): every cross section is a tumblehome superellipse, and
//  the quads of the loft are sorted into paint / glass / roof groups so the
//  greenhouse becomes real transparent glazing instead of a decal.
// -----------------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>

#include "../Graphics/Material.h"
#include "../Graphics/Mesh.h"

namespace vr {

class Renderer;
class ResourceManager;

/// Per frame visual state of a car (physics fills this in).
struct CarVisualState {
    glm::mat4 transform{1.0f};
    float     wheelSpin[4]   = {0.0f, 0.0f, 0.0f, 0.0f};  
    float     suspension[4]  = {0.0f, 0.0f, 0.0f, 0.0f};  
    float     steerAngle     = 0.0f;                      
    float     brakeAmount    = 0.0f;                      
    bool      reversing      = false;
    bool      headlights     = true;
    
    // Sơn xe màu Trắng hơi ngả Xám (Giống hình tham khảo)
    glm::vec3 bodyColor{0.90f, 0.90f, 0.92f}; 
};

class CarModel {
public:
// Dimensions (metres) - Phong cách NASCAR gầm thấp, bệ vệ
    static constexpr float kLength      = 4.85f;   // Kéo dài thân xe thêm một chút
    static constexpr float kHalfWidth   = 1.25f;   // Thân xe rộng và bè ra hơn
    static constexpr float kWheelBase   = 2.80f;   // Trục cơ sở dài hơn
    static constexpr float kTrackWidth  = 1.85f;   // Vết bánh xe rộng ra
    static constexpr float kWheelRadius = 0.355f;  
    static constexpr float kWheelWidth  = 0.35f;   // Lốp "béo" hơn (cũ là 0.30f)
    
    /// Underbody clearance. The wheel hubs sit at exactly kWheelRadius so the
    /// tyres touch y = 0 in car local space: the chassis origin IS the contact
    /// plane, which is what lets the physics glue the car to the road.
    static constexpr float kRideHeight  = 0.08f;   // Hạ gầm sát rạt (cũ là 0.115f)


    bool build(const ResourceManager& resources, int qualityLevel = 2);
    void collect(Renderer& renderer, const CarVisualState& state) const;

    /// Wheel hub positions in car local space (FL, FR, RL, RR).
    const glm::vec3* wheelOffsets() const { return m_wheelOffsets; }
    int triangleCount() const;
    /// Simple half-extents for a collision AABB in car local space (x=halfwidth, y=height, z=halflength)
    glm::vec3 collisionHalfExtents() const { return glm::vec3(kHalfWidth, kRideHeight + 0.95f, kLength * 0.5f); }
    static glm::vec3 collisionHalfExtentsStatic() { return glm::vec3(kHalfWidth, kRideHeight + 0.95f, kLength * 0.5f); }

private:
    void buildBodyShell(int quality);
    void buildAero();
    void buildLights();
    void buildDetails();
    void buildInterior();
    void buildWheel(int quality);

    MeshBuilder m_trimAccum;  ///< trim geometry is produced by two passes

    Mesh m_body;        ///< painted body panels (colour changes at runtime)
    Mesh m_glass;
    Mesh m_trim;        ///< black plastic, grille, arch liners, mirrors
    Mesh m_chrome;
    Mesh m_carbon;      ///< splitter, skirts, diffuser
    Mesh m_headlights;
    Mesh m_taillights;
    Mesh m_reverseLights;
    Mesh m_interior;
    Mesh m_tyre;
    Mesh m_rim;
    Mesh m_brake;

    const ResourceManager* m_res = nullptr;
    glm::vec3 m_wheelOffsets[4]{};
};

} // namespace vr
