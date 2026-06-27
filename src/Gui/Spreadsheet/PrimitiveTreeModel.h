#pragma once

#include <QAbstractItemModel>
#include <memory>
#include <vector>

namespace enzo {
class NodePacket;
}

namespace enzo::ui {

/// @brief Primitives of a packet arranged as a path based tree.
///
/// A primitive path such as `/geo/mesh1` becomes nested branches, and the leaf
/// carries the primitive index so selecting it can drive the attribute table.
/// Branches are the path folders and hold no primitive of their own.
class PrimitiveTreeModel : public QAbstractItemModel
{
    Q_OBJECT

  public:
    enum Roles
    {
        NameRole = Qt::UserRole + 1,
        TypeTagRole,
        PrimitiveIndexRole,
    };

    explicit PrimitiveTreeModel(QObject* parent = nullptr);
    ~PrimitiveTreeModel() override;

    /// @brief Rebuilds the tree from the primitives of a packet.
    void setPacket(std::shared_ptr<const NodePacket> packet);

    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// @brief Returns the primitive index for a leaf, or -1 for a branch.
    Q_INVOKABLE int primitiveIndexAt(const QModelIndex& index) const;

  private:
    /// One tree row, either a path folder branch or a primitive leaf.
    struct Node
    {
        QString name;
        QString typeTag;
        int primitiveIndex = -1;
        Node* parent = nullptr;
        std::vector<std::unique_ptr<Node>> children;

        int row() const;
        Node* childNamed(const QString& name);
    };

    Node* nodeAt(const QModelIndex& index) const;

    std::unique_ptr<Node> root_;
};

} // namespace enzo::ui
