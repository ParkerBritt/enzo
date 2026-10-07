#include "Graphics/DisplayGeometry.h"
#include "Engine/GeometryAlgorithms/MeshUtils.h"
#include "Engine/Network/NodePacket.h"
#include "Engine/Primitives/Camera.h"
#include "Engine/Primitives/Mesh.h"
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

namespace enzo::gfx {

namespace {

glm::vec3 toGlm(const Vector3& vector)
{
    return glm::vec3(float(vector.x()), float(vector.y()), float(vector.z()));
}

glm::mat4 toGlm(const Matrix4& matrix)
{
    glm::mat4 converted;
    for (int column = 0; column < 4; ++column)
    {
        for (int row = 0; row < 4; ++row)
            converted[column][row] = float(matrix(row, column));
    }
    return converted;
}

/// @brief Appends the index lists of one mesh, numbered from its first display vertex.
void appendTopology(DisplayTopology& topology, const geo::Mesh& mesh, std::uint32_t firstVertex)
{
    std::vector<Offset> closedFaceOffsets;
    const std::span<const Offset> faceStarts = mesh.getFaceStartVertices();
    for (Offset faceOffset = 0; faceOffset < mesh.getNumFaces(); ++faceOffset)
    {
        if (!mesh.isValidFace(faceOffset)) continue;

        const std::uint32_t faceVertexCount = mesh.getFaceVertCount(faceOffset);
        const std::uint32_t startVertex = firstVertex + std::uint32_t(faceStarts[faceOffset]);
        const bool closed = mesh.isClosed(faceOffset);

        // Joins the vertices of an open face into line segments.
        if (!closed && faceVertexCount >= 2)
        {
            for (std::uint32_t corner = 0; corner + 1 < faceVertexCount; ++corner)
            {
                topology.lineIndices.push_back(startVertex + corner);
                topology.lineIndices.push_back(startVertex + corner + 1);
            }
        }
        else if (faceVertexCount >= 3)
        {
            closedFaceOffsets.push_back(faceOffset);
            for (std::uint32_t corner = 0; corner < faceVertexCount; ++corner)
            {
                topology.edgeIndices.push_back(startVertex + corner);
                topology.edgeIndices.push_back(startVertex + (corner + 1) % faceVertexCount);
            }
        }
    }

    // Ear clips the closed faces so concave faces fill correctly.
    for (const std::array<Offset, 3>& triangle : utils::earClipTriangleIndices(mesh, closedFaceOffsets))
    {
        for (const Offset vertexOffset : triangle)
            topology.triangleIndices.push_back(firstVertex + std::uint32_t(vertexOffset));
    }
}

/// @brief Writes the position and normal of every vertex of one mesh, from its first display vertex on.
void writeVertices(DisplayGeometry& geometry, const geo::Mesh& mesh, std::uint32_t firstVertex)
{
    const std::span<const intT> vertexPoints = mesh.vertexPointSpan();
    const std::span<const Vector3> pointPositions = mesh.pointPosSpan();
    const geo::VertexNormalHandle vertexNormals = mesh.getVertexNormal();

    tbb::parallel_for(
        tbb::blocked_range<Offset>(0, mesh.getNumVerts()),
        [&](const tbb::blocked_range<Offset>& range) {
            for (Offset vertexOffset = range.begin(); vertexOffset < range.end(); ++vertexOffset)
            {
                const std::uint32_t displayVertex = firstVertex + std::uint32_t(vertexOffset);
                geometry.positions[displayVertex] = toGlm(pointPositions[vertexPoints[vertexOffset]]);
                geometry.normals[displayVertex] = toGlm(vertexNormals[vertexOffset]);
            }
        }
    );
}

/// @brief Appends the positions of the points of one mesh that belong to no face.
void appendSoloPoints(std::vector<glm::vec3>& soloPointPositions, const geo::Mesh& mesh)
{
    for (auto pointIt = mesh.soloPointsBegin(); pointIt != mesh.soloPointsEnd(); ++pointIt)
        soloPointPositions.push_back(toGlm(mesh.getPointPos(*pointIt)));
}

} // namespace

std::shared_ptr<const DisplayGeometry> buildDisplayGeometry(const NodePacket& packet)
{
    const std::vector<geo::PrimPtr> meshPrims = packet.getPrimitives(geo::PrimType::MESH);

    std::size_t vertexCount = 0;
    for (const geo::PrimPtr& prim : meshPrims)
        vertexCount += std::static_pointer_cast<const geo::Mesh>(prim)->getNumVerts();

    auto geometry = std::make_shared<DisplayGeometry>();
    geometry->positions.resize(vertexCount);
    geometry->normals.resize(vertexCount);

    std::uint32_t firstVertex = 0;
    for (const geo::PrimPtr& prim : meshPrims)
    {
        const auto mesh = std::static_pointer_cast<const geo::Mesh>(prim);
        appendTopology(geometry->topology, *mesh, firstVertex);
        writeVertices(*geometry, *mesh, firstVertex);
        appendSoloPoints(geometry->soloPointPositions, *mesh);
        firstVertex += std::uint32_t(mesh->getNumVerts());
    }

    for (const geo::PrimPtr& prim : packet.getPrimitives(geo::PrimType::CAMERA))
    {
        const auto camera = std::static_pointer_cast<const geo::Camera>(prim);
        geometry->cameraTransforms.push_back(toGlm(camera->getTransform()));
    }
    return geometry;
}

} // namespace enzo::gfx
