#include "Gui/Viewport/ViewportNode.h"
#include "Gui/Viewport/QtGraphicsBridge.h"
#include <QQuickWindow>

namespace enzo::ui {

ViewportNode::ViewportNode(QQuickWindow* window)
    : window_(window), renderer_(gfx::GraphicsDevice::get())
{
    setOwnsTexture(true);
    connect(window, &QQuickWindow::beforeRendering, this, &ViewportNode::render, Qt::DirectConnection);
}

void ViewportNode::sync(const gfx::FrameState& frame, const QRectF& rect)
{
    pendingFrame_ = frame;
    setRect(rect);
}

void ViewportNode::render()
{
    const bool hasPixels = pendingFrame_.pixelSize.x > 0 && pendingFrame_.pixelSize.y > 0;
    const bool upToDate = texture() && pendingFrame_ == renderedFrame_;
    if (!hasPixels || upToDate) return;

    window_->beginExternalCommands();
    const gfx::VulkanImage image = renderer_.render(pendingFrame_);
    window_->endExternalCommands();
    renderedFrame_ = pendingFrame_;

    if (image.image != shownImage_)
    {
        setTexture(QtGraphicsBridge::wrapImage(window_, image));
        shownImage_ = image.image;
    }
}

} // namespace enzo::ui
