#pragma once
#include "Graphics/VulkanHandles.h"
#include <memory>
#include <string>
#include <vector>

namespace enzo::gfx {

/// @brief The one Vulkan device in the process, shared by the host and every viewport.
///
/// @note Destroy every host object using the device first, since destroying
/// this destroys the Vulkan device and instance.
class GraphicsDevice
{
  public:
    /// @brief Creates the device.
    ///
    /// @param instanceExtensions the extra instance extensions the host needs.
    /// @throws std::runtime_error when a device already exists or no Vulkan device is found.
    explicit GraphicsDevice(const std::vector<std::string>& instanceExtensions = {});
    ~GraphicsDevice();

    GraphicsDevice(const GraphicsDevice&) = delete;
    GraphicsDevice& operator=(const GraphicsDevice&) = delete;

    /// @brief Returns the device of this process.
    ///
    /// @throws std::runtime_error when no device exists.
    static GraphicsDevice& get();

    /// @brief Returns the handles a host adopts to share the device.
    VulkanHandles getVulkanHandles() const;

    struct DiligentObjects;

    /// @brief Returns the Diligent device and context the renderer draws with.
    ///
    /// @note The type is complete only inside enzoGraphics, through `Graphics/DiligentObjects.h`.
    DiligentObjects& getDiligentObjects() const;

  private:
    std::unique_ptr<DiligentObjects> diligent_;
};

} // namespace enzo::gfx
