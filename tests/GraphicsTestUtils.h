#pragma once
#include "Graphics/GraphicsDevice.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <stdexcept>

// Returns a new device, or skips the test on a machine with no Vulkan device.
inline std::unique_ptr<enzo::gfx::GraphicsDevice> createDeviceOrSkip()
{
    try
    {
        return std::make_unique<enzo::gfx::GraphicsDevice>();
    }
    catch (const std::runtime_error& error)
    {
        SKIP(error.what());
    }
    return nullptr;
}
