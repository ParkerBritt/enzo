#pragma once

#include "Gui/Network/EdgeListModel.h"
#include "Gui/Network/NodeListModel.h"
#include <QObject>
#include <QVariantList>
#include <boost/signals2/connection.hpp>

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

  public:
    explicit NetworkViewModel(QObject* parent = nullptr);

    QAbstractListModel* nodes();

    QAbstractListModel* edges();

    /// @brief Returns every operator type the tab menu can create, each a {label, name} map.
    QVariantList getNodeTypes() const;

    /// @brief Creates a node of the given operator type at a network position.
    Q_INVOKABLE void createNode(const QString& internalName, qreal x, qreal y);

    /// @brief Selects a node, optionally adding it to the current selection.
    ///
    /// @param additive Toggles the node within the selection rather than
    /// replacing it, the modifier click behaviour.
    Q_INVOKABLE void selectNode(qulonglong opId, bool additive);

    /// @brief Deletes every selected node as one undo step.
    Q_INVOKABLE void deleteSelected();

    /// @brief Wires a source node's output slot into a target node's input slot.
    Q_INVOKABLE void
    connectNodes(qulonglong sourceOp, int sourceOutput, qulonglong targetOp, int targetInput);

    /// @brief Sets the given node as the one whose geometry the viewport shows.
    Q_INVOKABLE void setDisplayNode(qulonglong opId);

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
    NodeListModel nodes_;
    EdgeListModel edges_;
    boost::signals2::scoped_connection operatorCreatedSubscription_;
    boost::signals2::scoped_connection operatorRemovedSubscription_;
    boost::signals2::scoped_connection networkClearedSubscription_;
    boost::signals2::scoped_connection selectedNodesSubscription_;
    boost::signals2::scoped_connection primaryNodeSubscription_;
    boost::signals2::scoped_connection displayNodeSubscription_;
    boost::signals2::scoped_connection nodePositionSubscription_;
    boost::signals2::scoped_connection connectionCreatedSubscription_;
    boost::signals2::scoped_connection connectionRemovedSubscription_;
};

} // namespace enzo::ui
