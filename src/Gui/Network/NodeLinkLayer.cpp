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

        links.push_back(Link{*output, *input});
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
