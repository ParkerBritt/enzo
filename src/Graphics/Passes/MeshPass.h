#pragma once
#include "Graphics/DisplayGeometry.h"
#include "Graphics/Passes/PassSetup.h"

#include <Buffer.h>
#include <DeviceContext.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>

namespace enzo::gfx {

/// @brief The display mesh, drawn shaded with an optional wireframe over it.
class MeshPass
{
  public:
    /// @brief Builds the pipelines that draw the mesh.
    ///
    /// @param frameConstants the buffer the renderer fills with the frame constants.
    MeshPass(Diligent::IRenderDevice* device, Diligent::IBuffer* frameConstants);

    /// @brief Replaces the drawn mesh with the geometry.
    void upload(Diligent::IRenderDevice* device, const DisplayGeometry& geometry);

    /// @brief Draws the mesh into the bound targets.
    void draw(Diligent::IDeviceContext* context, bool wireframeVisible);

  private:
    /// @brief A buffer of indices into the vertex buffers, empty when there are none.
    struct IndexBuffer
    {
        Diligent::RefCntAutoPtr<Diligent::IBuffer> buffer;
        Diligent::Uint32 indexCount = 0;
    };

    /// @brief Draws the indexed vertices with the pipeline.
    void drawIndexed(Diligent::IDeviceContext* context, const Pipeline& pipeline, const IndexBuffer& indices);

    Pipeline shadedTrianglePipeline_;
    Pipeline shadedLinePipeline_;
    Pipeline wireframePipeline_;

    Diligent::RefCntAutoPtr<Diligent::IBuffer> positionBuffer_;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> normalBuffer_;
    IndexBuffer triangleIndices_;
    IndexBuffer edgeIndices_;
    IndexBuffer lineIndices_;
};

} // namespace enzo::gfx
