#pragma once
#include "Engine/Core/SharedCache.h"
#include "Engine/Primitives/Primitive.h"
#include <span>
#include <unordered_set>

namespace enzo::geo {
class Mesh;
class FaceNormalHandle;
class PointNormalHandle;
class VertexNormalHandle;

/// @brief The cusp angle used when nothing asks for a particular one, in degrees.
inline constexpr double kDefaultCuspAngle = 60;

/**
 * @class enzo::geo::Mesh
 * @brief Polygonal mesh primitive with point, vertex, and face attributes.
 *
 * Extends Primitive with mesh-specific topology: points, vertices, faces,
 * and the intrinsic attributes that describe connectivity (vertexCount,
 * closed, point offset, and Position).
 */
class Mesh : public Primitive
{
  public:
    Mesh(std::string_view path = "/mesh");
    Mesh(const Mesh& other) = default;
    Mesh& operator=(const Mesh& rhs) = default;
    ~Mesh() override = default;

    PrimType getType() const override { return PrimType::MESH; }
    std::shared_ptr<Primitive> clone() const override { return std::make_shared<Mesh>(*this); }
    TransformClass transformType() const override
    {
        return TransformClass::POINT | TransformClass::PRIMITIVE;
    }
    void applyTransform(
        const Matrix4& mat,
        TransformClass transformClass = TransformClass::POINT
    ) override;
    void applyTransform(
        const Transform& transform,
        TransformClass transformClass = TransformClass::POINT
    )
    {
        applyTransform(transform.getMatrix(), transformClass);
    }
    bool canMerge() const override { return true; }
    void merge(std::shared_ptr<Primitive> other) override;
    bool hasOwner(attr::AttributeOwner owner) const override { return true; }

    /**
     * @brief Adds a single face. Avoid for multiple faces in a loop as a single call to @ref
     * addFaces is much more performant.
     * @return Offset of the new face.
     */
    // TODO: use more efficient addFaces() function
    Offset addFace(const std::vector<Offset>& pointOffsets, bool closed = true);
    /**
     * @brief Adds many faces in a single call.
     *
     * @param pointOffsetsFlat All point offsets for every face listed one after another.
     * @param vertexCounts How many points each face has. Read in the same order as
     * pointOffsetsFlat.
     * @return Offsets of the newly created faces in the order they were added.
     */
    std::vector<Offset> addFaces(
        std::span<const Offset> pointOffsetsFlat,
        std::span<const Offset> vertexCounts,
        bool closed = true
    );
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
     * @param copyAttributes When true, copy every point attribute from each source point. When
     * false, only positions are copied.
     * @return Offsets of the newly created points in the same order as srcPointOffsets.
     */
    std::vector<Offset>
    duplicatePoints(std::span<const Offset> srcPointOffsets, bool copyAttributes = true);

    void deleteFaces(const std::vector<Offset>& faceOffsets, bool andPoints = true);
    /// @brief Removes every face from the mesh.
    void deleteAllFaces(bool andPoints = true);
    void deletePoints(const std::vector<Offset>& pointOffsets) override
    {
        deletePoints(pointOffsets, false);
    }
    void deletePoints(const std::vector<Offset>& pointOffsets, bool andFaces);
    void deleteVertices(const std::vector<Offset>& vertOffsets);
    /// @brief Reverses the vertex order of each face, carrying every vertex attribute along.
    void reverseFaces(const std::vector<Offset>& faceOffsets);
    bool isValidFace(Offset offset) const;
    bool isValidVertex(Offset offset) const;
    bool isValidPoint(Offset offset) const override;

    void defragment() override;

    void merge(Mesh& other);

    /// @brief Returns a new mesh holding one copy of this mesh for each transform.
    std::shared_ptr<Mesh> copyToTransforms(std::span<const Transform> transforms) const;
    /// @brief Adds a copy of this mesh for each transform, keeping the original.
    void duplicateToTransforms(std::span<const Transform> transforms);

    /// @brief Returns the valid points that no valid vertex uses.
    const std::unordered_set<Offset>& getSoloPoints() const;

    void setPointPos(const Offset offset, const Vector3& pos);
    /**
     * @brief Contiguous view of each face's starting vertex offset.
     * @return Span indexed by face offset. Invalid after any write to the mesh.
     */
    std::span<const Offset> getFaceStartVertices() const;
    Vector3 getPosFromVert(Offset vertexOffset) const;
    Vector3 getPointPosition(Offset pointOffset) const;
    unsigned int getFaceVertexCount(Offset faceOffset) const
    {
        return getFaceVertexCounts()[faceOffset];
    }
    unsigned int getFacePointCount(Offset faceOffset) const;
    Offset getVertexFace(Offset vertexOffset) const;

    Offset getVertexPoint(Offset vertexOffset) const
    {
        return getVertexPoints()[vertexOffset];
    }

