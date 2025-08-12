#pragma once
#include "Engine/Operator/Attribute.h"
#include "Engine/Types.h"
#include <CGAL/Surface_mesh/Surface_mesh.h>
#include <CGAL/Simple_cartesian.h>
#include "Engine/Operator/AttributeHandle.h"
#include <memory>
#include <oneapi/tbb/spin_mutex.h>
#include <tbb/spin_mutex.h>
#include <variant>



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

using attributeIterator = std::vector<std::shared_ptr<ga::Attribute>>::iterator;

class Geometry
{
public:
    Geometry();
    Geometry(const Geometry& other);
    Geometry& operator=(const Geometry& rhs);
    ga::AttributeHandle<bt::intT> addIntAttribute(ga::AttributeOwner owner, std::string name);
    ga::AttributeHandleBool addBoolAttribute(ga::AttributeOwner owner, std::string name);
    ga::AttributeHandle<bt::Vector3> addVector3Attribute(ga::AttributeOwner owner, std::string name);
    // TODO: return weak ptr
    std::shared_ptr<ga::Attribute> getAttribByName(ga::AttributeOwner owner, std::string name);

    const size_t getNumAttributes(const ga::AttributeOwner owner) const;
    std::weak_ptr<const ga::Attribute> getAttributeByIndex(ga::AttributeOwner owner, unsigned int index) const;
    // attributeIterator attributesBegin(const ga::AttributeOwner owner);
    // attributeIterator attributesEnd(const ga::AttributeOwner owner);

    std::vector<bt::Vector3> derivePointNormals();
    HeMesh computeHalfEdgeMesh();

    // build geometry
    void addFace(const std::vector<ga::Offset>& pointOffsets, bool closed=true);
    template <typename... Args>
    void addFace(Args... args)
    {
        std::vector<ga::Offset> pointOffsets = { static_cast<ga::Offset>(args)... };
        addFace(pointOffsets);

    }
    void addPoint(const bt::Vector3& pos);

    std::set<ga::Offset>::const_iterator soloPointsBegin();
    std::set<ga::Offset>::const_iterator soloPointsEnd();

    void setPointPos(const ga::Offset offset, const bt::Vector3& pos);

    ga::Offset      getPrimStartVertex(ga::Offset primOffset) const; // returns the first vertex of the primitive
    bt::Vector3     getPosFromVert(ga::Offset vertexOffset) const;
    bt::Vector3     getPointPos(ga::Offset pointOffset) const;
    unsigned int    getPrimVertCount(ga::Offset primOffset) const;
    ga::Offset      getVertexPrim(ga::Offset vertexOffset) const;
    ga::Offset      getNumPrims() const;
    ga::Offset      getNumVerts() const;
    ga::Offset      getNumPoints() const;
    ga::Offset      getNumSoloPoints() const;

    bt::boolT isClosed(ga::Offset primOffset) const;

    void computePrimStartVertices() const;
private:
    using attribVector = std::vector<std::shared_ptr<ga::Attribute>>;
    geo::Geometry::attribVector& getAttributeStore(const ga::AttributeOwner& owner);
    const geo::Geometry::attribVector& getAttributeStore(const ga::AttributeOwner& owner) const;

    attribVector deepCopyAttributes(attribVector source);

    attribVector pointAttributes_;
    attribVector vertexAttributes_;
    attribVector primitiveAttributes_;
    attribVector globalAttributes_;

    std::set<ga::Offset> soloPoints_;

    mutable std::vector<ga::Offset> primStarts_;
    mutable std::vector<ga::Offset> vertexPrims_;

    mutable std::atomic<bool> primStartsDirty_{true};
    mutable tbb::spin_mutex primStartsMutex_;

    // handles
    enzo::ga::AttributeHandleInt vertexCountHandlePrim_;
    enzo::ga::AttributeHandleBool closedHandlePrim_;
    enzo::ga::AttributeHandleInt pointOffsetHandleVert_;
    enzo::ga::AttributeHandleVector3 posHandlePoint_;
};
}
