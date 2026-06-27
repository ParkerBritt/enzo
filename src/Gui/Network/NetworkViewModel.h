#pragma once

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
    Q_PROPERTY(QVariantList nodeTypes READ getNodeTypes CONSTANT)

  public:
    explicit NetworkViewModel(QObject* parent = nullptr);

    QAbstractListModel* nodes();

    /// @brief Returns every operator type the tab menu can create, each a {label, name} map.
    QVariantList getNodeTypes() const;

    /// @brief Creates a node of the given operator type at a network position.
    Q_INVOKABLE void createNode(const QString& internalName, qreal x, qreal y);

    /// @brief Selects a node, optionally adding it to the current selection.
    ///
    /// @param additive Toggles the node within the selection rather than
    /// replacing it, the modifier click behaviour.
    Q_INVOKABLE void selectNode(qulonglong opId, bool additive);

    /// @brief Clears the selection.
    Q_INVOKABLE void clearSelection();

  private:
    NodeListModel nodes_;
    boost::signals2::scoped_connection operatorCreatedConnection_;
    boost::signals2::scoped_connection operatorRemovedConnection_;
    boost::signals2::scoped_connection networkClearedConnection_;
    boost::signals2::scoped_connection selectedNodesConnection_;
    boost::signals2::scoped_connection primaryNodeConnection_;
};

} // namespace enzo::ui
