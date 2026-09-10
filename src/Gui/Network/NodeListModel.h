#pragma once

#include "Engine/Core/Types.h"
#include <QAbstractListModel>
#include <QPointF>
#include <QRectF>
#include <QVariant>
#include <QVariantMap>
#include <functional>
#include <optional>
#include <vector>

namespace enzo::ui {

/// @brief Nodes of the network as a flat list for QML to repeat over.
///
/// Each row is one node carrying its identity, name, type label, and graph
/// position. The model holds no engine state of its own. The network view-model
/// drives it from the engine signals, so a row only ever changes in response to
/// the engine, never from QML directly.
class NodeListModel : public QAbstractListModel
{
    Q_OBJECT

  public:
    /// The node card dimensions, the single source for the port geometry and the card.
    static constexpr qreal nodeWidth = 80;
    static constexpr qreal nodeHeight = 25;

    explicit NodeListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// @brief Returns the port a press would grab, nearest a canvas point across both edges.
    ///
    /// The reach is looser than a snap so aiming roughly at a port is enough.
    /// @return A {nodeId, index, isOutput, x, y} map, empty when none is within reach.
    Q_INVOKABLE QVariantMap getGrabPort(QPointF canvasPoint) const;

    /// @brief Returns the port a dragged link would snap onto, nearest a canvas point.
    /// @param wantOutput Searches output ports when true, input ports when false.
    /// @return A {nodeId, index, isOutput, x, y} map, empty when none is within reach.
    Q_INVOKABLE QVariantMap getSnapPort(QPointF canvasPoint, bool wantOutput) const;

    /// @brief Returns the stretch of card edge one port covers, in card coordinates.
    ///
    /// Every port on an edge covers an equal share of it, so a single port centers
    /// its dot in that share and a multi input port fills it as a bar.
    ///
    /// @note The height is zero. A port's thickness is drawn, not laid out.
    Q_INVOKABLE QRectF getPortBox(qulonglong nodeId, int index, bool isOutput) const;

    /// @brief Returns the ids of every node whose card the given canvas rectangle
    /// touches.
    std::vector<nt::NodeId> getNodesInRect(QRectF canvasRect) const;

    /// @brief Whether a canvas point lies over a node's card or within reach of one
    /// of its ports.
    Q_INVOKABLE bool isOverNodeOrPort(QPointF canvasPoint) const;

    /// @brief Returns the canvas position of one port, or nothing when the node is absent.
    ///
    /// The node may exist in the engine yet not in this snapshot mid update, so the
    /// link layer skips drawing an endpoint it cannot place.
    std::optional<QPointF> getPortPosition(nt::NodeId nodeId, int index, bool isOutput) const;

    /// @brief Replaces every row with the nodes currently in the network.
    void resetFromNetwork();

    /// @brief Appends a row for the node with the given id.
    void addNode(nt::NodeId nodeId);

    /// @brief Removes the row for the node with the given id.
    void removeNode(nt::NodeId nodeId);

    /// @brief Removes every row.
    void clear();

    /// @brief Marks the rows in @p selectedIds selected and the rest unselected.
    void setSelection(const std::vector<nt::NodeId>& selectedIds);

    /// @brief Marks the row matching @p nodeId primary and the rest not.
    void setPrimary(std::optional<nt::NodeId> nodeId);

    /// @brief Marks the row matching @p nodeId the display node and the rest not.
    void setDisplay(std::optional<nt::NodeId> nodeId);

    /// @brief Moves the row matching @p nodeId to a new graph position.
    void setPosition(nt::NodeId nodeId, float x, float y);

    /// @brief Shifts every selected row by a delta, for a live group drag.
    void moveSelectedBy(float dx, float dy);

    /// @brief Returns the graph position of the row matching @p nodeId.
    QPointF getPosition(nt::NodeId nodeId) const;

  private:
    /// One node row, a snapshot of the node's display data.
    struct Node
    {
        nt::NodeId nodeId;
        QString name;
        QString type;
        float x;
        float y;
        int inputPortCount;
        int outputPortCount;
        bool multiInput;
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

    /// @brief Reads the display data for a node into a row.
    static Node makeNode(nt::NodeId nodeId);

    /// @brief Whether a canvas point lies over any node's card.
    bool isOverNodeBody(QPointF canvasPoint) const;

    /// @brief Returns the row index of a node, or -1 when absent.
    int rowOf(nt::NodeId nodeId) const;

    /// @brief Returns the canvas rectangle a node's card covers.
    static QRectF getNodeBody(const Node& node);

    /// @brief Returns the canvas position of one output port on @p node's bottom edge.
    ///
    /// Ports spread evenly along the edge, so one lands at the middle and two land
    /// at the third marks.
    QPointF getOutputPosition(const Node& node, int outputIndex) const;

    /// @brief Returns the canvas position of one input on @p node's top edge.
    ///
    /// Every declared port covers an equal share of the edge. A single port sits at
    /// the center of its share, while the multi input port fills its share as a bar
    /// and spreads @p multiCount connections evenly inside it.
    ///
    /// @note Passing one more than the port holds gives a position at each end of
    /// the bar and one between every adjacent pair, where a new link would insert.
    QPointF getInputPosition(const Node& node, int inputIndex, int multiCount) const;

    /// @brief Returns how many connections @p node's multi input port holds.
    int getMultiInputCount(const Node& node) const;

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
