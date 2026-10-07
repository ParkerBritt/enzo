#pragma once
#include "Graphics/DisplayGeometry.h"
#include "Graphics/ViewportCamera.h"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <cstddef>
#include <memory>
#include <optional>

namespace enzo::gfx {

/// @brief Everything one viewport frame is drawn from.
struct FrameState
{
    glm::uvec2 pixelSize{0, 0};
    /// @brief The number of device pixels per interface pixel.
    float pixelRatio = 1.f;
    glm::vec4 backgroundColor{0.f, 0.f, 0.f, 1.f};
    glm::vec4 geometryColor{1.f, 1.f, 1.f, 1.f};
    bool wireframeVisible = true;
    /// @brief Whether every mesh point is drawn, rather than only the points that belong to no face.
    bool pointsVisible = false;
    ViewportCamera camera;
    /// @brief The index of the camera primitive the view looks through, or empty to look through
    /// the orbit camera.
    std::optional<std::size_t> viewCameraIndex;
    /// @brief The geometry drawn in the frame, or null to draw none.
    std::shared_ptr<const DisplayGeometry> geometry;

    bool operator==(const FrameState&) const = default;
};

} // namespace enzo::gfx
