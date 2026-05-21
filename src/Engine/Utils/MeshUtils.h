#pragma once
#include "Engine/Types.h"
#include <span>

namespace enzo::utils {

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