    std::span<const intT> getFacePoints(Offset faceOffset) const
    {
        const Offset start = getFaceStartVertices()[faceOffset];
        const unsigned int count = getFaceVertexCount(faceOffset);
        return getVertexPoints().subspan(start, count);
    }

    /**
     * @brief Returns the point of every vertex, indexed by vertex offset.
     *
     * @note The span is invalid after any write to the mesh.
     */
    std::span<const intT> getVertexPoints() const
    {
        return getValues<intT>(*vertexAttributes_[vertexPointIndex_]);
    }

    /**
     * @brief Returns the vertex count of every face, indexed by face offset.
     *
     * @note The span is invalid after any write to the mesh.
     */
    std::span<const intT> getFaceVertexCounts() const
    {
        return getValues<intT>(*faceAttributes_[faceVertexCountIndex_]);
    }

    /**
     * @brief Returns the position of every point, indexed by point offset.
     *
     * @note The span is invalid after any write to the mesh.
     */
    std::span<const Vector3> getPointPositions() const
    {
        return getValues<Vector3>(*pointAttributes_[pointPositionIndex_]);
    }

    Offset getNumFaces() const { return getElementCount(attr::AttributeOwner::FACE); }
    Offset getNumVerts() const { return getElementCount(attr::AttributeOwner::VERTEX); }
    Offset getNumSoloPoints() const;

    // Face Iterator
    struct FaceOffsets
    {
        FaceOffsets(const Mesh& mesh) : mesh_(mesh) {}
        struct Iterator
        {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = Offset;

            explicit Iterator(Offset current) : curOffset_(current) {}
            value_type operator*() const { return curOffset_; }
            Iterator& operator++()
            {
                ++curOffset_;
                return *this;
            }
            Iterator operator++(int)
            {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }
            friend bool operator==(const Iterator& a, const Iterator& b)
            {
                return a.curOffset_ == b.curOffset_;
            }
            friend bool operator!=(const Iterator& a, const Iterator& b)
            {
                return a.curOffset_ != b.curOffset_;
            }

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
            for (const Offset faceOffset : *this)
                offsets.push_back(faceOffset);
            return offsets;
        }

      private:
        const Mesh& mesh_;
    };
    // TODO walk only valid faces like Primitive::PointOffsets once deletion settles. For now it is
    // dense.
    FaceOffsets getFaces() const { return FaceOffsets(*this); }

    /// @brief Returns the vertex group of this name, adding one when the name is free.
    attr::AttributeHandleBool addVertexGroup(std::string name)
    {
        return addGroup(attr::AttributeOwner::VERTEX, std::move(name));
    }
    /// @brief Returns the face group of this name, adding one when the name is free.
    attr::AttributeHandleBool addFaceGroup(std::string name)
    {
        return addGroup(attr::AttributeOwner::FACE, std::move(name));
    }
    /// @brief Marks the given offsets as members of the vertex group.
    void addToVertexGroup(const std::string& name, const std::vector<Offset>& offsets)
    {
        addToGroup(attr::AttributeOwner::VERTEX, name, offsets);
    }
    /// @brief Marks the given offsets as members of the face group.
    void addToFaceGroup(const std::string& name, const std::vector<Offset>& offsets)
    {
        addToGroup(attr::AttributeOwner::FACE, name, offsets);
    }

    boolT isClosed(Offset faceOffset) const;

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
     * A vertex attribute named Normal is read first, then a point attribute
     * of the same name, and failing both the normals are computed at the
     * given cusp angle.
     *
     * @param cuspAngle The widest angle between two faces that still counts
     *                  as smooth, in degrees. Only reached when neither
     *                  attribute is present.
     * @return Handle whose operator[] returns the normal for a vertex offset.
     */
    VertexNormalHandle getVertexNormal(double cuspAngle = kDefaultCuspAngle) const;

    /**
     * @brief Returns a handle for reading per-point normals.
     *
     * A point attribute named Normal is read when present, otherwise the
     * normals are computed.
     *
     * @return Handle whose operator[] returns the normal for a point offset.
     */
    PointNormalHandle getPointNormal() const;

    friend class FaceNormalHandle;
    friend class VertexNormalHandle;

  protected:
    attr::attribVector& getAttributeStore(const attr::AttributeOwner& owner) override;
    const attr::attribVector& getAttributeStore(const attr::AttributeOwner& owner) const override;
    attr::attribVector& getGroupStore(const attr::AttributeOwner& owner) override;
    const attr::attribVector& getGroupStore(const attr::AttributeOwner& owner) const override;

  private:
    /// @brief Adds a copy of source onto the end of this mesh for each transform.
    void copySourceToTransforms(const Mesh& source, std::span<const Transform> transforms);

    attr::attribVector vertexAttributes_;
    attr::attribVector faceAttributes_;
    attr::attribVector vertexGroups_;
    attr::attribVector faceGroups_;

    std::unordered_set<Offset> buildSoloPoints() const;
    std::vector<Offset> buildFaceStarts() const;
    bool needsDefrag_ = false;

