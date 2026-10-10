#pragma once
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <memory>
#include <string>
#include <vector>

namespace enzo {
class NodePacket;
}

namespace enzo::gfx {

/// @brief The index lists that say how display vertices join up.
struct DisplayTopology
{
    /// @brief Three vertex indices per triangle of the closed faces.
    std::vector<std::uint32_t> triangleIndices;
    /// @brief Two vertex indices per edge around the closed faces.
    std::vector<std::uint32_t> edgeIndices;
    /// @brief Two vertex indices per segment of the open faces.
    std::vector<std::uint32_t> lineIndices;
};

/// @brief The geometry a viewport draws, flattened from every primitive in a packet.
///
/// @note Display vertex n is the mesh vertex at offset n, counting on from the
/// vertices of the meshes before it.
struct DisplayGeometry
{
    DisplayTopology topology;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    /// @brief The positions of every mesh point.
    std::vector<glm::vec3> pointPositions;
    /// @brief The positions of the mesh points that belong to no face.
    std::vector<glm::vec3> soloPointPositions;
    /// @brief The world transform of each camera.
    std::vector<glm::mat4> cameraTransforms;
    /// @brief The path of each camera, in the same order as the transforms.
    std::vector<std::string> cameraPaths;
};

/**
 * @brief Fills the geometry with every mesh and camera in the packet, replacing what it held.
 *
 * @note Keeps the memory of the geometry's buffers, so refilling a geometry of a
 * similar size allocates nothing.
 */
void buildDisplayGeometry(DisplayGeometry& geometry, const NodePacket& packet);

} // namespace enzo::gfx
