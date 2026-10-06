#include "Gui/Viewport/ViewportItem.h"
#include "Gui/Viewport/ViewportNode.h"
#include <QQuickWindow>

namespace enzo::ui {

ViewportItem::ViewportItem(QQuickItem* parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents);
}

void ViewportItem::setBackgroundColor(const QColor& colour)
{
    if (backgroundColor_ == colour) return;
    backgroundColor_ = colour;
    update();
    Q_EMIT backgroundColorChanged();
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
    frame.backgroundColor = glm::vec4(
        backgroundColor_.redF(),
        backgroundColor_.greenF(),
        backgroundColor_.blueF(),
        backgroundColor_.alphaF()
    );
    frame.camera = camera_;
    node->sync(frame, boundingRect());
    return node;
}

void ViewportItem::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    update();
}

} // namespace enzo::ui
