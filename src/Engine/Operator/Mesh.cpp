#include "Engine/Operator/Mesh.h"
#include "Engine/Operator/Attribute.h"
#include "Engine/Operator/AttributeHandle.h"
#include "Engine/Operator/Primitive.h"
#include "Engine/Types.h"
#include "Engine/Utils/MeshUtils.h"
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

geo::Mesh::Mesh(std::string_view path) :
    vertexCountFaceHandle_{addIntAttribute(attr::AttrOwner::FACE, "vertexCount", true)},
    closedFaceHandle_{addBoolAttribute(attr::AttrOwner::FACE, "closed", true)},
    pointOffsetVertexHandle_{addIntAttribute(attr::AttrOwner::VERTEX, "point", true)},
    posPointHandle_{addVector3Attribute(attr::AttrOwner::POINT, "P", true)},
    validFaceHandle_{addBoolAttribute(attr::AttrOwner::FACE, "__valid", true, true)},
    validVertexHandle_{addBoolAttribute(attr::AttrOwner::VERTEX, "__valid", true, true)},
    validPointHandle_{addBoolAttribute(attr::AttrOwner::POINT, "__valid", true, true)},
    Primitive(path)
{
}

geo::Mesh::Mesh(const Mesh& other):
    Primitive(other),
    // attributes
    vertexAttributes_{deepCopyAttributes(other.vertexAttributes_)},
    faceAttributes_{deepCopyAttributes(other.faceAttributes_)},
    // groups
    vertexGroups_{deepCopyAttributes(other.vertexGroups_)},
    faceGroups_{deepCopyAttributes(other.faceGroups_)},

    // handles
    vertexCountFaceHandle_{enzo::attr::AttributeHandleInt(getAttribByName(attr::AttrOwner::FACE, "vertexCount", true))},
    closedFaceHandle_{enzo::attr::AttributeHandleBool(getAttribByName(attr::AttrOwner::FACE, "closed", true))},
    pointOffsetVertexHandle_{enzo::attr::AttributeHandleInt(getAttribByName(attr::AttrOwner::VERTEX, "point", true))},
    posPointHandle_{enzo::attr::AttributeHandleVector3(getAttribByName(attr::AttrOwner::POINT, "P", true))},
    validFaceHandle_{enzo::attr::AttributeHandleBool(getAttribByName(attr::AttrOwner::FACE, "__valid", true))},
    validVertexHandle_{enzo::attr::AttributeHandleBool(getAttribByName(attr::AttrOwner::VERTEX, "__valid", true))},
    validPointHandle_{enzo::attr::AttributeHandleBool(getAttribByName(attr::AttrOwner::POINT, "__valid", true))},

    // other
    soloPoints_{other.soloPoints_},
    soloPointsDirty_{other.soloPointsDirty_},
    needsDefrag_{other.needsDefrag_},
    vertexFaces_{other.vertexFaces_},
    faceStarts_{other.faceStarts_},
    faceStartsDirty_{other.faceStartsDirty_.load()}
{
}

