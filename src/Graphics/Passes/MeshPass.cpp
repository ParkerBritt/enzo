#include "Graphics/Passes/MeshPass.h"
#include "Graphics/Passes/NearestLights.h"
#include "Graphics/Passes/PassSetup.h"
#include "Graphics/Shaders/Mesh.h"

#include <BasicMath.hpp>
#include <ColorConversion.h>
#include <PBR_Renderer.hpp>
#include <cstddef>
#include <cstring>
#include <glm/matrix.hpp>
#include <stdexcept>

namespace Diligent::HLSL {

#include "Shaders/Common/public/BasicStructures.fxh"
#include "Shaders/PBR/public/PBR_Structures.fxh"

} // namespace Diligent::HLSL

namespace enzo::gfx {

namespace {

using Diligent::PBR_Renderer;

const PBR_Renderer::PSO_FLAGS facePipelineFlags = PBR_Renderer::PSO_FLAG_USE_VERTEX_NORMALS |
                                                  PBR_Renderer::PSO_FLAG_USE_LIGHTS | PBR_Renderer::PSO_FLAG_USE_IBL |
                                                  PBR_Renderer::PSO_FLAG_CONVERT_OUTPUT_TO_SRGB;

/// @brief The light arriving from every direction of the uniform surroundings, in linear color.
constexpr float ambientRadiance = 0.3f;

constexpr float faceRoughness = 0.5f;

/// @brief The most lights shading the faces, picked nearest the camera.
constexpr Diligent::Uint32 maxShadingLightCount = 256;

/// @brief The start of the frame attributes the face shaders read, which the lights follow.
struct FaceFrameAttribsHeader
{
    Diligent::HLSL::CameraAttribs camera;
    Diligent::HLSL::CameraAttribs previousCamera;
    Diligent::HLSL::PBRRendererShaderParameters renderer;
};

/// @brief The per draw values the face shaders read, laid out as `PBRPrimitiveAttribs` for the face
/// flags.
struct FacePrimitiveAttribs
{
    Diligent::float4x4 nodeMatrix;
    Diligent::float4 fallbackColor;
    Diligent::float4 customData;
};

/// @brief Returns the matrix laid out for a PBR shader, which multiplies a row vector on its left.
Diligent::float4x4 toShaderMatrix(const glm::mat4& matrix)
{
    const glm::mat4 transposed = glm::transpose(matrix);
    Diligent::float4x4 shaderMatrix;
    std::memcpy(&shaderMatrix, &transposed, sizeof(shaderMatrix));
    return shaderMatrix;
}

/// @brief Returns the settings of a PBR renderer that shades untextured faces from vertex positions
/// and normals.
PBR_Renderer::CreateInfo getPbrSettings(
    Diligent::IBuffer* primitiveAttribs,
    Diligent::IBuffer* materialAttribs,
    Diligent::InputLayoutDesc inputLayout
)
{
    PBR_Renderer::CreateInfo settings;
    settings.EnableAO = false;
    settings.EnableEmissive = false;
    settings.MaxJointCount = 0;
    settings.MaxActiveMorphTargetCount = 0;
    settings.MaxLightCount = maxShadingLightCount;
    settings.pPrimitiveAttribsCB = primitiveAttribs;
    settings.pMaterialAttribsCB = materialAttribs;
    settings.InputLayout = inputLayout;
    return settings;
}

/// @brief Returns the description of the pipeline that draws the faces into the viewport targets.
Diligent::GraphicsPipelineDesc getFaceGraphicsDesc()
{
    Diligent::GraphicsPipelineDesc graphics;
    setViewportTargets(graphics);
    graphics.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    graphics.RasterizerDesc.FrontCounterClockwise = true;
    graphics.DepthStencilDesc.DepthFunc = Diligent::COMPARISON_FUNC_LESS_EQUAL;

    // Pushes the faces back so the wireframe draws on top.
    graphics.RasterizerDesc.DepthBias = 1;
    graphics.RasterizerDesc.SlopeScaledDepthBias = 1.f;
    return graphics;
}

/// @brief Returns a pipeline that draws lines of the mesh with our own shaders.
Pipeline createLinePipeline(
    Diligent::IRenderDevice* device,
    Diligent::IBuffer* frameConstants,
    const char* name,
    Diligent::IShader* vertexShader,
    Diligent::IShader* pixelShader,
    const Diligent::InputLayoutDesc& inputLayout
)
{
    Diligent::GraphicsPipelineStateCreateInfo pipelineInfo;
    pipelineInfo.PSODesc.Name = name;

    Diligent::GraphicsPipelineDesc& graphics = pipelineInfo.GraphicsPipeline;
    graphics.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST;
    graphics.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
    graphics.DepthStencilDesc.DepthFunc = Diligent::COMPARISON_FUNC_LESS_EQUAL;
    graphics.InputLayout = inputLayout;

    pipelineInfo.pVS = vertexShader;
    pipelineInfo.pPS = pixelShader;
    return createPipeline(device, pipelineInfo, frameConstants);
}

} // namespace

MeshPass::MeshPass(
    Diligent::IRenderDevice* device,
    Diligent::IDeviceContext* context,
    Diligent::IBuffer* frameConstants
)
{
    // Reads positions from buffer slot 0 and normals from slot 1.
    const Diligent::LayoutElement layoutElements[] = {
        {0, 0, 3, Diligent::VT_FLOAT32, false},
        {1, 1, 3, Diligent::VT_FLOAT32, false},
    };
    const Diligent::InputLayoutDesc inputLayout{layoutElements, 2};

    // Face renderer
    const FacePrimitiveAttribs primitiveAttribs{
        Diligent::float4x4::Identity(),
        Diligent::float4{1.f},
        Diligent::float4{0.f}
    };
    Diligent::RefCntAutoPtr<Diligent::IBuffer> facePrimitiveAttribs =
        createConstantBuffer(device, "Mesh face primitive attributes", sizeof(primitiveAttribs), &primitiveAttribs);
    faceMaterialAttribs_ =
        createConstantBuffer(device, "Mesh face material attributes", sizeof(Diligent::HLSL::PBRMaterialBasicAttribs));
    pbrRenderer_ = std::make_unique<PBR_Renderer>(
        device,
        nullptr,
        context,
        getPbrSettings(facePrimitiveAttribs, faceMaterialAttribs_, inputLayout)
    );

    // Throws when the DiligentFX shaders lay out the buffers differently from the structs above.
    const bool primitiveLayoutMatches =
        pbrRenderer_->GetPBRPrimitiveAttribsSize(facePipelineFlags) == sizeof(FacePrimitiveAttribs);
    const bool materialLayoutMatches =
        pbrRenderer_->GetPBRMaterialAttribsSize(facePipelineFlags) == sizeof(Diligent::HLSL::PBRMaterialBasicAttribs);
    if (!primitiveLayoutMatches || !materialLayoutMatches)
        throw std::runtime_error("The PBR renderer expects a different face attribute layout");

    faceFrameAttribs_ =
        createConstantBuffer(device, "Mesh face frame attributes", pbrRenderer_->GetPRBFrameAttribsSize());

    // Uniform surroundings
    const float ambientColor[] = {ambientRadiance, ambientRadiance, ambientRadiance, 1.f};
    constexpr Diligent::Uint32 environmentCubeSize = 8;
    irradianceCube_ = pbrRenderer_->CreateIrradianceCube(
        context,
        "Mesh irradiance",
        PBR_Renderer::IrradianceCubeFmt,
        environmentCubeSize,
        ambientColor
    );
    prefilteredEnvironmentCube_ = pbrRenderer_->CreatePrefilteredEnvMap(
        context,
        "Mesh prefiltered environment",
        PBR_Renderer::PrefilteredEnvMapFmt,
        environmentCubeSize,
        ambientColor
    );
    Diligent::ITextureView* irradianceView = irradianceCube_->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
    Diligent::ITextureView* prefilteredEnvironmentView =
        prefilteredEnvironmentCube_->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);

