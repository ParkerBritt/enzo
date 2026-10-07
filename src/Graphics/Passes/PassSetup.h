#pragma once
#include <Buffer.h>
#include <DeviceContext.h>
#include <PipelineState.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>
#include <Shader.h>
#include <ShaderResourceBinding.h>
#include <initializer_list>
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

/// @brief Binds the vertex buffers to slots 0 onward in order and unbinds the rest.
void setVertexBuffers(Diligent::IDeviceContext* context, std::initializer_list<Diligent::IBuffer*> vertexBuffers);

/// @brief Binds the pipeline and its frame constants for the next draw.
void setPipeline(Diligent::IDeviceContext* context, const Pipeline& pipeline);

/// @brief An unchanging GPU buffer and the number of values it holds.
struct GpuArray
{
    /// @brief The buffer, null when it holds no values.
    Diligent::RefCntAutoPtr<Diligent::IBuffer> buffer;
    Diligent::Uint32 count = 0;
};

/// @brief Returns a GPU array holding the values.
template <typename Value>
GpuArray createGpuArray(
    Diligent::IRenderDevice* device,
    const char* name,
    Diligent::BIND_FLAGS bindFlags,
    const std::vector<Value>& values
)
{
    GpuArray array;
    array.count = Diligent::Uint32(values.size());
    if (values.empty()) return array;

    Diligent::BufferDesc bufferDesc;
    bufferDesc.Name = name;
    bufferDesc.Usage = Diligent::USAGE_IMMUTABLE;
    bufferDesc.BindFlags = bindFlags;
    bufferDesc.Size = values.size() * sizeof(Value);
    Diligent::BufferData bufferData{values.data(), bufferDesc.Size};

    device->CreateBuffer(bufferDesc, &bufferData, &array.buffer);
    return array;
}

} // namespace enzo::gfx
