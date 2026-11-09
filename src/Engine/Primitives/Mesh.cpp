#include "Engine/Primitives/Mesh.h"
#include "Engine/Attribute/Attribute.h"
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Attribute/AttributeNames.h"
#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/AttributeTransfer.h"
#include "Engine/GeometryAlgorithms/MeshUtils.h"
#include "Engine/GeometryAlgorithms/Normals.h"
#include "Engine/Primitives/Primitive.h"
#include "icecream.hpp"
#include <array>
#include <memory>
#include <stdexcept>
#include <string>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/spin_mutex.h>
#include <tbb/task_group.h>

namespace enzo {

geo::Mesh::Mesh(std::string_view path)
    : vertexCountFaceHandle_{addAttribute<intT>(attr::AttrOwner::FACE, "vertexCount", true, true)},
      closedFaceHandle_{addAttribute<boolT>(attr::AttrOwner::FACE, "closed", true, true)},
      pointOffsetVertexHandle_{addAttribute<intT>(attr::AttrOwner::VERTEX, "point", true, true)},
      posPointHandle_{addAttribute<Vector3>(attr::AttrOwner::POINT, attr::names::position, true)},
      validFaceHandle_{addAttribute<boolT>(attr::AttrOwner::FACE, "__valid", true, true)},
      validVertexHandle_{addAttribute<boolT>(attr::AttrOwner::VERTEX, "__valid", true, true)},
      validPointHandle_{addAttribute<boolT>(attr::AttrOwner::POINT, "__valid", true, true)},
      Primitive(path)
{
}

geo::Mesh::Mesh(const Mesh& other)
    : Primitive(other),
      // attributes
      vertexAttributes_{deepCopyAttributes(other.vertexAttributes_)},
      faceAttributes_{deepCopyAttributes(other.faceAttributes_)},
      // groups
      vertexGroups_{deepCopyAttributes(other.vertexGroups_)},
      faceGroups_{deepCopyAttributes(other.faceGroups_)},

      // handles
      vertexCountFaceHandle_{
          attr::AttributeHandleInt(getAttribByName(attr::AttrOwner::FACE, "vertexCount", true))
      },
      closedFaceHandle_{
          attr::AttributeHandleBool(getAttribByName(attr::AttrOwner::FACE, "closed", true))
      },
      pointOffsetVertexHandle_{
          attr::AttributeHandleInt(getAttribByName(attr::AttrOwner::VERTEX, "point", true))
      },
      posPointHandle_{attr::AttributeHandleVector3(
          getAttribByName(attr::AttrOwner::POINT, attr::names::position, true)
      )},
      validFaceHandle_{
          attr::AttributeHandleBool(getAttribByName(attr::AttrOwner::FACE, "__valid", true))
      },
      validVertexHandle_{
          attr::AttributeHandleBool(getAttribByName(attr::AttrOwner::VERTEX, "__valid", true))
      },
      validPointHandle_{
          attr::AttributeHandleBool(getAttribByName(attr::AttrOwner::POINT, "__valid", true))
      },

