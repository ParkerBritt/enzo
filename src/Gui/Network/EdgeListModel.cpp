#include "Gui/Network/EdgeListModel.h"
#include "Engine/Network/NetworkManager.h"

namespace enzo::ui {

EdgeListModel::EdgeListModel(QObject* parent) : QAbstractListModel(parent) {}

const std::vector<EdgeListModel::RoleDef>& EdgeListModel::getRoleDefs()
{
    static const std::vector<RoleDef> defs = {
        {"sourceOp", [](const nt::Connection& edge) { return QVariant::fromValue(edge.sourceOp); }},
        {"sourceOutput", [](const nt::Connection& edge) { return QVariant(edge.sourceOutput); }},
        {"targetOp", [](const nt::Connection& edge) { return QVariant::fromValue(edge.targetOp); }},
        {"targetInput", [](const nt::Connection& edge) { return QVariant(edge.targetInput); }},
    };
    return defs;
}

int EdgeListModel::getRole(const QByteArray& name)
{
    const std::vector<RoleDef>& defs = getRoleDefs();
    for (int index = 0; index < static_cast<int>(defs.size()); ++index)
        if (defs[index].name == name) return Qt::UserRole + 1 + index;
    return -1;
}

int EdgeListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(edges_.size());
}

QVariant EdgeListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= static_cast<int>(edges_.size())) return {};

    const int roleIndex = role - (Qt::UserRole + 1);
    const std::vector<RoleDef>& defs = getRoleDefs();
    if (roleIndex < 0 || roleIndex >= static_cast<int>(defs.size())) return {};

    return defs[roleIndex].get(edges_[index.row()]);
}

QHash<int, QByteArray> EdgeListModel::roleNames() const
{
    QHash<int, QByteArray> names;
    const std::vector<RoleDef>& defs = getRoleDefs();
    for (int index = 0; index < static_cast<int>(defs.size()); ++index)
        names.insert(Qt::UserRole + 1 + index, defs[index].name);
    return names;
}

void EdgeListModel::resetFromNetwork()
{
    beginResetModel();
    edges_ = nt::nm().graph().getConnections();
    endResetModel();
}

void EdgeListModel::addEdge(const nt::Connection& connection)
{
    if (rowOf(connection) != -1) return;

    const int row = static_cast<int>(edges_.size());
    beginInsertRows(QModelIndex(), row, row);
    edges_.push_back(connection);
    endInsertRows();
}

void EdgeListModel::removeEdge(const nt::Connection& connection)
{
    const int row = rowOf(connection);
    if (row == -1) return;

    beginRemoveRows(QModelIndex(), row, row);
    edges_.erase(edges_.begin() + row);
    endRemoveRows();
}

void EdgeListModel::clear()
{
    beginResetModel();
    edges_.clear();
    endResetModel();
}

int EdgeListModel::rowOf(const nt::Connection& connection) const
{
    for (int row = 0; row < static_cast<int>(edges_.size()); ++row)
        if (edges_[row] == connection) return row;
    return -1;
}

} // namespace enzo::ui
