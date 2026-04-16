#include "Engine/Operator/Mesh.h"
#include "Engine/Operator/Attribute.h"
#include "Engine/Operator/AttributeHandle.h"
#include "Engine/Types.h"
#include <memory>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/spin_mutex.h>
#include <tbb/task_group.h>
#include <stdexcept>
#include <string>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include "icecream.hpp"

using namespace enzo;

geo::Mesh::Mesh() :
    vertexCountHandlePrim_{addIntAttribute(ga::AttrOwner::FACE, "vertexCount", true)},
    closedHandlePrim_{addBoolAttribute(ga::AttrOwner::FACE, "closed", true)},
    pointOffsetHandleVert_{addIntAttribute(ga::AttrOwner::VERTEX, "point", true)},
    posHandlePoint_{addVector3Attribute(ga::AttrOwner::POINT, "P", true)}
{
    path_ = "/mesh";
}

geo::Mesh::Mesh(const Mesh& other):
    Primitive(other),
    // attributes
    vertexAttributes_{deepCopyAttributes(other.vertexAttributes_)},
    faceAttributes_{deepCopyAttributes(other.faceAttributes_)},

    // handles
    vertexCountHandlePrim_{enzo::ga::AttributeHandleInt(getAttribByName(ga::AttrOwner::FACE, "vertexCount", true))},
    closedHandlePrim_{enzo::ga::AttributeHandleBool(getAttribByName(ga::AttrOwner::FACE, "closed", true))},
    pointOffsetHandleVert_{enzo::ga::AttributeHandleInt(getAttribByName(ga::AttrOwner::VERTEX, "point", true))},
    posHandlePoint_{enzo::ga::AttributeHandleVector3(getAttribByName(ga::AttrOwner::POINT, "P", true))},

    // other
    soloPoints_{other.soloPoints_},
    vertexPrims_{other.vertexPrims_},
    primStarts_{other.primStarts_},
    primStartsDirty_{other.primStartsDirty_.load()}
{
}

enzo::geo::Mesh& enzo::geo::Mesh::operator=(const enzo::geo::Mesh& rhs) {
    if (this == &rhs) return *this;

    Primitive::operator=(rhs);

    // attributes
    vertexAttributes_     = deepCopyAttributes(rhs.vertexAttributes_);
    faceAttributes_       = deepCopyAttributes(rhs.faceAttributes_);

    // handles
    vertexCountHandlePrim_ = enzo::ga::AttributeHandleInt(getAttribByName(ga::AttrOwner::FACE, "vertexCount", true));
    closedHandlePrim_ = enzo::ga::AttributeHandleBool(getAttribByName(ga::AttrOwner::FACE, "closed", true));
    pointOffsetHandleVert_ = enzo::ga::AttributeHandleInt(getAttribByName(ga::AttrOwner::VERTEX, "point", true));
    posHandlePoint_ = enzo::ga::AttributeHandleVector3(getAttribByName(ga::AttrOwner::POINT, "P", true));

    // other
    soloPoints_           = rhs.soloPoints_;
    vertexPrims_          = rhs.vertexPrims_;
    primStarts_           = rhs.primStarts_;

    primStartsDirty_.store(rhs.primStartsDirty_.load());

    return *this;
}

void geo::Mesh::mergeAppend(std::shared_ptr<ga::Attribute> dst, std::shared_ptr<ga::Attribute> src)
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

void geo::Mesh::applyTransform(const bt::Matrix4 &mat, TransformClass transformClass) {
    if ((transformClass & TransformClass::POINT) != TransformClass::NONE) {
        const ga::Offset numPoints = getNumPoints();
        tbb::parallel_for(tbb::blocked_range<size_t>(0, numPoints),
            [&](const tbb::blocked_range<size_t> &range) {
                for (size_t i = range.begin(); i < range.end(); ++i) {
                    const bt::Vector3 pos = posHandlePoint_.getValue(i);
                    // w=1.0 extends to homogeneous coords so the 4x4 matrix
                    // applies translation, rotation, and scale in one multiply
                    const bt::Vector4 pos4(pos.x(), pos.y(), pos.z(), 1.0);
                    const bt::Vector3 transformed = (mat * pos4).head<3>();
                    posHandlePoint_.setValue(i, transformed);
                }
            });
    }
}

void geo::Mesh::merge(std::shared_ptr<Primitive> other) {
    auto otherMesh = std::dynamic_pointer_cast<Mesh>(other);
    if (!otherMesh) throw std::runtime_error("Mesh::merge: type mismatch");
    merge(*otherMesh);
}

