#include "Gui/Network/NodeListModel.h"
#include "Engine/Network/GeometryOperator.h"
#include "Engine/Network/NetworkManager.h"

namespace enzo::ui {

NodeListModel::NodeListModel(QObject* parent) : QAbstractListModel(parent) {}

int NodeListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(nodes_.size());
}

QVariant NodeListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= static_cast<int>(nodes_.size())) return {};

    const Node& node = nodes_[index.row()];
    switch (role)
    {
    case OpIdRole:
        return QVariant::fromValue(node.opId);
    case NameRole:
        return node.name;
    case TypeRole:
        return node.type;
    case XRole:
        return node.x;
    case YRole:
        return node.y;
    default:
        return {};
    }
}

QHash<int, QByteArray> NodeListModel::roleNames() const
{
    return {
        {OpIdRole, "opId"},
        {NameRole, "name"},
        {TypeRole, "type"},
        {XRole, "x"},
        {YRole, "y"},
    };
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
    };
}

int NodeListModel::rowOf(nt::OpId opId) const
{
    for (int row = 0; row < static_cast<int>(nodes_.size()); ++row)
        if (nodes_[row].opId == opId) return row;
    return -1;
}

} // namespace enzo::ui