    SharedCache<std::vector<Offset>> faceStarts_;
    SharedCache<std::unordered_set<Offset>> soloPoints_;

    // Indexes of the intrinsic attributes, in the order the constructor adds them to each store
    static constexpr size_t faceVertexCountIndex_ = 0;
    static constexpr size_t faceClosedIndex_ = 1;
    static constexpr size_t faceValidIndex_ = 2;
    static constexpr size_t vertexPointIndex_ = 0;
    static constexpr size_t vertexValidIndex_ = 1;
    static constexpr size_t vertexFaceIndex_ = 2;
    static constexpr size_t pointPositionIndex_ = 0;
    static constexpr size_t pointValidIndex_ = 1;

    const attr::StoreContainer<boolT>& getFaceClosedFlags() const
    {
        return getValues<boolT>(*faceAttributes_[faceClosedIndex_]);
    }
    const attr::StoreContainer<boolT>& getFaceValidFlags() const
    {
        return getValues<boolT>(*faceAttributes_[faceValidIndex_]);
    }
    const attr::StoreContainer<boolT>& getVertexValidFlags() const
    {
        return getValues<boolT>(*vertexAttributes_[vertexValidIndex_]);
    }
    const attr::StoreContainer<intT>& getVertexFaces() const
    {
        return getValues<intT>(*vertexAttributes_[vertexFaceIndex_]);
    }
    const attr::StoreContainer<boolT>& getPointValidFlags() const
    {
        return getValues<boolT>(*pointAttributes_[pointValidIndex_]);
    }

    attr::StoreContainer<intT>& getWritableFaceVertexCounts()
    {
        return getWritableValues<intT>(faceAttributes_, faceVertexCountIndex_);
    }
    attr::StoreContainer<boolT>& getWritableFaceClosedFlags()
    {
        return getWritableValues<boolT>(faceAttributes_, faceClosedIndex_);
    }
    attr::StoreContainer<boolT>& getWritableFaceValidFlags()
    {
        return getWritableValues<boolT>(faceAttributes_, faceValidIndex_);
    }
    attr::StoreContainer<intT>& getWritableVertexPoints()
    {
        return getWritableValues<intT>(vertexAttributes_, vertexPointIndex_);
    }
    attr::StoreContainer<boolT>& getWritableVertexValidFlags()
    {
        return getWritableValues<boolT>(vertexAttributes_, vertexValidIndex_);
    }
    attr::StoreContainer<intT>& getWritableVertexFaces()
    {
        return getWritableValues<intT>(vertexAttributes_, vertexFaceIndex_);
    }
    attr::StoreContainer<Vector3>& getWritablePointPositions()
    {
        return getWritableValues<Vector3>(pointAttributes_, pointPositionIndex_);
    }
    attr::StoreContainer<boolT>& getWritablePointValidFlags()
    {
        return getWritableValues<boolT>(pointAttributes_, pointValidIndex_);
    }
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
    Vector3 operator[](Offset faceOffset) const
    {
        if (cached_) return (*cached_)[faceOffset];
        if (!precomputed_.empty()) return precomputed_[faceOffset];
        return computeNormal(faceOffset);
    }

  private:
    friend class Mesh;
    FaceNormalHandle(const Mesh& mesh, bool precompute);
    Vector3 computeNormal(Offset faceOffset) const;
    const Mesh& mesh_;
    std::optional<attr::AttributeHandleRO<Vector3>> cached_;
    std::vector<Vector3> precomputed_;
};

/**
 * @class enzo::geo::PointNormalHandle
 * @brief Read accessor for per-point normals.
 *
 * Built by @ref Mesh::getPointNormal. The constructor resolves the Normal
 * point attribute, and computes every point normal up front when the mesh
 * has none.
 */
class PointNormalHandle
{
  public:
    Vector3 operator[](Offset pointOffset) const
    {
        if (attribute_) return (*attribute_)[pointOffset];
        return computed_[pointOffset];
    }

  private:
    friend class Mesh;
    explicit PointNormalHandle(const Mesh& mesh);
    std::optional<attr::AttributeHandleRO<Vector3>> attribute_;
    std::vector<Vector3> computed_;
};

/**
 * @class enzo::geo::VertexNormalHandle
 * @brief Read accessor for per-vertex normals.
 *
 * Built by @ref Mesh::getVertexNormal. The constructor resolves the Normal
 * vertex attribute, then the Normal point attribute, and computes every
 * vertex normal up front when the mesh carries neither.
 */
class VertexNormalHandle
{
  public:
    Vector3 operator[](Offset vertexOffset) const;

  private:
    friend class Mesh;
    VertexNormalHandle(const Mesh& mesh, double cuspAngle);
    const Mesh& mesh_;
    std::optional<attr::AttributeHandleRO<Vector3>> vertexAttribute_;
    std::optional<attr::AttributeHandleRO<Vector3>> pointAttribute_;
    std::vector<Vector3> computed_;
};
} // namespace enzo::geo
