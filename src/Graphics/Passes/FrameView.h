#pragma once
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace enzo::gfx {

/// @brief The camera and target size a frame is drawn with.
struct FrameView
{
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec3 cameraPosition;
    glm::uvec2 pixelSize;
};

} // namespace enzo::gfx
