#pragma once
#include <Buffer.h>
#include <PipelineState.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>
#include <Shader.h>
#include <ShaderResourceBinding.h>
#include <vector>

namespace enzo::gfx {

/// @brief A pipeline and the resource binding that feeds it the frame constants.
struct Pipeline
{
    Diligent::RefCntAutoPtr<Diligent::IPipelineState> state;
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> resources;
};

/// @brief Returns a shader compiled from HLSL source with `Common.hlsli` placed before it.
///
/// @param source the HLSL source of the pass.
/// @param entryPoint the function the shader starts in.
Diligent::RefCntAutoPtr<Diligent::IShader> createShader(
    Diligent::IRenderDevice* device, const char* source, Diligent::SHADER_TYPE type, const char* entryPoint
);

/// @brief Returns a pipeline that draws into the viewport targets and reads the frame constants.
///
/// @param pipelineInfo the settings of the pass, which leave the target formats and sample count unset.
/// @param frameConstants the buffer the renderer fills with the frame constants.
Pipeline createPipeline(
    Diligent::IRenderDevice* device,
    Diligent::GraphicsPipelineStateCreateInfo pipelineInfo,
    Diligent::IBuffer* frameConstants
);

/// @brief Returns an unchanging GPU buffer holding the values, or null when there are none.
template <typename Value>
Diligent::RefCntAutoPtr<Diligent::IBuffer> createImmutableBuffer(
    Diligent::IRenderDevice* device,
    const char* name,
    Diligent::BIND_FLAGS bindFlags,
    const std::vector<Value>& values
)
{
    if (values.empty()) return {};

    Diligent::BufferDesc bufferDesc;
    bufferDesc.Name = name;
    bufferDesc.Usage = Diligent::USAGE_IMMUTABLE;
    bufferDesc.BindFlags = bindFlags;
    bufferDesc.Size = values.size() * sizeof(Value);
    Diligent::BufferData bufferData{values.data(), bufferDesc.Size};

    Diligent::RefCntAutoPtr<Diligent::IBuffer> buffer;
    device->CreateBuffer(bufferDesc, &bufferData, &buffer);
    return buffer;
}

} // namespace enzo::gfx
