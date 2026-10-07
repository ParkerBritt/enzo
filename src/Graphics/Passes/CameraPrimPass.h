#pragma once
#include "Graphics/DisplayGeometry.h"
#include "Graphics/Passes/PassSetup.h"

#include <Buffer.h>
#include <DeviceContext.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>
#include <cstddef>
#include <optional>

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
    ///
    /// @param hiddenCamera the index of a camera left out, or empty to draw every camera.
    void draw(Diligent::IDeviceContext* context, std::optional<std::size_t> hiddenCamera);

  private:
    /// @brief Draws the cameras from the first index up to but not including the end index.
    void drawRange(Diligent::IDeviceContext* context, std::size_t first, std::size_t end);

    Pipeline pipeline_;
    GpuArray frameVertices_;
    GpuArray transforms_;
};

} // namespace enzo::gfx
