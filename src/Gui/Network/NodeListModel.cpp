#include "Gui/Network/NodeListModel.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include <QLineF>
#include <QRectF>
#include <algorithm>

namespace enzo::ui {

namespace {

// How close the cursor must be to a port to grab it.
constexpr qreal kGrabRadius = 60;

// How close a dragged link must be to a port to snap onto it.
constexpr qreal kSnapRadius = 60;

// Returns the width one port covers on a card edge holding portCount of them.
qreal getPortWidth(int portCount) { return NodeListModel::nodeWidth / (portCount + 1); }

// Returns the center of one port, measured from the card's left edge.
qreal getPortCenter(int portIndex, int portCount)
{
    return getPortWidth(portCount) * (portIndex + 1);
}

} // namespace

NodeListModel::NodeListModel(QObject* parent) : QAbstractListModel(parent) {}

const std::vector<NodeListModel::RoleDef>& NodeListModel::getRoleDefs()
{
    static const std::vector<RoleDef> defs = {
        {"nodeId", [](const Node& node) { return QVariant::fromValue(node.nodeId); }},
        {"name", [](const Node& node) { return QVariant(node.name); }},
        {"type", [](const Node& node) { return QVariant(node.type); }},
        {"x", [](const Node& node) { return QVariant(node.x); }},
        {"y", [](const Node& node) { return QVariant(node.y); }},
        {"inputPortCount", [](const Node& node) { return QVariant(node.inputPortCount); }},
        {"outputPortCount", [](const Node& node) { return QVariant(node.outputPortCount); }},
        {"multiInput", [](const Node& node) { return QVariant(node.multiInput); }},
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
    for (auto [nodeId, node] : nt::nm().nodes())
        nodes_.push_back(makeNode(nodeId));
    endResetModel();
}

void NodeListModel::addNode(nt::NodeId nodeId)
{
    if (rowOf(nodeId) != -1) return;

    const int row = static_cast<int>(nodes_.size());
    beginInsertRows(QModelIndex(), row, row);
    nodes_.push_back(makeNode(nodeId));
    endInsertRows();
}

void NodeListModel::removeNode(nt::NodeId nodeId)
{
    const int row = rowOf(nodeId);
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

void NodeListModel::setSelection(const std::vector<nt::NodeId>& selectedIds)
{
    if (nodes_.empty()) return;

    for (Node& node : nodes_)
    {
        const auto found = std::find(selectedIds.begin(), selectedIds.end(), node.nodeId);
        node.selected = found != selectedIds.end();
    }

    Q_EMIT dataChanged(index(0), index(static_cast<int>(nodes_.size()) - 1), {getRole("selected")});
}

void NodeListModel::setPrimary(std::optional<nt::NodeId> nodeId)
{
    if (nodes_.empty()) return;

    for (Node& node : nodes_)
        node.primary = nodeId.has_value() && node.nodeId == *nodeId;

    Q_EMIT dataChanged(index(0), index(static_cast<int>(nodes_.size()) - 1), {getRole("primary")});
}

void NodeListModel::setDisplay(std::optional<nt::NodeId> nodeId)
{
    if (nodes_.empty()) return;

    for (Node& node : nodes_)
        node.display = nodeId.has_value() && node.nodeId == *nodeId;

    Q_EMIT dataChanged(index(0), index(static_cast<int>(nodes_.size()) - 1), {getRole("display")});
}

void NodeListModel::setPosition(nt::NodeId nodeId, float x, float y)
{
    const int row = rowOf(nodeId);
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

QPointF NodeListModel::getPosition(nt::NodeId nodeId) const
{
    const int row = rowOf(nodeId);
    if (row == -1) return {};

    return QPointF(nodes_[row].x, nodes_[row].y);
}

QRectF NodeListModel::getPortBox(qulonglong nodeId, int index, bool isOutput) const
{
    const int row = rowOf(nodeId);
    if (row == -1) return {};

    const Node& node = nodes_[row];
    const int portCount = isOutput ? node.outputPortCount : node.inputPortCount;
    const qreal width = getPortWidth(portCount);
    const qreal left = getPortCenter(index, portCount) - width / 2;
    return QRectF(left, isOutput ? nodeHeight : 0, width, 0);
}

QPointF NodeListModel::getOutputPosition(const Node& node, int outputIndex) const
{
    // Shifts the stored center to the bottom left corner the ports measure from.
    const qreal left = node.x - nodeWidth / 2;
    const qreal bottom = node.y + nodeHeight / 2;
    return QPointF(left + getPortCenter(outputIndex, node.outputPortCount), bottom);
}

int NodeListModel::getMultiInputCount(const Node& node) const
{
    if (!node.multiInput || !nt::nm().isValidNode(node.nodeId)) return 0;

    // Assigns every input past the single ports to the multi input port.
    const int singlePortCount = node.inputPortCount - 1;
    return static_cast<int>(nt::nm().getInputCount(node.nodeId)) - singlePortCount;
}

QPointF NodeListModel::getInputPosition(const Node& node, int inputIndex, int multiCount) const
{
    const qreal left = node.x - nodeWidth / 2;
    const qreal top = node.y - nodeHeight / 2;
    const int portCount = node.inputPortCount;
    const int singlePortCount = node.multiInput ? portCount - 1 : portCount;

    if (inputIndex < singlePortCount)
        return QPointF(left + getPortCenter(inputIndex, portCount), top);

    // Spreads the multi input port's connections evenly across the width it covers.
    const qreal barWidth = getPortWidth(portCount);
    const qreal barLeft = left + getPortCenter(portCount - 1, portCount) - barWidth / 2;
    const int barPosition = inputIndex - singlePortCount;
    return QPointF(barLeft + barWidth * (barPosition + 1) / (multiCount + 1), top);
}

std::optional<QPointF>
NodeListModel::getPortPosition(nt::NodeId nodeId, int index, bool isOutput) const
{
    const int row = rowOf(nodeId);
    if (row == -1) return std::nullopt;

    const Node& node = nodes_[row];
    if (isOutput) return getOutputPosition(node, index);
    return getInputPosition(node, index, getMultiInputCount(node));
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
        // Lays the inputs out with one more than the multi input port holds, so a
        // link can land at either end of the bar or between any two connections.
        const int multiCount = getMultiInputCount(node);
        const int inputDropCount = node.inputPortCount + multiCount;

        const int portCount = isOutput ? node.outputPortCount : inputDropCount;
        for (int index = 0; index < portCount; ++index)
        {
            const QPointF port = isOutput ? getOutputPosition(node, index)
                                          : getInputPosition(node, index, multiCount + 1);
            const qreal distance = QLineF(port, canvasPoint).length();
            if (distance < nearestDistance)
            {
                nearestDistance = distance;
                nearest = QVariantMap{
                    {"nodeId", QVariant::fromValue(node.nodeId)},
                    {"index", index},
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

QRectF NodeListModel::getNodeBody(const Node& node)
{
    return QRectF(node.x - nodeWidth / 2, node.y - nodeHeight / 2, nodeWidth, nodeHeight);
}

std::vector<nt::NodeId> NodeListModel::getNodesInRect(QRectF canvasRect) const
{
    const QRectF box = canvasRect.normalized();

    std::vector<nt::NodeId> boxedIds;
    for (const Node& node : nodes_)
    {
        if (box.intersects(getNodeBody(node))) boxedIds.push_back(node.nodeId);
    }
    return boxedIds;
}

bool NodeListModel::isOverNodeBody(QPointF canvasPoint) const
{
    for (const Node& node : nodes_)
    {
        if (getNodeBody(node).contains(canvasPoint)) return true;
    }
    return false;
}

bool NodeListModel::isOverNodeOrPort(QPointF canvasPoint) const
{
    if (isOverNodeBody(canvasPoint)) return true;
    return !getNearestPort(canvasPoint, true, true, kGrabRadius).isEmpty();
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

NodeListModel::Node NodeListModel::makeNode(nt::NodeId nodeId)
{
    nt::Node& node = nt::nm().getNode(nodeId);
    const Vector2 position = node.getPosition();
    return Node{
        nodeId,
        QString::fromStdString(node.getName()),
        QString::fromStdString(node.getType().getLabel()),
        position.x(),
        position.y(),
        static_cast<int>(node.getType().inputPorts.size()),
        static_cast<int>(node.getMaxOutputs()),
        node.getType().hasMultiInputPort(),
    };
}

int NodeListModel::rowOf(nt::NodeId nodeId) const
{
    for (int row = 0; row < static_cast<int>(nodes_.size()); ++row)
        if (nodes_[row].nodeId == nodeId) return row;
    return -1;
}

} // namespace enzo::ui
