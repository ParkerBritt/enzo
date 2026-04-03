#include "PrimitiveTreeItem.h"

PrimitiveTreeItem::PrimitiveTreeItem(QVariantList data, PrimitiveTreeItem *parent)
    : itemData_(std::move(data)), parentItems_(parent)
{
}

PrimitiveTreeItem *PrimitiveTreeItem::child(int number)
{
    if (number < 0 || number >= childCount())
        return nullptr;
    return childItems_[number].get();
}

int PrimitiveTreeItem::childCount() const
{
    return static_cast<int>(childItems_.size());
}

int PrimitiveTreeItem::columnCount() const
{
    return static_cast<int>(itemData_.size());
}

QVariant PrimitiveTreeItem::data(int column) const
{
    if (column < 0 || column >= columnCount())
        return {};
    return itemData_[column];
}

bool PrimitiveTreeItem::insertChildren(int position, int count, int columns)
{
    if (position < 0 || position > childCount())
        return false;

    for (int row = 0; row < count; ++row) {
        QVariantList data(columns);
        auto item = std::make_unique<PrimitiveTreeItem>(data, this);
        childItems_.insert(childItems_.begin() + position, std::move(item));
    }

    return true;
}

bool PrimitiveTreeItem::insertColumns(int position, int columns)
{
    if (position < 0 || position > columnCount())
        return false;

    for (int col = 0; col < columns; ++col)
        itemData_.insert(position, QVariant());

    for (auto &child : childItems_)
        child->insertColumns(position, columns);

    return true;
}

PrimitiveTreeItem *PrimitiveTreeItem::parent()
{
    return parentItems_;
}

bool PrimitiveTreeItem::removeChildren(int position, int count)
{
    if (position < 0 || position + count > childCount())
        return false;

    childItems_.erase(childItems_.begin() + position,
                      childItems_.begin() + position + count);
    return true;
}

bool PrimitiveTreeItem::removeColumns(int position, int columns)
{
    if (position < 0 || position + columns > columnCount())
        return false;

    for (int col = 0; col < columns; ++col)
        itemData_.removeAt(position);

    for (auto &child : childItems_)
        child->removeColumns(position, columns);

    return true;
}

int PrimitiveTreeItem::row() const
{
    if (!parentItems_)
        return 0;

    const auto &siblings = parentItems_->childItems_;
    for (int i = 0; i < static_cast<int>(siblings.size()); ++i) {
        if (siblings[i].get() == this)
            return i;
    }
    return 0;
}

bool PrimitiveTreeItem::setData(int column, const QVariant &value)
{
    if (column < 0 || column >= columnCount())
        return false;

    itemData_[column] = value;
    return true;
}
