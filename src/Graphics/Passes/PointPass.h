#pragma once
#include "Graphics/DisplayGeometry.h"
#include "Graphics/Passes/PassSetup.h"

#include <Buffer.h>
#include <DeviceContext.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>

namespace enzo::gfx {

/// @brief The solo points, drawn as discs facing the camera.
class PointPass
{
  public:
    /// @brief Builds the disc shape and the pipeline that draws it.
    ///
    /// @param frameConstants the buffer the renderer fills with the frame constants.
    PointPass(Diligent::IRenderDevice* device, Diligent::IBuffer* frameConstants);

    /// @brief Replaces the drawn points with the solo points of the geometry.
    void upload(Diligent::IRenderDevice* device, const DisplayGeometry& geometry);

    /// @brief Draws the points into the bound targets.
    void draw(Diligent::IDeviceContext* context);

  private:
    Pipeline pipeline_;
    GpuArray corners_;
    GpuArray positions_;
};

} // namespace enzo::gfx
