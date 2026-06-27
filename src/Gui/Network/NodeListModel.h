#pragma once

#include "Engine/Core/Types.h"
#include <QAbstractListModel>
#include <vector>

namespace enzo::ui {

/// @brief Nodes of the network as a flat list for QML to repeat over.
///
/// Each row is one operator carrying its identity, name, type label, and graph
/// position. The model holds no engine state of its own. The network view-model
/// drives it from the engine signals, so a row only ever changes in response to
/// the engine, never from QML directly.
class NodeListModel : public QAbstractListModel
{
    Q_OBJECT

  public:
    enum Roles
    {
        OpIdRole = Qt::UserRole + 1,
        NameRole,
        TypeRole,
        XRole,
        YRole,
    };

    explicit NodeListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// @brief Replaces every row with the operators currently in the network.
    void resetFromNetwork();

    /// @brief Appends a row for the operator with the given id.
    void addNode(nt::OpId opId);

    /// @brief Removes the row for the operator with the given id.
    void removeNode(nt::OpId opId);

    /// @brief Removes every row.
    void clear();

  private:
    /// One node row, a snapshot of the operator's display data.
    struct Node
    {
        nt::OpId opId;
        QString name;
        QString type;
        float x;
        float y;
    };

    /// @brief Reads the display data for an operator into a row.
    static Node makeNode(nt::OpId opId);

    /// @brief Returns the row index of an operator, or -1 when absent.
    int rowOf(nt::OpId opId) const;

    std::vector<Node> nodes_;
};

} // namespace enzo::ui
