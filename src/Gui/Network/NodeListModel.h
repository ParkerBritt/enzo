#pragma once

#include "Engine/Core/Types.h"
#include <QAbstractListModel>
#include <QPointF>
#include <QVariant>
#include <QVariantMap>
#include <functional>
#include <optional>
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
    explicit NodeListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// @brief Returns the port a press would grab, nearest a canvas point across both edges.
    ///
    /// The reach is looser than a snap so aiming roughly at a port is enough.
    /// @return A {opId, slot, isOutput, x, y} map, empty when none is within reach.
    Q_INVOKABLE QVariantMap getGrabPort(QPointF canvasPoint) const;

    /// @brief Returns the port a dragged link would snap onto, nearest a canvas point.
    /// @param wantOutput Searches output ports when true, input ports when false.
    /// @return A {opId, slot, isOutput, x, y} map, empty when none is within reach.
    Q_INVOKABLE QVariantMap getSnapPort(QPointF canvasPoint, bool wantOutput) const;

    /// @brief Returns the canvas position of one port, or nothing when the op is absent.
    ///
    /// The op may exist in the engine yet not in this snapshot mid update, so the
    /// link layer skips drawing an endpoint it cannot place.
    std::optional<QPointF> getPortPosition(nt::OpId opId, int slot, bool isOutput) const;

    /// @brief Replaces every row with the operators currently in the network.
    void resetFromNetwork();

    /// @brief Appends a row for the operator with the given id.
    void addNode(nt::OpId opId);

    /// @brief Removes the row for the operator with the given id.
    void removeNode(nt::OpId opId);

    /// @brief Removes every row.
    void clear();

    /// @brief Marks the rows in @p selectedIds selected and the rest unselected.
    void setSelection(const std::vector<nt::OpId>& selectedIds);

    /// @brief Marks the row matching @p opId primary and the rest not.
    void setPrimary(std::optional<nt::OpId> opId);

    /// @brief Marks the row matching @p opId the display node and the rest not.
    void setDisplay(std::optional<nt::OpId> opId);

    /// @brief Moves the row matching @p opId to a new graph position.
    void setPosition(nt::OpId opId, float x, float y);

    /// @brief Shifts every selected row by a delta, for a live group drag.
    void moveSelectedBy(float dx, float dy);

    /// @brief Returns the graph position of the row matching @p opId.
    QPointF getPosition(nt::OpId opId) const;

  private:
    /// One node row, a snapshot of the operator's display data.
    struct Node
    {
        nt::OpId opId;
        QString name;
        QString type;
        float x;
        float y;
        int inputSlotCount;
        int outputSlotCount;
        bool selected = false;
        bool primary = false;
        bool display = false;
    };

    /// One model role paired with the field it exposes from a row.
    struct RoleDef
    {
        QByteArray name;
        std::function<QVariant(const Node&)> get;
    };

    /// @brief Returns the role table that both data and roleNames are built from.
    ///
    /// @note A role's int is its position in the table offset from Qt::UserRole,
    /// so adding a field means adding one entry here and nothing else.
    static const std::vector<RoleDef>& getRoleDefs();

    /// @brief Returns the model role for a field name, or -1 when absent.
    static int getRole(const QByteArray& name);

    /// @brief Reads the display data for an operator into a row.
    static Node makeNode(nt::OpId opId);

    /// @brief Returns the row index of an operator, or -1 when absent.
    int rowOf(nt::OpId opId) const;

    /// @brief Returns the canvas position of one port on @p node.
    ///
    /// Ports spread evenly along an edge, so one lands at the middle and two land
    /// at the third marks. Inputs sit on the top edge, outputs on the bottom.
    QPointF getPortPosition(const Node& node, int slot, bool isOutput) const;

    /// @brief Returns the nearest port within @p pickRadius across the chosen edges.
    QVariantMap getNearestPort(
        QPointF canvasPoint,
        bool searchOutputs,
        bool searchInputs,
        qreal pickRadius
    ) const;

    std::vector<Node> nodes_;
};

} // namespace enzo::ui
