#include "Graphics/Passes/CameraPrimPass.h"
#include "Graphics/Shaders/CameraPrim.h"
#include <glm/vec3.hpp>
#include <vector>

namespace enzo::gfx {

namespace {

constexpr float frameHalfWidth = 0.5f;
constexpr float frameHalfHeight = 0.375f;

/// @brief Returns the edges of the camera frame, two endpoints per edge.
std::vector<glm::vec3> buildFrameVertices()
{
    const glm::vec3 bottomLeft{-frameHalfWidth, -frameHalfHeight, 0.f};
    const glm::vec3 bottomRight{frameHalfWidth, -frameHalfHeight, 0.f};
    const glm::vec3 topRight{frameHalfWidth, frameHalfHeight, 0.f};
    const glm::vec3 topLeft{-frameHalfWidth, frameHalfHeight, 0.f};
    return {bottomLeft, bottomRight, bottomRight, topRight, topRight, topLeft, topLeft, bottomLeft};
}

} // namespace

CameraPrimPass::CameraPrimPass(Diligent::IRenderDevice* device, Diligent::IBuffer* frameConstants)
{
    // Frame vertex buffer
    frameVertices_ =
        createGpuArray(device, "Camera frame vertices", Diligent::BIND_VERTEX_BUFFER, buildFrameVertices());

    // Pipeline
    Diligent::GraphicsPipelineStateCreateInfo pipelineInfo;
    pipelineInfo.PSODesc.Name = "Camera primitives";

    Diligent::GraphicsPipelineDesc& graphics = pipelineInfo.GraphicsPipeline;
    graphics.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST;
    graphics.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;

    // Reads the frame vertices from buffer slot 0 and one transform per camera, column by column, from slot 1.
    constexpr Diligent::INPUT_ELEMENT_FREQUENCY perCamera = Diligent::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE;
    const Diligent::LayoutElement layoutElements[] = {
        {0, 0, 3, Diligent::VT_FLOAT32, false},
        {1, 1, 4, Diligent::VT_FLOAT32, false, perCamera},
        {2, 1, 4, Diligent::VT_FLOAT32, false, perCamera},
        {3, 1, 4, Diligent::VT_FLOAT32, false, perCamera},
        {4, 1, 4, Diligent::VT_FLOAT32, false, perCamera},
    };
    graphics.InputLayout.LayoutElements = layoutElements;
    graphics.InputLayout.NumElements = 5;

    Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader =
        createShader(device, shaders::cameraPrimSource, Diligent::SHADER_TYPE_VERTEX, "vertexMain");
    Diligent::RefCntAutoPtr<Diligent::IShader> pixelShader =
        createShader(device, shaders::cameraPrimSource, Diligent::SHADER_TYPE_PIXEL, "pixelMain");
    pipelineInfo.pVS = vertexShader;
    pipelineInfo.pPS = pixelShader;
    pipeline_ = createPipeline(device, pipelineInfo, frameConstants);
}

void CameraPrimPass::upload(Diligent::IRenderDevice* device, const DisplayGeometry& geometry)
{
    transforms_ =
        createGpuArray(device, "Camera transforms", Diligent::BIND_VERTEX_BUFFER, geometry.cameraTransforms);
}

void CameraPrimPass::draw(Diligent::IDeviceContext* context, std::optional<std::size_t> hiddenCamera)
{
    if (!transforms_.buffer) return;

    setVertexBuffers(context, {frameVertices_.buffer, transforms_.buffer});
    setPipeline(context, pipeline_);

    if (!hiddenCamera.has_value())
    {
        drawRange(context, 0, transforms_.count);
        return;
    }
    drawRange(context, 0, *hiddenCamera);
    drawRange(context, *hiddenCamera + 1, transforms_.count);
}

void CameraPrimPass::drawRange(Diligent::IDeviceContext* context, std::size_t first, std::size_t end)
{
    if (first >= end) return;

    Diligent::DrawAttribs drawAttribs;
    drawAttribs.NumVertices = frameVertices_.count;
    drawAttribs.FirstInstanceLocation = Diligent::Uint32(first);
    drawAttribs.NumInstances = Diligent::Uint32(end - first);
    context->Draw(drawAttribs);
}

} // namespace enzo::gfx
