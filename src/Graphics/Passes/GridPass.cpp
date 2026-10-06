#include "Graphics/Passes/GridPass.h"
#include "Graphics/Shaders/Grid.h"
#include <glm/mat4x4.hpp>
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

Diligent::RefCntAutoPtr<Diligent::IShader> createShader(
    Diligent::IRenderDevice* device, Diligent::SHADER_TYPE type, const char* entryPoint
)
{
    Diligent::ShaderCreateInfo shaderInfo;
    shaderInfo.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
    shaderInfo.Source = shaders::gridSource;
    shaderInfo.EntryPoint = entryPoint;
    shaderInfo.Desc.ShaderType = type;
    shaderInfo.Desc.Name = entryPoint;

    Diligent::RefCntAutoPtr<Diligent::IShader> shader;
    device->CreateShader(shaderInfo, &shader);
    return shader;
}

} // namespace

GridPass::GridPass(
    Diligent::IRenderDevice* device, Diligent::TEXTURE_FORMAT colorFormat, Diligent::Uint8 sampleCount
)
{
    // Vertex buffer
    const std::vector<glm::vec3> vertices = buildGridVertices();
    vertexCount_ = static_cast<Diligent::Uint32>(vertices.size());

    Diligent::BufferDesc vertexBufferDesc;
    vertexBufferDesc.Name = "Grid vertices";
    vertexBufferDesc.Usage = Diligent::USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
    vertexBufferDesc.Size = vertices.size() * sizeof(glm::vec3);
    Diligent::BufferData vertexData{vertices.data(), vertexBufferDesc.Size};
    device->CreateBuffer(vertexBufferDesc, &vertexData, &vertexBuffer_);

    // Constant buffer
    Diligent::BufferDesc constantBufferDesc;
    constantBufferDesc.Name = "Grid constants";
    constantBufferDesc.Usage = Diligent::USAGE_DEFAULT;
    constantBufferDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
    constantBufferDesc.Size = sizeof(glm::mat4);
    device->CreateBuffer(constantBufferDesc, nullptr, &constantBuffer_);

    // Pipeline
    Diligent::GraphicsPipelineStateCreateInfo pipelineInfo;
    pipelineInfo.PSODesc.Name = "Grid";
    pipelineInfo.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;

    Diligent::GraphicsPipelineDesc& graphics = pipelineInfo.GraphicsPipeline;
    graphics.NumRenderTargets = 1;
    graphics.RTVFormats[0] = colorFormat;
    graphics.SmplDesc.Count = sampleCount;
    graphics.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST;
    graphics.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
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
        createShader(device, Diligent::SHADER_TYPE_VERTEX, "vertexMain");
    Diligent::RefCntAutoPtr<Diligent::IShader> pixelShader =
        createShader(device, Diligent::SHADER_TYPE_PIXEL, "pixelMain");
    pipelineInfo.pVS = vertexShader;
    pipelineInfo.pPS = pixelShader;
    device->CreateGraphicsPipelineState(pipelineInfo, &pipeline_);

    pipeline_->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "Constants")->Set(constantBuffer_);
    pipeline_->CreateShaderResourceBinding(&resources_, true);
}

void GridPass::draw(Diligent::IDeviceContext* context, const FrameState& frame)
{
    const float aspect = float(frame.pixelSize.x) / float(frame.pixelSize.y);
    const glm::mat4 viewProjection =
        frame.camera.getProjectionMatrix(aspect) * frame.camera.getViewMatrix();
    context->UpdateBuffer(
        constantBuffer_,
        0,
        sizeof(viewProjection),
        &viewProjection,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    );

    Diligent::IBuffer* vertexBuffers[] = {vertexBuffer_};
    context->SetVertexBuffers(
        0,
        1,
        vertexBuffers,
        nullptr,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
        Diligent::SET_VERTEX_BUFFERS_FLAG_RESET
    );
    context->SetPipelineState(pipeline_);
    context->CommitShaderResources(resources_, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    Diligent::DrawAttribs drawAttribs;
    drawAttribs.NumVertices = vertexCount_;
    context->Draw(drawAttribs);
}

} // namespace enzo::gfx
