#include "Engine/Operator/Primitive.h"
#include "Engine/Operator/Attribute.h"
#include "Engine/Operator/AttributeHandle.h"
#include "Engine/Types.h"
#include <memory>
#include <tbb/spin_mutex.h>
#include <tbb/task_group.h>
#include <stdexcept>
#include <string>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include "icecream.hpp"

using namespace enzo;
geo::Primitive::Primitive() :
    vertexCountHandlePrim_{addIntAttribute(ga::AttrOwner::PRIMITIVE, "vertexCount", true)},
    closedHandlePrim_{addBoolAttribute(ga::AttrOwner::PRIMITIVE, "closed", true)},
    pointOffsetHandleVert_{addIntAttribute(ga::AttrOwner::VERTEX, "point", true)},
    posHandlePoint_{addVector3Attribute(ga::AttrOwner::POINT, "P", true)}
{
    addIntAttribute(ga::AttrOwner::PRIMITIVE, "foo");
}

geo::Primitive::Primitive(const Primitive& other):
    // attributes
    pointAttributes_{deepCopyAttributes(other.pointAttributes_)},
    vertexAttributes_{deepCopyAttributes(other.vertexAttributes_)},
    primitiveAttributes_{deepCopyAttributes(other.primitiveAttributes_)},
    globalAttributes_{deepCopyAttributes(other.globalAttributes_)},

    // handles
    vertexCountHandlePrim_{enzo::ga::AttributeHandleInt(getAttribByName(ga::AttrOwner::PRIMITIVE, "vertexCount", true))},
    closedHandlePrim_{enzo::ga::AttributeHandleBool(getAttribByName(ga::AttrOwner::PRIMITIVE, "closed", true))},
    pointOffsetHandleVert_{enzo::ga::AttributeHandleInt(getAttribByName(ga::AttrOwner::VERTEX, "point", true))},
    posHandlePoint_{enzo::ga::AttributeHandleVector3(getAttribByName(ga::AttrOwner::POINT, "P", true))},

    // other
    path_{other.path_},
    soloPoints_{other.soloPoints_},
    vertexPrims_{other.vertexPrims_},
    primStarts_{other.primStarts_},
    primStartsDirty_{other.primStartsDirty_.load()}
{
}

enzo::geo::Primitive& enzo::geo::Primitive::operator=(const enzo::geo::Primitive& rhs) {
    if (this == &rhs) return *this;

    // attributes
    pointAttributes_      = deepCopyAttributes(rhs.pointAttributes_);
    vertexAttributes_     = deepCopyAttributes(rhs.vertexAttributes_);
    primitiveAttributes_  = deepCopyAttributes(rhs.primitiveAttributes_);
    globalAttributes_     = deepCopyAttributes(rhs.globalAttributes_);

    // handles
    vertexCountHandlePrim_ = enzo::ga::AttributeHandleInt(getAttribByName(ga::AttrOwner::PRIMITIVE, "vertexCount", true));
    closedHandlePrim_ = enzo::ga::AttributeHandleBool(getAttribByName(ga::AttrOwner::PRIMITIVE, "closed", true));
    pointOffsetHandleVert_ = enzo::ga::AttributeHandleInt(getAttribByName(ga::AttrOwner::VERTEX, "point", true));
    posHandlePoint_ = enzo::ga::AttributeHandleVector3(getAttribByName(ga::AttrOwner::POINT, "P", true));

    // other
    path_                 = rhs.path_;
    soloPoints_           = rhs.soloPoints_;
    vertexPrims_          = rhs.vertexPrims_;
    primStarts_           = rhs.primStarts_;

    primStartsDirty_.store(rhs.primStartsDirty_.load());

    return *this;
}

void geo::Primitive::mergeAppend(std::shared_ptr<ga::Attribute> dst, std::shared_ptr<ga::Attribute> src)
{
    if(!dst) throw std::runtime_error("Dst empty.");
    if(!src) throw std::runtime_error("Src empty.");

    auto dstType = dst->getType();
    auto srcType = src->getType();

    if(dstType != srcType) throw std::runtime_error("mergeAppend type missmatch.");

    switch(srcType)
    {
        case ga::AttributeType::intT:
            mergeAppendImpl<bt::intT>(dst, src);  
            break;
        case ga::AttributeType::floatT:
            mergeAppendImpl<bt::floatT>(dst, src);  
            break;
        case ga::AttributeType::listT:
            break;
        case ga::AttributeType::vectorT:
            // mergeAppendImpl<bt::vector3>(dst, src);  
            break;
        case ga::AttributeType::boolT:
            mergeAppendImpl<bt::boolT>(dst, src);  
            break;
        default:
            throw std::runtime_error("mergeAppend: Attribute type not accounted for.");

    }
}

