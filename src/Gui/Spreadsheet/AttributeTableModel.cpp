#include "Gui/Spreadsheet/AttributeTableModel.h"
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/Primitives/Mesh.h"

namespace enzo::ui {

namespace {

/// @brief Formats a float with a fixed number of decimals for the cell text.
QString formatFloat(float value) { return QString::number(value, 'f', 3); }

/// @brief Builds the column label for one component of an attribute.
///
/// Single component attributes use the bare name. Three component attributes
/// suffix with x y z, anything wider falls back to a bracketed index.
QString componentHeader(const QString& name, unsigned int size, unsigned int component)
{
    if (size == 1) return name;
    if (size == 3)
    {
        static const char* axes[] = {"x", "y", "z"};
        return name + "." + axes[component];
    }
    return name + "[" + QString::number(component) + "]";
}

} // namespace

AttributeTableModel::AttributeTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void AttributeTableModel::setPrimitive(std::shared_ptr<const geo::Primitive> primitive)
{
    std::vector<Column> newColumns = buildColumns(primitive.get(), owner_);
    const bool sameColumns = primitive && primitive_ && newColumns == columns_;

    // A changed attribute set is rare and resets the whole table. The common
    // recook keeps the same columns, so the row count is adjusted and the cells
    // repainted in place rather than tearing the view down, which reads as a flash.
    if (!sameColumns)
    {
        beginResetModel();
        primitive_ = std::move(primitive);
        columns_ = std::move(newColumns);
        endResetModel();
        return;
    }

    const int oldRowCount = rowCount();
    const int newRowCount = rowCountFor(primitive.get(), owner_);
    if (newRowCount > oldRowCount)
    {
        beginInsertRows(QModelIndex(), oldRowCount, newRowCount - 1);
        primitive_ = std::move(primitive);
        endInsertRows();
    }
    else if (newRowCount < oldRowCount)
    {
        beginRemoveRows(QModelIndex(), newRowCount, oldRowCount - 1);
        primitive_ = std::move(primitive);
        endRemoveRows();
    }
    else
    {
        primitive_ = std::move(primitive);
    }
    refreshValues();
}

void AttributeTableModel::setOwner(attr::AttributeOwner owner)
{
    beginResetModel();
    owner_ = owner;
    columns_ = buildColumns(primitive_.get(), owner_);
    endResetModel();
}

void AttributeTableModel::refreshValues()
{
    const int rows = rowCount();
    const int cols = columnCount();
    if (rows == 0 || cols == 0) return;
    Q_EMIT dataChanged(index(0, 0), index(rows - 1, cols - 1), {Qt::DisplayRole, AxisRole});
}

std::vector<AttributeTableModel::Column>
AttributeTableModel::buildColumns(const geo::Primitive* primitive, attr::AttributeOwner owner)
{
    std::vector<Column> columns;
    if (!primitive) return columns;

    const size_t attribCount = primitive->getNumAttributes(owner);
    for (size_t attributeIndex = 0; attributeIndex < attribCount; ++attributeIndex)
    {
        auto attribute = primitive->getAttributeByIndex(owner, attributeIndex).lock();
        if (!attribute) continue;

        const QString name = QString::fromStdString(attribute->getName());
        const unsigned int size = attribute->getTypeSize();
        // Two and three component attributes carry x y z style axis colours.
        const bool axial = size == 2 || size == 3;
        for (unsigned int component = 0; component < size; ++component)
        {
            columns.push_back(
                {static_cast<unsigned int>(attributeIndex),
                 component,
                 axial ? static_cast<int>(component) : -1,
                 componentHeader(name, size, component)}
            );
        }
    }
    return columns;
}

int AttributeTableModel::rowCountFor(const geo::Primitive* primitive, attr::AttributeOwner owner)
{
    if (!primitive) return 0;

    switch (owner)
    {
    case attr::AttributeOwner::POINT:
        return primitive->hasPoints() ? primitive->getNumPoints() : 0;
    case attr::AttributeOwner::VERTEX:
    case attr::AttributeOwner::FACE:
        if (auto mesh = dynamic_cast<const geo::Mesh*>(primitive))
            return owner == attr::AttributeOwner::VERTEX ? mesh->getNumVerts()
                                                         : mesh->getNumFaces();
        return 0;
    case attr::AttributeOwner::PRIMITIVE:
        return 1;
    }
    return 0;
}

int AttributeTableModel::rowCount(const QModelIndex&) const
{
    return rowCountFor(primitive_.get(), owner_);
}

int AttributeTableModel::columnCount(const QModelIndex&) const { return columns_.size(); }

QVariant AttributeTableModel::data(const QModelIndex& index, int role) const
{
    if (!primitive_) return {};
    if (index.column() < 0 || index.column() >= static_cast<int>(columns_.size())) return {};

    const Column& column = columns_[index.column()];
    if (role == AxisRole) return column.axis;
    if (role != Qt::DisplayRole) return {};

    auto attribute = primitive_->getAttributeByIndex(owner_, column.attributeIndex).lock();
    if (!attribute) return {};

    const size_t row = index.row();
    using namespace enzo::attr;
    switch (attribute->getType())
    {
    case AttributeType::intT:
        return static_cast<qlonglong>(AttributeHandleRO<intT>(attribute).getValue(row));
    case AttributeType::floatT:
        return formatFloat(AttributeHandleRO<floatT>(attribute).getValue(row));
    case AttributeType::boolT:
        return AttributeHandleRO<boolT>(attribute).getValue(row) ? "true" : "false";
    case AttributeType::vectorT:
        return formatFloat(AttributeHandleRO<Vector3>(attribute).getValue(row)[column.component]);
    case AttributeType::matrixT:
    {
        const auto& matrix = AttributeHandleRO<Matrix4>(attribute).getValue(row);
        return formatFloat(matrix(column.component / 4, column.component % 4));
    }
    default:
        return {};
    }
}

QVariant AttributeTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) return {};
    if (orientation == Qt::Horizontal)
    {
        if (section < 0 || section >= static_cast<int>(columns_.size())) return {};
        return columns_[section].header;
    }
    return section;
}

QHash<int, QByteArray> AttributeTableModel::roleNames() const
{
    auto names = QAbstractTableModel::roleNames();
    names[AxisRole] = "axis";
    return names;
}

} // namespace enzo::ui
