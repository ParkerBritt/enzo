#include "Engine/Operator/Geometry.h"
#include "Engine/Operator/Attribute.h"
#include "Engine/Operator/AttributeHandle.h"
#include "Engine/Types.h"
#include <memory>
#include <oneapi/tbb/spin_mutex.h>
#include <oneapi/tbb/task_group.h>
#include <stdexcept>
#include <string>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include "icecream.hpp"

using namespace enzo;
geo::Geometry::Geometry() :
    vertexCountHandlePrim_{addIntAttribute(ga::AttrOwner::PRIMITIVE, "vertexCount")},
    closedHandlePrim_{addBoolAttribute(ga::AttrOwner::PRIMITIVE, "closed")},
    pointOffsetHandleVert_{addIntAttribute(ga::AttrOwner::VERTEX, "point")},
    posHandlePoint_{addVector3Attribute(ga::AttrOwner::POINT, "P")}
{
    
}

geo::Geometry::Geometry(const Geometry& other):
    // attributes
    pointAttributes_{deepCopyAttributes(other.pointAttributes_)},
    vertexAttributes_{deepCopyAttributes(other.vertexAttributes_)},
    primitiveAttributes_{deepCopyAttributes(other.primitiveAttributes_)},
    globalAttributes_{deepCopyAttributes(other.globalAttributes_)},

    // handles
    vertexCountHandlePrim_{enzo::ga::AttributeHandleInt(getAttribByName(ga::AttrOwner::PRIMITIVE, "vertexCount"))},
    closedHandlePrim_{enzo::ga::AttributeHandleBool(getAttribByName(ga::AttrOwner::PRIMITIVE, "closed"))},
    pointOffsetHandleVert_{enzo::ga::AttributeHandleInt(getAttribByName(ga::AttrOwner::VERTEX, "point"))},
    posHandlePoint_{enzo::ga::AttributeHandleVector3(getAttribByName(ga::AttrOwner::POINT, "P"))},

    // other
    soloPoints_{other.soloPoints_},
    vertexPrims_{other.vertexPrims_},
    primStarts_{other.primStarts_},
    primStartsDirty_{other.primStartsDirty_.load()}
{
}

enzo::geo::Geometry& enzo::geo::Geometry::operator=(const enzo::geo::Geometry& rhs) {
    if (this == &rhs) return *this;

    // attributes
    pointAttributes_      = deepCopyAttributes(rhs.pointAttributes_);
    vertexAttributes_     = deepCopyAttributes(rhs.vertexAttributes_);
    primitiveAttributes_  = deepCopyAttributes(rhs.primitiveAttributes_);
    globalAttributes_     = deepCopyAttributes(rhs.globalAttributes_);

    // handles
    vertexCountHandlePrim_ = enzo::ga::AttributeHandleInt(getAttribByName(ga::AttrOwner::PRIMITIVE, "vertexCount"));
    closedHandlePrim_ = enzo::ga::AttributeHandleBool(getAttribByName(ga::AttrOwner::PRIMITIVE, "closed"));
    pointOffsetHandleVert_ = enzo::ga::AttributeHandleInt(getAttribByName(ga::AttrOwner::VERTEX, "point"));
    posHandlePoint_ = enzo::ga::AttributeHandleVector3(getAttribByName(ga::AttrOwner::POINT, "P"));

    // other
    soloPoints_           = rhs.soloPoints_;
    vertexPrims_          = rhs.vertexPrims_;
    primStarts_           = rhs.primStarts_;

    primStartsDirty_.store(rhs.primStartsDirty_.load());

    return *this;
}

void geo::Geometry::addFace(const std::vector<ga::Offset>& pointOffsets, bool closed)
{
    const ga::Offset primNum = vertexCountHandlePrim_.getSize();
    for(ga::Offset pointOffset : pointOffsets)
    {
        pointOffsetHandleVert_.addValue(pointOffset);
        vertexPrims_.push_back(primNum);
        soloPoints_.erase(pointOffset);
    }
    vertexCountHandlePrim_.addValue(pointOffsets.size());
    closedHandlePrim_.addValue(closed);
    
}

void geo::Geometry::addPoint(const bt::Vector3& pos)
{
    posHandlePoint_.addValue(pos);
    soloPoints_.emplace(posHandlePoint_.getSize()-1);
}

ga::Offset geo::Geometry::getNumSoloPoints() const
{
    return soloPoints_.size();

}


// enzo::geo::attributeIterator geo::Geometry::attributesBegin(const ga::AttributeOwner owner)
// {
//     return getAttributeStore(owner).begin();

// }

// enzo::geo::attributeIterator geo::Geometry::attributesEnd(const ga::AttributeOwner owner)
// {
//     return getAttributeStore(owner).end();
// }

