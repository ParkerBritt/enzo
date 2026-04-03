#pragma once
#include "Engine/Operator/Attribute.h"
#include "Engine/Types.h"
#include <CGAL/Surface_mesh/Surface_mesh.h>
#include <CGAL/Simple_cartesian.h>
#include "Engine/Operator/AttributeHandle.h"
#include <memory>
#include <tbb/spin_mutex.h>
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

/**
* @class enzo::geo::Geometry
* @brief [Attribute](@ref enzo::ga::Attribute) based geometry container exchanged and modified by nodes.
*
* @todo outdated docs, rewrite
*
* The Geometry class represents the core data being passed between nodes
* in the engine. It is the operand of the network. Nodes read, modify,
* and output geometry, which is then used to populate the viewport and
* the geometry spreadsheet. Geometry is stored in an [attribute](@ref enzo::ga::Attribute) based
* structure, similar to a spreadsheet, where each column is an [attribute](@ref enzo::ga::Attribute)
* owned by points, vertices, primitives, or the global context. Most geometry
* information, including built in properties such as positions and connectivity,
* is stored through this same system. These built in [attributes](@ref enzo::ga::Attribute) are intrinsic,
* but they share the same base as user-defined [attributes](@ref enzo::ga::Attribute) to keep the data
* model consistent.
*
* The class provides methods for constructing and querying geometry, including
* adding points and faces, computing normals, and building a CGAL half-edge
* mesh representation. It also exposes accessors for reading positions,
* primitive connectivity, and vertex data, while tracking special cases
* such as isolated points.
*
* @post Geometry in instatiated with some intrinsic attributes required
* to construct geometry, such as P (position) on point attributes, vertexCount
* and closed on primitive attributes, and point on vertices. It is recommended
* not to modify these directy but instead use the helper functions.
*/
class Primitive
{
public:
    Primitive();
    Primitive(const Primitive& other);

   /**
    * @brief Assignment operator. Performs a deep copy of another Geometry.
    * @param rhs The Geometry object to copy from.
    * @return Reference to this Geometry.
    */
    Primitive& operator=(const Primitive& rhs);

    /**
    * @brief Adds an integer attribute to the geometry.
    * @param owner The attribute owner (point, vertex, primitive, detail).
    * @param name The name of the attribute.
    * @return Handle to the created integer attribute.
    */
    ga::AttributeHandle<bt::intT> addIntAttribute(ga::AttributeOwner owner, std::string name, bool intrinsic=false);

    /**
    * @brief Adds a boolean attribute to the geometry.
    * @param owner The attribute owner (point, vertex, primitive, detail).
    * @param name The name of the attribute.
    * @return Handle to the created boolean attribute.
    */
    ga::AttributeHandleBool addBoolAttribute(ga::AttributeOwner owner, std::string name, bool intrinsic=false);

    /**
    * @brief Adds a 3D vector attribute to the geometry.
    * @param owner The attribute owner (point, vertex, primitive, detail).
    * @param name The name of the attribute.
    * @return Handle to the created Vector3 attribute.
    */
    ga::AttributeHandle<bt::Vector3> addVector3Attribute(ga::AttributeOwner owner, std::string name, bool intrinsic=false);

    /**
    * @brief Retrieves an attribute by its name.
    * @param owner The attribute owner.
    * @param name The name of the attribute.
    * @return Shared pointer to the attribute if found, otherwise nullptr.
    */
    std::shared_ptr<ga::Attribute> getAttribByName(ga::AttributeOwner owner, std::string name, bool includeIntrinsics=false);

    /**
    * @brief Gets the number of attributes owned by a specific element type.
    * @param owner The attribute owner.
    * @return Number of attributes.
    */
    const size_t getNumAttributes(const ga::AttributeOwner owner) const;

    /**
    * @brief Retrieves an attribute by its index.
    * @param owner The attribute owner.
    * @param index Index of the attribute.
    * @return Weak pointer to the attribute.
    */
    std::weak_ptr<const ga::Attribute> getAttributeByIndex(ga::AttributeOwner owner, unsigned int index) const;

    /**
    * @brief Computes per-point normals for the geometry.
    * @return Vector of computed normals corresponding to each point.
    */
    // std::vector<bt::Vector3> derivePointNormals();

