#pragma once
#include "Engine/Core/Types.h"
#include <memory>
#include <utility>
#include <vector>

namespace enzo::geo {
class Mesh;
}

namespace enzo::utils {

/// @brief A mesh cut by a plane, along with the edges the cut left on the plane.
struct PlaneCut
{
    std::shared_ptr<geo::Mesh> mesh;
    /// @brief The edges around the opening the cut left, as point offset pairs that follow the
    /// winding of their face.
    std::vector<std::pair<Offset, Offset>> cutEdges;
};

/**
 * @brief Returns the mesh with everything on the far side of a plane removed.
 *
 * @param planePoint Any point on the plane.
 * @param keptSideNormal The plane's normal, pointing toward the side that is kept.
 *
 * @note Faces that are not closed are dropped. A concave face whose kept side falls into several
 * pieces comes out as one face for each piece.
 */
PlaneCut
cutMeshByPlane(const geo::Mesh& mesh, const Vector3& planePoint, const Vector3& keptSideNormal);

/**
 * @brief Adds a face across each closed loop of cut edges, facing away from the kept side.
 *
 * @return The offsets of the new faces.
 *
 * @note Cut edges that never close into a loop, such as those left by cutting an open surface,
 * get no face.
 */
std::vector<Offset>
fillCutCaps(geo::Mesh& mesh, const std::vector<std::pair<Offset, Offset>>& cutEdges);

} // namespace enzo::utils
