#include "Gui/GeometrySpreadsheetPanel/AttributeSpreadsheetModel.h"
#include "Engine/Attribute/Attribute.h"
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/Primitives/Mesh.h"
#include <QBrush>
#include <icecream.hpp>
#include <memory>
#include <qnamespace.h>
#include <stdexcept>
#include <string>

AttributeSpreadsheetModel::AttributeSpreadsheetModel(QObject* parent) : QAbstractListModel(parent)
{
}

void AttributeSpreadsheetModel::primitiveChanged(
    std::shared_ptr<const enzo::geo::Primitive> primitive
)
{
    beginResetModel();
    primitive_ = std::move(primitive);
    initBuffers();

    endResetModel();
}

void AttributeSpreadsheetModel::clear() { primitiveChanged(nullptr); }

void AttributeSpreadsheetModel::initBuffers()
{
    attribSizes_.clear();
    sectionAttribMap_.clear();

    if (!primitive_) return;

    const auto attribCount = primitive_->getNumAttributes(attributeOwner_);
    attribSizes_.reserve(attribCount);

    for (size_t i = 0; i < attribCount; ++i)
    {
        if (auto attrib = primitive_->getAttributeByIndex(attributeOwner_, i).lock())
        {
            const auto size = attrib->getTypeSize();
            attribSizes_.push_back(size);

            for (size_t j = 0; j < size; ++j)
            {
                sectionAttribMap_.push_back(i);
            }
        }
    }
}

void AttributeSpreadsheetModel::setOwner(const enzo::attr::AttributeOwner owner)
{
    beginResetModel();
    attributeOwner_ = owner;
    initBuffers();
    endResetModel();
}

int AttributeSpreadsheetModel::rowCount(const QModelIndex& parent) const
{
    if (!primitive_) return 0;

    switch (attributeOwner_)
    {
    case enzo::attr::AttributeOwner::POINT:
    {
        if (primitive_->hasPoints()) return primitive_->getNumPoints();
        return 0;
    }
    case enzo::attr::AttributeOwner::VERTEX:
    case enzo::attr::AttributeOwner::FACE:
    {
        if (auto mesh = std::dynamic_pointer_cast<const enzo::geo::Mesh>(primitive_))
        {
            if (attributeOwner_ == enzo::attr::AttributeOwner::VERTEX)
                return mesh->getNumVerts();
            else
                return mesh->getNumFaces();
        }
        return 0;
    }
    case enzo::attr::AttributeOwner::PRIMITIVE:
    {
        return 1;
    }
    }
    return 0;
}

int AttributeSpreadsheetModel::columnCount(const QModelIndex& parent) const
{
    if (!primitive_) return attributeColumnPadding_;
    // padding column for indices, then attribute columns, then group columns
    return attributeColumnPadding_ + sectionAttribMap_.size() +
           primitive_->getNumGroups(attributeOwner_);
}

