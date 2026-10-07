#include "Gui/Viewport/ViewportItem.h"
#include "Gui/Viewport/ViewportNode.h"
#include <QQuickWindow>

namespace enzo::ui {

namespace {

glm::vec4 toGlm(const QColor& colour)
{
    return glm::vec4(colour.redF(), colour.greenF(), colour.blueF(), colour.alphaF());
}

} // namespace

ViewportItem::ViewportItem(QQuickItem* parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents);
}

void ViewportItem::setViewModel(ViewportViewModel* viewModel)
{
    if (viewModel_ == viewModel) return;
    if (viewModel_) disconnect(viewModel_, nullptr, this, nullptr);
    viewModel_ = viewModel;

    if (viewModel_)
        connect(viewModel_, &ViewportViewModel::geometryChanged, this, &QQuickItem::update);
    update();
    Q_EMIT viewModelChanged();
}

void ViewportItem::setBackgroundColor(const QColor& colour)
{
    if (backgroundColor_ == colour) return;
    backgroundColor_ = colour;
    update();
    Q_EMIT backgroundColorChanged();
}

void ViewportItem::setGeometryColor(const QColor& colour)
{
    if (geometryColor_ == colour) return;
    geometryColor_ = colour;
    update();
    Q_EMIT geometryColorChanged();
}

void ViewportItem::toggleWireframe()
{
    wireframeVisible_ = !wireframeVisible_;
    update();
}

void ViewportItem::orbit(qreal dx, qreal dy)
{
    camera_.orbit(glm::vec2(dx, dy));
    update();
}

void ViewportItem::pan(qreal dx, qreal dy)
{
    camera_.pan(glm::vec2(dx, dy));
    update();
}

void ViewportItem::dolly(qreal amount)
{
    camera_.dolly(float(amount));
    update();
}

QSGNode* ViewportItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*)
{
    auto* node = static_cast<ViewportNode*>(oldNode);
    if (!node) node = new ViewportNode(window());

    const QSizeF pixelSize = size() * window()->effectiveDevicePixelRatio();
    gfx::FrameState frame;
    frame.pixelSize = glm::uvec2(pixelSize.width(), pixelSize.height());
    frame.backgroundColor = toGlm(backgroundColor_);
    frame.geometryColor = toGlm(geometryColor_);
    frame.wireframeVisible = wireframeVisible_;
    frame.camera = camera_;
    frame.geometry = viewModel_ ? viewModel_->getGeometry() : nullptr;
    node->sync(frame, boundingRect());
    return node;
}

void ViewportItem::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    update();
}

} // namespace enzo::ui
