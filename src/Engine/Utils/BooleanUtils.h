#pragma once
#include <memory>

namespace enzo::geo { class Mesh; }

namespace enzo::utils {

enum class BooleanOp
{
    UNION,
    INTERSECT,
    SUBTRACT,
};

/**
 * @brief Computes a boolean operation between two meshes.
 * @return A new triangulated mesh holding the result of @p op applied to @p meshA and @p meshB.
 */
std::shared_ptr<geo::Mesh> booleanMesh(const geo::Mesh& meshA,
                                       const geo::Mesh& meshB,
                                       BooleanOp op);

} // namespace enzo::utils
