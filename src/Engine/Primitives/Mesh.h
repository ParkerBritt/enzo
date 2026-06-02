#pragma once
#include "Engine/Primitives/Primitive.h"
#include <CGAL/Surface_mesh/Surface_mesh.h>
#include <CGAL/Simple_cartesian.h>
#include <tbb/spin_mutex.h>
#include <span>
#include <unordered_set>

namespace enzo::geo
{
using Kernel = CGAL::Simple_cartesian<double>;
using Point  = Kernel::Point_3;
using Vector   = Kernel::Vector_3;
using HeMesh      = CGAL::Surface_mesh<Point>;
using vertexDescriptor = HeMesh::Vertex_index;
using faceDescriptor = HeMesh::Face_index;
using V_index  = HeMesh::Vertex_index;
using F_index  = HeMesh::Face_index;

class Mesh;
class FaceNormalHandle;
class VertexNormalHandle;

/**
* @class enzo::geo::Mesh
* @brief Polygonal mesh primitive with point, vertex, and face attributes.
*
* Extends Primitive with mesh-specific topology: points, vertices, faces,
* and the intrinsic attributes that describe connectivity (vertexCount,
* closed, point offset, and position P).
*/
class Mesh : public Primitive
{
public:
    Mesh(std::string_view path="/mesh");
    Mesh(const Mesh& other);
    Mesh& operator=(const Mesh& rhs);
    ~Mesh() override = default;

    PrimType getType() const override { return PrimType::MESH; }
    std::shared_ptr<Primitive> clone() const override { return std::make_shared<Mesh>(*this); }
    TransformClass transformType() const override { return TransformClass::POINT | TransformClass::PRIMITIVE; }
    void applyTransform(const Matrix4 &mat, TransformClass transformClass = TransformClass::POINT) override;
    bool canMerge() const override { return true; }
    void merge(std::shared_ptr<Primitive> other) override;
    bool hasPoints() const override { return true; }

    /**
     * @brief Adds a single face. Avoid for multiple faces in a loop as a single call to @ref addFaces is much more performant.
     * @return Offset of the new face.
     */
    // TODO: benchmark addFace vs addFaces to quantify the speedup
    Offset addFace(const std::vector<Offset>& pointOffsets, bool closed=true);
    /**
     * @brief Adds many faces in a single call.
     *
     * @param pointOffsetsFlat All point offsets for every face listed one after another.
     * @param vertexCounts How many points each face has. Read in the same order as pointOffsetsFlat.
     * @return Offsets of the newly created faces in the order they were added.
     */
    std::vector<Offset> addFaces(std::span<const Offset> pointOffsetsFlat,
                                     std::span<const Offset> vertexCounts,
                                     bool closed=true);
    Offset addPoint(const Vector3& pos);
    /**
     * @brief Adds many points in a single call.
     *
     * @param positions Position for each new point, read in order.
     * @return Offsets of the newly created points in the order they were added.
     */
    std::vector<Offset> addPoints(std::span<const Vector3> positions);
    /**
     * @brief Duplicates existing points into new points carrying the same positions.
     *
     * @param srcPointOffsets Offsets of the points to duplicate, read in order.
     * @param copyAttributes When true, copy every point attribute from each source point. When false, only positions are copied.
     * @return Offsets of the newly created points in the same order as srcPointOffsets.
     */
    std::vector<Offset> duplicatePoints(std::span<const Offset> srcPointOffsets, bool copyAttributes = true);

    void deleteFaces(const std::vector<Offset>& faceOffsets, bool andPoints = true);
    void deletePoints(const std::vector<Offset>& pointOffsets) override
    {
        deletePoints(pointOffsets, false);
    }
    void deletePoints(const std::vector<Offset>& pointOffsets, bool andFaces);
    void deleteVertices(const std::vector<Offset>& vertOffsets);
    bool isValidFace(Offset offset) const;
    bool isValidVertex(Offset offset) const;
    bool isValidPoint(Offset offset) const override;

    void defragment() override;

    void merge(Mesh& other);

    HeMesh computeHalfEdgeMesh();

    std::unordered_set<Offset>::const_iterator soloPointsBegin() const;
    std::unordered_set<Offset>::const_iterator soloPointsEnd() const;

