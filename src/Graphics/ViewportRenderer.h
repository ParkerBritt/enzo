#pragma once
#include "Graphics/FrameState.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/VulkanImage.h"
#include <memory>

namespace enzo::gfx {

/// @brief A renderer that draws viewport frames into an image a host samples.
class ViewportRenderer
{
  public:
    explicit ViewportRenderer(GraphicsDevice& device);
    ~ViewportRenderer();

    ViewportRenderer(const ViewportRenderer&) = delete;
    ViewportRenderer& operator=(const ViewportRenderer&) = delete;

    /// @brief Draws one frame and returns the image it was drawn into.
    ///
    /// @note The image changes whenever the frame size does.
    VulkanImage render(const FrameState& frame);

  private:
    struct DiligentObjects;
    GraphicsDevice& device_;
    std::unique_ptr<DiligentObjects> diligent_;
    /// @brief The geometry the mesh pass holds on the GPU.
    std::shared_ptr<const DisplayGeometry> uploadedGeometry_;
};

} // namespace enzo::gfx
