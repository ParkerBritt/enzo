#pragma once
#include "Graphics/DisplayGeometry.h"
#include "Graphics/Passes/PassSetup.h"

#include <Buffer.h>
#include <DeviceContext.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>

namespace enzo::gfx {

/// @brief The mesh points, drawn as discs facing the camera.
class PointPass
{
  public:
    /// @brief Builds the disc shape and the pipeline that draws it.
    ///
    /// @param frameConstants the buffer the renderer fills with the frame constants.
    PointPass(Diligent::IRenderDevice* device, Diligent::IBuffer* frameConstants);

    /// @brief Replaces the drawn points with the points of the geometry.
    void upload(Diligent::IRenderDevice* device, const DisplayGeometry& geometry);

    /// @brief Draws the points into the bound targets.
    ///
    /// @param pointsVisible whether every point is drawn. The points that belong to no face are
    /// always drawn.
    void draw(Diligent::IDeviceContext* context, bool pointsVisible);

  private:
    Pipeline pipeline_;
    GpuArray corners_;
    GpuArray pointPositions_;
    GpuArray soloPointPositions_;
};

} // namespace enzo::gfx