    void setPointPos(const Offset offset, const Vector3& pos);
    Offset getFaceStartVertex(Offset faceOffset) const;
    Vector3 getPosFromVert(Offset vertexOffset) const;
    Vector3 getPointPos(Offset pointOffset) const;
    unsigned int getFaceVertCount(Offset faceOffset) const;
    unsigned int getFacePointCount(Offset faceOffset) const;
    Offset getVertexFace(Offset vertexOffset) const;

    Offset getPointVertex(Offset vertexOffset) const
    {
        return pointOffsetVertexHandle_.getValue(vertexOffset);
    }

    std::span<const intT> getFacePoints(Offset faceOffset) const
    {
        const Offset start = getFaceStartVertex(faceOffset);
        const unsigned int count = getFaceVertCount(faceOffset);
        return pointOffsetVertexHandle_.getSpan().subspan(start, count);
    }

    Offset getNumFaces() const { return getElementCount(attr::AttributeOwner::FACE); }
    Offset getNumVerts() const { return getElementCount(attr::AttributeOwner::VERTEX); }
    Offset getNumSoloPoints() const;

    // Face Iterator
    struct FaceOffsets {
        FaceOffsets(const Mesh& mesh) : mesh_(mesh) {}
        struct Iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = Offset;

            explicit Iterator(Offset current) : curOffset_(current) {}
            value_type operator*() const { return curOffset_; }
            Iterator& operator++() { ++curOffset_; return *this; }
            Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }
            friend bool operator==(const Iterator& a, const Iterator& b) { return a.curOffset_ == b.curOffset_; }
            friend bool operator!=(const Iterator& a, const Iterator& b) { return a.curOffset_ != b.curOffset_; }

          private:
            Offset curOffset_ = 0;
        };
        Iterator begin() const { return Iterator(0); }
        Iterator end() const { return Iterator(mesh_.getNumFaces()); }

        /// @brief Collects the face offsets into a vector for callers that need one.
        std::vector<Offset> toVector() const
        {
            std::vector<Offset> offsets;
            offsets.reserve(mesh_.getNumFaces());
            for(const Offset faceOffset : *this) offsets.push_back(faceOffset);
            return offsets;
        }

      private:
        const Mesh& mesh_;
    };
    // TODO walk only valid faces like Primitive::PointOffsets once deletion settles. For now it is dense.
    FaceOffsets getFaces() const { return FaceOffsets(*this); }

    /// @brief Creates a vertex group.
    /// @return Handle to the new group.
    attr::AttributeHandleBool createVertexGroup(std::string name) {
        return createGroup(attr::AttributeOwner::VERTEX, std::move(name));
    }
    /// @brief Creates a face group.
    /// @return Handle to the new group.
    attr::AttributeHandleBool createFaceGroup(std::string name) {
        return createGroup(attr::AttributeOwner::FACE, std::move(name));
    }
    /// @brief Marks the given offsets as members of the vertex group.
    void addToVertexGroup(const std::string& name, const std::vector<Offset>& offsets) {
        addToGroup(attr::AttributeOwner::VERTEX, name, offsets);
    }
    /// @brief Marks the given offsets as members of the face group.
    void addToFaceGroup(const std::string& name, const std::vector<Offset>& offsets) {
        addToGroup(attr::AttributeOwner::FACE, name, offsets);
    }

    boolT isClosed(Offset faceOffset) const;

    void computeFaceStartVertices() const;

    /**
     * @brief Returns a handle for reading per-face normals.
     *
     * Normals follow the right hand rule on CCW winding viewed from outside
     * the surface. When the mesh has a face attribute named Normal the handle
     * reads it directly. Otherwise normals are computed via Newell's method.
     *
     * @param precompute When true and no Normal face attribute is present,
     *                   every face normal is computed up front so subsequent
     *                   reads are O(1). Pay this cost when the caller plans
     *                   to read most faces; skip it for sparse access.
     * @return Handle whose operator[] returns the normal for a face offset.
     */
    FaceNormalHandle getFaceNormal(bool precompute = false) const;

    /**
     * @brief Returns a handle for reading per-vertex normals.
     *
     * When the mesh has a vertex attribute named Normal the handle reads it
     * directly. Otherwise the handle returns the owning face's normal.
     *
     * @param precompute Forwarded to the internal FaceNormalHandle when the
     *                   vertex attribute is absent. Useful when many vertices
     *                   on a small number of faces are read.
     * @return Handle whose operator[] returns the normal for a vertex offset.
     */
    VertexNormalHandle getVertexNormal(bool precompute = false) const;

    friend class FaceNormalHandle;
    friend class VertexNormalHandle;