void geo::Mesh::merge(Mesh& other)
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
    }

    for(std::shared_ptr<ga::Attribute> otherAttribute : other.faceAttributes_)
    {
        bt::String otherAttributeName = otherAttribute->getName();
        std::shared_ptr<ga::Attribute> attribute = getAttribByName(ga::AttrOwner::FACE, otherAttributeName);

        bool alreadyExists = static_cast<bool>(attribute);

        if(alreadyExists)
        {
            mergeAppend(attribute, otherAttribute);
        }
    }

}

void geo::Mesh::addFace(const std::vector<ga::Offset>& pointOffsets, bool closed)
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
    for(auto faceAttribute : faceAttributes_)
    {
        if(faceAttribute->isIntrinsic())
        {
            continue;
        }

        faceAttribute->resize(primNum+1);

    }

}

ga::Offset  geo::Mesh::addPoint(const bt::Vector3& pos)
{
    const ga::Offset pointOffset = posHandlePoint_.getSize();

    posHandlePoint_.addValue(pos);
    soloPoints_.emplace(posHandlePoint_.getSize()-1);

    return pointOffset;
}

ga::Offset geo::Mesh::getNumSoloPoints() const
{
    return soloPoints_.size();
}

std::set<ga::Offset>::const_iterator geo::Mesh::soloPointsBegin() const
{
    return soloPoints_.begin();
}

std::set<ga::Offset>::const_iterator geo::Mesh::soloPointsEnd() const
{
    return soloPoints_.end();
}

bt::Vector3 geo::Mesh::getPosFromVert(ga::Offset vertexOffset) const
{
    const ga::Offset pointOffset = pointOffsetHandleVert_.getValue(vertexOffset);
    return posHandlePoint_.getValue(pointOffset);
}

bt::Vector3 geo::Mesh::getPointPos(ga::Offset pointOffset) const
{
    return posHandlePoint_.getValue(pointOffset);
}

ga::Offset geo::Mesh::getVertexPrim(ga::Offset vertexOffset) const
{
    return vertexPrims_[vertexOffset];
}

void geo::Mesh::setPointPos(const ga::Offset offset, const bt::Vector3& pos)
{
    posHandlePoint_.setValue(offset, pos);
}

unsigned int geo::Mesh::getPrimVertCount(ga::Offset primOffset) const
{
    return vertexCountHandlePrim_.getValue(primOffset);
}

ga::Offset geo::Mesh::getNumPrims() const
{
    return vertexCountHandlePrim_.getSize();
}

ga::Offset geo::Mesh::getNumVerts() const
{
    return pointOffsetHandleVert_.getSize();
}

ga::Offset geo::Mesh::getNumPoints() const
{
    return posHandlePoint_.getSize();
}

enzo::geo::HeMesh geo::Mesh::computeHalfEdgeMesh()
{
    HeMesh heMesh;

    std::shared_ptr<enzo::ga::Attribute> PAttr = getAttribByName(enzo::ga::AttrOwner::POINT, "P");
    enzo::ga::AttributeHandleVector3 PAttrHandle = enzo::ga::AttributeHandleVector3(PAttr);
    auto pointPositions = PAttrHandle.getAllValues();

    std::shared_ptr<enzo::ga::Attribute> pointAttr = getAttribByName(enzo::ga::AttrOwner::VERTEX, "point");
    enzo::ga::AttributeHandleInt pointAttrHandle = enzo::ga::AttributeHandleInt(pointAttr);
    auto vertexPointIndices = pointAttrHandle.getAllValues();

    std::shared_ptr<enzo::ga::Attribute> vertexCountAttr = getAttribByName(enzo::ga::AttrOwner::FACE, "vertexCount");
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

ga::Offset geo::Mesh::getPrimStartVertex(ga::Offset primOffset) const
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

void geo::Mesh::computePrimStartVertices() const
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

bt::boolT geo::Mesh::isClosed(ga::Offset primOffset) const
{
    return closedHandlePrim_.getValue(primOffset);
}

ga::attribVector& geo::Mesh::getAttributeStore(const ga::AttributeOwner& owner)
{
    switch(owner)
    {
        case ga::AttributeOwner::VERTEX:
            return vertexAttributes_;
        case ga::AttributeOwner::FACE:
            return faceAttributes_;
        default:
            return Primitive::getAttributeStore(owner);
    }
}

const ga::attribVector& geo::Mesh::getAttributeStore(const ga::AttributeOwner& owner) const
{
    switch(owner)
    {
        case ga::AttributeOwner::VERTEX:
            return vertexAttributes_;
        case ga::AttributeOwner::FACE:
            return faceAttributes_;
        default:
            return Primitive::getAttributeStore(owner);
    }
}
