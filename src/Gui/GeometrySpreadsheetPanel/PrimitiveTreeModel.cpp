#include "PrimitiveTreeModel.h"
#include "PrimitiveTreeItem.h"
#include <icecream.hpp>

PrimitiveTreeModel::PrimitiveTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem_ = std::make_unique<PrimitiveTreeItem>(QVariantList{tr("Path")});
}

PrimitiveTreeModel::~PrimitiveTreeModel() = default;

PrimitiveTreeItem *PrimitiveTreeModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        auto *item = static_cast<PrimitiveTreeItem *>(index.internalPointer());
        if (item)
            return item;
    }
    return rootItem_.get();
}

QVariant PrimitiveTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return {};

    const auto *item = getItem(index);
    return item->data(index.column());
}

QVariant PrimitiveTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem_->data(section);

    return {};
}

QModelIndex PrimitiveTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return {};

    PrimitiveTreeItem *parentItem = getItem(parent);
    PrimitiveTreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);

    return {};
}

QModelIndex PrimitiveTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    auto *childItem = getItem(index);
    auto *parentItem = childItem->parent();

    if (!parentItem || parentItem == rootItem_.get())
        return {};

    return createIndex(parentItem->row(), 0, parentItem);
}

int PrimitiveTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return 0;

    const auto *parentItem = getItem(parent);
    return parentItem->childCount();
}

int PrimitiveTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return rootItem_->columnCount();
}

void PrimitiveTreeModel::setPacket(const enzo::NodePacket &packet)
{
    beginResetModel();
    rootItem_->removeChildren(0, rootItem_->childCount());
    const int count = static_cast<int>(packet.size());
    rootItem_->insertChildren(0, count, rootItem_->columnCount());
    for (int i = 0; i < count; ++i) {
        std::string primPath = packet.getPrimitive(i).getPath();
        IC(primPath);
        rootItem_->child(i)->setData(0, QString::fromStdString(primPath));
    }
    endResetModel();
}

void PrimitiveTreeModel::clear()
{
    beginResetModel();
    rootItem_->removeChildren(0, rootItem_->childCount());
    endResetModel();
}
