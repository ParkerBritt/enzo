#pragma once
#include "Graphics/DisplayGeometry.h"
#include "Graphics/Passes/PassSetup.h"

#include <Buffer.h>
#include <DeviceContext.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>

namespace enzo::gfx {

/// @brief The light primitives, drawn as rings with rays facing the camera.
class LightPrimPass
{
  public:
    /// @brief Builds the marker shape and the pipeline that draws it.
    ///
    /// @param frameConstants the buffer the renderer fills with the frame constants.
    LightPrimPass(Diligent::IRenderDevice* device, Diligent::IBuffer* frameConstants);

    /// @brief Replaces the drawn lights with the lights of the geometry.
    void upload(Diligent::IRenderDevice* device, const DisplayGeometry& geometry);

    /// @brief Draws the lights into the bound targets.
    void draw(Diligent::IDeviceContext* context);

  private:
    Pipeline pipeline_;
    GpuArray markerVertices_;
    GpuArray positions_;
    GpuArray colors_;
};

} // namespace enzo::gfx
