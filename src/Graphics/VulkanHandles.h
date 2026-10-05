#pragma once
#include <cstdint>

namespace enzo::gfx {

/// @brief The Vulkan handles a host adopts to draw with the same device as the viewport.
///
/// Each handle holds its Vulkan object as a `void*`, so `instance` holds a
/// `VkInstance` and `physicalDevice` a `VkPhysicalDevice`.
struct VulkanHandles
{
    void* instance = nullptr;
    void* physicalDevice = nullptr;
    void* device = nullptr;
    void* queue = nullptr;
    uint32_t queueFamilyIndex = 0;

    /// @brief The Vulkan version the instance was created with, packed as `VK_MAKE_API_VERSION`.
    uint32_t apiVersion = 0;
};

} // namespace enzo::gfx
