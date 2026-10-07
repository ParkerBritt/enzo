#include "Graphics/Passes/MeshPass.h"
#include "Graphics/Passes/PassSetup.h"
#include "Graphics/Shaders/Mesh.h"

namespace enzo::gfx {

namespace {

/// @brief The parts that differ between the pipelines of the mesh pass.
struct PipelineSettings
{
    const char* name;
    Diligent::PRIMITIVE_TOPOLOGY topology;
    Diligent::IShader* pixelShader;
    /// @brief Whether the faces get a depth bias so the wireframe draws on top.
    bool pushedBack;
};

Pipeline createMeshPipeline(
    Diligent::IRenderDevice* device,
    Diligent::IBuffer* frameConstants,
    Diligent::IShader* vertexShader,
    const PipelineSettings& settings
)
{
    Diligent::GraphicsPipelineStateCreateInfo pipelineInfo;
    pipelineInfo.PSODesc.Name = settings.name;

    Diligent::GraphicsPipelineDesc& graphics = pipelineInfo.GraphicsPipeline;
    graphics.PrimitiveTopology = settings.topology;
    graphics.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
    graphics.DepthStencilDesc.DepthFunc = Diligent::COMPARISON_FUNC_LESS_EQUAL;
    if (settings.pushedBack)
    {
        graphics.RasterizerDesc.DepthBias = 1;
        graphics.RasterizerDesc.SlopeScaledDepthBias = 1.f;
    }

    // Reads positions from buffer slot 0 and normals from slot 1.
    const Diligent::LayoutElement layoutElements[] = {
        {0, 0, 3, Diligent::VT_FLOAT32, false},
        {1, 1, 3, Diligent::VT_FLOAT32, false},
    };
    graphics.InputLayout.LayoutElements = layoutElements;
    graphics.InputLayout.NumElements = 2;

    pipelineInfo.pVS = vertexShader;
    pipelineInfo.pPS = settings.pixelShader;
    return createPipeline(device, pipelineInfo, frameConstants);
}

} // namespace

MeshPass::MeshPass(Diligent::IRenderDevice* device, Diligent::IBuffer* frameConstants)
{
    Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader =
        createShader(device, shaders::meshSource, Diligent::SHADER_TYPE_VERTEX, "vertexMain");
    Diligent::RefCntAutoPtr<Diligent::IShader> shadedPixelShader =
        createShader(device, shaders::meshSource, Diligent::SHADER_TYPE_PIXEL, "shadedPixelMain");
    Diligent::RefCntAutoPtr<Diligent::IShader> wireframePixelShader =
        createShader(device, shaders::meshSource, Diligent::SHADER_TYPE_PIXEL, "wireframePixelMain");

    shadedTrianglePipeline_ = createMeshPipeline(
        device,
        frameConstants,
        vertexShader,
        {"Mesh faces", Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shadedPixelShader, true}
    );
    shadedLinePipeline_ = createMeshPipeline(
        device,
        frameConstants,
        vertexShader,
        {"Mesh open faces", Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST, shadedPixelShader, false}
    );
    wireframePipeline_ = createMeshPipeline(
        device,
        frameConstants,
        vertexShader,
        {"Mesh wireframe", Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST, wireframePixelShader, false}
    );
}

void MeshPass::upload(Diligent::IRenderDevice* device, const DisplayGeometry& geometry)
{
    const DisplayTopology& topology = geometry.topology;
    positionBuffer_ =
        createImmutableBuffer(device, "Mesh positions", Diligent::BIND_VERTEX_BUFFER, geometry.positions);
    normalBuffer_ = createImmutableBuffer(device, "Mesh normals", Diligent::BIND_VERTEX_BUFFER, geometry.normals);

    triangleIndices_.buffer =
        createImmutableBuffer(device, "Mesh triangles", Diligent::BIND_INDEX_BUFFER, topology.triangleIndices);
    triangleIndices_.indexCount = Diligent::Uint32(topology.triangleIndices.size());
    edgeIndices_.buffer =
        createImmutableBuffer(device, "Mesh edges", Diligent::BIND_INDEX_BUFFER, topology.edgeIndices);
    edgeIndices_.indexCount = Diligent::Uint32(topology.edgeIndices.size());
    lineIndices_.buffer =
        createImmutableBuffer(device, "Mesh open faces", Diligent::BIND_INDEX_BUFFER, topology.lineIndices);
    lineIndices_.indexCount = Diligent::Uint32(topology.lineIndices.size());
}

void MeshPass::draw(Diligent::IDeviceContext* context, bool wireframeVisible)
{
    if (!positionBuffer_) return;

    Diligent::IBuffer* vertexBuffers[] = {positionBuffer_, normalBuffer_};
    context->SetVertexBuffers(
        0,
        2,
        vertexBuffers,
        nullptr,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
        Diligent::SET_VERTEX_BUFFERS_FLAG_RESET
    );

    drawIndexed(context, shadedTrianglePipeline_, triangleIndices_);
    drawIndexed(context, shadedLinePipeline_, lineIndices_);
    if (wireframeVisible) drawIndexed(context, wireframePipeline_, edgeIndices_);
}

void MeshPass::drawIndexed(
    Diligent::IDeviceContext* context, const Pipeline& pipeline, const IndexBuffer& indices
)
{
    if (!indices.buffer) return;

    context->SetIndexBuffer(indices.buffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    context->SetPipelineState(pipeline.state);
    context->CommitShaderResources(
        pipeline.resources, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    );

    Diligent::DrawIndexedAttribs drawAttribs;
    drawAttribs.NumIndices = indices.indexCount;
    drawAttribs.IndexType = Diligent::VT_UINT32;
    context->DrawIndexed(drawAttribs);
}

} // namespace enzo::gfx
