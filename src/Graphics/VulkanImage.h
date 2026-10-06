#pragma once
#include <cstdint>
#include <glm/vec2.hpp>

namespace enzo::gfx {

/// @brief A rendered image a host samples, as raw Vulkan values.
struct VulkanImage
{
    /// @brief The `VkImage`.
    void* image = nullptr;

    /// @brief The `VkImageLayout` the image is left in.
    uint32_t layout = 0;

    glm::uvec2 size{0, 0};
};

} // namespace enzo::gfx
