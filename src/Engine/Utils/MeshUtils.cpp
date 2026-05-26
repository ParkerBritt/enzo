#include "Engine/Utils/MeshUtils.h"
#include "Engine/Operator/Mesh.h"

namespace enzo::utils {

TriangulatedMesh triangulateMesh(const geo::Mesh& src)
{
    TriangulatedMesh result;
    result.mesh = std::make_shared<geo::Mesh>(src.getPath());

    // Copy every point so output offsets match the source one to one.
    const ga::Offset numPoints = src.getNumPoints();
    std::vector<bt::Vector3> positions;
    positions.reserve(numPoints);
    for (ga::Offset pointOffset = 0; pointOffset < numPoints; ++pointOffset)
    {
        positions.push_back(src.getPointPos(pointOffset));
    }
    result.mesh->addPoints(positions);

    // Fan triangulate each face from its first corner.
    std::vector<ga::Offset> triPointsFlat;
    std::vector<ga::Offset> triVertexCounts;
    const ga::Offset numFaces = src.getNumFaces();
    for (ga::Offset faceOffset = 0; faceOffset < numFaces; ++faceOffset)
    {
        const std::span<const bt::intT> facePoints = src.getFacePoints(faceOffset);
        const size_t cornerCount = facePoints.size();
        if (cornerCount < 3) continue;
        for (size_t fanIndex = 1; fanIndex + 1 < cornerCount; ++fanIndex)
        {
            triPointsFlat.push_back(facePoints[0]);
            triPointsFlat.push_back(facePoints[fanIndex]);
            triPointsFlat.push_back(facePoints[fanIndex + 1]);
            triVertexCounts.push_back(3);
            result.faceToOriginal.push_back(faceOffset);
        }
    }
    result.mesh->addFaces(triPointsFlat, triVertexCounts);

    return result;
}


bt::Vector3 polygonNormal(std::span<const bt::Vector3> positions,
                          std::span<const bt::intT> polygonPoints)
{
    bt::Vector3 normal(0, 0, 0);
    const size_t count = polygonPoints.size();
    for(size_t cornerIndex=0; cornerIndex<count; ++cornerIndex)
    {
        const bt::Vector3& cur = positions[polygonPoints[cornerIndex]];
        const bt::Vector3& nxt = positions[polygonPoints[(cornerIndex+1)%count]];
        normal.x() += (cur.y() - nxt.y()) * (cur.z() + nxt.z());
        normal.y() += (cur.z() - nxt.z()) * (cur.x() + nxt.x());
        normal.z() += (cur.x() - nxt.x()) * (cur.y() + nxt.y());
    }
    normal.normalize();
    return normal;
}

} // namespace enzo::utils
