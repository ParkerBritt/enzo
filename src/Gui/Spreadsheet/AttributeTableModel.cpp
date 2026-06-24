#include "Gui/Spreadsheet/AttributeTableModel.h"
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"

namespace enzo::ui
{

namespace
{

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
    beginResetModel();
    primitive_ = std::move(primitive);
    rebuildColumns();
    endResetModel();
}

void AttributeTableModel::rebuildColumns()
{
    columns_.clear();
    if (!primitive_) return;

    const size_t attribCount = primitive_->getNumAttributes(owner_);
    for (size_t attributeIndex = 0; attributeIndex < attribCount; ++attributeIndex)
    {
        auto attribute = primitive_->getAttributeByIndex(owner_, attributeIndex).lock();
        if (!attribute) continue;

        const QString name = QString::fromStdString(attribute->getName());
        const unsigned int size = attribute->getTypeSize();
        for (unsigned int component = 0; component < size; ++component)
        {
            columns_.push_back(
                {static_cast<unsigned int>(attributeIndex), component,
                 componentHeader(name, size, component)}
            );
        }
    }
}

int AttributeTableModel::rowCount(const QModelIndex&) const
{
    if (!primitive_ || !primitive_->hasPoints()) return 0;
    return primitive_->getNumPoints();
}

int AttributeTableModel::columnCount(const QModelIndex&) const { return columns_.size(); }

QVariant AttributeTableModel::data(const QModelIndex& index, int role) const
{
    if (!primitive_ || role != Qt::DisplayRole) return {};
    if (index.column() < 0 || index.column() >= static_cast<int>(columns_.size())) return {};

    const Column& column = columns_[index.column()];
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

} // namespace enzo::ui
