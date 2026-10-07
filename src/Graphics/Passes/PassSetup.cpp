#include "Graphics/Passes/PassSetup.h"
#include "Graphics/RenderTargetFormats.h"
#include "Graphics/Shaders/Common.h"
#include <string>

namespace enzo::gfx {

Diligent::RefCntAutoPtr<Diligent::IShader> createShader(
    Diligent::IRenderDevice* device, const char* source, Diligent::SHADER_TYPE type, const char* entryPoint
)
{
    const std::string fullSource = std::string(shaders::commonSource) + source;

    Diligent::ShaderCreateInfo shaderInfo;
    shaderInfo.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
    shaderInfo.Source = fullSource.c_str();
    shaderInfo.SourceLength = fullSource.size();
    shaderInfo.EntryPoint = entryPoint;
    shaderInfo.Desc.ShaderType = type;
    shaderInfo.Desc.Name = entryPoint;

    Diligent::RefCntAutoPtr<Diligent::IShader> shader;
    device->CreateShader(shaderInfo, &shader);
    return shader;
}

Pipeline createPipeline(
    Diligent::IRenderDevice* device,
    Diligent::GraphicsPipelineStateCreateInfo pipelineInfo,
    Diligent::IBuffer* frameConstants
)
{
    pipelineInfo.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
    Diligent::GraphicsPipelineDesc& graphics = pipelineInfo.GraphicsPipeline;
    graphics.NumRenderTargets = 1;
    graphics.RTVFormats[0] = colorFormat;
    graphics.DSVFormat = depthFormat;
    graphics.SmplDesc.Count = sampleCount;

    Pipeline pipeline;
    device->CreateGraphicsPipelineState(pipelineInfo, &pipeline.state);

    // Binds the frame constants in every stage that declares them.
    for (const Diligent::SHADER_TYPE stage : {Diligent::SHADER_TYPE_VERTEX, Diligent::SHADER_TYPE_PIXEL})
    {
        Diligent::IShaderResourceVariable* variable =
            pipeline.state->GetStaticVariableByName(stage, "FrameConstants");
        if (variable) variable->Set(frameConstants);
    }
    pipeline.state->CreateShaderResourceBinding(&pipeline.resources, true);
    return pipeline;
}

} // namespace enzo::gfx
