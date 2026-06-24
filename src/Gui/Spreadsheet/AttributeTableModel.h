#pragma once

#include "Engine/Attribute/Attribute.h"
#include "Engine/Primitives/Primitive.h"
#include <QAbstractTableModel>
#include <memory>
#include <vector>

namespace enzo::ui
{

/// @brief Geometry attributes of one primitive laid out as a table.
///
/// Rows are the elements of a single owner class and columns are the flattened
/// components of each attribute, so a vector attribute such as `P` spans three
/// columns `P.x P.y P.z`. The element index rides in the vertical header.
///
/// @note Only the point owner is exposed for now. Vertices, faces, and
/// primitives follow once the spreadsheet mode control lands.
class AttributeTableModel : public QAbstractTableModel
{
    Q_OBJECT

  public:
    explicit AttributeTableModel(QObject* parent = nullptr);

    /// @brief Points the table at a new primitive and refreshes the view.
    void setPrimitive(std::shared_ptr<const geo::Primitive> primitive);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

  private:
    /// One displayed column, tracing back to a component of one attribute.
    struct Column
    {
        unsigned int attributeIndex;
        unsigned int component;
        QString header;
    };

    void rebuildColumns();

    std::shared_ptr<const geo::Primitive> primitive_;
    attr::AttributeOwner owner_ = attr::AttributeOwner::POINT;
    std::vector<Column> columns_;
};

} // namespace enzo::ui
