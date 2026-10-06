#include "Gui/Viewport/QtGraphicsBridge.h"
#include <QQuickGraphicsConfiguration>
#include <QQuickGraphicsDevice>
#include <QQuickWindow>
#include <QtQuick/qsgtexture_platform.h>
#include <stdexcept>

namespace enzo::ui {

std::vector<std::string> QtGraphicsBridge::getInstanceExtensions()
{
    std::vector<std::string> extensions;
    for (const QByteArray& extension : QQuickGraphicsConfiguration::preferredInstanceExtensions())
    {
        extensions.push_back(extension.toStdString());
    }
    return extensions;
}

QtGraphicsBridge::QtGraphicsBridge(const gfx::VulkanHandles& handles) : handles_(handles)
{
    // Lists every extension the instance was created with, since Qt cannot read them from it.
    // The graphics device enables each surface extension this platform supports.
    const QByteArrayList surfaceExtensions{
        "VK_KHR_surface",
        "VK_KHR_win32_surface",
        "VK_KHR_wayland_surface",
        "VK_KHR_xlib_surface",
        "VK_KHR_xcb_surface",
        "VK_EXT_metal_surface",
    };
    QByteArrayList extensions = QQuickGraphicsConfiguration::preferredInstanceExtensions();
    for (const QByteArray& extension : surfaceExtensions)
    {
        if (vulkanInstance_.supportedExtensions().contains(extension)) extensions << extension;
    }

    const uint32_t apiVersion = handles.apiVersion;
    vulkanInstance_.setVkInstance(static_cast<VkInstance>(handles.instance));
    vulkanInstance_.setApiVersion(QVersionNumber(
        VK_API_VERSION_MAJOR(apiVersion), VK_API_VERSION_MINOR(apiVersion)
    ));
    vulkanInstance_.setExtensions(extensions);
    if (!vulkanInstance_.create())
    {
        throw std::runtime_error("Qt could not adopt the Vulkan instance");
    }
}

void QtGraphicsBridge::adoptDevice(QQuickWindow* window)
{
    window->setVulkanInstance(&vulkanInstance_);
    window->setGraphicsDevice(QQuickGraphicsDevice::fromDeviceObjects(
        static_cast<VkPhysicalDevice>(handles_.physicalDevice),
        static_cast<VkDevice>(handles_.device),
        static_cast<int>(handles_.queueFamilyIndex)
    ));
}

QSGTexture* QtGraphicsBridge::wrapImage(QQuickWindow* window, const gfx::VulkanImage& image)
{
    const QSize size(static_cast<int>(image.size.x), static_cast<int>(image.size.y));
    return QNativeInterface::QSGVulkanTexture::fromNative(
        static_cast<VkImage>(image.image), static_cast<VkImageLayout>(image.layout), window, size
    );
}

} // namespace enzo::ui
