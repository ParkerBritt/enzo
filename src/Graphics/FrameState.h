#pragma once
#include "Graphics/DisplayGeometry.h"
#include "Graphics/ViewportCamera.h"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <memory>

namespace enzo::gfx {

/// @brief Everything one viewport frame is drawn from.
struct FrameState
{
    glm::uvec2 pixelSize{0, 0};
    glm::vec4 backgroundColor{0.f, 0.f, 0.f, 1.f};
    glm::vec4 geometryColor{1.f, 1.f, 1.f, 1.f};
    bool wireframeVisible = true;
    ViewportCamera camera;
    /// @brief The geometry drawn in the frame, or null to draw none.
    std::shared_ptr<const DisplayGeometry> geometry;

    bool operator==(const FrameState&) const = default;
};

} // namespace enzo::gfx
