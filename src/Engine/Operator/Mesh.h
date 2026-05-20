#pragma once
#include "Engine/Operator/Primitive.h"
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
    void applyTransform(const bt::Matrix4 &mat, TransformClass transformClass) override;
    bool canMerge() const override { return true; }
    void merge(std::shared_ptr<Primitive> other) override;
    bool hasPoints() const override { return true; }

    /**
     * @brief Adds a single face. Avoid for multiple faces in a loop as a single call to @ref addFaces is much more performant.
     * @return Offset of the new face.
     */
    // TODO: benchmark addFace vs addFaces to quantify the speedup
    ga::Offset addFace(const std::vector<ga::Offset>& pointOffsets, bool closed=true);
    /**
     * @brief Adds many faces in a single call.
     *
     * @param pointOffsetsFlat All point offsets for every face listed one after another.
     * @param vertexCounts How many points each face has. Read in the same order as pointOffsetsFlat.
     * @return Offsets of the newly created faces in the order they were added.
     */
    std::vector<ga::Offset> addFaces(std::span<const ga::Offset> pointOffsetsFlat,
                                     std::span<const ga::Offset> vertexCounts,
                                     bool closed=true);
    ga::Offset addPoint(const bt::Vector3& pos);

    void deleteFaces(const std::vector<ga::Offset>& faceOffsets, bool andPoints = false);
    void deletePoints(const std::vector<ga::Offset>& pointOffsets) override
    {
        deletePoints(pointOffsets, false);
    }
    void deletePoints(const std::vector<ga::Offset>& pointOffsets, bool andFaces);
    void deleteVertices(const std::vector<ga::Offset>& vertOffsets);
    bool isValidFace(ga::Offset offset) const;
    bool isValidVertex(ga::Offset offset) const;
    bool isValidPoint(ga::Offset offset) const override;

    void defragment() override;

    void merge(Mesh& other);

    HeMesh computeHalfEdgeMesh();

    std::unordered_set<ga::Offset>::const_iterator soloPointsBegin() const;
    std::unordered_set<ga::Offset>::const_iterator soloPointsEnd() const;

    void setPointPos(const ga::Offset offset, const bt::Vector3& pos);
    ga::Offset getFaceStartVertex(ga::Offset faceOffset) const;
    bt::Vector3 getPosFromVert(ga::Offset vertexOffset) const;
    bt::Vector3 getPointPos(ga::Offset pointOffset) const;
    unsigned int getFaceVertCount(ga::Offset faceOffset) const;
    ga::Offset getVertexFace(ga::Offset vertexOffset) const;

    ga::Offset getPointVertex(ga::Offset vertexOffset) const
    {
        return pointOffsetVertexHandle_.getValue(vertexOffset);
    }

    std::span<const bt::intT> getFacePoints(ga::Offset faceOffset) const
    {
        const ga::Offset start = getFaceStartVertex(faceOffset);
        const unsigned int count = getFaceVertCount(faceOffset);
        return pointOffsetVertexHandle_.getSpan().subspan(start, count);
    }

    ga::Offset getNumFaces() const { return getElementCount(ga::AttributeOwner::FACE); }
    ga::Offset getNumVerts() const { return getElementCount(ga::AttributeOwner::VERTEX); }
    ga::Offset getNumSoloPoints() const;

    /// @brief Creates a vertex group.
    /// @return Handle to the new group.
    ga::AttributeHandleBool createVertexGroup(std::string name) {
        return createGroup(ga::AttributeOwner::VERTEX, std::move(name));
    }
    /// @brief Creates a face group.
    /// @return Handle to the new group.
    ga::AttributeHandleBool createFaceGroup(std::string name) {
        return createGroup(ga::AttributeOwner::FACE, std::move(name));
    }
    /// @brief Marks the given offsets as members of the vertex group.
    void addToVertexGroup(const std::string& name, const std::vector<ga::Offset>& offsets) {
        addToGroup(ga::AttributeOwner::VERTEX, name, offsets);
    }
    /// @brief Marks the given offsets as members of the face group.
    void addToFaceGroup(const std::string& name, const std::vector<ga::Offset>& offsets) {
        addToGroup(ga::AttributeOwner::FACE, name, offsets);
    }

    bt::boolT isClosed(ga::Offset faceOffset) const;

    void computeFaceStartVertices() const;

protected:
    ga::attribVector& getAttributeStore(const ga::AttributeOwner& owner) override;
    const ga::attribVector& getAttributeStore(const ga::AttributeOwner& owner) const override;
    ga::attribVector& getGroupStore(const ga::AttributeOwner& owner) override;
    const ga::attribVector& getGroupStore(const ga::AttributeOwner& owner) const override;

private:
    void mergeAppend(std::shared_ptr<ga::Attribute> dst, std::shared_ptr<ga::Attribute> src);
    template <typename T>
    void mergeAppendImpl(std::shared_ptr<ga::Attribute> dst, std::shared_ptr<ga::Attribute> src)
    {
        auto dstHandle = ga::AttributeHandle<T>(dst);
        auto srcHandle = ga::AttributeHandle<T>(src);

        const ga::Offset srcCount = srcHandle.getSize();
        const ga::Offset dstCount = dstHandle.getSize();

        dstHandle.resize(dstCount + srcCount);

        for(ga::Offset i = 0; i< srcCount; ++i)
        {
            const T value = srcHandle.getValue(i);
            const ga::Offset dstOffset = dstCount+i;
            dstHandle.setValue(dstOffset, value);
        }
    };

    ga::attribVector vertexAttributes_;
    ga::attribVector faceAttributes_;
    ga::attribVector vertexGroups_;
    ga::attribVector faceGroups_;

    mutable std::unordered_set<ga::Offset> soloPoints_;
    mutable bool soloPointsDirty_ = true;
    void rebuildSoloPoints() const;
    bool needsDefrag_ = false;

    mutable std::vector<ga::Offset> faceStarts_;
    mutable std::vector<ga::Offset> vertexFaces_;

    mutable std::atomic<bool> faceStartsDirty_{true};
    mutable tbb::spin_mutex faceStartsMutex_;

    // intrinsic handles
    enzo::ga::AttributeHandleInt vertexCountFaceHandle_;
    enzo::ga::AttributeHandleBool closedFaceHandle_;
    enzo::ga::AttributeHandleInt pointOffsetVertexHandle_;
    enzo::ga::AttributeHandleVector3 posPointHandle_;
    enzo::ga::AttributeHandleBool validFaceHandle_;
    enzo::ga::AttributeHandleBool validVertexHandle_;
    enzo::ga::AttributeHandleBool validPointHandle_;
};
}
