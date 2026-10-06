#include "Graphics/GraphicsDevice.h"
#include "Graphics/ViewportRenderer.h"
#include "GraphicsTestUtils.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>

using namespace enzo::gfx;

TEST_CASE("A viewport renderer draws into an image of the frame size")
{
    std::unique_ptr<GraphicsDevice> device = createDeviceOrSkip();
    ViewportRenderer renderer(*device);

    FrameState frame;
    frame.pixelSize = {64, 32};
    const VulkanImage image = renderer.render(frame);

    REQUIRE(image.image != nullptr);
    REQUIRE(image.size == frame.pixelSize);
}

TEST_CASE("A viewport renderer keeps its image until the frame size changes")
{
    std::unique_ptr<GraphicsDevice> device = createDeviceOrSkip();
    ViewportRenderer renderer(*device);

    FrameState frame;
    frame.pixelSize = {64, 32};
    const VulkanImage first = renderer.render(frame);

    frame.backgroundColor = {1.f, 0.f, 1.f, 1.f};
    REQUIRE(renderer.render(frame).image == first.image);

    frame.pixelSize = {128, 64};
    const VulkanImage resized = renderer.render(frame);
    REQUIRE(resized.image != first.image);
    REQUIRE(resized.size == frame.pixelSize);
}
