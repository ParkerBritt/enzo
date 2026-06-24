#include "Gui/Spreadsheet/PrimitiveTreeModel.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NodePacket.h"
#include "Engine/Primitives/Primitive.h"
#include "Engine/Selection/PrimPath.h"

namespace enzo::ui
{

namespace
{

/// @brief Returns the short uppercase tag shown at the end of a primitive row.
QString typeTag(geo::PrimType type)
{
    switch (type)
    {
    case geo::PrimType::MESH:
        return "MESH";
    case geo::PrimType::CAMERA:
        return "CAMERA";
    }
    return "PRIM";
}

} // namespace

int PrimitiveTreeModel::Node::row() const
{
    if (!parent) return 0;
    const auto& siblings = parent->children;
    for (size_t i = 0; i < siblings.size(); ++i)
        if (siblings[i].get() == this) return static_cast<int>(i);
    return 0;
}

PrimitiveTreeModel::Node* PrimitiveTreeModel::Node::childNamed(const QString& name)
{
    for (auto& child : children)
        if (child->name == name) return child.get();

    children.push_back(std::make_unique<Node>());
    Node* added = children.back().get();
    added->name = name;
    added->parent = this;
    return added;
}

PrimitiveTreeModel::PrimitiveTreeModel(QObject* parent)
    : QAbstractItemModel(parent), root_(std::make_unique<Node>())
{
}

PrimitiveTreeModel::~PrimitiveTreeModel() = default;

void PrimitiveTreeModel::setPacket(std::shared_ptr<const NodePacket> packet)
{
    beginResetModel();
    root_ = std::make_unique<Node>();

    if (packet)
    {
        const int count = static_cast<int>(packet->size());
        for (int i = 0; i < count; ++i)
        {
            const auto primitive = packet->getPrimitive(i);
            const PrimPath path(primitive->getPath());

            Node* current = root_.get();
            for (const auto& component : path.components())
                current = current->childNamed(QString::fromStdString(component));

            current->primitiveIndex = i;
            current->typeTag = typeTag(primitive->getType());
        }
    }

    endResetModel();
}

PrimitiveTreeModel::Node* PrimitiveTreeModel::nodeAt(const QModelIndex& index) const
{
    if (!index.isValid()) return root_.get();
    return static_cast<Node*>(index.internalPointer());
}

QModelIndex PrimitiveTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (column != 0) return {};

    Node* parentNode = nodeAt(parent);
    if (row < 0 || row >= static_cast<int>(parentNode->children.size())) return {};
    return createIndex(row, column, parentNode->children[row].get());
}

QModelIndex PrimitiveTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid()) return {};

    Node* parentNode = nodeAt(index)->parent;
    if (!parentNode || parentNode == root_.get()) return {};
    return createIndex(parentNode->row(), 0, parentNode);
}

int PrimitiveTreeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0) return 0;
    return static_cast<int>(nodeAt(parent)->children.size());
}

int PrimitiveTreeModel::columnCount(const QModelIndex&) const { return 1; }

QVariant PrimitiveTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) return {};

    const Node* node = nodeAt(index);
    switch (role)
    {
    case Qt::DisplayRole:
    case NameRole:
        return node->name;
    case TypeTagRole:
        return node->typeTag;
    case PrimitiveIndexRole:
        return node->primitiveIndex;
    default:
        return {};
    }
}

QHash<int, QByteArray> PrimitiveTreeModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {TypeTagRole, "typeTag"},
        {PrimitiveIndexRole, "primitiveIndex"},
    };
}

int PrimitiveTreeModel::primitiveIndexAt(const QModelIndex& index) const
{
    return nodeAt(index)->primitiveIndex;
}

} // namespace enzo::ui