    // Face pipeline
    const PBR_Renderer::PSOKey faceKey{PBR_Renderer::RenderPassType::Main, facePipelineFlags, Diligent::CULL_MODE_NONE};
    facePipeline_.state = pbrRenderer_->GetPsoCacheAccessor(getFaceGraphicsDesc())
                              .Get(faceKey, PBR_Renderer::PsoCacheAccessor::GET_FLAG_CREATE_IF_NULL);
    pbrRenderer_->CreateResourceBinding(&facePipeline_.resources);
    pbrRenderer_->InitCommonSRBVars(facePipeline_.resources, {faceFrameAttribs_});
    pbrRenderer_->SetIBLResourceViews(facePipeline_.resources, irradianceView, prefilteredEnvironmentView);
    pbrRenderer_->SetMaterialTexture(
        facePipeline_.resources,
        pbrRenderer_->GetWhiteTexSRV(),
        PBR_Renderer::TEXTURE_ATTRIB_ID_BASE_COLOR
    );
    pbrRenderer_->SetMaterialTexture(
        facePipeline_.resources,
        pbrRenderer_->GetDefaultNormalMapSRV(),
        PBR_Renderer::TEXTURE_ATTRIB_ID_NORMAL
    );
    pbrRenderer_->SetMaterialTexture(
        facePipeline_.resources,
        pbrRenderer_->GetWhiteTexSRV(),
        PBR_Renderer::TEXTURE_ATTRIB_ID_PHYS_DESC
    );

