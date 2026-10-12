#pragma once
#include "Graphics/DisplayGeometry.h"
#include "Graphics/Passes/FrameView.h"
#include "Graphics/Passes/PassSetup.h"

#include <Buffer.h>
#include <DeviceContext.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>
#include <Texture.h>
#include <glm/vec4.hpp>
#include <memory>
#include <vector>

namespace Diligent {
class PBR_Renderer;
}

namespace enzo::gfx {

/**
 * @brief The display mesh, with its faces lit by the point lights and an optional wireframe over it.
 *
 * @note Only the 256 lights nearest the camera shade the faces.
 */
class MeshPass
{
  public:
    /// @brief Builds the pipelines that draw the mesh.
    ///
    /// @param frameConstants the buffer the renderer fills with the frame constants.
    MeshPass(Diligent::IRenderDevice* device, Diligent::IDeviceContext* context, Diligent::IBuffer* frameConstants);
    MeshPass(MeshPass&&) noexcept;
    ~MeshPass();

    /// @brief Replaces the drawn mesh and the lights shading it with those of the geometry.
    void upload(Diligent::IRenderDevice* device, const DisplayGeometry& geometry);

    /// @brief Fills the buffers the faces are shaded from with the camera, lights and face color.
    ///
    /// @param geometryColor the color of the faces, in sRGB.
    void updateShading(
        Diligent::IDeviceContext* context, const FrameView& frameView, const glm::vec4& geometryColor
    );

    /// @brief Draws the mesh into the bound targets.
    void draw(Diligent::IDeviceContext* context, bool wireframeVisible);

  private:
    /// @brief Draws the indexed vertices with the pipeline.
    void drawIndexed(Diligent::IDeviceContext* context, const Pipeline& pipeline, const GpuArray& indices);

    std::unique_ptr<Diligent::PBR_Renderer> pbrRenderer_;
    /// @brief The faces, shaded by the PBR renderer.
    Pipeline facePipeline_;
    /// @brief The camera, renderer settings and lights the face pipeline reads.
    Diligent::RefCntAutoPtr<Diligent::IBuffer> faceFrameAttribs_;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> faceMaterialAttribs_;
    /// @brief The uniform surroundings that light the faces from every side.
    Diligent::RefCntAutoPtr<Diligent::ITexture> irradianceCube_;
    Diligent::RefCntAutoPtr<Diligent::ITexture> prefilteredEnvironmentCube_;

    Pipeline openFacePipeline_;
    Pipeline wireframePipeline_;

    GpuArray positions_;
    GpuArray normals_;
    GpuArray triangleIndices_;
    GpuArray edgeIndices_;
    GpuArray lineIndices_;
    std::vector<glm::vec3> pointLightPositions_;
    /// @brief The color of each point light scaled by its intensity.
    std::vector<glm::vec3> pointLightRadiances_;
};

} // namespace enzo::gfx
