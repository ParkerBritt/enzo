#pragma once
#include <cstdint>
#include <glm/vec3.hpp>
#include <memory>
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

/// @brief The geometry a viewport draws, flattened from every mesh in a packet.
///
/// @note Display vertex n is the mesh vertex at offset n, counting on from the
/// vertices of the meshes before it.
struct DisplayGeometry
{
    DisplayTopology topology;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
};

/// @brief Returns the display geometry for every mesh in the packet.
std::shared_ptr<const DisplayGeometry> buildDisplayGeometry(const NodePacket& packet);

} // namespace enzo::gfx
