#include "Gui/Parameters/RampCurveItem.h"
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGVertexColorMaterial>
#include <QVariantMap>
#include <algorithm>
#include <cmath>
#include <vector>

namespace enzo::ui {

namespace {

// How many segments the curve is flattened into across the item width.
constexpr int kCurveSegments = 128;

/// @brief Returns the interpolation a dropdown token names.
prm::Interpolation toInterpolation(const QString& token)
{
    if (token == "constant") return prm::Interpolation::CONSTANT;
    if (token == "bspline") return prm::Interpolation::BSPLINE;
    return prm::Interpolation::LINEAR;
}

/// @brief Writes one vertex with its color premultiplied by alpha.
void setVertex(QSGGeometry::ColoredPoint2D& vertex, QPointF point, const QColor& color, qreal alpha)
{
    const auto premultiply = [&](int channel) { return static_cast<uchar>(channel * alpha); };
    vertex.set(
        point.x(),
        point.y(),
        premultiply(color.red()),
        premultiply(color.green()),
        premultiply(color.blue()),
        static_cast<uchar>(alpha * 255)
    );
}

/// @brief Returns an empty geometry node set up to draw one triangle strip.
QSGGeometryNode* buildStripNode()
{
    auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), 0);
    geometry->setDrawingMode(QSGGeometry::DrawTriangleStrip);

    auto* node = new QSGGeometryNode;
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry);
    node->setMaterial(new QSGVertexColorMaterial);
    node->setFlag(QSGNode::OwnsMaterial);
    return node;
}

/// @brief Fills under the curve, fading out toward the plot floor.
void updateFillNode(
    QSGGeometryNode* node,
    const std::vector<QPointF>& samples,
    qreal floorY,
    const QColor& color
)
{
    QSGGeometry* geometry = node->geometry();
    geometry->allocate(static_cast<int>(samples.size()) * 2);
    QSGGeometry::ColoredPoint2D* vertices = geometry->vertexDataAsColoredPoint2D();
    int vertex = 0;
    for (const QPointF& sample : samples)
    {
        setVertex(vertices[vertex++], sample, color, color.alphaF());
        setVertex(vertices[vertex++], QPointF(sample.x(), floorY), color, 0);
    }
    geometry->markVertexDataDirty();
    node->markDirty(QSGNode::DirtyGeometry);
}

/// @brief Strokes the curve as a strip extruded sideways along its normals.
void updateStrokeNode(
    QSGGeometryNode* node,
    const std::vector<QPointF>& samples,
    const QColor& color,
    qreal width
)
{
    QSGGeometry* geometry = node->geometry();
    geometry->allocate(static_cast<int>(samples.size()) * 2);
    QSGGeometry::ColoredPoint2D* vertices = geometry->vertexDataAsColoredPoint2D();
    int vertex = 0;
    const qreal halfWidth = width / 2;
    for (std::size_t i = 0; i < samples.size(); ++i)
    {
        // The normal at a point is perpendicular to the chord between its neighbors.
        const QPointF ahead = samples[std::min(i + 1, samples.size() - 1)];
        const QPointF behind = samples[i > 0 ? i - 1 : 0];
        const QPointF chord = ahead - behind;
        const qreal chordLength = std::hypot(chord.x(), chord.y());
        const QPointF normal = chordLength > 0
                                   ? QPointF(-chord.y() / chordLength, chord.x() / chordLength)
                                   : QPointF(0, 1);

        for (const qreal side : {-halfWidth, halfWidth})
            setVertex(vertices[vertex++], samples[i] + normal * side, color, color.alphaF());
    }
    geometry->markVertexDataDirty();
    node->markDirty(QSGNode::DirtyGeometry);
}

} // namespace

RampCurveItem::RampCurveItem(QQuickItem* parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    connect(this, &RampCurveItem::styleChanged, this, &QQuickItem::update);
}

qreal RampCurveItem::valueAt(qreal position) const
{
    return static_cast<qreal>(ramp_.sample(static_cast<floatT>(position)));
}

void RampCurveItem::setPoints(const QVariantList& points)
{
    points_ = points;

    std::vector<prm::Ramp::Key> keys;
    keys.reserve(points.size());
    for (const QVariant& point : points)
    {
        const QVariantMap map = point.toMap();
        keys.push_back({
            static_cast<floatT>(map.value("position").toDouble()),
            static_cast<floatT>(map.value("value").toDouble()),
            toInterpolation(map.value("interp").toString()),
        });
    }
    std::sort(keys.begin(), keys.end(), [](const prm::Ramp::Key& a, const prm::Ramp::Key& b) {
        return a.position < b.position;
    });
    ramp_ = prm::Ramp(std::move(keys));

    update();
    Q_EMIT pointsChanged();
}

void RampCurveItem::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    update();
}

QSGNode* RampCurveItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*)
{
    // An empty geometry node gets culled for good, so hand back nothing until
    // there is a curve to draw.
    if (ramp_.empty() || width() <= 0 || height() <= 0)
    {
        delete oldNode;
        return nullptr;
    }

    QSGNode* root = oldNode;
    if (!root)
    {
        root = new QSGNode;
        root->appendChildNode(buildStripNode());
        root->appendChildNode(buildStripNode());
    }

    // Flatten the curve into evenly spaced samples across the plot.
    std::vector<QPointF> samples(kCurveSegments + 1);
    for (int i = 0; i <= kCurveSegments; ++i)
    {
        const qreal position = static_cast<qreal>(i) / kCurveSegments;
        const qreal value = std::clamp<qreal>(valueAt(position), 0, 1);
        samples[i] = QPointF(position * width(), (1 - value) * height());
    }

    updateFillNode(
        static_cast<QSGGeometryNode*>(root->firstChild()),
        samples,
        height(),
        fillColor_
    );
    updateStrokeNode(
        static_cast<QSGGeometryNode*>(root->lastChild()),
        samples,
        curveColor_,
        curveWidth_
    );
    return root;
}

} // namespace enzo::ui
