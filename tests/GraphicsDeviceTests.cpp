#include "Graphics/GraphicsDevice.h"
#include "GraphicsTestUtils.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <stdexcept>

using namespace enzo::gfx;

TEST_CASE("A graphics device hands out the handles a host adopts")
{
    std::unique_ptr<GraphicsDevice> device = createDeviceOrSkip();
    VulkanHandles vulkanHandles = device->getVulkanHandles();

    REQUIRE(vulkanHandles.instance != nullptr);
    REQUIRE(vulkanHandles.physicalDevice != nullptr);
    REQUIRE(vulkanHandles.device != nullptr);
    REQUIRE(vulkanHandles.queue != nullptr);

    // Packs version 1.3 the way VK_MAKE_API_VERSION does.
    const uint32_t vulkan13 = (1u << 22) | (3u << 12);
    REQUIRE(vulkanHandles.apiVersion >= vulkan13);
}

TEST_CASE("Only one graphics device exists per process")
{
    std::unique_ptr<GraphicsDevice> device = createDeviceOrSkip();

    REQUIRE(&GraphicsDevice::get() == device.get());
    REQUIRE_THROWS_AS(GraphicsDevice(), std::runtime_error);

    // Checks a new device can be made once the first is destroyed.
    device.reset();
    REQUIRE_THROWS_AS(GraphicsDevice::get(), std::runtime_error);
    device = createDeviceOrSkip();
    REQUIRE(&GraphicsDevice::get() == device.get());
}