    /**
    * @brief Builds a CGAL half-edge mesh representation of the geometry.
    * @return The constructed half-edge mesh.
    */
    HeMesh computeHalfEdgeMesh();

    /**
    * @brief Adds a polygonal face to the geometry.
    * @param pointOffsets Offsets of the points forming the face.
    * @param closed Whether the face is closed (default true).
    */
    void addFace(const std::vector<ga::Offset>& pointOffsets, bool closed=true);

    /**
    * @brief Adds a point to the geometry.
    * @param pos Position of the new point.
    * @returns Point offset of the newly created point
    */
    ga::Offset addPoint(const bt::Vector3& pos);

    /**
    * @brief Iterator to the beginning of the set of solo (isolated) points.
    * @return Const iterator to the first solo point.
    */
    std::set<ga::Offset>::const_iterator soloPointsBegin();

    /**
    * @brief Iterator to the end of the set of solo (isolated) points.
    * @return Const iterator past the last solo point.
    */
    std::set<ga::Offset>::const_iterator soloPointsEnd();

    /**
    * @brief Sets the position of a point.
    * @param offset Offset of the point.
    * @param pos New position.
    */
    void setPointPos(const ga::Offset offset, const bt::Vector3& pos);

    /**
    * @brief Gets the first vertex of a primitive.
    * @param primOffset Offset of the primitive.
    * @return Offset of the first vertex.
    */
    ga::Offset getPrimStartVertex(ga::Offset primOffset) const;

    /**
    * @brief Gets the 3 dimensional position from a vertex offset.
    * @param vertexOffset Offset of the vertex.
    * @return Position of the vertex.
    */
    bt::Vector3 getPosFromVert(ga::Offset vertexOffset) const;

    /**
    * @brief Gets the position of a point.
    * @param pointOffset Offset of the point.
    * @return Position of the point.
    */
    bt::Vector3 getPointPos(ga::Offset pointOffset) const;

    /**
    * @brief Gets the number of vertices in a primitive.
    * @param primOffset Offset of the primitive.
    * @return Number of vertices.
    */
    unsigned int getPrimVertCount(ga::Offset primOffset) const;

    /**
    * @brief Gets the primitive that owns a given vertex.
    * @param vertexOffset Offset of the vertex.
    * @return Offset of the owning primitive.
    */
    ga::Offset getVertexPrim(ga::Offset vertexOffset) const;

    /**
    * @brief Gets the number of primitives in the geometry.
    * @return Number of primitives.
    */
    ga::Offset getNumPrims() const;

    /**
    * @brief Gets the number of vertices in the geometry.
    * @return Number of vertices.
    */
    ga::Offset getNumVerts() const;

    /**
    * @brief Gets the number of points in the geometry.
    * @return Number of points.
    */
    ga::Offset getNumPoints() const;


    /**
    * @brief Gets the number of isolated points in the geometry.
    * Isolated points are points that are not connnected to any other point
    * and don't belong to a primitive.
    * @return Number of solo points.
    */
    ga::Offset getNumSoloPoints() const;

    /**
    * @brief Checks if a primitive is closed or open.
    *
    * Open primitives are treated and rendered as curves.
    *
    * @param primOffset Offset of the primitive.
    * @return True if the primitive is closed, false otherwise.
    */
    bt::boolT isClosed(ga::Offset primOffset) const;

    /**
    * @brief Merges another geometry into this one.
    * @param other The geometry to merge.
    */
    void merge(Primitive& other);

    /**
    * @brief Checks if the given attribute exists
    * @param attribName The name of the attribute to find
    */
    bool attributeExists(ga::AttributeOwner owner, std::string name);

    /**
    * @brief Computes the starting vertex for each primitive in the geometry.
    * @todo Automatically lazy evaluate when requested (make thread safe)
    */
    void computePrimStartVertices() const;
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

    // TODO: create PrimPath class instead of string
    std::string path_ = "/foo/bar";

    using attribVector = std::vector<std::shared_ptr<ga::Attribute>>;
    geo::Primitive::attribVector& getAttributeStore(const ga::AttributeOwner& owner);
    const geo::Primitive::attribVector& getAttributeStore(const ga::AttributeOwner& owner) const;

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
