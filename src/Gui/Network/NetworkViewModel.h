#pragma once

#include "Gui/Network/NodeListModel.h"
#include <QObject>
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

  public:
    explicit NetworkViewModel(QObject* parent = nullptr);

    QAbstractListModel* nodes();

  private:
    NodeListModel nodes_;
    boost::signals2::scoped_connection operatorCreatedConnection_;
    boost::signals2::scoped_connection operatorRemovedConnection_;
    boost::signals2::scoped_connection networkClearedConnection_;
};

} // namespace enzo::ui
