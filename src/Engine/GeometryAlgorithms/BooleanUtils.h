#pragma once
#include <memory>
#include <string>

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
 * @param error Optional sink for a failure message. When non-null and left non-empty
 *              on return, the boolean hit a geometry error the caller should report.
 * @return A new triangulated mesh holding the result of @p op applied to @p meshA and @p meshB.
 */
std::shared_ptr<geo::Mesh> booleanMesh(const geo::Mesh& meshA,
                                       const geo::Mesh& meshB,
                                       BooleanOp op,
                                       std::string* error = nullptr);

} // namespace enzo::utils
