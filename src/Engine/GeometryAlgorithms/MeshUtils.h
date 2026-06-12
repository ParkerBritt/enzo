#pragma once
#include "Engine/Core/Types.h"
#include <array>
#include <memory>
#include <span>
#include <vector>

namespace enzo::geo {
class Mesh;
}

namespace enzo::utils {

/**
 * @brief A triangulated mesh paired with a mapping from each output triangle to its source face.
 */
struct TriangulatedMesh
{
    std::shared_ptr<geo::Mesh> mesh;
    std::vector<Offset> faceToOriginal;
};

/**
 * @brief Triangulates every face of @p src using fan triangulation from the first corner.
 * @return The triangulated mesh and a mapping from each output triangle back to its source face.
 */
TriangulatedMesh triangulateMesh(const geo::Mesh& src);

/**
 * @brief Computes the normal of an arbitrary polygon.
 *
 * Uses Newell's method, which is stable for non-planar polygons and works
 * uniformly across triangles, quads, and n-gons. Follows the right hand
 * rule on CCW winding.
 *
 * @param positions Contiguous point positions indexed by the polygon's points.
 * @param polygonPoints Indices into @p positions for the points of the polygon,
 *                     in winding order.
 * @return The normalized polygon normal.
 */
Vector3 polygonNormal(std::span<const Vector3> positions, std::span<const intT> polygonPoints);

/**
 * @brief Ear clip triangulates every face of @p mesh into triangle indices.
 *
 * Faces are clipped one at a time so concave faces tessellate correctly. Faces
 * with fewer than three corners contribute nothing.
 *
 * @param mesh Mesh whose faces are triangulated.
 * @return Triangles as triples of the mesh's vertex offsets, grouped by face.
 */
std::vector<std::array<Offset, 3>> earClipTriangleIndices(const geo::Mesh& mesh);

/**
 * @brief Ear clip triangulates the given faces of @p mesh into triangle indices.
 *
 * Use this to triangulate a subset of faces. Faces with fewer than three
 * corners contribute nothing.
 *
 * @param mesh Mesh the faces belong to.
 * @param faceOffsets Offsets of the faces to triangulate.
 * @return Triangles as triples of the mesh's vertex offsets, grouped by face.
 */
std::vector<std::array<Offset, 3>>
earClipTriangleIndices(const geo::Mesh& mesh, std::span<const Offset> faceOffsets);

/// @brief How a point's tangent along its face is computed.
enum class TangentMode
{
    // Direction from each point to the next.
    FirstPoint,
    // Average of the incoming and outgoing segment directions.
    TwoPoint
};

/**
 * @brief Tangents along a face in winding order.
 *
 * Construct once and call per face.
 *
 * e.g.
 *   geo::FaceTangents faceTangents(mesh, TangentMode::TwoPoint);
 *   for (Offset face : mesh.getFaces())
 *       for (Vector3 tangent : faceTangents(face)) ...
 *
 * @note Each call replaces the previous result, so finish reading a face's
 * tangents before asking for the next.
 */
class FaceTangents
{
  public:
    FaceTangents(const geo::Mesh& mesh, TangentMode mode) : mesh_(mesh), mode_(mode) {}

    /// @brief Returns the tangents for the given face.
    std::span<const Vector3> operator()(Offset faceOffset);

  private:
    const geo::Mesh& mesh_;
    TangentMode mode_;
    std::vector<Vector3> tangents_;
};

} // namespace enzo::utils
