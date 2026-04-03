#include <QAbstractItemModel>
#include <string>
#include "PrimitiveTreeModel.h"

PrimitiveTreeModel::PrimitiveTreeModel(const QStringList &headers, const QString &data, QObject *parent)
{

}

QVariant PrimitiveTreeModel::data(const QModelIndex &index, int role) const
{
    if(role==Qt::DisplayRole)
    {
        return "Test";
    }

    return QVariant();
}

QVariant PrimitiveTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QVariant(section);

}

QModelIndex PrimitiveTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return {};

    return createIndex(row, column);
}

QModelIndex PrimitiveTreeModel::parent(const QModelIndex &index) const
{

}

int PrimitiveTreeModel::rowCount(const QModelIndex &parent) const
{
    return 10;

}

int PrimitiveTreeModel::columnCount(const QModelIndex &parent) const
{
    return 4;
}
