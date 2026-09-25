#pragma once

#include "Engine/NetworkGraph/NodeLink.h"
#include <QAbstractListModel>
#include <QVariant>
#include <functional>
#include <optional>
#include <vector>

namespace enzo::ui {

/// @brief Node links of the network as a flat list for the wire layer.
///
/// Each row is one node link carrying the endpoints it links, the source node's
/// output feeding the target node's input. The model holds no engine
/// state of its own. The network view-model drives it from the engine signals,
/// so a row only ever changes in response to the engine.
class NodeLinkListModel : public QAbstractListModel
{
    Q_OBJECT

  public:
    explicit NodeLinkListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// @brief Replaces every row with the node links currently in the network.
    void resetFromNetwork();

    /// @brief Appends a row for the given node link.
    void addNodeLink(const nt::NodeLink& nodeLink);

    /// @brief Removes the row for the given node link.
    void removeNodeLink(const nt::NodeLink& nodeLink);

    /// @brief Removes every row.
    void clear();

    /// @brief Returns the node link at a row, or nothing when the row is out of range.
    std::optional<nt::NodeLink> nodeLinkAt(int row) const;

  private:
    /// One model role paired with the field it exposes from a node link.
    struct RoleDef
    {
        QByteArray name;
        std::function<QVariant(const nt::NodeLink&)> get;
    };

    /// @brief Returns the role table that both data and roleNames are built from.
    ///
    /// @note A role's int is its position in the table offset from Qt::UserRole,
    /// so adding a field means adding one entry here and nothing else.
    static const std::vector<RoleDef>& getRoleDefs();

    /// @brief Returns the model role for a field name, or -1 when absent.
    static int getRole(const QByteArray& name);

    /// @brief Returns the row index of a node link, or -1 when absent.
    int rowOf(const nt::NodeLink& nodeLink) const;

    std::vector<nt::NodeLink> nodeLinks_;
};

} // namespace enzo::ui
