#include "Gui/Network/NodeLinkLayer.h"

#include <QHash>
#include <QSGFlatColorMaterial>
#include <QSGGeometry>
#include <QSGGeometryNode>

namespace enzo::ui {

namespace {

// How many straight segments approximate each bezier link.
constexpr int kSegmentsPerLink = 24;

// Two vertices per segment since the geometry draws disconnected line pairs.
constexpr int kVerticesPerLink = kSegmentsPerLink * 2;

/// @brief Returns the role number a model exposes under @p name, or -1 when absent.
int findRole(const QHash<int, QByteArray>& roles, const QByteArray& name)
{
    for (auto role = roles.cbegin(); role != roles.cend(); ++role)
        if (role.value() == name) return role.key();
    return -1;
}

/// @brief Returns a point on the cubic bezier through the four control points.
QPointF cubicBezier(
    const QPointF& start,
    const QPointF& control1,
    const QPointF& control2,
    const QPointF& end,
    qreal t
)
{
    const qreal inv = 1 - t;
    const qreal a = inv * inv * inv;
    const qreal b = 3 * inv * inv * t;
    const qreal c = 3 * inv * t * t;
    const qreal d = t * t * t;
    return a * start + b * control1 + c * control2 + d * end;
}

/// @brief Returns the points sampled along one link's bezier from output to input.
std::vector<QPointF> samplePolyline(const NodeLinkLayer::Link& link)
{
    // Pull the controls vertically so the curve leaves and enters straight.
    const qreal slack = std::max<qreal>(36, std::abs(link.input.y() - link.output.y()) * 0.5);
    const QPointF control1(link.output.x(), link.output.y() + slack);
    const QPointF control2(link.input.x(), link.input.y() - slack);

    std::vector<QPointF> points;
    points.reserve(kSegmentsPerLink + 1);
    points.push_back(link.output);
    for (int segment = 1; segment <= kSegmentsPerLink; ++segment)
    {
        const qreal t = static_cast<qreal>(segment) / kSegmentsPerLink;
        points.push_back(cubicBezier(link.output, control1, control2, link.input, t));
    }
    return points;
}

/// @brief Writes one link's bezier into the geometry as line pairs from @p vertex.
void tessellateLink(QSGGeometry::Point2D* vertices, int& vertex, const NodeLinkLayer::Link& link)
{
    const std::vector<QPointF> points = samplePolyline(link);
    for (std::size_t i = 1; i < points.size(); ++i)
    {
        vertices[vertex++].set(points[i - 1].x(), points[i - 1].y());
        vertices[vertex++].set(points[i].x(), points[i].y());
    }
}

/// @brief Returns the distance from a point to the nearest position on one segment.
qreal distanceToSegment(
    const QPointF& point,
    const QPointF& segmentStart,
    const QPointF& segmentEnd
)
{
    const QPointF along = segmentEnd - segmentStart;
    const qreal lengthSquared = along.x() * along.x() + along.y() * along.y();
    qreal t = 0;
    if (lengthSquared > 0)
    {
        const QPointF toPoint = point - segmentStart;
        t = std::clamp(
            (toPoint.x() * along.x() + toPoint.y() * along.y()) / lengthSquared,
            0.0,
            1.0
        );
    }
    const QPointF offset = point - (segmentStart + t * along);
    return std::hypot(offset.x(), offset.y());
}

/// @brief Returns which side of line @p a to @p b the point @p c lies on.
qreal orientation(const QPointF& a, const QPointF& b, const QPointF& c)
{
    return (b.x() - a.x()) * (c.y() - a.y()) - (b.y() - a.y()) * (c.x() - a.x());
}

/// @brief Whether segment @p p1 to @p p2 crosses segment @p q1 to @p q2.
bool segmentsIntersect(const QPointF& p1, const QPointF& p2, const QPointF& q1, const QPointF& q2)
{
    const qreal d1 = orientation(q1, q2, p1);
    const qreal d2 = orientation(q1, q2, p2);
    const qreal d3 = orientation(p1, p2, q1);
    const qreal d4 = orientation(p1, p2, q2);
    return ((d1 > 0) != (d2 > 0)) && ((d3 > 0) != (d4 > 0));
}

} // namespace

NodeLinkLayer::NodeLinkLayer(QQuickItem* parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

NodeListModel* NodeLinkLayer::nodes() const { return nodes_; }

QAbstractListModel* NodeLinkLayer::links() const { return links_; }

void NodeLinkLayer::setNodes(NodeListModel* model)
{
    if (nodes_ == model) return;
    if (nodes_) nodes_->disconnect(this);

    nodes_ = model;
    if (model) connectForRepaint(model);

    update();
    Q_EMIT nodesChanged();
}

void NodeLinkLayer::setLinks(QAbstractListModel* model)
{
    if (links_ == model) return;
    if (links_) links_->disconnect(this);

    links_ = model;
    if (model) connectForRepaint(model);

    update();
    Q_EMIT linksChanged();
}

bool NodeLinkLayer::floatingActive() const { return floatingActive_; }

void NodeLinkLayer::setFloatingActive(bool active)
{
    if (floatingActive_ == active) return;
    floatingActive_ = active;
    Q_EMIT floatingChanged();
    update();
}

QPointF NodeLinkLayer::floatingOutput() const { return floatingOutput_; }

void NodeLinkLayer::setFloatingOutput(QPointF point)
{
    if (floatingOutput_ == point) return;
    floatingOutput_ = point;
    Q_EMIT floatingChanged();
    update();
}

QPointF NodeLinkLayer::floatingInput() const { return floatingInput_; }

void NodeLinkLayer::setFloatingInput(QPointF point)
{
    if (floatingInput_ == point) return;
    floatingInput_ = point;
    Q_EMIT floatingChanged();
    update();
}

void NodeLinkLayer::connectForRepaint(QAbstractItemModel* model)
{
    // Any row or value change means the links must redraw, including the per-frame
    // x and y updates of a live node drag.
    auto repaint = [this] { update(); };
    connect(model, &QAbstractItemModel::dataChanged, this, repaint);
    connect(model, &QAbstractItemModel::rowsInserted, this, repaint);
    connect(model, &QAbstractItemModel::rowsRemoved, this, repaint);
    connect(model, &QAbstractItemModel::modelReset, this, repaint);
}

std::vector<NodeLinkLayer::Link> NodeLinkLayer::collectLinks() const
{
    std::vector<Link> links;
    if (!nodes_ || !links_) return links;

    const QHash<int, QByteArray> roles = links_->roleNames();
    const int sourceOpRole = findRole(roles, "sourceOp");
    const int sourceOutputRole = findRole(roles, "sourceOutput");
    const int targetOpRole = findRole(roles, "targetOp");
    const int targetInputRole = findRole(roles, "targetInput");
    if (sourceOpRole < 0 || sourceOutputRole < 0 || targetOpRole < 0 || targetInputRole < 0)
        return links;

    const int rows = links_->rowCount();
    for (int row = 0; row < rows; ++row)
    {
        const QModelIndex index = links_->index(row, 0);
        const quint64 sourceOp = links_->data(index, sourceOpRole).toULongLong();
        const quint64 targetOp = links_->data(index, targetOpRole).toULongLong();
        const int sourceOutput = links_->data(index, sourceOutputRole).toInt();
        const int targetInput = links_->data(index, targetInputRole).toInt();

        // The curve leaves the source's output and enters the target's input. A link
        // whose nodes are not both in the snapshot yet has no points to draw.
        const std::optional<QPointF> output = nodes_->getPortPosition(sourceOp, sourceOutput, true);
        const std::optional<QPointF> input = nodes_->getPortPosition(targetOp, targetInput, false);
        if (!output || !input) continue;

        links.push_back(Link{*output, *input, row});
    }
    return links;
}

int NodeLinkLayer::linkAt(QPointF canvasPoint, qreal radius) const
{
    int nearestLink = -1;
    qreal nearestDistance = radius;
    for (const Link& link : collectLinks())
    {
        const std::vector<QPointF> points = samplePolyline(link);
        for (std::size_t i = 1; i < points.size(); ++i)
        {
            const qreal distance = distanceToSegment(canvasPoint, points[i - 1], points[i]);
            if (distance < nearestDistance)
            {
                nearestDistance = distance;
                nearestLink = link.linkIndex;
            }
        }
    }
    return nearestLink;
}

int NodeLinkLayer::linkCrossing(QPointF from, QPointF to) const
{
    for (const Link& link : collectLinks())
    {
        const std::vector<QPointF> points = samplePolyline(link);
        for (std::size_t i = 1; i < points.size(); ++i)
            if (segmentsIntersect(from, to, points[i - 1], points[i])) return link.linkIndex;
    }
    return -1;
}

QSGNode* NodeLinkLayer::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*)
{
    std::vector<Link> links = collectLinks();

    // The dragged link is drawn alongside the committed ones while a port drag runs.
    if (floatingActive_) links.push_back(Link{floatingOutput_, floatingInput_});

    const int vertexCount = static_cast<int>(links.size()) * kVerticesPerLink;

    // With nothing to draw, drop the node entirely. An empty geometry node gets
    // culled and never revived, so a layer that starts empty would never show links.
    if (vertexCount == 0)
    {
        delete oldNode;
        return nullptr;
    }

    // Build the single geometry node on first paint, reuse it after.
    auto* node = static_cast<QSGGeometryNode*>(oldNode);
    if (!node)
    {
        auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount);
        geometry->setDrawingMode(QSGGeometry::DrawLines);
        geometry->setLineWidth(2);

        auto* material = new QSGFlatColorMaterial;
        material->setColor(linkColor_);

        node = new QSGGeometryNode;
        node->setGeometry(geometry);
        node->setFlag(QSGNode::OwnsGeometry);
        node->setMaterial(material);
        node->setFlag(QSGNode::OwnsMaterial);
    }
    else
    {
        node->geometry()->allocate(vertexCount);

        auto* material = static_cast<QSGFlatColorMaterial*>(node->material());
        if (material->color() != linkColor_)
        {
            material->setColor(linkColor_);
            node->markDirty(QSGNode::DirtyMaterial);
        }
    }

    // Lay every link's bezier into the geometry back to back.
    QSGGeometry::Point2D* vertices = node->geometry()->vertexDataAsPoint2D();
    int vertex = 0;
    for (const Link& link : links)
        tessellateLink(vertices, vertex, link);

    node->geometry()->markVertexDataDirty();
    node->markDirty(QSGNode::DirtyGeometry);
    return node;
}

} // namespace enzo::ui
