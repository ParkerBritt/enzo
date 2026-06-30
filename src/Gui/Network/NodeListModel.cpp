#include "Gui/Network/NodeListModel.h"
#include "Engine/Network/GeometryOperator.h"
#include "Engine/Network/NetworkManager.h"
#include <QLineF>
#include <QRectF>
#include <algorithm>

namespace enzo::ui {

namespace {

// How close the cursor must be to a port to grab it.
constexpr qreal kGrabRadius = 60;

// How close a dragged link must be to a port to snap onto it.
constexpr qreal kSnapRadius = 60;

} // namespace

NodeListModel::NodeListModel(QObject* parent) : QAbstractListModel(parent) {}

const std::vector<NodeListModel::RoleDef>& NodeListModel::getRoleDefs()
{
    static const std::vector<RoleDef> defs = {
        {"opId", [](const Node& node) { return QVariant::fromValue(node.opId); }},
        {"name", [](const Node& node) { return QVariant(node.name); }},
        {"type", [](const Node& node) { return QVariant(node.type); }},
        {"x", [](const Node& node) { return QVariant(node.x); }},
        {"y", [](const Node& node) { return QVariant(node.y); }},
        {"inputSlotCount", [](const Node& node) { return QVariant(node.inputSlotCount); }},
        {"outputSlotCount", [](const Node& node) { return QVariant(node.outputSlotCount); }},
        {"selected", [](const Node& node) { return QVariant(node.selected); }},
        {"primary", [](const Node& node) { return QVariant(node.primary); }},
        {"display", [](const Node& node) { return QVariant(node.display); }},
    };
    return defs;
}

int NodeListModel::getRole(const QByteArray& name)
{
    const std::vector<RoleDef>& defs = getRoleDefs();
    for (int index = 0; index < static_cast<int>(defs.size()); ++index)
        if (defs[index].name == name) return Qt::UserRole + 1 + index;
    return -1;
}

int NodeListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(nodes_.size());
}

QVariant NodeListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= static_cast<int>(nodes_.size())) return {};

    const int roleIndex = role - (Qt::UserRole + 1);
    const std::vector<RoleDef>& defs = getRoleDefs();
    if (roleIndex < 0 || roleIndex >= static_cast<int>(defs.size())) return {};

    return defs[roleIndex].get(nodes_[index.row()]);
}

QHash<int, QByteArray> NodeListModel::roleNames() const
{
    QHash<int, QByteArray> names;
    const std::vector<RoleDef>& defs = getRoleDefs();
    for (int index = 0; index < static_cast<int>(defs.size()); ++index)
        names.insert(Qt::UserRole + 1 + index, defs[index].name);
    return names;
}

void NodeListModel::resetFromNetwork()
{
    beginResetModel();
    nodes_.clear();
    for (auto [opId, op] : nt::nm().operators())
        nodes_.push_back(makeNode(opId));
    endResetModel();
}

void NodeListModel::addNode(nt::OpId opId)
{
    if (rowOf(opId) != -1) return;

    const int row = static_cast<int>(nodes_.size());
    beginInsertRows(QModelIndex(), row, row);
    nodes_.push_back(makeNode(opId));
    endInsertRows();
}

void NodeListModel::removeNode(nt::OpId opId)
{
    const int row = rowOf(opId);
    if (row == -1) return;

    beginRemoveRows(QModelIndex(), row, row);
    nodes_.erase(nodes_.begin() + row);
    endRemoveRows();
}

void NodeListModel::clear()
{
    beginResetModel();
    nodes_.clear();
    endResetModel();
}

void NodeListModel::setSelection(const std::vector<nt::OpId>& selectedIds)
{
    if (nodes_.empty()) return;

    for (Node& node : nodes_)
    {
        const auto found = std::find(selectedIds.begin(), selectedIds.end(), node.opId);
        node.selected = found != selectedIds.end();
    }

    Q_EMIT dataChanged(index(0), index(static_cast<int>(nodes_.size()) - 1), {getRole("selected")});
}

void NodeListModel::setPrimary(std::optional<nt::OpId> opId)
{
    if (nodes_.empty()) return;

    for (Node& node : nodes_)
        node.primary = opId.has_value() && node.opId == *opId;

    Q_EMIT dataChanged(index(0), index(static_cast<int>(nodes_.size()) - 1), {getRole("primary")});
}

void NodeListModel::setDisplay(std::optional<nt::OpId> opId)
{
    if (nodes_.empty()) return;

    for (Node& node : nodes_)
        node.display = opId.has_value() && node.opId == *opId;

    Q_EMIT dataChanged(index(0), index(static_cast<int>(nodes_.size()) - 1), {getRole("display")});
}

