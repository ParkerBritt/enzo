#pragma once

#include "Engine/NetworkGraph/Connection.h"
#include <QAbstractListModel>
#include <QVariant>
#include <functional>
#include <vector>

namespace enzo::ui {

/// @brief Wired connections of the network as a flat list for the wire layer.
///
/// Each row is one edge carrying the endpoints it links, the source node's
/// output slot feeding the target node's input slot. The model holds no engine
/// state of its own. The network view-model drives it from the engine signals,
/// so a row only ever changes in response to the engine.
class EdgeListModel : public QAbstractListModel
{
    Q_OBJECT

  public:
    explicit EdgeListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// @brief Replaces every row with the connections currently in the network.
    void resetFromNetwork();

    /// @brief Appends a row for the given connection.
    void addEdge(const nt::Connection& connection);

    /// @brief Removes the row for the given connection.
    void removeEdge(const nt::Connection& connection);

    /// @brief Removes every row.
    void clear();

  private:
    /// One model role paired with the field it exposes from a connection.
    struct RoleDef
    {
        QByteArray name;
        std::function<QVariant(const nt::Connection&)> get;
    };

    /// @brief Returns the role table that both data and roleNames are built from.
    ///
    /// @note A role's int is its position in the table offset from Qt::UserRole,
    /// so adding a field means adding one entry here and nothing else.
    static const std::vector<RoleDef>& getRoleDefs();

    /// @brief Returns the model role for a field name, or -1 when absent.
    static int getRole(const QByteArray& name);

    /// @brief Returns the row index of a connection, or -1 when absent.
    int rowOf(const nt::Connection& connection) const;

    std::vector<nt::Connection> edges_;
};

} // namespace enzo::ui
