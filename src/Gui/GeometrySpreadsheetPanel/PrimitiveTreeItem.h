#pragma once
#include <QVariant>

class PrimitiveTreeItem
{
public:
    explicit PrimitiveTreeItem(QVariantList data, PrimitiveTreeItem *parent = nullptr);

    PrimitiveTreeItem *child(int number);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    bool insertChildren(int position, int count, int columns);
    bool insertColumns(int position, int columns);
    PrimitiveTreeItem *parent();
    bool removeChildren(int position, int count);
    bool removeColumns(int position, int columns);
    int row() const;
    bool setData(int column, const QVariant &value);

private:
    std::vector<std::unique_ptr<PrimitiveTreeItem>> childItems_;
    QVariantList itemData_;
    PrimitiveTreeItem *parentItems_;
};
