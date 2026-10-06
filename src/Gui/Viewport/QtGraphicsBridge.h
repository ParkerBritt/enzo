#pragma once
#include "Graphics/VulkanHandles.h"
#include "Graphics/VulkanImage.h"
#include <QVulkanInstance>
#include <string>
#include <vector>

class QQuickWindow;
class QSGTexture;

namespace enzo::ui {

/// @brief A bridge sharing the graphics device and its images with Qt Quick windows.
class QtGraphicsBridge
{
  public:
    /// @brief Returns the instance extensions Qt Quick needs on top of the graphics device's own.
    static std::vector<std::string> getInstanceExtensions();

    /// @brief Wraps the graphics device's Vulkan instance for Qt.
    ///
    /// @throws std::runtime_error when Qt cannot adopt the Vulkan instance.
    explicit QtGraphicsBridge(const gfx::VulkanHandles& handles);

    /// @brief Makes the window draw with the shared device.
    ///
    /// @pre The window has not rendered yet.
    void adoptDevice(QQuickWindow* window);

    /// @brief Returns a scene graph texture showing the image, owned by the caller.
    static QSGTexture* wrapImage(QQuickWindow* window, const gfx::VulkanImage& image);

  private:
    gfx::VulkanHandles handles_;
    QVulkanInstance vulkanInstance_;
};

} // namespace enzo::ui
