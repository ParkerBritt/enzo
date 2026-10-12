#include "Graphics/Passes/LightPrimPass.h"
#include "Graphics/Shaders/LightPrim.h"
#include <glm/gtc/constants.hpp>
#include <glm/vec2.hpp>
#include <cmath>
#include <vector>

namespace enzo::gfx {

namespace {

// Sizes of the marker in interface pixels.
constexpr float ringRadius = 6.f;
constexpr float rayStart = 9.f;
constexpr float rayEnd = 13.f;
constexpr int ringSegmentCount = 16;
constexpr int rayCount = 8;

/// @brief Returns the point on a circle of this radius at a fraction of the way round.
glm::vec2 getCirclePoint(float radius, float turnFraction)
{
    const float angle = turnFraction * glm::two_pi<float>();
    return radius * glm::vec2(std::cos(angle), std::sin(angle));
}

/// @brief Returns the line segments of the marker in pixels from its centre, two endpoints per segment.
std::vector<glm::vec2> buildMarkerVertices()
{
    std::vector<glm::vec2> vertices;
    for (int segment = 0; segment < ringSegmentCount; ++segment)
    {
        vertices.push_back(getCirclePoint(ringRadius, float(segment) / ringSegmentCount));
        vertices.push_back(getCirclePoint(ringRadius, float(segment + 1) / ringSegmentCount));
    }
    for (int ray = 0; ray < rayCount; ++ray)
    {
        const float turnFraction = float(ray) / rayCount;
        vertices.push_back(getCirclePoint(rayStart, turnFraction));
        vertices.push_back(getCirclePoint(rayEnd, turnFraction));
    }
    return vertices;
}

} // namespace

LightPrimPass::LightPrimPass(Diligent::IRenderDevice* device, Diligent::IBuffer* frameConstants)
{
    // Marker vertex buffer
    markerVertices_ =
        createGpuArray(device, "Light marker vertices", Diligent::BIND_VERTEX_BUFFER, buildMarkerVertices());

    // Pipeline
    Diligent::GraphicsPipelineStateCreateInfo pipelineInfo;
    pipelineInfo.PSODesc.Name = "Light primitives";

    Diligent::GraphicsPipelineDesc& graphics = pipelineInfo.GraphicsPipeline;
    graphics.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST;
    graphics.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;

    // Reads the marker vertices from buffer slot 0, and one position and color per light from slots 1 and 2.
    constexpr Diligent::INPUT_ELEMENT_FREQUENCY perLight = Diligent::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE;
    const Diligent::LayoutElement layoutElements[] = {
        {0, 0, 2, Diligent::VT_FLOAT32, false},
        {1, 1, 3, Diligent::VT_FLOAT32, false, perLight},
        {2, 2, 3, Diligent::VT_FLOAT32, false, perLight},
    };
    graphics.InputLayout.LayoutElements = layoutElements;
    graphics.InputLayout.NumElements = 3;

    Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader =
        createShader(device, shaders::lightPrimSource, Diligent::SHADER_TYPE_VERTEX, "vertexMain");
    Diligent::RefCntAutoPtr<Diligent::IShader> pixelShader =
        createShader(device, shaders::lightPrimSource, Diligent::SHADER_TYPE_PIXEL, "pixelMain");
    pipelineInfo.pVS = vertexShader;
    pipelineInfo.pPS = pixelShader;
    pipeline_ = createPipeline(device, pipelineInfo, frameConstants);
}

void LightPrimPass::upload(Diligent::IRenderDevice* device, const DisplayGeometry& geometry)
{
    positions_ = createGpuArray(
        device, "Point light positions", Diligent::BIND_VERTEX_BUFFER, geometry.pointLightPositions
    );
    colors_ =
        createGpuArray(device, "Point light colors", Diligent::BIND_VERTEX_BUFFER, geometry.pointLightColors);
}

void LightPrimPass::draw(Diligent::IDeviceContext* context)
{
    if (!positions_.buffer) return;

    setVertexBuffers(context, {markerVertices_.buffer, positions_.buffer, colors_.buffer});
    setPipeline(context, pipeline_);

    Diligent::DrawAttribs drawAttribs;
    drawAttribs.NumVertices = markerVertices_.count;
    drawAttribs.NumInstances = positions_.count;
    context->Draw(drawAttribs);
}

} // namespace enzo::gfx
