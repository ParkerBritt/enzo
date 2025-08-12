#include "Gui/GeometrySpreadsheetPanel/GeometrySpreadsheetModel.h"
#include "Engine/Operator/Attribute.h"
#include "Engine/Operator/AttributeHandle.h"
#include "Engine/Operator/Geometry.h"
#include "Engine/Types.h"
#include <icecream.hpp>
#include <memory>
#include <qnamespace.h>
#include <stdexcept>
#include <string>
#include <QBrush>


GeometrySpreadsheetModel::GeometrySpreadsheetModel(QObject *parent)
: QAbstractListModel(parent)
{

}

void GeometrySpreadsheetModel::geometryChanged(enzo::geo::Geometry& geometry)
{
    beginResetModel();
    geometry_ = geometry;
    initBuffers();

    endResetModel();
}

void GeometrySpreadsheetModel::initBuffers()
{
    // get sizes
    const auto attribCount = geometry_.getNumAttributes(attributeOwner_);

    attribSizes_.clear();
    sectionAttribMap_.clear();
    attribSizes_.reserve(attribCount);

    for(size_t i=0; i<attribCount; ++i)
    {
        if(auto attrib = geometry_.getAttributeByIndex(attributeOwner_, i).lock())
        {
            const auto size = attrib->getTypeSize();
            attribSizes_.push_back(size);

            for(size_t j=0; j<size; ++j)
            {
                sectionAttribMap_.push_back(i);
            }
        }

    }

}


void GeometrySpreadsheetModel::setOwner(const enzo::ga::AttributeOwner owner)
{
    beginResetModel();
    attributeOwner_ = owner;
    initBuffers();
    endResetModel();
}

int GeometrySpreadsheetModel::rowCount(const QModelIndex &parent) const
{
    switch(attributeOwner_)
    {
        case enzo::ga::AttributeOwner::POINT:
        {
            return geometry_.getNumPoints();
        }
        case enzo::ga::AttributeOwner::VERTEX:
        {
            return geometry_.getNumVerts();
        }
        case enzo::ga::AttributeOwner::PRIMITIVE:
        {
            return geometry_.getNumPrims();
        }
        case enzo::ga::AttributeOwner::GLOBAL:
        {
            return 1;
        }

    }
    return 1;
}

int GeometrySpreadsheetModel::columnCount(const QModelIndex &parent) const
{

    int columnCount = attributeColumnPadding_; // first column is for indices
    for(auto size : attribSizes_)
    {
        columnCount += size;
    }

    return columnCount;
}



QVariant GeometrySpreadsheetModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    // TODO: reimplement check
    // if (index.row() >= geometry_.getNumPoints())
    // {
    //     return QVariant();
    // }

    if(role == Qt::BackgroundRole && index.column()==0)
    {
        return QBrush("#1B1B1B");

    }
    else if (role == Qt::DisplayRole)
    {

        if(index.column()==0)
        {
            switch(attributeOwner_)
            {
                case enzo::ga::AttributeOwner::POINT:
                case enzo::ga::AttributeOwner::PRIMITIVE:
                {
                    return index.row();
                }
                case enzo::ga::AttributeOwner::VERTEX:
                {
                    const enzo::ga::Offset primOffset = geometry_.getVertexPrim(index.row());
                    const enzo::ga::Offset startVert = geometry_.getPrimStartVertex(primOffset);
                    const enzo::ga::Offset vertexNumber = index.row()-startVert;
                    return QString::fromStdString(std::to_string(primOffset)+":"+std::to_string(vertexNumber));

                }
                case enzo::ga::AttributeOwner::GLOBAL:
                {
                    return "global";
                }

            }
        }
        int attributeIndex = indexFromSection(index.column()-attributeColumnPadding_);
        if(std::shared_ptr<const enzo::ga::Attribute> attrib = geometry_.getAttributeByIndex(attributeOwner_, attributeIndex).lock())
        {
            const unsigned int valueIndex = index.column()-attributeIndex-attributeColumnPadding_;
            using namespace enzo::ga;

            switch(attrib->getType())
            {
                case(AttributeType::intT):
                {
                    const auto attribHandle = enzo::ga::AttributeHandleRO<enzo::bt::intT>(attrib);
                    return static_cast<float>(attribHandle.getValue(index.row()));
                }
                case(AttributeType::floatT):
                {
                    const auto attribHandle = enzo::ga::AttributeHandleRO<enzo::bt::floatT>(attrib);
                    return attribHandle.getValue(index.row());
                }
                case(AttributeType::boolT):
                {
                    const auto attribHandle = enzo::ga::AttributeHandleRO<enzo::bt::boolT>(attrib);
                    return attribHandle.getValue(index.row()) ? "true" : "false";
                }
                case(AttributeType::vectorT):
                {
                    const auto attribHandle = enzo::ga::AttributeHandleRO<enzo::bt::Vector3>(attrib);
                    return attribHandle.getValue(index.row())[valueIndex];
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
        // return geometry_.getPointPos(index.row()).x();
    }
    else
    {
        return QVariant();
    }
}

int GeometrySpreadsheetModel::indexFromSection(unsigned int section) const
{
    if(section>=sectionAttribMap_.size())
    {
        throw std::out_of_range("Section is out of range of sectionAttributMap_, value: " + std::to_string(section) + " expected: <"+std::to_string(sectionAttribMap_.size()));
    }
    return sectionAttribMap_[section];
}


QVariant GeometrySpreadsheetModel::headerData(int section, Qt::Orientation orientation,
                                     int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        if(section==0) return "Index";
        auto attributeIndex = indexFromSection(section-attributeColumnPadding_);
        if(auto attrib = geometry_.getAttributeByIndex(attributeOwner_, attributeIndex).lock())
        {
            if(attribSizes_[attributeIndex]>1)
            {
                const unsigned int valueIndex = section-attributeIndex-attributeColumnPadding_;

                std::string valueIndexString;
                if(attrib->getType()==enzo::ga::AttrType::vectorT)
                {
                    valueIndexString = std::array{".x", ".y", ".z", ".w"}.at(valueIndex);
                }
                else
                {
                    valueIndexString = "["+std::to_string(valueIndex)+"]";
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