      // other
      soloPoints_{other.soloPoints_}, soloPointsDirty_{other.soloPointsDirty_},
      needsDefrag_{other.needsDefrag_}, vertexFaces_{other.vertexFaces_},
      faceStarts_{other.faceStarts_}, faceStartsDirty_{other.faceStartsDirty_.load()}
{
}

geo::Mesh& geo::Mesh::operator=(const geo::Mesh& rhs)
{
    if (this == &rhs) return *this;

    Primitive::operator=(rhs);

    // attributes
    vertexAttributes_ = deepCopyAttributes(rhs.vertexAttributes_);
    faceAttributes_ = deepCopyAttributes(rhs.faceAttributes_);

    // groups
    vertexGroups_ = deepCopyAttributes(rhs.vertexGroups_);
    faceGroups_ = deepCopyAttributes(rhs.faceGroups_);

    // handles
    vertexCountFaceHandle_ =
        attr::AttributeHandleInt(getAttribByName(attr::AttrOwner::FACE, "vertexCount", true));
    closedFaceHandle_ =
        attr::AttributeHandleBool(getAttribByName(attr::AttrOwner::FACE, "closed", true));
    pointOffsetVertexHandle_ =
        attr::AttributeHandleInt(getAttribByName(attr::AttrOwner::VERTEX, "point", true));
    posPointHandle_ = attr::AttributeHandleVector3(
        getAttribByName(attr::AttrOwner::POINT, attr::names::position, true)
    );
    validFaceHandle_ =
        attr::AttributeHandleBool(getAttribByName(attr::AttrOwner::FACE, "__valid", true));
    validVertexHandle_ =
        attr::AttributeHandleBool(getAttribByName(attr::AttrOwner::VERTEX, "__valid", true));
    validPointHandle_ =
        attr::AttributeHandleBool(getAttribByName(attr::AttrOwner::POINT, "__valid", true));

    // other
    soloPoints_ = rhs.soloPoints_;
    soloPointsDirty_ = rhs.soloPointsDirty_;
    needsDefrag_ = rhs.needsDefrag_;
    vertexFaces_ = rhs.vertexFaces_;
    faceStarts_ = rhs.faceStarts_;

    faceStartsDirty_.store(rhs.faceStartsDirty_.load());

    return *this;
}

void geo::Mesh::copySourceToTransforms(const Mesh& source, std::span<const Transform> transforms)
{
    const Offset copyCount = transforms.size();
    const Offset sourcePointCount = source.getNumPoints();
    const Offset sourceVertexCount = source.getNumVerts();
    const Offset sourceFaceCount = source.getNumFaces();
    const Offset firstPointOffset = getNumPoints();
    const Offset firstVertexOffset = getNumVerts();
    const Offset firstFaceOffset = getNumFaces();

    // Moves each copy's points by its transform
    const std::span<const Vector3> sourcePositions = source.pointPosSpan();
    std::vector<Vector3> positions(copyCount * sourcePointCount);
    tbb::parallel_for(Offset{0}, copyCount, [&](Offset copyIndex) {
        const Transform& transform = transforms[copyIndex];
        const Offset copyStart = copyIndex * sourcePointCount;
        for (Offset pointOffset = 0; pointOffset < sourcePointCount; ++pointOffset)
            positions[copyStart + pointOffset] = transform * sourcePositions[pointOffset];
    });

    // Joins each copy's faces to that copy's points
    const std::span<const intT> sourceVertexPoints = source.vertexPointSpan();
    std::vector<Offset> facePoints(copyCount * sourceVertexCount);
    tbb::parallel_for(Offset{0}, copyCount, [&](Offset copyIndex) {
        const Offset pointStart = firstPointOffset + copyIndex * sourcePointCount;
        const Offset copyStart = copyIndex * sourceVertexCount;
        for (Offset vertexOffset = 0; vertexOffset < sourceVertexCount; ++vertexOffset)
            facePoints[copyStart + vertexOffset] = pointStart + sourceVertexPoints[vertexOffset];
    });

    const std::span<const intT> sourceVertexCounts = source.vertexCountFaceHandle_.getSpan();
    std::vector<Offset> vertexCounts;
    vertexCounts.reserve(copyCount * sourceFaceCount);
    for (Offset copyIndex = 0; copyIndex < copyCount; ++copyIndex)
        vertexCounts
            .insert(vertexCounts.end(), sourceVertexCounts.begin(), sourceVertexCounts.end());

    addPoints(positions);
    addFaces(facePoints, vertexCounts);

    // Copies the source's closed and deleted states onto every copy
    const auto copyFlagsRepeatedly = [&](attr::AttributeHandleBool& destFlags,
                                         const attr::AttributeHandleBool& sourceFlags,
                                         Offset flagCount,
                                         Offset destStart) {
        for (Offset copyIndex = 0; copyIndex < copyCount; ++copyIndex)
        {
            const Offset copyStart = destStart + copyIndex * flagCount;
            for (Offset flagOffset = 0; flagOffset < flagCount; ++flagOffset)
                destFlags.setValue(copyStart + flagOffset, sourceFlags.getValue(flagOffset));
        }
    };
    copyFlagsRepeatedly(
        closedFaceHandle_,
        source.closedFaceHandle_,
        sourceFaceCount,
        firstFaceOffset
    );

    // Skips the deleted states when the source has no deletions, since new elements start valid
    const bool sourceHasDeletions = source.needsDefrag_;
    if (sourceHasDeletions)
    {
        copyFlagsRepeatedly(
            validFaceHandle_,
            source.validFaceHandle_,
            sourceFaceCount,
            firstFaceOffset
        );
        copyFlagsRepeatedly(
            validVertexHandle_,
            source.validVertexHandle_,
            sourceVertexCount,
            firstVertexOffset
        );
        copyFlagsRepeatedly(
            validPointHandle_,
            source.validPointHandle_,
            sourcePointCount,
            firstPointOffset
        );
        needsDefrag_ = true;
    }

    // Copies the source attributes and groups onto every copy
    utils::copyAttributesRepeatedly(
        source, *this, attr::AttributeOwner::POINT, sourcePointCount, firstPointOffset, copyCount
    );
    utils::copyAttributesRepeatedly(
        source, *this, attr::AttributeOwner::VERTEX, sourceVertexCount, firstVertexOffset, copyCount
    );
    utils::copyAttributesRepeatedly(
        source, *this, attr::AttributeOwner::FACE, sourceFaceCount, firstFaceOffset, copyCount
    );
}

std::shared_ptr<geo::Mesh> geo::Mesh::copyToTransforms(std::span<const Transform> transforms) const
{
    auto copies = std::make_shared<Mesh>(getPath());
    copies->copySourceToTransforms(*this, transforms);
    utils::copyAttributesRepeatedly(*this, *copies, attr::AttributeOwner::PRIMITIVE, 1, 0, 1);
    return copies;
}

void geo::Mesh::applyTransform(const Matrix4& mat, TransformClass transformClass)
{
    if ((transformClass & TransformClass::POINT) != TransformClass::NONE)
    {
        const Offset numPoints = getNumPoints();
        tbb::parallel_for(
            tbb::blocked_range<size_t>(0, numPoints),
            [&](const tbb::blocked_range<size_t>& range) {
                for (size_t i = range.begin(); i < range.end(); ++i)
                {
                    const Vector3& pos = posPointHandle_[i];
                    // Affine point form applies the linear block then adds the
                    // translation column, skipping the homogeneous row.
                    const Vector3 transformed =
                        mat.topLeftCorner<3, 3>() * pos + mat.col(3).head<3>();
                    posPointHandle_.setValue(i, transformed);
                }
            }
        );
    }
}

void geo::Mesh::merge(std::shared_ptr<Primitive> other)
{
    auto otherMesh = std::dynamic_pointer_cast<Mesh>(other);
    if (!otherMesh) throw std::runtime_error("Mesh::merge: type mismatch");
    merge(*otherMesh);
}

void geo::Mesh::duplicateToTransforms(std::span<const Transform> transforms)
{
    copySourceToTransforms(*this, transforms);
}

void geo::Mesh::merge(Mesh& other)
{
    const std::array<Transform, 1> untransformed{};
    copySourceToTransforms(other, untransformed);
}

Offset geo::Mesh::addFace(const std::vector<Offset>& pointOffsets, bool closed)
{
    const Offset faceNum = vertexCountFaceHandle_.getSize();
    for (Offset pointOffset : pointOffsets)
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
    for (auto faceAttribute : faceAttributes_)
    {
        if (faceAttribute->isIntrinsic())
        {
            continue;
        }

        faceAttribute->resize(faceNum + 1);
    }

    // resize face and vertex groups so they stay aligned with element counts
    const Offset newVertCount = pointOffsetVertexHandle_.getSize();
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

std::vector<Offset> geo::Mesh::addFaces(
    std::span<const Offset> pointOffsetsFlat,
    std::span<const Offset> vertexCounts,
    bool closed
)
{
    const Offset firstFaceOffset = vertexCountFaceHandle_.getSize();
    const Offset firstVertOffset = pointOffsetVertexHandle_.getSize();
    const size_t numFacesToAdd = vertexCounts.size();
    const size_t numVertsToAdd = pointOffsetsFlat.size();
    const Offset newFaceCount = firstFaceOffset + numFacesToAdd;
    const Offset newVertCount = firstVertOffset + numVertsToAdd;

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
    vertexFaces_.resize(newVertCount);

    std::vector<Offset> newFaceOffsets;
    newFaceOffsets.reserve(numFacesToAdd);

    Offset vertCursor = firstVertOffset;
    for (size_t faceIndex = 0; faceIndex < numFacesToAdd; ++faceIndex)
    {
        const Offset vertexCount = vertexCounts[faceIndex];
        const Offset faceOffset = firstFaceOffset + faceIndex;
        newFaceOffsets.push_back(faceOffset);

        for (Offset vertIndex = 0; vertIndex < vertexCount; ++vertIndex)
        {
            const Offset pointOffset = pointOffsetsFlat[vertCursor - firstVertOffset];
            pointOffsetVertexHandle_.setValue(vertCursor, pointOffset);
            validVertexHandle_.setValue(vertCursor, true);
            vertexFaces_[vertCursor] = faceOffset;
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

void geo::Mesh::deleteFaces(const std::vector<Offset>& faceOffsets, bool andPoints)
{
    if (faceOffsets.empty()) return;

    // Invalidate each face and its vertices.
    // When cascading to points, remember which points those vertices referenced.
    std::unordered_set<Offset> orphanCandidates;
    const std::span<const Offset> faceStarts = getFaceStartVertices();
    for (Offset faceOffset : faceOffsets)
    {
        validFaceHandle_.setValue(faceOffset, false);

        const Offset start = faceStarts[faceOffset];
        const Offset count = getFaceVertCount(faceOffset);
        for (Offset v = start; v < start + count; ++v)
        {
            if (andPoints) orphanCandidates.insert(pointOffsetVertexHandle_.getValue(v));
            validVertexHandle_.setValue(v, false);
        }
    }

    if (andPoints)
    {
        // Drop candidates still referenced by a surviving vertex.
        const Offset vertCount = pointOffsetVertexHandle_.getSize();
        for (Offset v = 0; v < vertCount; ++v)
        {
            if (!validVertexHandle_.getValue(v)) continue;
            orphanCandidates.erase(pointOffsetVertexHandle_.getValue(v));
        }

        // Invalidate the remaining orphans.
        for (Offset p : orphanCandidates)
            validPointHandle_.setValue(p, false);
    }

    needsDefrag_ = true;
    soloPointsDirty_ = true;
}

void geo::Mesh::deleteAllFaces(bool andPoints) { deleteFaces(getFaces().toVector(), andPoints); }

void geo::Mesh::deletePoints(const std::vector<Offset>& pointOffsets, bool andFaces)
{
    if (pointOffsets.empty()) return;

    std::unordered_set<Offset> deletedPoints(pointOffsets.begin(), pointOffsets.end());

    for (Offset pointOffset : pointOffsets)
    {
        validPointHandle_.setValue(pointOffset, false);
    }

    // Single O(V) pass: mark every vertex referencing a deleted point as invalid.
    // If andFaces is set, also collect the owning faces for cascaded deletion.
    const Offset vertCount = pointOffsetVertexHandle_.getSize();
    const Offset faceCount = vertexCountFaceHandle_.getSize();
    std::vector<bool> faceMarked(andFaces ? faceCount : 0, false);
    for (Offset v = 0; v < vertCount; ++v)
    {
        if (!validVertexHandle_.getValue(v)) continue;
        const Offset pt = pointOffsetVertexHandle_.getValue(v);
        if (!deletedPoints.count(pt)) continue;
        validVertexHandle_.setValue(v, false);
        if (andFaces) faceMarked[getVertexFace(v)] = true;
    }
    if (andFaces)
    {
        std::vector<Offset> facesToDelete;
        for (Offset f = 0; f < faceCount; ++f)
        {
            if (faceMarked[f]) facesToDelete.push_back(f);
        }
        deleteFaces(facesToDelete);
    }

    needsDefrag_ = true;
    soloPointsDirty_ = true;
}

void geo::Mesh::deleteVertices(const std::vector<Offset>& vertOffsets)
{
    if (vertOffsets.empty()) return;

    // Mark vertices invalid and remember which faces lost vertices
    std::unordered_set<Offset> affectedFaces;
    for (Offset v : vertOffsets)
    {
        validVertexHandle_.setValue(v, false);
        affectedFaces.insert(getVertexFace(v));
    }

    // Invalidate any face that has no valid vertices left
    const std::span<const Offset> faceStarts = getFaceStartVertices();
    for (Offset f : affectedFaces)
    {
        if (!validFaceHandle_.getValue(f)) continue;
        const Offset start = faceStarts[f];
        const Offset count = getFaceVertCount(f);
        bool anyValid = false;
        for (Offset v = start; v < start + count; ++v)
        {
            if (validVertexHandle_.getValue(v))
            {
                anyValid = true;
                break;
            }
        }
        if (!anyValid) validFaceHandle_.setValue(f, false);
    }

    needsDefrag_ = true;
    soloPointsDirty_ = true;
}

void geo::Mesh::reverseFaces(const std::vector<Offset>& faceOffsets)
{
    const std::span<const Offset> faceStarts = getFaceStartVertices();
    for (const Offset faceOffset : faceOffsets)
    {
        const Offset start = faceStarts[faceOffset];
        const Offset count = getFaceVertCount(faceOffset);
        for (auto& attr : vertexAttributes_)
            if (attr) attr->reverse(start, count);
        for (auto& group : vertexGroups_)
            if (group) group->reverse(start, count);
    }
}

bool geo::Mesh::isValidFace(Offset offset) const { return validFaceHandle_.getValue(offset); }

bool geo::Mesh::isValidVertex(Offset offset) const { return validVertexHandle_.getValue(offset); }

bool geo::Mesh::isValidPoint(Offset offset) const { return validPointHandle_.getValue(offset); }

void geo::Mesh::defragment()
{
    Primitive::defragment();

    if (!needsDefrag_) return;

    // Update each face's vertex count in case any vertices were deleted
    const Offset oldFaceCount = vertexCountFaceHandle_.getSize();
    const std::span<const Offset> faceStarts = getFaceStartVertices();
    for (Offset f = 0; f < oldFaceCount; ++f)
    {
        if (!validFaceHandle_.getValue(f)) continue;
        const Offset start = faceStarts[f];
        const Offset oldCount = vertexCountFaceHandle_.getValue(f);
        Offset validCount = 0;
        for (Offset v = start; v < start + oldCount; ++v)
        {
            if (validVertexHandle_.getValue(v)) ++validCount;
        }
        vertexCountFaceHandle_.setValue(f, validCount);
    }

    // Compact every face attribute and group by face validity.
    std::vector<bool> faceKeep = validFaceHandle_.getAllValues();
    for (auto& attr : faceAttributes_)
        if (attr) attr->compact(faceKeep);
    for (auto& group : faceGroups_)
        if (group) group->compact(faceKeep);

    // Compact every vertex attribute and group by vertex validity.
    std::vector<bool> vertKeep = validVertexHandle_.getAllValues();
    for (auto& attr : vertexAttributes_)
        if (attr) attr->compact(vertKeep);
    for (auto& group : vertexGroups_)
        if (group) group->compact(vertKeep);

    // Compact every point attribute by point validity, building an old → new
    // offset map so we can update everything that references point offsets.
    std::vector<bool> pointKeep = validPointHandle_.getAllValues();
    std::vector<Offset> pointRemap(pointKeep.size(), 0);
    Offset newPointOffset = 0;
    for (size_t i = 0; i < pointKeep.size(); ++i)
    {
        if (pointKeep[i]) pointRemap[i] = newPointOffset++;
    }
    for (auto& attr : pointAttributes_)
        if (attr) attr->compact(pointKeep);
    for (auto& group : pointGroups_)
        if (group) group->compact(pointKeep);

    // Remap surviving vertex → point references to the compacted point offsets.
    const Offset newVertCount = pointOffsetVertexHandle_.getSize();
    for (Offset v = 0; v < newVertCount; ++v)
    {
        const Offset oldPointOffset = pointOffsetVertexHandle_.getValue(v);
        pointOffsetVertexHandle_.setValue(v, pointRemap[oldPointOffset]);
    }

    soloPointsDirty_ = true;

    // Rebuild vertexFaces_ from the compacted face data.
    vertexFaces_.clear();
    vertexFaces_.reserve(newVertCount);
    const Offset faceCount = vertexCountFaceHandle_.getSize();
    for (Offset f = 0; f < faceCount; ++f)
    {
        const Offset count = getFaceVertCount(f);
        for (Offset v = 0; v < count; ++v)
            vertexFaces_.push_back(f);
    }

    faceStartsDirty_.store(true);
    needsDefrag_ = false;
}

Offset geo::Mesh::addPoint(const Vector3& pos)
{
    const Offset pointOffset = posPointHandle_.getSize();

    posPointHandle_.addValue(pos);
    validPointHandle_.addValue(true);
    soloPointsDirty_ = true;

    return pointOffset;
}

std::vector<Offset> geo::Mesh::addPoints(std::span<const Vector3> positions)
{
    const Offset firstPointOffset = posPointHandle_.getSize();
    const size_t numPointsToAdd = positions.size();
    const Offset newPointCount = firstPointOffset + numPointsToAdd;

    // Grow every point side store in one shot
    for (auto& attribute : pointAttributes_)
    {
        if (attribute) attribute->resize(newPointCount);
    }
    for (auto& group : pointGroups_)
    {
        if (group) group->resize(newPointCount);
    }

    std::vector<Offset> newPointOffsets;
    newPointOffsets.reserve(numPointsToAdd);
    for (size_t pointIndex = 0; pointIndex < numPointsToAdd; ++pointIndex)
    {
        const Offset pointOffset = firstPointOffset + pointIndex;
        posPointHandle_.setValue(pointOffset, positions[pointIndex]);
        validPointHandle_.setValue(pointOffset, true);
        newPointOffsets.push_back(pointOffset);
    }

    soloPointsDirty_ = true;

    return newPointOffsets;
}

std::vector<Offset>
geo::Mesh::duplicatePoints(std::span<const Offset> srcPointOffsets, bool copyAttributes)
{
    // Gather the source positions, then let addPoints grow the stores in one shot
    std::vector<Vector3> positions;
    positions.reserve(srcPointOffsets.size());
    for (Offset srcPointOffset : srcPointOffsets)
    {
        positions.push_back(posPointHandle_.getValue(srcPointOffset));
    }

    std::vector<Offset> newPointOffsets = addPoints(positions);

    // TODO: when copyAttributes is set, copy every point attribute value from each
    // source point to its duplicate. Only positions are carried over for now.
    (void)copyAttributes;

    return newPointOffsets;
}

void geo::Mesh::rebuildSoloPoints() const
{
    soloPoints_.clear();

    // Marks every point a valid vertex uses
    const Offset pointCount = posPointHandle_.getSize();
    std::vector<char> pointUsed(pointCount, false);
    const Offset vertCount = pointOffsetVertexHandle_.getSize();
    for (Offset vertexOffset = 0; vertexOffset < vertCount; ++vertexOffset)
    {
        if (!validVertexHandle_.getValue(vertexOffset)) continue;
        pointUsed[pointOffsetVertexHandle_.getValue(vertexOffset)] = true;
    }

    // Keeps the valid points no vertex uses
    for (Offset pointOffset = 0; pointOffset < pointCount; ++pointOffset)
    {
        if (!pointUsed[pointOffset] && validPointHandle_.getValue(pointOffset))
            soloPoints_.insert(pointOffset);
    }

    soloPointsDirty_ = false;
}

Offset geo::Mesh::getNumSoloPoints() const
{
    if (soloPointsDirty_) rebuildSoloPoints();
    return soloPoints_.size();
}

std::unordered_set<Offset>::const_iterator geo::Mesh::soloPointsBegin() const
{
    if (soloPointsDirty_) rebuildSoloPoints();
    return soloPoints_.begin();
}

std::unordered_set<Offset>::const_iterator geo::Mesh::soloPointsEnd() const
{
    if (soloPointsDirty_) rebuildSoloPoints();
    return soloPoints_.end();
}

Vector3 geo::Mesh::getPosFromVert(Offset vertexOffset) const
{
    const Offset pointOffset = pointOffsetVertexHandle_.getValue(vertexOffset);
    return posPointHandle_.getValue(pointOffset);
}

Vector3 geo::Mesh::getPointPos(Offset pointOffset) const
{
    return posPointHandle_.getValue(pointOffset);
}

Offset geo::Mesh::getVertexFace(Offset vertexOffset) const { return vertexFaces_[vertexOffset]; }

void geo::Mesh::setPointPos(const Offset offset, const Vector3& pos)
{
    posPointHandle_.setValue(offset, pos);
}

unsigned int geo::Mesh::getFacePointCount(Offset faceOffset) const
{
    return getFaceVertCount(faceOffset);
}

std::span<const Offset> geo::Mesh::getFaceStartVertices() const
{
    if (faceStartsDirty_.load())
    {
        tbb::spin_mutex::scoped_lock lock(faceStartsMutex_); // lock
        if (faceStartsDirty_.load())                         // double check
        {
            computeFaceStartVertices();
        }
    }
    return faceStarts_;
}

void geo::Mesh::computeFaceStartVertices() const
{
    const Offset handleSize = vertexCountFaceHandle_.getSize();
    faceStarts_.clear();
    faceStarts_.reserve(handleSize);
    intT faceStart = 0;
    for (size_t i = 0; i < handleSize; ++i)
    {
        faceStarts_.push_back(faceStart);
        faceStart += vertexCountFaceHandle_.getValue(i);
    }
    faceStartsDirty_.store(false);
}

boolT geo::Mesh::isClosed(Offset faceOffset) const
{
    return closedFaceHandle_.getValue(faceOffset);
}

attr::attribVector& geo::Mesh::getAttributeStore(const attr::AttributeOwner& owner)
{
    switch (owner)
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
    switch (owner)
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
    switch (owner)
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
    switch (owner)
    {
    case attr::AttributeOwner::VERTEX:
        return vertexGroups_;
    case attr::AttributeOwner::FACE:
        return faceGroups_;
    default:
        return Primitive::getGroupStore(owner);
    }
}

geo::FaceNormalHandle::FaceNormalHandle(const Mesh& mesh, bool precompute) : mesh_(mesh)
{
    std::shared_ptr<const attr::Attribute> normalAttr =
        mesh.getAttribByName(attr::AttrOwner::FACE, attr::names::normal);
    if (normalAttr)
    {
        cached_.emplace(normalAttr);
        return;
    }

    if (precompute)
    {
        const std::span<const Vector3> positions = mesh.posPointHandle_.getSpan();
        precomputed_.resize(mesh.getNumFaces());
        tbb::parallel_for(
            tbb::blocked_range<Offset>(0, mesh.getNumFaces()),
            [&](const tbb::blocked_range<Offset>& range) {
                for (Offset faceOffset = range.begin(); faceOffset < range.end(); ++faceOffset)
                    precomputed_[faceOffset] =
                        utils::polygonNormal(positions, mesh.getFacePoints(faceOffset));
            }
        );
    }
}

Vector3 geo::FaceNormalHandle::computeNormal(Offset faceOffset) const
{
    // Re-resolve the position span on each call so addPoint between reads
    // can't dangle the data pointer through a std::vector reallocation.
    return utils::polygonNormal(mesh_.posPointHandle_.getSpan(), mesh_.getFacePoints(faceOffset));
}

geo::PointNormalHandle::PointNormalHandle(const Mesh& mesh)
{
    std::shared_ptr<const attr::Attribute> pointNormals =
        mesh.getAttribByName(attr::AttrOwner::POINT, attr::names::normal);
    if (pointNormals)
    {
        attribute_.emplace(pointNormals);
        return;
    }

    computed_ = utils::computePointNormals(mesh);
}

geo::VertexNormalHandle::VertexNormalHandle(const Mesh& mesh, double cuspAngle) : mesh_(mesh)
{
    std::shared_ptr<const attr::Attribute> vertexNormals =
        mesh.getAttribByName(attr::AttrOwner::VERTEX, attr::names::normal);
    if (vertexNormals)
    {
        vertexAttribute_.emplace(vertexNormals);
        return;
    }

    std::shared_ptr<const attr::Attribute> pointNormals =
        mesh.getAttribByName(attr::AttrOwner::POINT, attr::names::normal);
    if (pointNormals)
    {
        pointAttribute_.emplace(pointNormals);
        return;
    }

    computed_ = utils::computeVertexNormals(mesh, cuspAngle);
}

Vector3 geo::VertexNormalHandle::operator[](Offset vertexOffset) const
{
    if (vertexAttribute_) return vertexAttribute_->getValue(vertexOffset);
    if (pointAttribute_) return pointAttribute_->getValue(mesh_.getVertexPoint(vertexOffset));
    return computed_[vertexOffset];
}

geo::FaceNormalHandle geo::Mesh::getFaceNormal(bool precompute) const
{
    return FaceNormalHandle(*this, precompute);
}

geo::PointNormalHandle geo::Mesh::getPointNormal() const
{
    return PointNormalHandle(*this);
}

geo::VertexNormalHandle geo::Mesh::getVertexNormal(double cuspAngle) const
{
    return VertexNormalHandle(*this, cuspAngle);
}

} // namespace enzo
