#include "Gui/Viewport/ViewportItem.h"
#include "Gui/Viewport/ViewportNode.h"
#include <QQuickWindow>
#include <algorithm>

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
    {
        connect(viewModel_, &ViewportViewModel::geometryChanged, this, &ViewportItem::dropMissingViewCamera);
        connect(viewModel_, &ViewportViewModel::geometryChanged, this, &QQuickItem::update);
    }
    dropMissingViewCamera();
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

void ViewportItem::setWireframeVisible(bool visible)
{
    if (wireframeVisible_ == visible) return;
    wireframeVisible_ = visible;
    update();
    Q_EMIT wireframeVisibleChanged();
}

void ViewportItem::setPointsVisible(bool visible)
{
    if (pointsVisible_ == visible) return;
    pointsVisible_ = visible;
    update();
    Q_EMIT pointsVisibleChanged();
}

void ViewportItem::setViewCameraPath(const QString& path)
{
    if (viewCameraPath_ == path) return;
    viewCameraPath_ = path;
    update();
    Q_EMIT viewCameraPathChanged();
}

void ViewportItem::orbit(qreal dx, qreal dy)
{
    detachFromViewCamera();
    camera_.orbit(glm::vec2(dx, dy));
    update();
}

void ViewportItem::pan(qreal dx, qreal dy)
{
    detachFromViewCamera();
    camera_.pan(glm::vec2(dx, dy));
    update();
}

void ViewportItem::dolly(qreal amount)
{
    detachFromViewCamera();
    camera_.dolly(float(amount));
    update();
}

std::optional<std::size_t> ViewportItem::getViewCameraIndex() const
{
    if (viewCameraPath_.isEmpty() || !viewModel_) return std::nullopt;
    const std::shared_ptr<const gfx::DisplayGeometry> geometry = viewModel_->getGeometry();
    if (!geometry) return std::nullopt;

    const std::vector<std::string>& cameraPaths = geometry->cameraPaths;
    const auto pathIt = std::ranges::find(cameraPaths, viewCameraPath_.toStdString());
    if (pathIt == cameraPaths.end()) return std::nullopt;
    return std::size_t(pathIt - cameraPaths.begin());
}

void ViewportItem::detachFromViewCamera()
{
    const std::optional<std::size_t> viewCameraIndex = getViewCameraIndex();
    if (!viewCameraIndex.has_value()) return;

    camera_.placeAt(viewModel_->getGeometry()->cameraTransforms[*viewCameraIndex]);
    setViewCameraPath(QString());
}

void ViewportItem::dropMissingViewCamera()
{
    if (!getViewCameraIndex().has_value()) setViewCameraPath(QString());
}

QSGNode* ViewportItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*)
{
    auto* node = static_cast<ViewportNode*>(oldNode);
    if (!node) node = new ViewportNode(window());

    const qreal pixelRatio = window()->effectiveDevicePixelRatio();
    const QSizeF pixelSize = size() * pixelRatio;
    gfx::FrameState frame;
    frame.pixelSize = glm::uvec2(pixelSize.width(), pixelSize.height());
    frame.pixelRatio = float(pixelRatio);
    frame.backgroundColor = toGlm(backgroundColor_);
    frame.geometryColor = toGlm(geometryColor_);
    frame.wireframeVisible = wireframeVisible_;
    frame.pointsVisible = pointsVisible_;
    frame.camera = camera_;
    frame.geometry = viewModel_ ? viewModel_->getGeometry() : nullptr;
    frame.viewCameraIndex = getViewCameraIndex();
    node->sync(frame, boundingRect());
    return node;
}

void ViewportItem::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    update();
}

} // namespace enzo::ui
