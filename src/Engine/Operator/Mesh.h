#pragma once
#include "Engine/Operator/Primitive.h"
#include <CGAL/Surface_mesh/Surface_mesh.h>
#include <CGAL/Simple_cartesian.h>
#include <tbb/spin_mutex.h>
#include <set>

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
    Mesh();
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

    void addFace(const std::vector<ga::Offset>& pointOffsets, bool closed=true);
    ga::Offset addPoint(const bt::Vector3& pos);

    void merge(Mesh& other);

    HeMesh computeHalfEdgeMesh();

    std::set<ga::Offset>::const_iterator soloPointsBegin() const;
    std::set<ga::Offset>::const_iterator soloPointsEnd() const;

    void setPointPos(const ga::Offset offset, const bt::Vector3& pos);
    ga::Offset getPrimStartVertex(ga::Offset primOffset) const;
    bt::Vector3 getPosFromVert(ga::Offset vertexOffset) const;
    bt::Vector3 getPointPos(ga::Offset pointOffset) const;
    unsigned int getPrimVertCount(ga::Offset primOffset) const;
    ga::Offset getVertexPrim(ga::Offset vertexOffset) const;

    ga::Offset getNumPrims() const;
    ga::Offset getNumVerts() const;
    ga::Offset getNumPoints() const override;
    ga::Offset getNumSoloPoints() const;

    bt::boolT isClosed(ga::Offset primOffset) const;

    void computePrimStartVertices() const;

protected:
    ga::attribVector& getAttributeStore(const ga::AttributeOwner& owner) override;
    const ga::attribVector& getAttributeStore(const ga::AttributeOwner& owner) const override;

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

    std::set<ga::Offset> soloPoints_;

    mutable std::vector<ga::Offset> primStarts_;
    mutable std::vector<ga::Offset> vertexPrims_;

    mutable std::atomic<bool> primStartsDirty_{true};
    mutable tbb::spin_mutex primStartsMutex_;

    // intrinsic handles
    enzo::ga::AttributeHandleInt vertexCountHandlePrim_;
    enzo::ga::AttributeHandleBool closedHandlePrim_;
    enzo::ga::AttributeHandleInt pointOffsetHandleVert_;
    enzo::ga::AttributeHandleVector3 posHandlePoint_;
};
}