protected:
    attr::attribVector& getAttributeStore(const attr::AttributeOwner& owner) override;
    const attr::attribVector& getAttributeStore(const attr::AttributeOwner& owner) const override;
    attr::attribVector& getGroupStore(const attr::AttributeOwner& owner) override;
    const attr::attribVector& getGroupStore(const attr::AttributeOwner& owner) const override;

private:
    void mergeAppend(std::shared_ptr<attr::Attribute> dst, std::shared_ptr<attr::Attribute> src);
    template <typename T>
    void mergeAppendImpl(std::shared_ptr<attr::Attribute> dst, std::shared_ptr<attr::Attribute> src)
    {
        auto dstHandle = attr::AttributeHandle<T>(dst);
        auto srcHandle = attr::AttributeHandle<T>(src);

        const Offset srcCount = srcHandle.getSize();
        const Offset dstCount = dstHandle.getSize();

        dstHandle.resize(dstCount + srcCount);

        for(Offset i = 0; i< srcCount; ++i)
        {
            const T value = srcHandle.getValue(i);
            const Offset dstOffset = dstCount+i;
            dstHandle.setValue(dstOffset, value);
        }
    };

    attr::attribVector vertexAttributes_;
    attr::attribVector faceAttributes_;
    attr::attribVector vertexGroups_;
    attr::attribVector faceGroups_;

    mutable std::unordered_set<Offset> soloPoints_;
    mutable bool soloPointsDirty_ = true;
    void rebuildSoloPoints() const;
    bool needsDefrag_ = false;

    mutable std::vector<Offset> faceStarts_;
    mutable std::vector<Offset> vertexFaces_;

    mutable std::atomic<bool> faceStartsDirty_{true};
    mutable tbb::spin_mutex faceStartsMutex_;

    // intrinsic handles
    enzo::attr::AttributeHandleInt vertexCountFaceHandle_;
    enzo::attr::AttributeHandleBool closedFaceHandle_;
    enzo::attr::AttributeHandleInt pointOffsetVertexHandle_;
    enzo::attr::AttributeHandleVector3 posPointHandle_;
    enzo::attr::AttributeHandleBool validFaceHandle_;
    enzo::attr::AttributeHandleBool validVertexHandle_;
    enzo::attr::AttributeHandleBool validPointHandle_;
};

/**
 * @class enzo::geo::FaceNormalHandle
 * @brief Read accessor for per-face normals.
 *
 * Built by @ref Mesh::getFaceNormal. The constructor resolves the Normal
 * face attribute once. If present, operator[] reads it. If absent and
 * precompute was true, operator[] reads a buffer filled at construction.
 * If absent and precompute was false, operator[] runs Newell's method
 * on the requested face's points.
 */
class FaceNormalHandle
{
public:
    Vector3 operator[](Offset faceOffset) const;
private:
    friend class Mesh;
    FaceNormalHandle(const Mesh& mesh, bool precompute);
    const Mesh& mesh_;
    std::optional<attr::AttributeHandleRO<Vector3>> cached_;
    std::vector<Vector3> precomputed_;
};

/**
 * @class enzo::geo::VertexNormalHandle
 * @brief Read accessor for per-vertex normals.
 *
 * Built by @ref Mesh::getVertexNormal. The constructor resolves the Normal
 * vertex attribute once. If present, operator[] reads it. If absent, the
 * handle returns the owning face's normal via an internal FaceNormalHandle.
 */
class VertexNormalHandle
{
public:
    Vector3 operator[](Offset vertexOffset) const;
private:
    friend class Mesh;
    VertexNormalHandle(const Mesh& mesh, bool precompute);
    const Mesh& mesh_;
    std::optional<attr::AttributeHandleRO<Vector3>> cached_;
    FaceNormalHandle faceNormals_;
};
}
