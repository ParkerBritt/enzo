#pragma once
#include "Engine/Types.h"
#include <memory>

namespace enzo::geo { class Mesh; }

namespace enzo::utils {

/**
 * @brief Builds an axis aligned cube as a Mesh with 8 points and 6 quad faces.
 *
 * Face windings follow the right hand rule on CCW order viewed from outside.
 * @param size Extent of the cube along each axis.
 * @param center World position of the cube's center.
 * @return Shared pointer to the new cube mesh.
 */
std::shared_ptr<geo::Mesh> buildCube(const Vector3& size, const Vector3& center);

} // namespace enzo::utils
