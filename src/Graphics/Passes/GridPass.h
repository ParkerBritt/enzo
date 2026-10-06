#pragma once
#include "Graphics/FrameState.h"

#include <Buffer.h>
#include <DeviceContext.h>
#include <PipelineState.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>
#include <ShaderResourceBinding.h>

namespace enzo::gfx {

/// @brief The ground grid drawn under the scene.
class GridPass
{
  public:
    /// @brief Builds the grid lines and the pipeline that draws them.
    ///
    /// @param colorFormat the format of the colour target the grid draws into.
    /// @param sampleCount the sample count of that target.
    GridPass(Diligent::IRenderDevice* device, Diligent::TEXTURE_FORMAT colorFormat, Diligent::Uint8 sampleCount);

    /// @brief Draws the grid into the bound colour target.
    void draw(Diligent::IDeviceContext* context, const FrameState& frame);

  private:
    Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_;
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> resources_;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> vertexBuffer_;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> constantBuffer_;
    Diligent::Uint32 vertexCount_ = 0;
};

} // namespace enzo::gfx