    // Open face and wireframe pipelines
    Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader =
        createShader(device, shaders::meshSource, Diligent::SHADER_TYPE_VERTEX, "vertexMain");
    Diligent::RefCntAutoPtr<Diligent::IShader> openFacePixelShader =
        createShader(device, shaders::meshSource, Diligent::SHADER_TYPE_PIXEL, "openFacePixelMain");
    Diligent::RefCntAutoPtr<Diligent::IShader> wireframePixelShader =
        createShader(device, shaders::meshSource, Diligent::SHADER_TYPE_PIXEL, "wireframePixelMain");
    openFacePipeline_ =
        createLinePipeline(device, frameConstants, "Mesh open faces", vertexShader, openFacePixelShader, inputLayout);
    wireframePipeline_ =
        createLinePipeline(device, frameConstants, "Mesh wireframe", vertexShader, wireframePixelShader, inputLayout);
}

MeshPass::MeshPass(MeshPass&&) noexcept = default;

MeshPass::~MeshPass() = default;

void MeshPass::upload(Diligent::IRenderDevice* device, const DisplayGeometry& geometry)
{
    const DisplayTopology& topology = geometry.topology;
    positions_ = createGpuArray(device, "Mesh positions", Diligent::BIND_VERTEX_BUFFER, geometry.positions);
    normals_ = createGpuArray(device, "Mesh normals", Diligent::BIND_VERTEX_BUFFER, geometry.normals);

    triangleIndices_ = createGpuArray(device, "Mesh triangles", Diligent::BIND_INDEX_BUFFER, topology.triangleIndices);
    edgeIndices_ = createGpuArray(device, "Mesh edges", Diligent::BIND_INDEX_BUFFER, topology.edgeIndices);
    lineIndices_ = createGpuArray(device, "Mesh open faces", Diligent::BIND_INDEX_BUFFER, topology.lineIndices);

    pointLightPositions_ = geometry.pointLightPositions;
    pointLightRadiances_.clear();
    for (std::size_t lightIndex = 0; lightIndex < geometry.pointLightColors.size(); ++lightIndex)
        pointLightRadiances_.push_back(
            geometry.pointLightColors[lightIndex] * geometry.pointLightIntensities[lightIndex]
        );
}

void MeshPass::draw(Diligent::IDeviceContext* context, bool wireframeVisible)
{
    if (!positions_.buffer) return;

    setVertexBuffers(context, {positions_.buffer, normals_.buffer});

    drawIndexed(context, facePipeline_, triangleIndices_);
    drawIndexed(context, openFacePipeline_, lineIndices_);
    if (wireframeVisible) drawIndexed(context, wireframePipeline_, edgeIndices_);
}

