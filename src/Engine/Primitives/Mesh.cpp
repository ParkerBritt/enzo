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
#include <tbb/task_group.h>

namespace enzo {

geo::Mesh::Mesh(std::string_view path) : Primitive(path)
{
    addAttribute<intT>(attr::AttrOwner::FACE, "vertexCount", true, true);
    addAttribute<boolT>(attr::AttrOwner::FACE, "closed", true, true);
    addAttribute<boolT>(attr::AttrOwner::FACE, "__valid", true, true);
    addAttribute<intT>(attr::AttrOwner::VERTEX, "point", true, true);
    addAttribute<boolT>(attr::AttrOwner::VERTEX, "__valid", true, true);
    addAttribute<intT>(attr::AttrOwner::VERTEX, "face", true, true);
    addAttribute<Vector3>(attr::AttrOwner::POINT, attr::names::position, true);
    addAttribute<boolT>(attr::AttrOwner::POINT, "__valid", true, true);
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
    const std::span<const Vector3> sourcePositions = source.getPointPositions();
    std::vector<Vector3> positions(copyCount * sourcePointCount);
    tbb::parallel_for(Offset{0}, copyCount, [&](Offset copyIndex) {
        const Transform& transform = transforms[copyIndex];
        const Offset copyStart = copyIndex * sourcePointCount;
        for (Offset pointOffset = 0; pointOffset < sourcePointCount; ++pointOffset)
            positions[copyStart + pointOffset] = transform * sourcePositions[pointOffset];
    });

    // Joins each copy's faces to that copy's points
    const std::span<const intT> sourceVertexPoints = source.getVertexPoints();
    std::vector<Offset> facePoints(copyCount * sourceVertexCount);
    tbb::parallel_for(Offset{0}, copyCount, [&](Offset copyIndex) {
        const Offset pointStart = firstPointOffset + copyIndex * sourcePointCount;
        const Offset copyStart = copyIndex * sourceVertexCount;
        for (Offset vertexOffset = 0; vertexOffset < sourceVertexCount; ++vertexOffset)
            facePoints[copyStart + vertexOffset] = pointStart + sourceVertexPoints[vertexOffset];
    });

    const std::span<const intT> sourceVertexCounts = source.getFaceVertexCounts();
    std::vector<Offset> vertexCounts;
    vertexCounts.reserve(copyCount * sourceFaceCount);
    for (Offset copyIndex = 0; copyIndex < copyCount; ++copyIndex)
        vertexCounts
            .insert(vertexCounts.end(), sourceVertexCounts.begin(), sourceVertexCounts.end());

    addPoints(positions);
    addFaces(facePoints, vertexCounts);

    // Copies the source's closed and deleted states onto every copy
    const auto copyFlagsRepeatedly = [&](attr::StoreContainer<boolT>& destFlags,
                                         const attr::StoreContainer<boolT>& sourceFlags,
                                         Offset flagCount,
                                         Offset destStart) {
        for (Offset copyIndex = 0; copyIndex < copyCount; ++copyIndex)
        {
            const Offset copyStart = destStart + copyIndex * flagCount;
            for (Offset flagOffset = 0; flagOffset < flagCount; ++flagOffset)
                destFlags[copyStart + flagOffset] = sourceFlags[flagOffset];
        }
    };
    copyFlagsRepeatedly(
        getWritableFaceClosedFlags(),
        source.getFaceClosedFlags(),
        sourceFaceCount,
        firstFaceOffset
    );

    // Skips the deleted states when the source has no deletions, since new elements start valid
    const bool sourceHasDeletions = source.needsDefrag_;
    if (sourceHasDeletions)
    {
        copyFlagsRepeatedly(
            getWritableFaceValidFlags(),
            source.getFaceValidFlags(),
            sourceFaceCount,
            firstFaceOffset
        );
        copyFlagsRepeatedly(
            getWritableVertexValidFlags(),
            source.getVertexValidFlags(),
            sourceVertexCount,
            firstVertexOffset
        );
        copyFlagsRepeatedly(
            getWritablePointValidFlags(),
            source.getPointValidFlags(),
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
        attr::StoreContainer<Vector3>& positions = getWritablePointPositions();
        tbb::parallel_for(
            tbb::blocked_range<size_t>(0, positions.size()),
            [&](const tbb::blocked_range<size_t>& range) {
                for (size_t i = range.begin(); i < range.end(); ++i)
                {
                    // Affine point form applies the linear block then adds the
                    // translation column, skipping the homogeneous row.
                    positions[i] = mat.topLeftCorner<3, 3>() * positions[i] + mat.col(3).head<3>();
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
    const std::array<Offset, 1> vertexCounts{pointOffsets.size()};
    return addFaces(pointOffsets, vertexCounts, closed).front();
}

std::vector<Offset> geo::Mesh::addFaces(
    std::span<const Offset> pointOffsetsFlat,
    std::span<const Offset> vertexCounts,
    bool closed
)
{
    const std::vector<bool> closedFlags(vertexCounts.size(), closed);
    return addFaces(pointOffsetsFlat, vertexCounts, closedFlags);
}

std::vector<Offset> geo::Mesh::addFaces(
    std::span<const Offset> pointOffsetsFlat,
    std::span<const Offset> vertexCounts,
    const std::vector<bool>& closedFlags
)
{
    const Offset firstFaceOffset = getNumFaces();
    const Offset firstVertOffset = getNumVerts();
    const size_t numFacesToAdd = vertexCounts.size();
    const size_t numVertsToAdd = pointOffsetsFlat.size();
    const Offset newFaceCount = firstFaceOffset + numFacesToAdd;
    const Offset newVertCount = firstVertOffset + numVertsToAdd;

    // Grow every face side and vertex side store in one shot
    for (attr::Attribute* attribute : getWritableAttributesAndGroups(attr::AttributeOwner::FACE))
        attribute->resize(newFaceCount);
    for (attr::Attribute* attribute : getWritableAttributesAndGroups(attr::AttributeOwner::VERTEX))
        attribute->resize(newVertCount);

    std::vector<Offset> newFaceOffsets;
    newFaceOffsets.reserve(numFacesToAdd);

    attr::StoreContainer<intT>& vertexPoints = getWritableVertexPoints();
    attr::StoreContainer<boolT>& vertexValidFlags = getWritableVertexValidFlags();
    attr::StoreContainer<intT>& faceVertexCounts = getWritableFaceVertexCounts();
    attr::StoreContainer<boolT>& faceClosedFlags = getWritableFaceClosedFlags();
    attr::StoreContainer<boolT>& faceValidFlags = getWritableFaceValidFlags();
    attr::StoreContainer<intT>& vertexFaces = getWritableVertexFaces();

    Offset vertCursor = firstVertOffset;
    for (size_t faceIndex = 0; faceIndex < numFacesToAdd; ++faceIndex)
    {
        const Offset vertexCount = vertexCounts[faceIndex];
        const Offset faceOffset = firstFaceOffset + faceIndex;
        newFaceOffsets.push_back(faceOffset);

        for (Offset vertIndex = 0; vertIndex < vertexCount; ++vertIndex)
        {
            const Offset pointOffset = pointOffsetsFlat[vertCursor - firstVertOffset];
            vertexPoints[vertCursor] = pointOffset;
            vertexValidFlags[vertCursor] = true;
            vertexFaces[vertCursor] = faceOffset;
            ++vertCursor;
        }

        faceVertexCounts[faceOffset] = vertexCount;
        faceClosedFlags[faceOffset] = closedFlags[faceIndex];
        faceValidFlags[faceOffset] = true;
    }

    soloPoints_.clear();
    faceStarts_.clear();

    return newFaceOffsets;
}

void geo::Mesh::deleteFaces(const std::vector<Offset>& faceOffsets, bool andPoints)
{
    if (faceOffsets.empty()) return;

    // Invalidate each face and its vertices.
    // When cascading to points, remember which points those vertices referenced.
    std::unordered_set<Offset> orphanCandidates;
    const std::span<const Offset> faceStarts = getFaceStartVertices();
    const std::span<const intT> vertexPoints = getVertexPoints();
    attr::StoreContainer<boolT>& faceValidFlags = getWritableFaceValidFlags();
    attr::StoreContainer<boolT>& vertexValidFlags = getWritableVertexValidFlags();
    for (Offset faceOffset : faceOffsets)
    {
        faceValidFlags[faceOffset] = false;

        const Offset start = faceStarts[faceOffset];
        const Offset count = getFaceVertexCount(faceOffset);
        for (Offset v = start; v < start + count; ++v)
        {
            if (andPoints) orphanCandidates.insert(vertexPoints[v]);
            vertexValidFlags[v] = false;
        }
    }

    if (andPoints)
    {
        // Drop candidates still referenced by a surviving vertex.
        const Offset vertCount = vertexPoints.size();
        for (Offset v = 0; v < vertCount; ++v)
        {
            if (!vertexValidFlags[v]) continue;
            orphanCandidates.erase(vertexPoints[v]);
        }

        // Invalidate the remaining orphans.
        attr::StoreContainer<boolT>& pointValidFlags = getWritablePointValidFlags();
        for (Offset p : orphanCandidates)
            pointValidFlags[p] = false;
    }

    needsDefrag_ = true;
    soloPoints_.clear();
}

void geo::Mesh::deleteAllFaces(bool andPoints) { deleteFaces(getFaces().toVector(), andPoints); }

void geo::Mesh::deletePoints(const std::vector<Offset>& pointOffsets, bool andFaces)
{
    if (pointOffsets.empty()) return;

    std::unordered_set<Offset> deletedPoints(pointOffsets.begin(), pointOffsets.end());

    attr::StoreContainer<boolT>& pointValidFlags = getWritablePointValidFlags();
    for (Offset pointOffset : pointOffsets)
    {
        pointValidFlags[pointOffset] = false;
    }

    // Single O(V) pass: mark every vertex referencing a deleted point as invalid.
    // If andFaces is set, also collect the owning faces for cascaded deletion.
    const std::span<const intT> vertexPoints = getVertexPoints();
    attr::StoreContainer<boolT>& vertexValidFlags = getWritableVertexValidFlags();
    const Offset vertCount = vertexPoints.size();
    const Offset faceCount = getNumFaces();
    std::vector<bool> faceMarked(andFaces ? faceCount : 0, false);
    for (Offset v = 0; v < vertCount; ++v)
    {
        if (!vertexValidFlags[v]) continue;
        const Offset pt = vertexPoints[v];
        if (!deletedPoints.count(pt)) continue;
        vertexValidFlags[v] = false;
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
    soloPoints_.clear();
}

void geo::Mesh::deleteVertices(const std::vector<Offset>& vertOffsets)
{
    if (vertOffsets.empty()) return;

    // Mark vertices invalid and remember which faces lost vertices
    std::unordered_set<Offset> affectedFaces;
    attr::StoreContainer<boolT>& vertexValidFlags = getWritableVertexValidFlags();
    for (Offset v : vertOffsets)
    {
        vertexValidFlags[v] = false;
        affectedFaces.insert(getVertexFace(v));
    }

    // Invalidate any face that has no valid vertices left
    const std::span<const Offset> faceStarts = getFaceStartVertices();
    attr::StoreContainer<boolT>& faceValidFlags = getWritableFaceValidFlags();
    for (Offset f : affectedFaces)
    {
        if (!faceValidFlags[f]) continue;
        const Offset start = faceStarts[f];
        const Offset count = getFaceVertexCount(f);
        bool anyValid = false;
        for (Offset v = start; v < start + count; ++v)
        {
            if (vertexValidFlags[v])
            {
                anyValid = true;
                break;
            }
        }
        if (!anyValid) faceValidFlags[f] = false;
    }

    needsDefrag_ = true;
    soloPoints_.clear();
}

void geo::Mesh::reverseFaces(const std::vector<Offset>& faceOffsets)
{
    const std::span<const Offset> faceStarts = getFaceStartVertices();
    const std::vector<attr::Attribute*> vertexAttributes =
        getWritableAttributesAndGroups(attr::AttributeOwner::VERTEX);
    for (const Offset faceOffset : faceOffsets)
    {
        const Offset start = faceStarts[faceOffset];
        const Offset count = getFaceVertexCount(faceOffset);
        for (attr::Attribute* attribute : vertexAttributes)
            attribute->reverse(start, count);
    }
}

bool geo::Mesh::isValidFace(Offset offset) const { return getFaceValidFlags()[offset]; }

bool geo::Mesh::isValidVertex(Offset offset) const { return getVertexValidFlags()[offset]; }

bool geo::Mesh::isValidPoint(Offset offset) const { return getPointValidFlags()[offset]; }

void geo::Mesh::defragment()
{
    Primitive::defragment();

    if (!needsDefrag_) return;

    // Update each face's vertex count in case any vertices were deleted
    const std::span<const Offset> faceStarts = getFaceStartVertices();
    const attr::StoreContainer<boolT>& faceValidFlags = getFaceValidFlags();
    const attr::StoreContainer<boolT>& vertexValidFlags = getVertexValidFlags();
    attr::StoreContainer<intT>& faceVertexCounts = getWritableFaceVertexCounts();
    const Offset oldFaceCount = faceVertexCounts.size();
    for (Offset f = 0; f < oldFaceCount; ++f)
    {
        if (!faceValidFlags[f]) continue;
        const Offset start = faceStarts[f];
        const Offset oldCount = faceVertexCounts[f];
        Offset validCount = 0;
        for (Offset v = start; v < start + oldCount; ++v)
        {
            if (vertexValidFlags[v]) ++validCount;
        }
        faceVertexCounts[f] = validCount;
    }

    // Compact every face attribute and group by face validity.
    std::vector<bool> faceKeep = getFaceValidFlags();
    for (attr::Attribute* attribute : getWritableAttributesAndGroups(attr::AttributeOwner::FACE))
        attribute->compact(faceKeep);

    // Compact every vertex attribute and group by vertex validity.
    std::vector<bool> vertKeep = getVertexValidFlags();
    for (attr::Attribute* attribute : getWritableAttributesAndGroups(attr::AttributeOwner::VERTEX))
        attribute->compact(vertKeep);

    // Compact every point attribute by point validity, building an old → new
    // offset map so we can update everything that references point offsets.
    std::vector<bool> pointKeep = getPointValidFlags();
    std::vector<Offset> pointRemap(pointKeep.size(), 0);
    Offset newPointOffset = 0;
    for (size_t i = 0; i < pointKeep.size(); ++i)
    {
        if (pointKeep[i]) pointRemap[i] = newPointOffset++;
    }
    for (attr::Attribute* attribute : getWritableAttributesAndGroups(attr::AttributeOwner::POINT))
        attribute->compact(pointKeep);

    // Remap surviving vertex → point references to the compacted point offsets.
    attr::StoreContainer<intT>& vertexPoints = getWritableVertexPoints();
    const Offset newVertCount = vertexPoints.size();
    for (Offset v = 0; v < newVertCount; ++v)
    {
        vertexPoints[v] = pointRemap[vertexPoints[v]];
    }

    // Rebuild each vertex's face from the compacted face data.
    attr::StoreContainer<intT>& vertexFaces = getWritableVertexFaces();
    vertexFaces.clear();
    vertexFaces.reserve(newVertCount);
    const Offset faceCount = getNumFaces();
    for (Offset f = 0; f < faceCount; ++f)
    {
        const Offset count = getFaceVertexCount(f);
        for (Offset v = 0; v < count; ++v)
            vertexFaces.push_back(f);
    }

    soloPoints_.clear();
    faceStarts_.clear();
    needsDefrag_ = false;
}

Offset geo::Mesh::addPoint(const Vector3& pos)
{
    return addPoints(std::span<const Vector3>(&pos, 1)).front();
}

std::vector<Offset> geo::Mesh::addPoints(std::span<const Vector3> positions)
{
    const Offset firstPointOffset = getNumPoints();
    const size_t numPointsToAdd = positions.size();
    const Offset newPointCount = firstPointOffset + numPointsToAdd;

    // Grow every point side store in one shot
    for (attr::Attribute* attribute : getWritableAttributesAndGroups(attr::AttributeOwner::POINT))
        attribute->resize(newPointCount);

    attr::StoreContainer<Vector3>& pointPositions = getWritablePointPositions();
    attr::StoreContainer<boolT>& pointValidFlags = getWritablePointValidFlags();
    std::vector<Offset> newPointOffsets;
    newPointOffsets.reserve(numPointsToAdd);
    for (size_t pointIndex = 0; pointIndex < numPointsToAdd; ++pointIndex)
    {
        const Offset pointOffset = firstPointOffset + pointIndex;
        pointPositions[pointOffset] = positions[pointIndex];
        pointValidFlags[pointOffset] = true;
        newPointOffsets.push_back(pointOffset);
    }

    soloPoints_.clear();

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
        positions.push_back(getPointPosition(srcPointOffset));
    }

    std::vector<Offset> newPointOffsets = addPoints(positions);

    // TODO: when copyAttributes is set, copy every point attribute value from each
    // source point to its duplicate. Only positions are carried over for now.
    (void)copyAttributes;

    return newPointOffsets;
}

const std::unordered_set<Offset>& geo::Mesh::getSoloPoints() const
{
    return soloPoints_.get([this] { return buildSoloPoints(); });
}

std::unordered_set<Offset> geo::Mesh::buildSoloPoints() const
{
    std::unordered_set<Offset> soloPoints;

    // Marks every point a valid vertex uses
    const std::span<const intT> vertexPoints = getVertexPoints();
    const attr::StoreContainer<boolT>& vertexValidFlags = getVertexValidFlags();
    const attr::StoreContainer<boolT>& pointValidFlags = getPointValidFlags();
    const Offset pointCount = getNumPoints();
    std::vector<char> pointUsed(pointCount, false);
    const Offset vertCount = vertexPoints.size();
    for (Offset vertexOffset = 0; vertexOffset < vertCount; ++vertexOffset)
    {
        if (!vertexValidFlags[vertexOffset]) continue;
        pointUsed[vertexPoints[vertexOffset]] = true;
    }

    // Keeps the valid points no vertex uses
    for (Offset pointOffset = 0; pointOffset < pointCount; ++pointOffset)
    {
        if (!pointUsed[pointOffset] && pointValidFlags[pointOffset])
            soloPoints.insert(pointOffset);
    }

    return soloPoints;
}

Offset geo::Mesh::getNumSoloPoints() const
{
    return getSoloPoints().size();
}

Vector3 geo::Mesh::getPosFromVert(Offset vertexOffset) const
{
    return getPointPositions()[getVertexPoint(vertexOffset)];
}

Vector3 geo::Mesh::getPointPosition(Offset pointOffset) const
{
    return getPointPositions()[pointOffset];
}

Offset geo::Mesh::getVertexFace(Offset vertexOffset) const { return getVertexFaces()[vertexOffset]; }

void geo::Mesh::setPointPos(const Offset offset, const Vector3& pos)
{
    getWritablePointPositions()[offset] = pos;
}

unsigned int geo::Mesh::getFacePointCount(Offset faceOffset) const
{
    return getFaceVertexCount(faceOffset);
}

std::span<const Offset> geo::Mesh::getFaceStartVertices() const
{
    return faceStarts_.get([this] { return buildFaceStarts(); });
}

std::vector<Offset> geo::Mesh::buildFaceStarts() const
{
    const std::span<const intT> faceVertexCounts = getFaceVertexCounts();
    std::vector<Offset> faceStarts;
    faceStarts.reserve(faceVertexCounts.size());
    intT faceStart = 0;
    for (const intT vertexCount : faceVertexCounts)
    {
        faceStarts.push_back(faceStart);
        faceStart += vertexCount;
    }
    return faceStarts;
}

boolT geo::Mesh::isClosed(Offset faceOffset) const
{
    return getFaceClosedFlags()[faceOffset];
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
        const std::span<const Vector3> positions = mesh.getPointPositions();
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
    return utils::polygonNormal(mesh_.getPointPositions(), mesh_.getFacePoints(faceOffset));
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
