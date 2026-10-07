#pragma once
#include "Graphics/DisplayGeometry.h"
#include "Graphics/Passes/PassSetup.h"

#include <Buffer.h>
#include <DeviceContext.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>

namespace enzo::gfx {

/// @brief The camera primitives, drawn as rectangular frames.
class CameraPrimPass
{
  public:
    /// @brief Builds the frame shape and the pipeline that draws it.
    ///
    /// @param frameConstants the buffer the renderer fills with the frame constants.
    CameraPrimPass(Diligent::IRenderDevice* device, Diligent::IBuffer* frameConstants);

    /// @brief Replaces the drawn cameras with the cameras of the geometry.
    void upload(Diligent::IRenderDevice* device, const DisplayGeometry& geometry);

    /// @brief Draws the cameras into the bound targets.
    void draw(Diligent::IDeviceContext* context);

  private:
    Pipeline pipeline_;
    GpuArray frameVertices_;
    GpuArray transforms_;
};

} // namespace enzo::gfx
