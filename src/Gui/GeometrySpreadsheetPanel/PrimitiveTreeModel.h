#pragma once
#include <QAbstractItemModel>
#include <memory>
#include <string>
#include "Engine/Operator/NodePacket.h"

class PrimitiveTreeItem;

class PrimitiveTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    Q_DISABLE_COPY_MOVE(PrimitiveTreeModel)

    explicit PrimitiveTreeModel(QObject *parent = nullptr);
    ~PrimitiveTreeModel() override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column,
                    const QModelIndex &parent = {}) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;

    void setPacket(const enzo::NodePacket& packet);
    void clear();

private:
    PrimitiveTreeItem *getItem(const QModelIndex &index) const;
    PrimitiveTreeItem *findOrCreateChild(PrimitiveTreeItem *parent, const std::string &name);
    std::unique_ptr<PrimitiveTreeItem> rootItem_;
};
