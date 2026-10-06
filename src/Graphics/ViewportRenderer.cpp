#include "Graphics/ViewportRenderer.h"
#include "Graphics/DiligentObjects.h"
#include "Graphics/Passes/GridPass.h"

#include <TextureVk.h>

namespace enzo::gfx {

namespace {

constexpr Diligent::TEXTURE_FORMAT colorFormat = Diligent::TEX_FORMAT_RGBA8_UNORM;
constexpr Diligent::Uint8 sampleCount = 4;

Diligent::RefCntAutoPtr<Diligent::ITexture> createColorTexture(
    Diligent::IRenderDevice* device, glm::uvec2 size, Diligent::Uint8 textureSampleCount, const char* name
)
{
    Diligent::TextureDesc textureDesc;
    textureDesc.Name = name;
    textureDesc.Type = Diligent::RESOURCE_DIM_TEX_2D;
    textureDesc.Width = size.x;
    textureDesc.Height = size.y;
    textureDesc.Format = colorFormat;
    textureDesc.SampleCount = textureSampleCount;
    textureDesc.BindFlags = Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE;

    Diligent::RefCntAutoPtr<Diligent::ITexture> texture;
    device->CreateTexture(textureDesc, nullptr, &texture);
    return texture;
}

} // namespace

struct ViewportRenderer::DiligentObjects
{
    GridPass gridPass;
    /// @brief The multisampled target the passes draw into.
    Diligent::RefCntAutoPtr<Diligent::ITexture> multisampleTexture;
    /// @brief The single sample texture the host samples.
    Diligent::RefCntAutoPtr<Diligent::ITexture> colorTexture;
};

ViewportRenderer::ViewportRenderer(GraphicsDevice& device)
    : device_(device),
      diligent_(std::make_unique<DiligentObjects>(
          GridPass(device.getDiligentObjects().device, colorFormat, sampleCount)
      ))
{
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
        diligent_->multisampleTexture =
            createColorTexture(device, frame.pixelSize, sampleCount, "Viewport multisample colour");
        diligent_->colorTexture = createColorTexture(device, frame.pixelSize, 1, "Viewport colour");
    }

    // Draws the frame.
    Diligent::ITextureView* renderTarget =
        diligent_->multisampleTexture->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
    context->SetRenderTargets(
        1, &renderTarget, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    );
    const float* backgroundColor = &frame.backgroundColor.x;
    context->ClearRenderTarget(
        renderTarget, backgroundColor, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    );
    diligent_->gridPass.draw(context, frame);
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
