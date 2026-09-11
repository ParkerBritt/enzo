#pragma once
#include "Engine/Core/Types.h"
#include <vector>

namespace enzo::geo {
class Mesh;
}

namespace enzo::utils {

/**
 * @brief Returns one smooth normal per point, indexed by point offset.
 *
 * Every face meeting at a point contributes, weighted by the angle it turns
 * through at that point, so a face split into many small faces does not pull
 * the average towards itself. A point on no face gets a zero normal.
 */
std::vector<Vector3> computePointNormals(const geo::Mesh& mesh);

/**
 * @brief Returns one normal per vertex, indexed by vertex offset.
 *
 * A vertex averages the faces meeting at its point that lie within
 * @p cuspAngle of its own face, so a crease sharper than the angle stays
 * sharp while a gentler join goes smooth. Weighting matches
 * @ref computePointNormals.
 *
 * @param cuspAngle The widest angle between two faces that still counts as
 *                  smooth, in degrees.
 */
std::vector<Vector3> computeVertexNormals(const geo::Mesh& mesh, double cuspAngle);

} // namespace enzo::utils