enzo::geo::Mesh& enzo::geo::Mesh::operator=(const enzo::geo::Mesh& rhs) {
    if (this == &rhs) return *this;

    Primitive::operator=(rhs);

    // attributes
    vertexAttributes_     = deepCopyAttributes(rhs.vertexAttributes_);
    faceAttributes_       = deepCopyAttributes(rhs.faceAttributes_);

    // groups
    vertexGroups_         = deepCopyAttributes(rhs.vertexGroups_);
    faceGroups_           = deepCopyAttributes(rhs.faceGroups_);

    // handles
    vertexCountFaceHandle_ = enzo::attr::AttributeHandleInt(getAttribByName(attr::AttrOwner::FACE, "vertexCount", true));
    closedFaceHandle_ = enzo::attr::AttributeHandleBool(getAttribByName(attr::AttrOwner::FACE, "closed", true));
    pointOffsetVertexHandle_ = enzo::attr::AttributeHandleInt(getAttribByName(attr::AttrOwner::VERTEX, "point", true));
    posPointHandle_ = enzo::attr::AttributeHandleVector3(getAttribByName(attr::AttrOwner::POINT, "P", true));
    validFaceHandle_ = enzo::attr::AttributeHandleBool(getAttribByName(attr::AttrOwner::FACE, "__valid", true));
    validVertexHandle_ = enzo::attr::AttributeHandleBool(getAttribByName(attr::AttrOwner::VERTEX, "__valid", true));
    validPointHandle_ = enzo::attr::AttributeHandleBool(getAttribByName(attr::AttrOwner::POINT, "__valid", true));

    // other
    soloPoints_           = rhs.soloPoints_;
    soloPointsDirty_      = rhs.soloPointsDirty_;
    needsDefrag_          = rhs.needsDefrag_;
    vertexFaces_          = rhs.vertexFaces_;
    faceStarts_           = rhs.faceStarts_;

    faceStartsDirty_.store(rhs.faceStartsDirty_.load());

    return *this;
}

void geo::Mesh::mergeAppend(std::shared_ptr<attr::Attribute> dst, std::shared_ptr<attr::Attribute> src)
{
    if(!dst) throw std::runtime_error("Dst empty.");
    if(!src) throw std::runtime_error("Src empty.");

    auto dstType = dst->getType();
    auto srcType = src->getType();

    if(dstType != srcType) throw std::runtime_error("mergeAppend type missmatch.");

    switch(srcType)
    {
        case attr::AttributeType::intT:
            mergeAppendImpl<bt::intT>(dst, src);
            break;
        case attr::AttributeType::floatT:
            mergeAppendImpl<bt::floatT>(dst, src);
            break;
        case attr::AttributeType::listT:
            break;
        case attr::AttributeType::vectorT:
            // mergeAppendImpl<bt::vector3>(dst, src);
            break;
        case attr::AttributeType::boolT:
            mergeAppendImpl<bt::boolT>(dst, src);
            break;
        default:
            throw std::runtime_error("mergeAppend: Attribute type not accounted for.");

    }
}

