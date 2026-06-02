#include "Engine/GeometryAlgorithms/MeshShapes.h"
#include "Engine/Primitives/Mesh.h"

namespace enzo::utils {

std::shared_ptr<geo::Mesh> buildCube(const Vector3& size, const Vector3& center)
{
    auto mesh = std::make_shared<geo::Mesh>();

    // Half extents along each axis.
    const floatT halfX = size.x() * 0.5;
    const floatT halfY = size.y() * 0.5;
    const floatT halfZ = size.z() * 0.5;

    // Eight corner points, ordered so bottom face is 0-3 and top face is 4-7.
    const std::vector<Vector3> positions = {
        {center.x() - halfX, center.y() - halfY, center.z() - halfZ},
        {center.x() + halfX, center.y() - halfY, center.z() - halfZ},
        {center.x() + halfX, center.y() + halfY, center.z() - halfZ},
        {center.x() - halfX, center.y() + halfY, center.z() - halfZ},
        {center.x() - halfX, center.y() - halfY, center.z() + halfZ},
        {center.x() + halfX, center.y() - halfY, center.z() + halfZ},
        {center.x() + halfX, center.y() + halfY, center.z() + halfZ},
        {center.x() - halfX, center.y() + halfY, center.z() + halfZ},
    };
    mesh->addPoints(positions);

    // Six quad faces, each wound CCW when viewed from outside the cube.
    const std::vector<Offset> flatPointOffsets = {
        3, 2, 1, 0,   // -Z bottom
        4, 5, 6, 7,   // +Z top
        0, 1, 5, 4,   // -Y front
        2, 3, 7, 6,   // +Y back
        1, 2, 6, 5,   // +X right
        3, 0, 4, 7,   // -X left
    };
    const std::vector<Offset> vertexCounts(6, 4);
    mesh->addFaces(flatPointOffsets, vertexCounts);

    return mesh;
}

} // namespace utils