QVariant AttributeSpreadsheetModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || !primitive_)
    {
        return QVariant();
    }

    if (role == Qt::BackgroundRole && index.column() == 0)
    {
        return QBrush("#1B1B1B");
    }
    else if (role == Qt::DisplayRole)
    {

        if (index.column() == 0)
        {
            switch (attributeOwner_)
            {
            case enzo::attr::AttributeOwner::POINT:
            case enzo::attr::AttributeOwner::FACE:
            {
                return index.row();
            }
            case enzo::attr::AttributeOwner::VERTEX:
            {
                if (auto mesh = std::dynamic_pointer_cast<const enzo::geo::Mesh>(primitive_))
                {
                    const enzo::Offset faceOffset = mesh->getVertexFace(index.row());
                    const enzo::Offset startVert = mesh->getFaceStartVertex(faceOffset);
                    const enzo::Offset vertexNumber = index.row() - startVert;
                    return QString::fromStdString(
                        std::to_string(faceOffset) + ":" + std::to_string(vertexNumber)
                    );
                }
                return index.row();
            }
            case enzo::attr::AttributeOwner::PRIMITIVE:
            {
                return "primitive";
            }
            }
        }
        const int section = index.column() - attributeColumnPadding_;
        if (section >= static_cast<int>(sectionAttribMap_.size()))
        {
            // Group columns sit after all attribute columns
            const unsigned int groupIndex = section - sectionAttribMap_.size();
            if (auto group = primitive_->getGroupByIndex(attributeOwner_, groupIndex).lock())
            {
                const auto groupHandle = enzo::attr::AttributeHandleRO<enzo::boolT>(group);
                return groupHandle.getValue(index.row()) ? "true" : "false";
            }
            throw std::runtime_error("Couldn't lock group");
        }

        int attributeIndex = indexFromSection(section);
        if (std::shared_ptr<const enzo::attr::Attribute> attrib =
                primitive_->getAttributeByIndex(attributeOwner_, attributeIndex).lock())
        {
            const unsigned int valueIndex =
                index.column() - attributeIndex - attributeColumnPadding_;
            using namespace enzo::attr;

            switch (attrib->getType())
            {
            case (AttributeType::intT):
            {
                const auto attribHandle = enzo::attr::AttributeHandleRO<enzo::intT>(attrib);
                return static_cast<float>(attribHandle.getValue(index.row()));
            }
            case (AttributeType::floatT):
            {
                const auto attribHandle = enzo::attr::AttributeHandleRO<enzo::floatT>(attrib);
                return attribHandle.getValue(index.row());
            }
            case (AttributeType::boolT):
            {
                const auto attribHandle = enzo::attr::AttributeHandleRO<enzo::boolT>(attrib);
                return attribHandle.getValue(index.row()) ? "true" : "false";
            }
            case (AttributeType::vectorT):
            {
                const auto attribHandle = enzo::attr::AttributeHandleRO<enzo::Vector3>(attrib);
                return attribHandle.getValue(index.row())[valueIndex];
            }
            case (AttributeType::matrixT):
            {
                const auto attribHandle = enzo::attr::AttributeHandleRO<enzo::Matrix4>(attrib);
                const auto& mat = attribHandle.getValue(index.row());
                return mat(valueIndex / 4, valueIndex % 4);
            }
            default:
            {
                return "Failed";
                break;
            }
            }
        }
        else
        {
            throw std::runtime_error("Couldn't lock attribute");
        }
    }
    else
    {
        return QVariant();
    }
}

int AttributeSpreadsheetModel::indexFromSection(unsigned int section) const
{
    if (section >= sectionAttribMap_.size())
    {
        throw std::out_of_range(
            "Section is out of range of sectionAttributMap_, value: " + std::to_string(section) +
            " expected: <" + std::to_string(sectionAttribMap_.size())
        );
    }
    return sectionAttribMap_[section];
}

QVariant
AttributeSpreadsheetModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) return QVariant();

    if (orientation == Qt::Horizontal)
    {
        if (section == 0) return "Index";
        if (!primitive_) return QVariant();

        const int sectionIndex = section - attributeColumnPadding_;
        if (sectionIndex >= static_cast<int>(sectionAttribMap_.size()))
        {
            const unsigned int groupIndex = sectionIndex - sectionAttribMap_.size();
            if (auto group = primitive_->getGroupByIndex(attributeOwner_, groupIndex).lock())
            {
                return QString::fromStdString("group: " + group->getName());
            }
            throw std::runtime_error("failed to lock group index");
        }

        auto attributeIndex = indexFromSection(sectionIndex);
        if (auto attrib = primitive_->getAttributeByIndex(attributeOwner_, attributeIndex).lock())
        {
            if (attribSizes_[attributeIndex] > 1)
            {
                const unsigned int valueIndex = section - attributeIndex - attributeColumnPadding_;

                std::string valueIndexString;
                if (attrib->getType() == enzo::attr::AttrType::vectorT)
                {
                    valueIndexString = std::array{".x", ".y", ".z", ".w"}.at(valueIndex);
                }
                else
                {
                    valueIndexString = "[" + std::to_string(valueIndex) + "]";
                }

                return QString::fromStdString(attrib->getName() + valueIndexString);
            }
            else
            {
                return QString::fromStdString(attrib->getName());
            }
        }
        else
        {
            throw std::runtime_error("failed to lock attriubte index");
        }
    }
    else
    {
        return QStringLiteral("Row %1").arg(section);
    }
}
