#pragma once
#include <istream>

namespace enzo::geo {
class Mesh;
}

namespace enzo::utils {

/**
 * @brief Reads the vertex and face records of an obj file into a mesh.
 *
 * A "f" record closes its face and an "l" record leaves it open.
 *
 * @note Records the mesh has nowhere to put, such as materials, normals, and
 * texture coordinates, are skipped.
 */
void readObjInto(std::istream& file, geo::Mesh& mesh);

} // namespace enzo::utils
