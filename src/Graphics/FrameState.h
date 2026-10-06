#pragma once
#include "Graphics/ViewportCamera.h"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace enzo::gfx {

/// @brief Everything one viewport frame is drawn from.
struct FrameState
{
    glm::uvec2 pixelSize{0, 0};
    glm::vec4 backgroundColor{0.f, 0.f, 0.f, 1.f};
    ViewportCamera camera;

    bool operator==(const FrameState&) const = default;
};

} // namespace enzo::gfx
