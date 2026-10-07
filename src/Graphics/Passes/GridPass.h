#pragma once
#include "Graphics/Passes/PassSetup.h"

#include <Buffer.h>
#include <DeviceContext.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>

namespace enzo::gfx {

/// @brief The ground grid drawn under the scene.
class GridPass
{
  public:
    /// @brief Builds the grid lines and the pipeline that draws them.
    ///
    /// @param frameConstants the buffer the renderer fills with the frame constants.
    GridPass(Diligent::IRenderDevice* device, Diligent::IBuffer* frameConstants);

    /// @brief Draws the grid into the bound targets.
    void draw(Diligent::IDeviceContext* context);

  private:
    Pipeline pipeline_;
    GpuArray vertices_;
};

} // namespace enzo::gfx
