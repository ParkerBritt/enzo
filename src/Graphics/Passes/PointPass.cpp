#include "Graphics/Passes/PointPass.h"
#include "Graphics/Shaders/Points.h"
#include <glm/vec2.hpp>
#include <vector>

namespace enzo::gfx {

PointPass::PointPass(Diligent::IRenderDevice* device, Diligent::IBuffer* frameConstants)
{
    // Corner buffer
    const std::vector<glm::vec2> corners = {{-0.5f, -0.5f}, {0.5f, -0.5f}, {-0.5f, 0.5f}, {0.5f, 0.5f}};
    corners_ = createGpuArray(device, "Point corners", Diligent::BIND_VERTEX_BUFFER, corners);

    // Pipeline
    Diligent::GraphicsPipelineStateCreateInfo pipelineInfo;
    pipelineInfo.PSODesc.Name = "Points";

    Diligent::GraphicsPipelineDesc& graphics = pipelineInfo.GraphicsPipeline;
    graphics.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    graphics.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;

    // Reads the disc corners from buffer slot 0 and one point position per disc from slot 1.
    const Diligent::LayoutElement layoutElements[] = {
        {0, 0, 2, Diligent::VT_FLOAT32, false},
        {1, 1, 3, Diligent::VT_FLOAT32, false, Diligent::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE},
    };
    graphics.InputLayout.LayoutElements = layoutElements;
    graphics.InputLayout.NumElements = 2;

    Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader =
        createShader(device, shaders::pointsSource, Diligent::SHADER_TYPE_VERTEX, "vertexMain");
    Diligent::RefCntAutoPtr<Diligent::IShader> pixelShader =
        createShader(device, shaders::pointsSource, Diligent::SHADER_TYPE_PIXEL, "pixelMain");
    pipelineInfo.pVS = vertexShader;
    pipelineInfo.pPS = pixelShader;
    pipeline_ = createPipeline(device, pipelineInfo, frameConstants);
}

void PointPass::upload(Diligent::IRenderDevice* device, const DisplayGeometry& geometry)
{
    positions_ =
        createGpuArray(device, "Point positions", Diligent::BIND_VERTEX_BUFFER, geometry.soloPointPositions);
}

void PointPass::draw(Diligent::IDeviceContext* context)
{
    if (!positions_.buffer) return;

    setVertexBuffers(context, {corners_.buffer, positions_.buffer});
    setPipeline(context, pipeline_);

    Diligent::DrawAttribs drawAttribs;
    drawAttribs.NumVertices = corners_.count;
    drawAttribs.NumInstances = positions_.count;
    context->Draw(drawAttribs);
}

} // namespace enzo::gfx
