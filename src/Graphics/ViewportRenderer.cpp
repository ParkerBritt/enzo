#include "Graphics/ViewportRenderer.h"
#include "Graphics/DiligentObjects.h"

#include <TextureVk.h>

namespace enzo::gfx {

struct ViewportRenderer::DiligentObjects
{
    Diligent::RefCntAutoPtr<Diligent::ITexture> colorTexture;
};

ViewportRenderer::ViewportRenderer(GraphicsDevice& device)
    : device_(device), diligent_(std::make_unique<DiligentObjects>())
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
        Diligent::TextureDesc textureDesc;
        textureDesc.Name = "Viewport colour";
        textureDesc.Type = Diligent::RESOURCE_DIM_TEX_2D;
        textureDesc.Width = frame.pixelSize.x;
        textureDesc.Height = frame.pixelSize.y;
        textureDesc.Format = Diligent::TEX_FORMAT_RGBA8_UNORM;
        textureDesc.BindFlags = Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE;
        diligent_->colorTexture.Release();
        device->CreateTexture(textureDesc, nullptr, &diligent_->colorTexture);
    }

    Diligent::ITextureView* renderTarget =
        diligent_->colorTexture->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
    context->SetRenderTargets(
        1, &renderTarget, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    );
    const float* backgroundColor = &frame.backgroundColor.x;
    context->ClearRenderTarget(
        renderTarget, backgroundColor, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    );
    context->SetRenderTargets(0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE);

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