const size_t geo::Geometry::getNumAttributes(const ga::AttributeOwner owner) const
{
    return getAttributeStore(owner).size();
}

std::weak_ptr<const ga::Attribute> geo::Geometry::getAttributeByIndex(ga::AttributeOwner owner, unsigned int index) const
{
    auto attribStore = getAttributeStore(owner);
    if(index>=attribStore.size())
    {
        throw std::out_of_range("Attribute index out of range: " + std::to_string(index) + " max size: " + std::to_string(attribStore.size()) + "\n");
    }
    return getAttributeStore(owner)[index];

}





std::set<ga::Offset>::const_iterator geo::Geometry::soloPointsBegin()
{
    return soloPoints_.begin();
}

std::set<ga::Offset>::const_iterator geo::Geometry::soloPointsEnd()
{
    return soloPoints_.end();
}



bt::Vector3 geo::Geometry::getPosFromVert(ga::Offset vertexOffset) const
{
    // get point offset
    const ga::Offset pointOffset = pointOffsetHandleVert_.getValue(vertexOffset);
    // get value at point offset
    return posHandlePoint_.getValue(pointOffset);
}

bt::Vector3 geo::Geometry::getPointPos(ga::Offset pointOffset) const
{
    return posHandlePoint_.getValue(pointOffset);
}

ga::Offset      geo::Geometry::getVertexPrim(ga::Offset vertexOffset) const
{
    return vertexPrims_[vertexOffset];
}

void geo::Geometry::setPointPos(const ga::Offset offset, const bt::Vector3& pos)
{
    posHandlePoint_.setValue(offset, pos);
}


unsigned int geo::Geometry::getPrimVertCount(ga::Offset primOffset) const
{
    return vertexCountHandlePrim_.getValue(primOffset);
}

ga::Offset geo::Geometry::getNumPrims() const
{
    return vertexCountHandlePrim_.getSize();
}

ga::Offset geo::Geometry::getNumVerts() const
{
    return pointOffsetHandleVert_.getSize();
}

ga::Offset geo::Geometry::getNumPoints() const
{
    return posHandlePoint_.getSize();
}




geo::Geometry::attribVector geo::Geometry::deepCopyAttributes(attribVector originalVector)
{
    geo::Geometry::attribVector copied;
    const size_t sourceSize = originalVector.size();

    copied.reserve(sourceSize);

    for(const std::shared_ptr<ga::Attribute> sourceAttrib : originalVector)
    {
        if(sourceAttrib)
        {
            copied.push_back(std::make_shared<ga::Attribute>(*sourceAttrib));
        }
        else
        {
            copied.push_back(nullptr);
        }
    }

    return copied;

}


enzo::geo::HeMesh geo::Geometry::computeHalfEdgeMesh()
{
    HeMesh heMesh;

    std::shared_ptr<enzo::ga::Attribute> PAttr = getAttribByName(enzo::ga::AttrOwner::POINT, "P");
    enzo::ga::AttributeHandleVector3 PAttrHandle = enzo::ga::AttributeHandleVector3(PAttr);
    auto pointPositions = PAttrHandle.getAllValues();
    
    std::shared_ptr<enzo::ga::Attribute> pointAttr = getAttribByName(enzo::ga::AttrOwner::VERTEX, "point");
    enzo::ga::AttributeHandleInt pointAttrHandle = enzo::ga::AttributeHandleInt(pointAttr);
    auto vertexPointIndices = pointAttrHandle.getAllValues();

    std::shared_ptr<enzo::ga::Attribute> vertexCountAttr = getAttribByName(enzo::ga::AttrOwner::PRIMITIVE, "vertexCount");
    enzo::ga::AttributeHandleInt vertexCountHandle = enzo::ga::AttributeHandleInt(vertexCountAttr);
    auto vertexCounts = vertexCountHandle.getAllValues();

    int vertexIndex = 0;
    std::vector<geo::vertexDescriptor> createdPoints;
    createdPoints.reserve(pointPositions.size());
    std::vector<geo::vertexDescriptor> facePoints;
    facePoints.reserve(16);

    for(auto pointPos : pointPositions)
    {
        enzo::geo::vertexDescriptor point = heMesh.add_vertex(geo::Point(pointPos.x(), pointPos.y(), pointPos.z()));
        createdPoints.push_back(point);
    }

    CGAL::Polygon_mesh_processing::orient(heMesh);

    // iterate through each prim
    for(int primIndx=0; primIndx<vertexCounts.size(); ++primIndx)
    {
        facePoints.clear();

        // represents how many vertices are in a primitive
        auto vertexCount = vertexCounts[primIndx];

        // create primtive vertices
        for(int i=0; i<vertexCount; ++i)
        {
            auto pointIndex = vertexPointIndices.at(vertexIndex);
            facePoints.push_back(createdPoints[pointIndex]);
            ++vertexIndex;
        }

        // debug
        std::cout << "Primitive " << primIndx << " has " << vertexCount << " vertices: ";
        for (auto& v : facePoints)
        {
            auto pt = heMesh.point(v);
            std::cout << "(" << pt.x() << ", " << pt.y() << ", " << pt.z() << ") ";
        }
        std::cout << std::endl;

        std::cout << "Point indices: ";
        for (int i = 0; i < vertexCount; ++i) {
            int pointIndex = vertexPointIndices.at(vertexIndex - vertexCount + i);
            std::cout << pointIndex << " ";
        }
        std::cout << std::endl;
        // debug

        auto face = heMesh.add_face(facePoints);
        if (face != HeMesh::null_face()) {
            // validFaceIndices.push_back(enzo::geo::F_index(primIndx));
        } else {
            // throw std::runtime_error("Warning: Face creation failed at primitive " + std::to_string(primIndx));
        }
    }


    return heMesh;
}