void geo::Mesh::applyTransform(const bt::Matrix4 &mat, TransformClass transformClass) {
    if ((transformClass & TransformClass::POINT) != TransformClass::NONE) {
        const attr::Offset numPoints = getNumPoints();
        tbb::parallel_for(tbb::blocked_range<size_t>(0, numPoints),
            [&](const tbb::blocked_range<size_t> &range) {
                for (size_t i = range.begin(); i < range.end(); ++i) {
                    const bt::Vector3& pos = posPointHandle_[i];
                    // w=1.0 extends to homogeneous coords so the 4x4 matrix
                    // applies translation, rotation, and scale in one multiply
                    const bt::Vector4 pos4(pos.x(), pos.y(), pos.z(), 1.0);
                    const bt::Vector3 transformed = (mat * pos4).head<3>();
                    posPointHandle_.setValue(i, transformed);
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
    const attr::Offset srcPointNum = other.getNumPoints();
    std::vector<attr::Offset> pointMapping(srcPointNum);
    for(attr::Offset pointOffset=0; pointOffset<srcPointNum; ++pointOffset)
    {
        pointMapping[pointOffset] = addPoint(other.getPointPos(pointOffset));
    }

    // Create faces using mapped point offsets
    const attr::Offset srcFaceNum = other.getNumFaces();
    for(attr::Offset faceOffset=0; faceOffset<srcFaceNum; ++faceOffset)
    {
        const attr::Offset faceStartVertex = other.getFaceStartVertex(faceOffset);
        const attr::Offset vertexCount = other.getFaceVertCount(faceOffset);

        std::vector<attr::Offset> pointOffsets;
        pointOffsets.reserve(vertexCount);
        for(attr::Offset i=0; i<vertexCount; ++i)
        {
            const attr::Offset otherPointOffset = other.pointOffsetVertexHandle_.getValue(faceStartVertex+i);
            pointOffsets.push_back(pointMapping[otherPointOffset]);
        }
        // TODO: check closed status
        addFace(pointOffsets, true);
    }

    // Merge vertex attributes
    for(std::shared_ptr<attr::Attribute> otherAttribute : other.vertexAttributes_)
    {
        bt::String otherAttributeName = otherAttribute->getName();
        std::shared_ptr<attr::Attribute> attribute = getAttribByName(attr::AttrOwner::VERTEX, otherAttributeName);

        if(attribute)
        {
            mergeAppend(attribute, otherAttribute);
        }
    }

    for(std::shared_ptr<attr::Attribute> otherAttribute : other.pointAttributes_)
    {
        bt::String otherAttributeName = otherAttribute->getName();
        std::shared_ptr<attr::Attribute> attribute = getAttribByName(attr::AttrOwner::POINT, otherAttributeName);

        bool alreadyExists = static_cast<bool>(attribute);

        if(alreadyExists)
        {
            mergeAppend(attribute, otherAttribute);
        }
    }

    for(std::shared_ptr<attr::Attribute> otherAttribute : other.faceAttributes_)
    {
        bt::String otherAttributeName = otherAttribute->getName();
        std::shared_ptr<attr::Attribute> attribute = getAttribByName(attr::AttrOwner::FACE, otherAttributeName);

        bool alreadyExists = static_cast<bool>(attribute);

        if(alreadyExists)
        {
            mergeAppend(attribute, otherAttribute);
        }
    }

}

attr::Offset geo::Mesh::addFace(const std::vector<attr::Offset>& pointOffsets, bool closed)
{
    const attr::Offset faceNum = vertexCountFaceHandle_.getSize();
    for(attr::Offset pointOffset : pointOffsets)
    {
        pointOffsetVertexHandle_.addValue(pointOffset);
        validVertexHandle_.addValue(true);
        vertexFaces_.push_back(faceNum);
    }
    vertexCountFaceHandle_.addValue(pointOffsets.size());
    closedFaceHandle_.addValue(closed);
    validFaceHandle_.addValue(true);
    soloPointsDirty_ = true;

    // resize other attributes
    // TODO: this seems terribly inefficient
    for(auto faceAttribute : faceAttributes_)
    {
        if(faceAttribute->isIntrinsic())
        {
            continue;
        }

        faceAttribute->resize(faceNum+1);

    }

    // resize face and vertex groups so they stay aligned with element counts
    const attr::Offset newVertCount = pointOffsetVertexHandle_.getSize();
    for (auto& faceGroup : faceGroups_)
    {
        if (faceGroup) faceGroup->resize(faceNum + 1);
    }
    for (auto& vertexGroup : vertexGroups_)
    {
        if (vertexGroup) vertexGroup->resize(newVertCount);
    }

    return faceNum;
}

std::vector<attr::Offset> geo::Mesh::addFaces(std::span<const attr::Offset> pointOffsetsFlat,
                                            std::span<const attr::Offset> vertexCounts,
                                            bool closed)
{
    const attr::Offset firstFaceOffset = vertexCountFaceHandle_.getSize();
    const attr::Offset firstVertOffset = pointOffsetVertexHandle_.getSize();
    const size_t numFacesToAdd = vertexCounts.size();
    const size_t numVertsToAdd = pointOffsetsFlat.size();
    const attr::Offset newFaceCount = firstFaceOffset + numFacesToAdd;
    const attr::Offset newVertCount = firstVertOffset + numVertsToAdd;

    // Grow every face side and vertex side store in one shot
    for (auto& attribute : faceAttributes_)
    {
        if (attribute) attribute->resize(newFaceCount);
    }
    for (auto& group : faceGroups_)
    {
        if (group) group->resize(newFaceCount);
    }
    for (auto& attribute : vertexAttributes_)
    {
        if (attribute) attribute->resize(newVertCount);
    }
    for (auto& group : vertexGroups_)
    {
        if (group) group->resize(newVertCount);
    }
    vertexFaces_.reserve(newVertCount);

    std::vector<attr::Offset> newFaceOffsets;
    newFaceOffsets.reserve(numFacesToAdd);

    attr::Offset vertCursor = firstVertOffset;
    for (size_t faceIndex = 0; faceIndex < numFacesToAdd; ++faceIndex)
    {
        const attr::Offset vertexCount = vertexCounts[faceIndex];
        const attr::Offset faceOffset = firstFaceOffset + faceIndex;
        newFaceOffsets.push_back(faceOffset);

        for (attr::Offset vertIndex = 0; vertIndex < vertexCount; ++vertIndex)
        {
            const attr::Offset pointOffset = pointOffsetsFlat[vertCursor - firstVertOffset];
            pointOffsetVertexHandle_.setValue(vertCursor, pointOffset);
            validVertexHandle_.setValue(vertCursor, true);
            vertexFaces_.push_back(faceOffset);
            ++vertCursor;
        }

        vertexCountFaceHandle_.setValue(faceOffset, vertexCount);
        closedFaceHandle_.setValue(faceOffset, closed);
        validFaceHandle_.setValue(faceOffset, true);
    }

    soloPointsDirty_ = true;
    faceStartsDirty_.store(true);

    return newFaceOffsets;
}

void geo::Mesh::deleteFaces(const std::vector<attr::Offset>& faceOffsets, bool andPoints)
{
    if (faceOffsets.empty()) return;

    // Invalidate each face and its vertices.
    // When cascading to points, remember which points those vertices referenced.
    std::unordered_set<attr::Offset> orphanCandidates;
    for (attr::Offset faceOffset : faceOffsets)
    {
        validFaceHandle_.setValue(faceOffset, false);

        const attr::Offset start = getFaceStartVertex(faceOffset);
        const attr::Offset count = getFaceVertCount(faceOffset);
        for (attr::Offset v = start; v < start + count; ++v)
        {
            if (andPoints) orphanCandidates.insert(pointOffsetVertexHandle_.getValue(v));
            validVertexHandle_.setValue(v, false);
        }
    }

    if (andPoints)
    {
        // Drop candidates still referenced by a surviving vertex.
        const attr::Offset vertCount = pointOffsetVertexHandle_.getSize();
        for (attr::Offset v = 0; v < vertCount; ++v)
        {
            if (!validVertexHandle_.getValue(v)) continue;
            orphanCandidates.erase(pointOffsetVertexHandle_.getValue(v));
        }

        // Invalidate the remaining orphans.
        for (attr::Offset p : orphanCandidates) validPointHandle_.setValue(p, false);
    }

    needsDefrag_ = true;
    soloPointsDirty_ = true;
}

void geo::Mesh::deletePoints(const std::vector<attr::Offset>& pointOffsets, bool andFaces)
{
    if (pointOffsets.empty()) return;

    std::unordered_set<attr::Offset> deletedPoints(pointOffsets.begin(), pointOffsets.end());

    for (attr::Offset pointOffset : pointOffsets)
    {
        validPointHandle_.setValue(pointOffset, false);
    }

    // Single O(V) pass: mark every vertex referencing a deleted point as invalid.
    // If andFaces is set, also collect the owning faces for cascaded deletion.
    const attr::Offset vertCount = pointOffsetVertexHandle_.getSize();
    const attr::Offset faceCount = vertexCountFaceHandle_.getSize();
    std::vector<bool> faceMarked(andFaces ? faceCount : 0, false);
    for (attr::Offset v = 0; v < vertCount; ++v)
    {
        if (!validVertexHandle_.getValue(v)) continue;
        const attr::Offset pt = pointOffsetVertexHandle_.getValue(v);
        if (!deletedPoints.count(pt)) continue;
        validVertexHandle_.setValue(v, false);
        if (andFaces) faceMarked[getVertexFace(v)] = true;
    }
    if (andFaces)
    {
        std::vector<attr::Offset> facesToDelete;
        for (attr::Offset f = 0; f < faceCount; ++f)
        {
            if (faceMarked[f]) facesToDelete.push_back(f);
        }
        deleteFaces(facesToDelete);
    }

    needsDefrag_ = true;
    soloPointsDirty_ = true;
}

void geo::Mesh::deleteVertices(const std::vector<attr::Offset>& vertOffsets)
{
    if (vertOffsets.empty()) return;

    // Mark vertices invalid and remember which faces lost vertices
    std::unordered_set<attr::Offset> affectedFaces;
    for (attr::Offset v : vertOffsets)
    {
        validVertexHandle_.setValue(v, false);
        affectedFaces.insert(getVertexFace(v));
    }

    // Invalidate any face that has no valid vertices left
    for (attr::Offset f : affectedFaces)
    {
        if (!validFaceHandle_.getValue(f)) continue;
        const attr::Offset start = getFaceStartVertex(f);
        const attr::Offset count = getFaceVertCount(f);
        bool anyValid = false;
        for (attr::Offset v = start; v < start + count; ++v)
        {
            if (validVertexHandle_.getValue(v)) { anyValid = true; break; }
        }
        if (!anyValid) validFaceHandle_.setValue(f, false);
    }

    needsDefrag_ = true;
    soloPointsDirty_ = true;
}

bool geo::Mesh::isValidFace(attr::Offset offset) const
{
    return validFaceHandle_.getValue(offset);
}

bool geo::Mesh::isValidVertex(attr::Offset offset) const
{
    return validVertexHandle_.getValue(offset);
}

bool geo::Mesh::isValidPoint(attr::Offset offset) const
{
    return validPointHandle_.getValue(offset);
}

void geo::Mesh::defragment()
{
    Primitive::defragment();

    if (!needsDefrag_) return;

    // Update each face's vertex count in case any vertices were deleted
    const attr::Offset oldFaceCount = vertexCountFaceHandle_.getSize();
    for (attr::Offset f = 0; f < oldFaceCount; ++f)
    {
        if (!validFaceHandle_.getValue(f)) continue;
        const attr::Offset start = getFaceStartVertex(f);
        const attr::Offset oldCount = vertexCountFaceHandle_.getValue(f);
        attr::Offset validCount = 0;
        for (attr::Offset v = start; v < start + oldCount; ++v)
        {
            if (validVertexHandle_.getValue(v)) ++validCount;
        }
        vertexCountFaceHandle_.setValue(f, validCount);
    }

    // Compact every face attribute and group by face validity.
    std::vector<bool> faceKeep = validFaceHandle_.getAllValues();
    for (auto& attr : faceAttributes_) if (attr) attr->compact(faceKeep);
    for (auto& group : faceGroups_) if (group) group->compact(faceKeep);

    // Compact every vertex attribute and group by vertex validity.
    std::vector<bool> vertKeep = validVertexHandle_.getAllValues();
    for (auto& attr : vertexAttributes_) if (attr) attr->compact(vertKeep);
    for (auto& group : vertexGroups_) if (group) group->compact(vertKeep);

    // Compact every point attribute by point validity, building an old → new
    // offset map so we can update everything that references point offsets.
    std::vector<bool> pointKeep = validPointHandle_.getAllValues();
    std::vector<attr::Offset> pointRemap(pointKeep.size(), 0);
    attr::Offset newPointOffset = 0;
    for (size_t i = 0; i < pointKeep.size(); ++i)
    {
        if (pointKeep[i]) pointRemap[i] = newPointOffset++;
    }
    for (auto& attr : pointAttributes_) if (attr) attr->compact(pointKeep);
    for (auto& group : pointGroups_) if (group) group->compact(pointKeep);

    // Remap surviving vertex → point references to the compacted point offsets.
    const attr::Offset newVertCount = pointOffsetVertexHandle_.getSize();
    for (attr::Offset v = 0; v < newVertCount; ++v)
    {
        const attr::Offset oldPointOffset = pointOffsetVertexHandle_.getValue(v);
        pointOffsetVertexHandle_.setValue(v, pointRemap[oldPointOffset]);
    }

    soloPointsDirty_ = true;

    // Rebuild vertexFaces_ from the compacted face data.
    vertexFaces_.clear();
    vertexFaces_.reserve(newVertCount);
    const attr::Offset faceCount = vertexCountFaceHandle_.getSize();
    for (attr::Offset f = 0; f < faceCount; ++f)
    {
        const attr::Offset count = getFaceVertCount(f);
        for (attr::Offset v = 0; v < count; ++v) vertexFaces_.push_back(f);
    }

    faceStartsDirty_.store(true);
    needsDefrag_ = false;
}

attr::Offset  geo::Mesh::addPoint(const bt::Vector3& pos)
{
    const attr::Offset pointOffset = posPointHandle_.getSize();

    posPointHandle_.addValue(pos);
    validPointHandle_.addValue(true);
    soloPointsDirty_ = true;

    return pointOffset;
}

std::vector<attr::Offset> geo::Mesh::addPoints(std::span<const bt::Vector3> positions)
{
    const attr::Offset firstPointOffset = posPointHandle_.getSize();
    const size_t numPointsToAdd = positions.size();
    const attr::Offset newPointCount = firstPointOffset + numPointsToAdd;

    // Grow every point side store in one shot
    for (auto& attribute : pointAttributes_)
    {
        if (attribute) attribute->resize(newPointCount);
    }
    for (auto& group : pointGroups_)
    {
        if (group) group->resize(newPointCount);
    }

    std::vector<attr::Offset> newPointOffsets;
    newPointOffsets.reserve(numPointsToAdd);
    for (size_t pointIndex = 0; pointIndex < numPointsToAdd; ++pointIndex)
    {
        const attr::Offset pointOffset = firstPointOffset + pointIndex;
        posPointHandle_.setValue(pointOffset, positions[pointIndex]);
        validPointHandle_.setValue(pointOffset, true);
        newPointOffsets.push_back(pointOffset);
    }

    soloPointsDirty_ = true;

    return newPointOffsets;
}

std::vector<attr::Offset> geo::Mesh::duplicatePoints(std::span<const attr::Offset> srcPointOffsets, bool copyAttributes)
{
    // Gather the source positions, then let addPoints grow the stores in one shot
    std::vector<bt::Vector3> positions;
    positions.reserve(srcPointOffsets.size());
    for (attr::Offset srcPointOffset : srcPointOffsets)
    {
        positions.push_back(posPointHandle_.getValue(srcPointOffset));
    }

    std::vector<attr::Offset> newPointOffsets = addPoints(positions);

    // TODO: when copyAttributes is set, copy every point attribute value from each
    // source point to its duplicate. Only positions are carried over for now.
    (void)copyAttributes;

    return newPointOffsets;
}

void geo::Mesh::rebuildSoloPoints() const
{
    soloPoints_.clear();

    // Seed with every valid point
    const attr::Offset pointCount = posPointHandle_.getSize();
    for (attr::Offset p = 0; p < pointCount; ++p)
    {
        if (validPointHandle_.getValue(p)) soloPoints_.insert(p);
    }

    // Drop any point referenced by a valid vertex
    const attr::Offset vertCount = pointOffsetVertexHandle_.getSize();
    for (attr::Offset v = 0; v < vertCount; ++v)
    {
        if (!validVertexHandle_.getValue(v)) continue;
        soloPoints_.erase(pointOffsetVertexHandle_.getValue(v));
    }

    soloPointsDirty_ = false;
}

attr::Offset geo::Mesh::getNumSoloPoints() const
{
    if (soloPointsDirty_) rebuildSoloPoints();
    return soloPoints_.size();
}

std::unordered_set<attr::Offset>::const_iterator geo::Mesh::soloPointsBegin() const
{
    if (soloPointsDirty_) rebuildSoloPoints();
    return soloPoints_.begin();
}

std::unordered_set<attr::Offset>::const_iterator geo::Mesh::soloPointsEnd() const
{
    if (soloPointsDirty_) rebuildSoloPoints();
    return soloPoints_.end();
}

bt::Vector3 geo::Mesh::getPosFromVert(attr::Offset vertexOffset) const
{
    const attr::Offset pointOffset = pointOffsetVertexHandle_.getValue(vertexOffset);
    return posPointHandle_.getValue(pointOffset);
}

bt::Vector3 geo::Mesh::getPointPos(attr::Offset pointOffset) const
{
    return posPointHandle_.getValue(pointOffset);
}

attr::Offset geo::Mesh::getVertexFace(attr::Offset vertexOffset) const
{
    return vertexFaces_[vertexOffset];
}

void geo::Mesh::setPointPos(const attr::Offset offset, const bt::Vector3& pos)
{
    posPointHandle_.setValue(offset, pos);
}

unsigned int geo::Mesh::getFaceVertCount(attr::Offset faceOffset) const
{
    return vertexCountFaceHandle_.getValue(faceOffset);
}

unsigned int geo::Mesh::getFacePointCount(attr::Offset faceOffset) const
{
    return getFaceVertCount(faceOffset);
}

enzo::geo::HeMesh geo::Mesh::computeHalfEdgeMesh()
{
    HeMesh heMesh;

    std::shared_ptr<enzo::attr::Attribute> PAttr = getAttribByName(enzo::attr::AttrOwner::POINT, "P");
    enzo::attr::AttributeHandleVector3 PAttrHandle = enzo::attr::AttributeHandleVector3(PAttr);
    auto pointPositions = PAttrHandle.getAllValues();

    std::shared_ptr<enzo::attr::Attribute> pointAttr = getAttribByName(enzo::attr::AttrOwner::VERTEX, "point");
    enzo::attr::AttributeHandleInt pointAttrHandle = enzo::attr::AttributeHandleInt(pointAttr);
    auto vertexPointIndices = pointAttrHandle.getAllValues();

    std::shared_ptr<enzo::attr::Attribute> vertexCountAttr = getAttribByName(enzo::attr::AttrOwner::FACE, "vertexCount");
    enzo::attr::AttributeHandleInt vertexCountHandle = enzo::attr::AttributeHandleInt(vertexCountAttr);
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

    // iterate through each face
    for(int faceIndx=0; faceIndx<vertexCounts.size(); ++faceIndx)
    {
        facePoints.clear();

        // represents how many vertices are in a face
        auto vertexCount = vertexCounts[faceIndx];

        // create face vertices
        for(int i=0; i<vertexCount; ++i)
        {
            auto pointIndex = vertexPointIndices.at(vertexIndex);
            facePoints.push_back(createdPoints[pointIndex]);
            ++vertexIndex;
        }

        // debug
        std::cout << "Primitive " << faceIndx << " has " << vertexCount << " vertices: ";
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
            // validFaceIndices.push_back(enzo::geo::F_index(faceIndx));
        } else {
            // throw std::runtime_error("Warning: Face creation failed at primitive " + std::to_string(faceIndx));
        }
    }


    return heMesh;
}

attr::Offset geo::Mesh::getFaceStartVertex(attr::Offset faceOffset) const
{

    if(faceStartsDirty_.load())
    {
        tbb::spin_mutex::scoped_lock lock(faceStartsMutex_); //lock
        if(faceStartsDirty_.load()) // double check
        {
            computeFaceStartVertices();
        }
    }
    return faceStarts_[faceOffset];
}

void geo::Mesh::computeFaceStartVertices() const
{
    const attr::Offset handleSize = vertexCountFaceHandle_.getSize();
    faceStarts_.clear();
    faceStarts_.reserve(handleSize);
    bt::intT faceStart = 0;
    for(size_t i=0; i<handleSize; ++i)
    {
        faceStarts_.push_back(faceStart);
        faceStart += vertexCountFaceHandle_.getValue(i);
    }
    faceStartsDirty_.store(false);
}

bt::boolT geo::Mesh::isClosed(attr::Offset faceOffset) const
{
    return closedFaceHandle_.getValue(faceOffset);
}

attr::attribVector& geo::Mesh::getAttributeStore(const attr::AttributeOwner& owner)
{
    switch(owner)
    {
        case attr::AttributeOwner::VERTEX:
            return vertexAttributes_;
        case attr::AttributeOwner::FACE:
            return faceAttributes_;
        default:
            return Primitive::getAttributeStore(owner);
    }
}

const attr::attribVector& geo::Mesh::getAttributeStore(const attr::AttributeOwner& owner) const
{
    switch(owner)
    {
        case attr::AttributeOwner::VERTEX:
            return vertexAttributes_;
        case attr::AttributeOwner::FACE:
            return faceAttributes_;
        default:
            return Primitive::getAttributeStore(owner);
    }
}

attr::attribVector& geo::Mesh::getGroupStore(const attr::AttributeOwner& owner)
{
    switch(owner)
    {
        case attr::AttributeOwner::VERTEX:
            return vertexGroups_;
        case attr::AttributeOwner::FACE:
            return faceGroups_;
        default:
            return Primitive::getGroupStore(owner);
    }
}

const attr::attribVector& geo::Mesh::getGroupStore(const attr::AttributeOwner& owner) const
{
    switch(owner)
    {
        case attr::AttributeOwner::VERTEX:
            return vertexGroups_;
        case attr::AttributeOwner::FACE:
            return faceGroups_;
        default:
            return Primitive::getGroupStore(owner);
    }
}

geo::FaceNormalHandle::FaceNormalHandle(const Mesh& mesh, bool precompute)
    : mesh_(mesh)
{
    std::shared_ptr<const attr::Attribute> normalAttr =
        mesh.getAttribByName(attr::AttrOwner::FACE, "Normal");
    if(normalAttr)
    {
        cached_.emplace(normalAttr);
        return;
    }

    if(precompute)
    {
        const std::span<const bt::Vector3> positions = mesh.posPointHandle_.getSpan();
        const attr::Offset numFaces = mesh.getNumFaces();
        precomputed_.reserve(numFaces);
        for(attr::Offset faceOffset=0; faceOffset<numFaces; ++faceOffset)
        {
            precomputed_.push_back(utils::polygonNormal(positions, mesh.getFacePoints(faceOffset)));
        }
    }
}

bt::Vector3 geo::FaceNormalHandle::operator[](attr::Offset faceOffset) const
{
    if(cached_) return cached_->getValue(faceOffset);
    if(!precomputed_.empty()) return precomputed_[faceOffset];
    // Re-resolve the position span on each call so addPoint between reads
    // can't dangle the data pointer through a std::vector reallocation.
    return utils::polygonNormal(mesh_.posPointHandle_.getSpan(), mesh_.getFacePoints(faceOffset));
}

geo::VertexNormalHandle::VertexNormalHandle(const Mesh& mesh, bool precompute)
    : mesh_(mesh),
      faceNormals_(mesh.getFaceNormal(precompute))
{
    std::shared_ptr<const attr::Attribute> normalAttr =
        mesh.getAttribByName(attr::AttrOwner::VERTEX, "Normal");
    if(normalAttr) cached_.emplace(normalAttr);
}

bt::Vector3 geo::VertexNormalHandle::operator[](attr::Offset vertexOffset) const
{
    if(cached_) return cached_->getValue(vertexOffset);
    return faceNormals_[mesh_.getVertexFace(vertexOffset)];
}

geo::FaceNormalHandle geo::Mesh::getFaceNormal(bool precompute) const
{
    return FaceNormalHandle(*this, precompute);
}

geo::VertexNormalHandle geo::Mesh::getVertexNormal(bool precompute) const
{
    return VertexNormalHandle(*this, precompute);
}
