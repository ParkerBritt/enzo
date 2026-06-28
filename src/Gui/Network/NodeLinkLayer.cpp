#include "Gui/Network/NodeLinkLayer.h"

#include <QLineF>
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

/// @brief Returns the x of slot @p slotIndex spread evenly across an edge of @p width.
///
/// e.g. one slot lands at the middle, two land at the third marks.
qreal getSlotX(int slotIndex, int slotCount, qreal width)
{
    return width * (slotIndex + 1) / (slotCount + 1);
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

/// @brief Writes one link's bezier into the geometry as line pairs from @p vertex.
void tessellateLink(QSGGeometry::Point2D* vertices, int& vertex, const NodeLinkLayer::Link& link)
{
    // Pull the controls vertically so the curve leaves and enters straight.
    const qreal slack = std::max<qreal>(36, std::abs(link.input.y() - link.output.y()) * 0.5);
    const QPointF control1(link.output.x(), link.output.y() + slack);
    const QPointF control2(link.input.x(), link.input.y() - slack);

    QPointF previous = link.output;
    for (int segment = 1; segment <= kSegmentsPerLink; ++segment)
    {
        const qreal t = static_cast<qreal>(segment) / kSegmentsPerLink;
        const QPointF current = cubicBezier(link.output, control1, control2, link.input, t);
        vertices[vertex++].set(previous.x(), previous.y());
        vertices[vertex++].set(current.x(), current.y());
        previous = current;
    }
}

} // namespace

NodeLinkLayer::NodeLinkLayer(QQuickItem* parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

QAbstractListModel* NodeLinkLayer::nodes() const { return nodes_; }

QAbstractListModel* NodeLinkLayer::links() const { return links_; }

void NodeLinkLayer::setNodes(QAbstractListModel* model)
{
    if (nodes_ == model) return;
    subscribe(nodes_, model);
    Q_EMIT nodesChanged();
}

void NodeLinkLayer::setLinks(QAbstractListModel* model)
{
    if (links_ == model) return;
    subscribe(links_, model);
    Q_EMIT linksChanged();
}

QVariantMap NodeLinkLayer::portAt(QPointF canvasPoint, bool wantOutput) const
{
    const QHash<quint64, NodeGeometry> geometries = nodeGeometries();

    // A port no further than this from the cursor counts as the drop target, so a
    // link snaps to the closest port rather than needing a precise hit.
    constexpr qreal kPickRadius = 30;

    QVariantMap nearest;
    qreal nearestDistance = kPickRadius;
    for (auto it = geometries.cbegin(); it != geometries.cend(); ++it)
    {
        const NodeGeometry& geometry = it.value();
        const int slotCount = wantOutput ? geometry.outputSlotCount : geometry.inputSlotCount;

        // Inputs sit on the top edge, outputs on the bottom edge.
        const qreal edgeY = geometry.position.y() + (wantOutput ? nodeHeight_ : 0);
        for (int slot = 0; slot < slotCount; ++slot)
        {
            const QPointF port(geometry.position.x() + getSlotX(slot, slotCount, nodeWidth_), edgeY);
            const qreal distance = QLineF(port, canvasPoint).length();
            if (distance < nearestDistance)
            {
                nearestDistance = distance;
                nearest = QVariantMap{
                    {"opId", QVariant::fromValue(it.key())},
                    {"slot", slot},
                    {"x", port.x()},
                    {"y", port.y()}
                };
            }
        }
    }
    return nearest;
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

void NodeLinkLayer::subscribe(QAbstractListModel*& slot, QAbstractListModel* model)
{
    if (slot) slot->disconnect(this);
    slot = model;

    if (model)
    {
        // Any row or value change in either model means the links must redraw,
        // including the per-frame x and y updates of a live node drag.
        auto repaint = [this] { update(); };
        connect(model, &QAbstractItemModel::dataChanged, this, repaint);
        connect(model, &QAbstractItemModel::rowsInserted, this, repaint);
        connect(model, &QAbstractItemModel::rowsRemoved, this, repaint);
        connect(model, &QAbstractItemModel::modelReset, this, repaint);
    }
    update();
}

QHash<quint64, NodeLinkLayer::NodeGeometry> NodeLinkLayer::nodeGeometries() const
{
    QHash<quint64, NodeGeometry> geometries;
    if (!nodes_) return geometries;

    const QHash<int, QByteArray> roles = nodes_->roleNames();
    const int opIdRole = findRole(roles, "opId");
    const int xRole = findRole(roles, "x");
    const int yRole = findRole(roles, "y");
    const int inputRole = findRole(roles, "inputSlotCount");
    const int outputRole = findRole(roles, "outputSlotCount");
    if (opIdRole < 0 || xRole < 0 || yRole < 0 || inputRole < 0 || outputRole < 0)
        return geometries;

    const int rows = nodes_->rowCount();
    for (int row = 0; row < rows; ++row)
    {
        const QModelIndex index = nodes_->index(row, 0);
        const quint64 opId = nodes_->data(index, opIdRole).toULongLong();

        NodeGeometry geometry;
        geometry.position =
            QPointF(nodes_->data(index, xRole).toReal(), nodes_->data(index, yRole).toReal());
        geometry.inputSlotCount = nodes_->data(index, inputRole).toInt();
        geometry.outputSlotCount = nodes_->data(index, outputRole).toInt();
        geometries.insert(opId, geometry);
    }
    return geometries;
}

std::vector<NodeLinkLayer::Link> NodeLinkLayer::collectLinks() const
{
    std::vector<Link> links;
    if (!nodes_ || !links_) return links;

    const QHash<quint64, NodeGeometry> geometries = nodeGeometries();
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
        if (!geometries.contains(sourceOp) || !geometries.contains(targetOp)) continue;

        const NodeGeometry source = geometries.value(sourceOp);
        const NodeGeometry target = geometries.value(targetOp);
        const int sourceOutput = links_->data(index, sourceOutputRole).toInt();
        const int targetInput = links_->data(index, targetInputRole).toInt();

        // The curve leaves the source's output slot and enters the target's input slot.
        Link link;
        link.output = QPointF(
            source.position.x() + getSlotX(sourceOutput, source.outputSlotCount, nodeWidth_),
            source.position.y() + nodeHeight_
        );
        link.input = QPointF(
            target.position.x() + getSlotX(targetInput, target.inputSlotCount, nodeWidth_),
            target.position.y()
        );
        links.push_back(link);
    }
    return links;
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