ga::Offset geo::Geometry::getPrimStartVertex(ga::Offset primOffset) const
{

    if(primStartsDirty_.load())
    {
        tbb::spin_mutex::scoped_lock lock(primStartsMutex_); //lock
        if(primStartsDirty_.load()) // double check
        {
            computePrimStartVertices();
        }
    }
    return primStarts_[primOffset];
}

// TODO: handle this automatically
void geo::Geometry::computePrimStartVertices() const
{
    const ga::Offset handleSize = vertexCountHandlePrim_.getSize();
    primStarts_.clear();
    primStarts_.reserve(handleSize);
    bt::intT primStart = 0;
    for(size_t i=0; i<handleSize; ++i)
    {
        primStarts_.push_back(primStart);
        primStart += vertexCountHandlePrim_.getValue(i);
    }
    primStartsDirty_.store(false);
}




ga::AttributeHandleInt geo::Geometry::addIntAttribute(ga::AttributeOwner owner, std::string name)
{
    auto newAttribute = std::make_shared<ga::Attribute>(name, ga::AttrType::intT);
    getAttributeStore(owner).push_back(newAttribute);
    return ga::AttributeHandleInt(newAttribute);
}

ga::AttributeHandleBool geo::Geometry::addBoolAttribute(ga::AttributeOwner owner, std::string name)
{
    auto newAttribute = std::make_shared<ga::Attribute>(name, ga::AttrType::boolT);
    getAttributeStore(owner).push_back(newAttribute);
    return ga::AttributeHandleBool(newAttribute);
}

bt::boolT geo::Geometry::isClosed(ga::Offset primOffset) const
{
    return closedHandlePrim_.getValue(primOffset);
}


ga::AttributeHandle<bt::Vector3> geo::Geometry::addVector3Attribute(ga::AttributeOwner owner, std::string name)
{
    auto newAttribute = std::make_shared<ga::Attribute>(name, ga::AttrType::vectorT);
    getAttributeStore(owner).push_back(newAttribute);
    return ga::AttributeHandle<bt::Vector3>(newAttribute);
}

geo::Geometry::attribVector& geo::Geometry::getAttributeStore(const ga::AttributeOwner& owner)
{
    switch(owner)
    {
        case ga::AttributeOwner::POINT:
            return pointAttributes_;
            break;
        case ga::AttributeOwner::VERTEX:
            return vertexAttributes_;
            break;
        case ga::AttributeOwner::PRIMITIVE:
            return primitiveAttributes_;
            break;
        case ga::AttributeOwner::GLOBAL:
            return globalAttributes_;
            break;
        default:
            throw std::runtime_error("Unexpected, owner could not be found");
    }
}

const geo::Geometry::attribVector& geo::Geometry::getAttributeStore(const ga::AttributeOwner& owner) const
{
    switch(owner)
    {
        case ga::AttributeOwner::POINT:
            return pointAttributes_;
            break;
        case ga::AttributeOwner::VERTEX:
            return vertexAttributes_;
            break;
        case ga::AttributeOwner::PRIMITIVE:
            return primitiveAttributes_;
            break;
        case ga::AttributeOwner::GLOBAL:
            return globalAttributes_;
            break;
        default:
            throw std::runtime_error("Unexpected, owner could not be found");
    }
}

std::shared_ptr<ga::Attribute> geo::Geometry::getAttribByName(ga::AttributeOwner owner, std::string name)
{
    auto& vector = getAttributeStore(owner);
    for(auto it=vector.begin(); it!=vector.end(); ++it)
    {
        if((*it)->getName()==name)
        {
            return (*it);
        }
    }
    return nullptr;
}

