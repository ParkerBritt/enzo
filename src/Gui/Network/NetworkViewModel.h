#pragma once

#include "Gui/Network/EdgeListModel.h"
#include "Gui/Network/NodeListModel.h"
#include <QObject>
#include <QRectF>
#include <QVariantList>
#include <QVariantMap>
#include <boost/signals2/connection.hpp>
#include <optional>
#include <vector>

namespace enzo::ui {

/// @brief View-model backing the network editor.
///
/// Subscribes to the engine network signals and republishes the graph as Qt
/// models that QML binds to. This is the boundary that turns the engine boost
/// signals into Qt properties. The node model changes only in response to the
/// engine, never from QML.
class NetworkViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractListModel* nodes READ nodes CONSTANT)
    Q_PROPERTY(QAbstractListModel* edges READ edges CONSTANT)
    Q_PROPERTY(QVariantList nodeTypes READ getNodeTypes CONSTANT)
    Q_PROPERTY(qreal nodeWidth READ getNodeWidth CONSTANT)
    Q_PROPERTY(qreal nodeHeight READ getNodeHeight CONSTANT)

  public:
    explicit NetworkViewModel(QObject* parent = nullptr);

    QAbstractListModel* nodes();

    QAbstractListModel* edges();

    /// @brief Returns the node card width.
    qreal getNodeWidth() const;

    /// @brief Returns the node card height.
    qreal getNodeHeight() const;

    /// @brief Returns every node type the tab menu can create, each a {label, name} map.
    QVariantList getNodeTypes() const;

    /// @brief Creates a node of the given node type at a network position.
    Q_INVOKABLE void createNode(const QString& fullName, qreal x, qreal y);

    /// @brief Creates a node below the primary node, selects it, and connects the two.
    /// @return False when there is no primary node to chain onto.
    Q_INVOKABLE bool chainNodeToPrimary(const QString& fullName);

    /// @brief Selects a node, optionally adding it to the current selection.
    ///
    /// @param additive Toggles the node within the selection rather than
    /// replacing it, the modifier click behaviour.
    Q_INVOKABLE void selectNode(qulonglong nodeId, bool additive);

    /// @brief Selects every node the drag selection box covers.
    ///
    /// @param additive Adds the boxed nodes to the current selection rather than
    /// replacing it, the modifier drag behaviour.
    Q_INVOKABLE void selectNodesInRect(QRectF canvasRect, bool additive);

    /// @brief Deletes every selected node as one undo step.
    Q_INVOKABLE void deleteSelected();

    /// @brief Wires a source node's output into a target node's input.
    Q_INVOKABLE void
    connectNodes(qulonglong sourceNode, int sourceOutput, qulonglong targetNode, int targetInput);

    /// @brief Removes the link at an index in the link model.
    Q_INVOKABLE void removeLink(int linkIndex);

    /// @brief Returns the rewiring that releasing a node drag would perform.
    ///
    /// @param hoveredLink The link the dragged node covers, -1 when it covers none.
    /// @param bypassing Whether to pull the selected nodes out of the graph rather
    /// than drop the dragged node into the link it covers.
    /// @return A {cutLinks, newLinks} map, cutLinks the link model indices the drop
    /// removes and newLinks the {sourceNode, sourceOutput, targetNode, targetInput}
    /// maps it wires in their place. Both are empty when the drop only moves nodes.
    Q_INVOKABLE QVariantMap
    getDropPreview(qulonglong nodeId, int hoveredLink, bool bypassing) const;

    /// @brief Cuts the links a drop preview names and wires its new ones, as one undo
    /// step.
    Q_INVOKABLE void applyDropPreview(const QVariantMap& preview);

    /// @brief Returns the ports the link at an index connects.
    /// @return A {sourceNode, sourceOutput, targetNode, targetInput} map, empty when
    /// the index is out of range.
    Q_INVOKABLE QVariantMap getLinkEndpoints(int linkIndex) const;

    /// @brief Sets the given node as the one whose geometry the viewport shows.
    Q_INVOKABLE void setDisplayNode(qulonglong nodeId);

    /// @brief Sets the primary node as the one whose geometry the viewport shows.
    /// @note Does nothing when there is no primary node.
    Q_INVOKABLE void setDisplayNodeToPrimary();

    /// @brief Moves the selected nodes in the ui, doesn't apply to engine until
    /// committed with commitSelectionMove.
    Q_INVOKABLE void stageSelectionMove(qreal dx, qreal dy);

    /// @brief Commits the staged node positions to the engine as one undo step.
    Q_INVOKABLE void commitSelectionMove();

    /// @brief Undoes the last change.
    Q_INVOKABLE void undo();

    /// @brief Redoes the last undone change.
    Q_INVOKABLE void redo();

    /// @brief Clears the selection.
    Q_INVOKABLE void clearSelection();

  private:
    /// @brief Selects a set of nodes and their primary as one undo step.
    ///
    /// @param primaryId The node leading the selection, empty to keep the current
    /// one.
    void selectNodes(const std::vector<nt::NodeId>& selection, std::optional<nt::NodeId> primaryId);

    /// @brief Returns the preview of dropping a node into the link it covers.
    ///
    /// @note The preview is empty when the node is part of a wider selection, is
    /// already fed by something, has no ports to wire, or is an end of that link.
    QVariantMap getInsertPreview(qulonglong nodeId, int hoveredLink) const;

    /// @brief Returns the preview of pulling the selected nodes out of the graph.
    ///
    /// @note Every link touching the selection is cut, and what fed a node is wired
    /// on to what that node fed.
    QVariantMap getBypassPreview() const;

    NodeListModel nodes_;
    EdgeListModel edges_;
    boost::signals2::scoped_connection nodeCreatedSubscription_;
    boost::signals2::scoped_connection nodeRemovedSubscription_;
    boost::signals2::scoped_connection networkClearedSubscription_;
    boost::signals2::scoped_connection selectedNodesSubscription_;
    boost::signals2::scoped_connection primaryNodeSubscription_;
    boost::signals2::scoped_connection displayNodeSubscription_;
    boost::signals2::scoped_connection nodePositionSubscription_;
    boost::signals2::scoped_connection connectionCreatedSubscription_;
    boost::signals2::scoped_connection connectionRemovedSubscription_;
};

} // namespace enzo::ui