void MeshPass::updateShading(
    Diligent::IDeviceContext* context,
    const FrameView& frameView,
    const glm::vec4& geometryColor
)
{
    // Camera
    const glm::mat4 viewProjection = frameView.projection * frameView.view;
    const glm::vec2 pixelSize{frameView.pixelSize};
    Diligent::HLSL::CameraAttribs camera{};
    camera.f4Position =
        Diligent::float4{frameView.cameraPosition.x, frameView.cameraPosition.y, frameView.cameraPosition.z, 1.f};
    camera.f4ViewportSize = Diligent::float4{pixelSize.x, pixelSize.y, 1.f / pixelSize.x, 1.f / pixelSize.y};
    camera.fHandness = 1.f;
    camera.mView = toShaderMatrix(frameView.view);
    camera.mProj = toShaderMatrix(frameView.projection);
    camera.mViewProj = toShaderMatrix(viewProjection);
    camera.mViewInv = toShaderMatrix(glm::inverse(frameView.view));
    camera.mProjInv = toShaderMatrix(glm::inverse(frameView.projection));
    camera.mViewProjInv = toShaderMatrix(glm::inverse(viewProjection));

    // Renderer settings
    const std::vector<std::size_t> shadingLightIndices =
        getNearestLightIndices(pointLightPositions_, frameView.cameraPosition, maxShadingLightCount);
    Diligent::HLSL::PBRRendererShaderParameters renderer{};
    renderer.IBLScale = Diligent::float4{1.f};
    renderer.OcclusionStrength = 1.f;
    renderer.EmissionScale = 1.f;
    renderer.LightCount = int(shadingLightIndices.size());
    pbrRenderer_->SetInternalShaderParameters(
        renderer,
        prefilteredEnvironmentCube_->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE)
    );

    // Frame attributes with the lights after the header
    std::vector<std::byte> frameAttribs(faceFrameAttribs_->GetDesc().Size);
    const FaceFrameAttribsHeader header{camera, camera, renderer};
    std::memcpy(frameAttribs.data(), &header, sizeof(header));
    auto* lights = reinterpret_cast<Diligent::HLSL::PBRLightAttribs*>(frameAttribs.data() + sizeof(header));
    for (std::size_t shadingIndex = 0; shadingIndex < shadingLightIndices.size(); ++shadingIndex)
    {
        const std::size_t lightIndex = shadingLightIndices[shadingIndex];
        const glm::vec3& position = pointLightPositions_[lightIndex];
        const glm::vec3& radiance = pointLightRadiances_[lightIndex];
        Diligent::HLSL::PBRLightAttribs& light = lights[shadingIndex];
        light.Type = PBR_Renderer::LIGHT_TYPE_POINT;
        light.PosX = position.x;
        light.PosY = position.y;
        light.PosZ = position.z;
        light.ShadowMapIndex = -1;
        light.IntensityR = radiance.r;
        light.IntensityG = radiance.g;
        light.IntensityB = radiance.b;
    }
    context->UpdateBuffer(
        faceFrameAttribs_,
        0,
        frameAttribs.size(),
        frameAttribs.data(),
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    );

    // Material
    Diligent::HLSL::PBRMaterialBasicAttribs material{};
    material.BaseColorFactor =
        Diligent::SRGBAToLinear(Diligent::float4{geometryColor.r, geometryColor.g, geometryColor.b, geometryColor.a});
    material.NormalScale = 1.f;
    material.SpecularFactorR = 1.f;
    material.SpecularFactorG = 1.f;
    material.SpecularFactorB = 1.f;
    material.Workflow = PBR_Renderer::PBR_WORKFLOW_METALL_ROUGH;
    material.AlphaMode = PBR_Renderer::ALPHA_MODE_OPAQUE;
    material.MetallicFactor = 0.f;
    material.RoughnessFactor = faceRoughness;
    material.OcclusionFactor = 1.f;
    context->UpdateBuffer(
        faceMaterialAttribs_,
        0,
        sizeof(material),
        &material,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    );
}

void MeshPass::drawIndexed(Diligent::IDeviceContext* context, const Pipeline& pipeline, const GpuArray& indices)
{
    if (!indices.buffer) return;

    context->SetIndexBuffer(indices.buffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    setPipeline(context, pipeline);

    Diligent::DrawIndexedAttribs drawAttribs;
    drawAttribs.NumIndices = indices.count;
    drawAttribs.IndexType = Diligent::VT_UINT32;
    context->DrawIndexed(drawAttribs);
}

} // namespace enzo::gfx