void NodeListModel::setPosition(nt::OpId opId, float x, float y)
{
    const int row = rowOf(opId);
    if (row == -1) return;

    nodes_[row].x = x;
    nodes_[row].y = y;

    Q_EMIT dataChanged(index(row), index(row), {getRole("x"), getRole("y")});
}

void NodeListModel::moveSelectedBy(float dx, float dy)
{
    bool moved = false;
    for (Node& node : nodes_)
    {
        if (!node.selected) continue;
        node.x += dx;
        node.y += dy;
        moved = true;
    }
    if (!moved) return;

    Q_EMIT dataChanged(
        index(0),
        index(static_cast<int>(nodes_.size()) - 1),
        {getRole("x"), getRole("y")}
    );
}

QPointF NodeListModel::getPosition(nt::OpId opId) const
{
    const int row = rowOf(opId);
    if (row == -1) return {};

    return QPointF(nodes_[row].x, nodes_[row].y);
}

QPointF NodeListModel::getPortPosition(const Node& node, int slot, bool isOutput) const
{
    // Nodes store their center, so shift to the top left the ports measure from.
    const qreal left = node.x - nodeWidth / 2;
    const qreal top = node.y - nodeHeight / 2;

    const int slotCount = isOutput ? node.outputSlotCount : node.inputSlotCount;
    const qreal x = left + nodeWidth * (slot + 1) / (slotCount + 1);
    const qreal y = top + (isOutput ? nodeHeight : 0);
    return QPointF(x, y);
}

std::optional<QPointF> NodeListModel::getPortPosition(nt::OpId opId, int slot, bool isOutput) const
{
    const int row = rowOf(opId);
    if (row == -1) return std::nullopt;

    return getPortPosition(nodes_[row], slot, isOutput);
}

QVariantMap NodeListModel::getNearestPort(
    QPointF canvasPoint,
    bool searchOutputs,
    bool searchInputs,
    qreal pickRadius
) const
{
    QVariantMap nearest;
    qreal nearestDistance = pickRadius;

    auto consider = [&](const Node& node, bool isOutput) {
        const int slotCount = isOutput ? node.outputSlotCount : node.inputSlotCount;
        for (int slot = 0; slot < slotCount; ++slot)
        {
            const QPointF port = getPortPosition(node, slot, isOutput);
            const qreal distance = QLineF(port, canvasPoint).length();
            if (distance < nearestDistance)
            {
                nearestDistance = distance;
                nearest = QVariantMap{
                    {"opId", QVariant::fromValue(node.opId)},
                    {"slot", slot},
                    {"isOutput", isOutput},
                    {"x", port.x()},
                    {"y", port.y()}
                };
            }
        }
    };

    for (const Node& node : nodes_)
    {
        if (searchOutputs) consider(node, true);
        if (searchInputs) consider(node, false);
    }
    return nearest;
}

bool NodeListModel::isOverNodeBody(QPointF canvasPoint) const
{
    for (const Node& node : nodes_)
    {
        const QRectF body(node.x - nodeWidth / 2, node.y - nodeHeight / 2, nodeWidth, nodeHeight);
        if (body.contains(canvasPoint)) return true;
    }
    return false;
}

QVariantMap NodeListModel::getGrabPort(QPointF canvasPoint) const
{
    // A press over a card grabs the node, not a port, so no port is grabbable there.
    if (isOverNodeBody(canvasPoint)) return {};
    return getNearestPort(canvasPoint, true, true, kGrabRadius);
}

QVariantMap NodeListModel::getSnapPort(QPointF canvasPoint, bool wantOutput) const
{
    return getNearestPort(canvasPoint, wantOutput, !wantOutput, kSnapRadius);
}

NodeListModel::Node NodeListModel::makeNode(nt::OpId opId)
{
    nt::GeometryOperator& op = nt::nm().getGeoOperator(opId);
    const Vector2 position = op.getPosition();
    return Node{
        opId,
        QString::fromStdString(op.getName()),
        QString::fromStdString(op.getType().getLabel()),
        position.x(),
        position.y(),
        static_cast<int>(op.getMaxInputs()),
        static_cast<int>(op.getMaxOutputs()),
    };
}

int NodeListModel::rowOf(nt::OpId opId) const
{
    for (int row = 0; row < static_cast<int>(nodes_.size()); ++row)
        if (nodes_[row].opId == opId) return row;
    return -1;
}

} // namespace enzo::ui