void geo::Primitive::merge(Primitive& other)
{
    // Copy all unique points and build offset mapping
    const ga::Offset srcPointNum = other.getNumPoints();
    std::vector<ga::Offset> pointMapping(srcPointNum);
    for(ga::Offset pointOffset=0; pointOffset<srcPointNum; ++pointOffset)
    {
        pointMapping[pointOffset] = addPoint(other.getPointPos(pointOffset));
    }

    // Create prims using mapped point offsets
    const ga::Offset srcPrimNum = other.getNumPrims();
    for(ga::Offset primOffset=0; primOffset<srcPrimNum; ++primOffset)
    {
        const ga::Offset primStartVertex = other.getPrimStartVertex(primOffset);
        const ga::Offset vertexCount = other.getPrimVertCount(primOffset);

        std::vector<ga::Offset> pointOffsets;
        pointOffsets.reserve(vertexCount);
        for(ga::Offset i=0; i<vertexCount; ++i)
        {
            const ga::Offset otherPointOffset = other.pointOffsetHandleVert_.getValue(primStartVertex+i);
            pointOffsets.push_back(pointMapping[otherPointOffset]);
        }
        // TODO: check closed status
        addFace(pointOffsets, true);
    }

    // Merge vertex attributes
    for(std::shared_ptr<ga::Attribute> otherAttribute : other.vertexAttributes_)
    {
        bt::String otherAttributeName = otherAttribute->getName();
        std::shared_ptr<ga::Attribute> attribute = getAttribByName(ga::AttrOwner::VERTEX, otherAttributeName);

        if(attribute)
        {
            mergeAppend(attribute, otherAttribute);
        }
    }

    for(std::shared_ptr<ga::Attribute> otherAttribute : other.pointAttributes_)
    {
        bt::String otherAttributeName = otherAttribute->getName();
        std::shared_ptr<ga::Attribute> attribute = getAttribByName(ga::AttrOwner::POINT, otherAttributeName);
        
        bool alreadyExists = static_cast<bool>(attribute);

        if(alreadyExists)
        {
            mergeAppend(attribute, otherAttribute);
        }
        // else
        // {
        //     otherAttribute->resize(attributeSize);
        // }
        
    }

    for(std::shared_ptr<ga::Attribute> otherAttribute : other.primitiveAttributes_)
    {
        bt::String otherAttributeName = otherAttribute->getName();
        std::shared_ptr<ga::Attribute> attribute = getAttribByName(ga::AttrOwner::PRIMITIVE, otherAttributeName);
        
        bool alreadyExists = static_cast<bool>(attribute);

        if(alreadyExists)
        {
            mergeAppend(attribute, otherAttribute);
        }
        // else
        // {
        //     otherAttribute->resize(attributeSize);
        // }
        
    }

}

void geo::Primitive::addFace(const std::vector<ga::Offset>& pointOffsets, bool closed)
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

    // resize other attributes
    // TODO: make lazy or resize ahead of time
    // prims
    for(auto primAttribute : primitiveAttributes_)
    {
        if(primAttribute->isIntrinsic())
        {
            continue;
        }

        primAttribute->resize(primNum+1);

    }
    
}

ga::Offset  geo::Primitive::addPoint(const bt::Vector3& pos)
{
    const ga::Offset pointOffset = posHandlePoint_.getSize();

    posHandlePoint_.addValue(pos);
    soloPoints_.emplace(posHandlePoint_.getSize()-1);

    return pointOffset;
}

ga::Offset geo::Primitive::getNumSoloPoints() const
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

const size_t geo::Primitive::getNumAttributes(const ga::AttributeOwner owner) const
{
    return getAttributeStore(owner).size();
}

std::weak_ptr<const ga::Attribute> geo::Primitive::getAttributeByIndex(ga::AttributeOwner owner, unsigned int index) const
{
    auto attribStore = getAttributeStore(owner);
    if(index>=attribStore.size())
    {
        throw std::out_of_range("Attribute index out of range: " + std::to_string(index) + " max size: " + std::to_string(attribStore.size()) + "\n");
    }
    return getAttributeStore(owner)[index];

}





