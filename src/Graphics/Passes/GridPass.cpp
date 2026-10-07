#include "Graphics/Passes/GridPass.h"
#include "Graphics/Passes/PassSetup.h"
#include "Graphics/Shaders/Grid.h"
#include <glm/vec3.hpp>
#include <vector>

namespace enzo::gfx {

namespace {

constexpr int lineCount = 40;
constexpr float lineSpacing = 2.f;
constexpr float halfLength = 50.f;

/// @brief Returns the endpoints of the grid lines on the ground plane, two per line.
std::vector<glm::vec3> buildGridVertices()
{
    std::vector<glm::vec3> vertices;
    const float firstOffset = -0.5f * (lineCount - 1) * lineSpacing;
    for (int lineIndex = 0; lineIndex < lineCount; ++lineIndex)
    {
        const float offset = firstOffset + lineIndex * lineSpacing;

        // Runs one line along z and one along x at the same offset.
        vertices.emplace_back(offset, 0.f, -halfLength);
        vertices.emplace_back(offset, 0.f, halfLength);
        vertices.emplace_back(-halfLength, 0.f, offset);
        vertices.emplace_back(halfLength, 0.f, offset);
    }
    return vertices;
}

} // namespace

GridPass::GridPass(Diligent::IRenderDevice* device, Diligent::IBuffer* frameConstants)
{
    // Vertex buffer
    vertices_ = createGpuArray(device, "Grid vertices", Diligent::BIND_VERTEX_BUFFER, buildGridVertices());

    // Pipeline
    Diligent::GraphicsPipelineStateCreateInfo pipelineInfo;
    pipelineInfo.PSODesc.Name = "Grid";

    Diligent::GraphicsPipelineDesc& graphics = pipelineInfo.GraphicsPipeline;
    graphics.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST;
    graphics.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
    // Skips the depth test and write so the mesh always draws over the grid.
    graphics.DepthStencilDesc.DepthEnable = false;

    // Blends the faded lines over the background and leaves the target opaque.
    Diligent::RenderTargetBlendDesc& blend = graphics.BlendDesc.RenderTargets[0];
    blend.BlendEnable = true;
    blend.SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
    blend.DestBlend = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
    blend.RenderTargetWriteMask = Diligent::COLOR_MASK_RGB;

    const Diligent::LayoutElement positionElement{0, 0, 3, Diligent::VT_FLOAT32, false};
    graphics.InputLayout.LayoutElements = &positionElement;
    graphics.InputLayout.NumElements = 1;

    Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader =
        createShader(device, shaders::gridSource, Diligent::SHADER_TYPE_VERTEX, "vertexMain");
    Diligent::RefCntAutoPtr<Diligent::IShader> pixelShader =
        createShader(device, shaders::gridSource, Diligent::SHADER_TYPE_PIXEL, "pixelMain");
    pipelineInfo.pVS = vertexShader;
    pipelineInfo.pPS = pixelShader;
    pipeline_ = createPipeline(device, pipelineInfo, frameConstants);
}

void GridPass::draw(Diligent::IDeviceContext* context)
{
    setVertexBuffers(context, {vertices_.buffer});
    setPipeline(context, pipeline_);

    Diligent::DrawAttribs drawAttribs;
    drawAttribs.NumVertices = vertices_.count;
    context->Draw(drawAttribs);
}

} // namespace enzo::gfx
