#pragma once
#include "Graphics/GraphicsDevice.h"

// Includes Vulkan first since DiligentCore's Vulkan headers use its types without including them.
#include <vulkan/vulkan.h>

#include <DeviceContext.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>

namespace enzo::gfx {

struct GraphicsDevice::DiligentObjects
{
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context;
};

} // namespace enzo::gfx