std::set<ga::Offset>::const_iterator geo::Primitive::soloPointsBegin()
{
    return soloPoints_.begin();
}

std::set<ga::Offset>::const_iterator geo::Primitive::soloPointsEnd()
{
    return soloPoints_.end();
}



bt::Vector3 geo::Primitive::getPosFromVert(ga::Offset vertexOffset) const
{
    // get point offset
    const ga::Offset pointOffset = pointOffsetHandleVert_.getValue(vertexOffset);
    // get value at point offset
    return posHandlePoint_.getValue(pointOffset);
}

bt::Vector3 geo::Primitive::getPointPos(ga::Offset pointOffset) const
{
    return posHandlePoint_.getValue(pointOffset);
}

ga::Offset      geo::Primitive::getVertexPrim(ga::Offset vertexOffset) const
{
    return vertexPrims_[vertexOffset];
}

void geo::Primitive::setPointPos(const ga::Offset offset, const bt::Vector3& pos)
{
    posHandlePoint_.setValue(offset, pos);
}


unsigned int geo::Primitive::getPrimVertCount(ga::Offset primOffset) const
{
    return vertexCountHandlePrim_.getValue(primOffset);
}

ga::Offset geo::Primitive::getNumPrims() const
{
    return vertexCountHandlePrim_.getSize();
}

ga::Offset geo::Primitive::getNumVerts() const
{
    return pointOffsetHandleVert_.getSize();
}

ga::Offset geo::Primitive::getNumPoints() const
{
    return posHandlePoint_.getSize();
}




geo::Primitive::attribVector geo::Primitive::deepCopyAttributes(attribVector originalVector)
{
    geo::Primitive::attribVector copied;
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


enzo::geo::HeMesh geo::Primitive::computeHalfEdgeMesh()
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

ga::Offset geo::Primitive::getPrimStartVertex(ga::Offset primOffset) const
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
void geo::Primitive::computePrimStartVertices() const
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




ga::AttributeHandleInt geo::Primitive::addIntAttribute(ga::AttributeOwner owner, std::string name, bool intrinsic)
{
    auto newAttribute = std::make_shared<ga::Attribute>(name, ga::AttrType::intT, intrinsic);
    getAttributeStore(owner).push_back(newAttribute);
    return ga::AttributeHandleInt(newAttribute);
}

ga::AttributeHandleBool geo::Primitive::addBoolAttribute(ga::AttributeOwner owner, std::string name, bool intrinsic)
{
    auto newAttribute = std::make_shared<ga::Attribute>(name, ga::AttrType::boolT, intrinsic);
    getAttributeStore(owner).push_back(newAttribute);
    return ga::AttributeHandleBool(newAttribute);
}

bt::boolT geo::Primitive::isClosed(ga::Offset primOffset) const
{
    return closedHandlePrim_.getValue(primOffset);
}


ga::AttributeHandle<bt::Vector3> geo::Primitive::addVector3Attribute(ga::AttributeOwner owner, std::string name, bool intrinsic)
{
    auto newAttribute = std::make_shared<ga::Attribute>(name, ga::AttrType::vectorT, intrinsic);
    getAttributeStore(owner).push_back(newAttribute);
    return ga::AttributeHandle<bt::Vector3>(newAttribute);
}

geo::Primitive::attribVector& geo::Primitive::getAttributeStore(const ga::AttributeOwner& owner)
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

const geo::Primitive::attribVector& geo::Primitive::getAttributeStore(const ga::AttributeOwner& owner) const
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

bool geo::Primitive::attributeExists(ga::AttributeOwner owner, std::string name)
{
    return static_cast<bool>(getAttribByName(owner, name));
}

std::shared_ptr<ga::Attribute> geo::Primitive::getAttribByName(ga::AttributeOwner owner, std::string name, bool includeIntrinsics)
{
    auto& vector = getAttributeStore(owner);
    for(auto it=vector.begin(); it!=vector.end(); ++it)
    {
        std::shared_ptr<ga::Attribute> attribute = (*it);
        if(attribute->getName()==name)
        {
            if(!includeIntrinsics && attribute->isIntrinsic()) continue;
            return attribute;
        }
    }
    return nullptr;
}

