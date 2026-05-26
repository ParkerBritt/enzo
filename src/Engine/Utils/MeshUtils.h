#pragma once
#include "Engine/Types.h"
#include <memory>
#include <span>
#include <vector>

namespace enzo::geo { class Mesh; }

namespace enzo::utils {

/**
 * @brief A triangulated mesh paired with a mapping from each output triangle to its source face.
 */
struct TriangulatedMesh
{
    std::shared_ptr<geo::Mesh> mesh;
    std::vector<ga::Offset> faceToOriginal;
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
bt::Vector3 polygonNormal(std::span<const bt::Vector3> positions,
                          std::span<const bt::intT> polygonPoints);

} // namespace enzo::utils
