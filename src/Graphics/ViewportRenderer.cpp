#include "Graphics/ViewportRenderer.h"
#include "Graphics/DiligentObjects.h"
#include "Graphics/Passes/CameraPrimPass.h"
#include "Graphics/Passes/GridPass.h"
#include "Graphics/Passes/MeshPass.h"
#include "Graphics/Passes/PointPass.h"
#include "Graphics/RenderTargetFormats.h"

#include <TextureVk.h>
#include <algorithm>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec4.hpp>

namespace enzo::gfx {

namespace {

/// @brief The values every pass reads, laid out as `FrameConstants` in `Common.hlsli`.
struct FrameConstants
{
    glm::mat4 viewProjection;
    glm::vec4 cameraPosition;
    glm::vec4 geometryColor;
    glm::vec2 viewportPixelSize;
    float pixelRatio;
    float padding;
};

Diligent::RefCntAutoPtr<Diligent::IBuffer> createFrameConstantsBuffer(Diligent::IRenderDevice* device)
{
    Diligent::BufferDesc bufferDesc;
    bufferDesc.Name = "Frame constants";
    bufferDesc.Usage = Diligent::USAGE_DEFAULT;
    bufferDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
    bufferDesc.Size = sizeof(FrameConstants);

    Diligent::RefCntAutoPtr<Diligent::IBuffer> buffer;
    device->CreateBuffer(bufferDesc, nullptr, &buffer);
    return buffer;
}

FrameConstants getFrameConstants(const FrameState& frame)
{
    glm::mat4 view = frame.camera.getViewMatrix();
    glm::vec4 cameraPosition{frame.camera.getPosition(), 1.f};
    if (frame.viewCameraIndex.has_value())
    {
        const glm::mat4& viewCameraTransform = frame.geometry->cameraTransforms[*frame.viewCameraIndex];
        view = glm::inverse(viewCameraTransform);
        cameraPosition = viewCameraTransform[3];
    }

    const float aspect = float(frame.pixelSize.x) / float(std::max(frame.pixelSize.y, 1u));
    const glm::mat4 viewProjection = frame.camera.getProjectionMatrix(aspect) * view;
    const glm::vec2 viewportPixelSize{frame.pixelSize};
    return FrameConstants{
        viewProjection, cameraPosition, frame.geometryColor, viewportPixelSize, frame.pixelRatio, 0.f
    };
}

Diligent::RefCntAutoPtr<Diligent::ITexture> createTargetTexture(
    Diligent::IRenderDevice* device,
    glm::uvec2 size,
    Diligent::TEXTURE_FORMAT format,
    Diligent::Uint8 textureSampleCount,
    Diligent::BIND_FLAGS bindFlags,
    const char* name
)
{
    Diligent::TextureDesc textureDesc;
    textureDesc.Name = name;
    textureDesc.Type = Diligent::RESOURCE_DIM_TEX_2D;
    textureDesc.Width = size.x;
    textureDesc.Height = size.y;
    textureDesc.Format = format;
    textureDesc.SampleCount = textureSampleCount;
    textureDesc.BindFlags = bindFlags;

    Diligent::RefCntAutoPtr<Diligent::ITexture> texture;
    device->CreateTexture(textureDesc, nullptr, &texture);
    return texture;
}

} // namespace

struct ViewportRenderer::DiligentObjects
{
    Diligent::RefCntAutoPtr<Diligent::IBuffer> frameConstants;
    GridPass gridPass;
    MeshPass meshPass;
    PointPass pointPass;
    CameraPrimPass cameraPrimPass;
    /// @brief The multisampled target the passes draw into.
    Diligent::RefCntAutoPtr<Diligent::ITexture> multisampleTexture;
    /// @brief The multisampled depth the passes test against.
    Diligent::RefCntAutoPtr<Diligent::ITexture> depthTexture;
    /// @brief The single sample texture the host samples.
    Diligent::RefCntAutoPtr<Diligent::ITexture> colorTexture;
};

ViewportRenderer::ViewportRenderer(GraphicsDevice& device) : device_(device)
{
    Diligent::IRenderDevice* renderDevice = device.getDiligentObjects().device;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> frameConstants = createFrameConstantsBuffer(renderDevice);
    diligent_ = std::make_unique<DiligentObjects>(DiligentObjects{
        frameConstants,
        GridPass(renderDevice, frameConstants),
        MeshPass(renderDevice, frameConstants),
        PointPass(renderDevice, frameConstants),
        CameraPrimPass(renderDevice, frameConstants),
    });
}

ViewportRenderer::~ViewportRenderer() = default;

VulkanImage ViewportRenderer::render(const FrameState& frame)
{
    GraphicsDevice::DiligentObjects& deviceObjects = device_.getDiligentObjects();
    Diligent::IRenderDevice* device = deviceObjects.device;
    Diligent::IDeviceContext* context = deviceObjects.context;

    const bool hasTexture = diligent_->colorTexture != nullptr;
    const bool sizeChanged = hasTexture &&
                             (diligent_->colorTexture->GetDesc().Width != frame.pixelSize.x ||
                              diligent_->colorTexture->GetDesc().Height != frame.pixelSize.y);
    if (!hasTexture || sizeChanged)
    {
        diligent_->multisampleTexture = createTargetTexture(
            device,
            frame.pixelSize,
            colorFormat,
            sampleCount,
            Diligent::BIND_RENDER_TARGET,
            "Viewport multisample colour"
        );
        diligent_->depthTexture = createTargetTexture(
            device, frame.pixelSize, depthFormat, sampleCount, Diligent::BIND_DEPTH_STENCIL, "Viewport depth"
        );
        diligent_->colorTexture = createTargetTexture(
            device,
            frame.pixelSize,
            colorFormat,
            1,
            Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE,
            "Viewport colour"
        );
    }

    if (frame.geometry != uploadedGeometry_)
    {
        const DisplayGeometry noGeometry;
        const DisplayGeometry& geometry = frame.geometry ? *frame.geometry : noGeometry;
        diligent_->meshPass.upload(device, geometry);
        diligent_->pointPass.upload(device, geometry);
        diligent_->cameraPrimPass.upload(device, geometry);
        uploadedGeometry_ = frame.geometry;
    }

    const FrameConstants frameConstants = getFrameConstants(frame);
    context->UpdateBuffer(
        diligent_->frameConstants,
        0,
        sizeof(frameConstants),
        &frameConstants,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    );

    // Draws the frame.
    Diligent::ITextureView* renderTarget =
        diligent_->multisampleTexture->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
    Diligent::ITextureView* depthTarget =
        diligent_->depthTexture->GetDefaultView(Diligent::TEXTURE_VIEW_DEPTH_STENCIL);
    context->SetRenderTargets(
        1, &renderTarget, depthTarget, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    );
    const float* backgroundColor = &frame.backgroundColor.x;
    context->ClearRenderTarget(
        renderTarget, backgroundColor, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    );
    context->ClearDepthStencil(
        depthTarget, Diligent::CLEAR_DEPTH_FLAG, 1.f, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    );
    diligent_->gridPass.draw(context);
    diligent_->meshPass.draw(context, frame.wireframeVisible);
    diligent_->pointPass.draw(context, frame.pointsVisible);
    diligent_->cameraPrimPass.draw(context, frame.viewCameraIndex);
    context->SetRenderTargets(0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE);

    // Resolves the samples into the shown texture.
    Diligent::ResolveTextureSubresourceAttribs resolveAttribs;
    resolveAttribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    resolveAttribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    context->ResolveTextureSubresource(
        diligent_->multisampleTexture, diligent_->colorTexture, resolveAttribs
    );

    // Leaves the image ready for the host to sample.
    Diligent::StateTransitionDesc toShaderResource{
        diligent_->colorTexture,
        Diligent::RESOURCE_STATE_UNKNOWN,
        Diligent::RESOURCE_STATE_SHADER_RESOURCE,
        Diligent::STATE_TRANSITION_FLAG_UPDATE_STATE
    };
    context->TransitionResourceStates(1, &toShaderResource);

    // Ends the frame here, since no swap chain does it.
    context->Flush();
    context->FinishFrame();
    device->ReleaseStaleResources();

    Diligent::RefCntAutoPtr<Diligent::ITextureVk> vulkanTexture{
        diligent_->colorTexture, Diligent::IID_TextureVk
    };
    VulkanImage image;
    image.image = vulkanTexture->GetVkImage();
    image.layout = vulkanTexture->GetLayout();
    image.size = frame.pixelSize;
    return image;
}

} // namespace enzo::gfx
