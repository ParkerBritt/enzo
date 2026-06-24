#pragma once

#include "Gui/Spreadsheet/AttributeTableModel.h"
#include "Gui/Spreadsheet/PrimitiveTreeModel.h"
#include <QAbstractItemModel>
#include <QObject>
#include <boost/signals2/connection.hpp>
#include <memory>

namespace enzo
{
class NodePacket;
}

namespace enzo::ui
{

/// @brief View-model backing the geometry spreadsheet.
///
/// Subscribes to the engine selection signals and republishes the selected
/// node geometry as a Qt table model that QML binds to. This is the boundary
/// that turns the engine boost signals into Qt properties.
class SpreadsheetViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* tableModel READ tableModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel* primitiveTree READ primitiveTree CONSTANT)

  public:
    explicit SpreadsheetViewModel(QObject* parent = nullptr);

    QAbstractItemModel* tableModel();
    QAbstractItemModel* primitiveTree();

    /// @brief Shows the attributes of the primitive at @p index in the table.
    Q_INVOKABLE void selectPrimitive(int index);

  private:
    /// @brief Rebuilds the tree from a packet and shows its first primitive.
    void showPacket(std::shared_ptr<const NodePacket> packet);

    AttributeTableModel tableModel_;
    PrimitiveTreeModel primitiveTree_;
    std::shared_ptr<const NodePacket> packet_;
    boost::signals2::scoped_connection selectedNodesConnection_;
    boost::signals2::scoped_connection selectedGeoConnection_;
};

} // namespace enzo::ui
