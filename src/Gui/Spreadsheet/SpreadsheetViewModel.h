#pragma once

#include "Gui/Spreadsheet/AttributeTableModel.h"
#include "Gui/Spreadsheet/PrimitiveTreeModel.h"
#include <QAbstractItemModel>
#include <QObject>
#include <boost/signals2/connection.hpp>
#include <memory>

namespace enzo {
class NodePacket;
}

namespace enzo::ui {

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
    Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(int primitiveCount READ primitiveCount NOTIFY geometryChanged)
    Q_PROPERTY(int elementCount READ elementCount NOTIFY tableChanged)
    Q_PROPERTY(QString elementNoun READ elementNoun NOTIFY modeChanged)

  public:
    /// The element class the table shows, ordered to match `attr::AttributeOwner`.
    enum Mode
    {
        Points,
        Vertices,
        Faces,
        Primitive,
    };
    Q_ENUM(Mode)

    explicit SpreadsheetViewModel(QObject* parent = nullptr);

    QAbstractItemModel* tableModel();
    QAbstractItemModel* primitiveTree();

    Mode mode() const;
    void setMode(Mode mode);

    int primitiveCount() const;
    int elementCount() const;
    QString elementNoun() const;

    /// @brief Shows the attributes of the primitive at @p index in the table.
    Q_INVOKABLE void selectPrimitive(int index);

  Q_SIGNALS:
    void modeChanged();
    void geometryChanged();
    void tableChanged();

  private:
    /// @brief Rebuilds the tree from a packet and shows its first primitive.
    void showPacket(std::shared_ptr<const NodePacket> packet);

    AttributeTableModel tableModel_;
    PrimitiveTreeModel primitiveTree_;
    std::shared_ptr<const NodePacket> packet_;
    Mode mode_ = Points;
    boost::signals2::scoped_connection selectedNodesConnection_;
    boost::signals2::scoped_connection selectedGeoConnection_;
};

} // namespace enzo::ui
