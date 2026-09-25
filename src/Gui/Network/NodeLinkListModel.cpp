#include "Gui/Network/NodeLinkListModel.h"
#include "Engine/Network/NetworkManager.h"

namespace enzo::ui {

NodeLinkListModel::NodeLinkListModel(QObject* parent) : QAbstractListModel(parent) {}

const std::vector<NodeLinkListModel::RoleDef>& NodeLinkListModel::getRoleDefs()
{
    static const std::vector<RoleDef> defs = {
        {"sourceNode",
         [](const nt::NodeLink& nodeLink) { return QVariant::fromValue(nodeLink.sourceNode); }},
        {"sourceOutput", [](const nt::NodeLink& nodeLink) { return QVariant(nodeLink.sourceOutput); }},
        {"targetNode",
         [](const nt::NodeLink& nodeLink) { return QVariant::fromValue(nodeLink.targetNode); }},
        {"targetInput", [](const nt::NodeLink& nodeLink) { return QVariant(nodeLink.targetInput); }},
    };
    return defs;
}

int NodeLinkListModel::getRole(const QByteArray& name)
{
    const std::vector<RoleDef>& defs = getRoleDefs();
    for (int index = 0; index < static_cast<int>(defs.size()); ++index)
        if (defs[index].name == name) return Qt::UserRole + 1 + index;
    return -1;
}

int NodeLinkListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(nodeLinks_.size());
}

QVariant NodeLinkListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= static_cast<int>(nodeLinks_.size())) return {};

    const int roleIndex = role - (Qt::UserRole + 1);
    const std::vector<RoleDef>& defs = getRoleDefs();
    if (roleIndex < 0 || roleIndex >= static_cast<int>(defs.size())) return {};

    return defs[roleIndex].get(nodeLinks_[index.row()]);
}

QHash<int, QByteArray> NodeLinkListModel::roleNames() const
{
    QHash<int, QByteArray> names;
    const std::vector<RoleDef>& defs = getRoleDefs();
    for (int index = 0; index < static_cast<int>(defs.size()); ++index)
        names.insert(Qt::UserRole + 1 + index, defs[index].name);
    return names;
}

void NodeLinkListModel::resetFromNetwork()
{
    beginResetModel();
    nodeLinks_ = nt::nm().graph().getNodeLinks();
    endResetModel();
}

void NodeLinkListModel::addNodeLink(const nt::NodeLink& nodeLink)
{
    if (rowOf(nodeLink) != -1) return;

    const int row = static_cast<int>(nodeLinks_.size());
    beginInsertRows(QModelIndex(), row, row);
    nodeLinks_.push_back(nodeLink);
    endInsertRows();
}

void NodeLinkListModel::removeNodeLink(const nt::NodeLink& nodeLink)
{
    const int row = rowOf(nodeLink);
    if (row == -1) return;

    beginRemoveRows(QModelIndex(), row, row);
    nodeLinks_.erase(nodeLinks_.begin() + row);
    endRemoveRows();
}

void NodeLinkListModel::clear()
{
    beginResetModel();
    nodeLinks_.clear();
    endResetModel();
}

std::optional<nt::NodeLink> NodeLinkListModel::nodeLinkAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(nodeLinks_.size())) return std::nullopt;
    return nodeLinks_[index];
}

int NodeLinkListModel::rowOf(const nt::NodeLink& nodeLink) const
{
    for (int row = 0; row < static_cast<int>(nodeLinks_.size()); ++row)
        if (nodeLinks_[row] == nodeLink) return row;
    return -1;
}

} // namespace enzo::ui
