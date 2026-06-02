#include "Engine/Utils/MeshUtils.h"
#include "Engine/Operator/Mesh.h"

#include <cmath>
#include <numeric>

namespace enzo::utils {

namespace {

// Ear clip one simple polygon given its corner positions in winding order.
// Returns triangles as index triples into corners, keeping the input winding.
std::vector<std::array<int, 3>> earClipPolygon(std::span<const Vector3> corners)
{
    std::vector<std::array<int, 3>> triangles;
    const int cornerCount = static_cast<int>(corners.size());
    if(cornerCount < 3) return triangles;

    auto fanRing = [&](const std::vector<int>& ring)
    {
        for(size_t fanIndex=1; fanIndex+1<ring.size(); ++fanIndex)
            triangles.push_back({ring[0], ring[static_cast<int>(fanIndex)], ring[static_cast<int>(fanIndex)+1]});
    };

    std::vector<intT> identityPoints(cornerCount);
    std::iota(identityPoints.begin(), identityPoints.end(), 0);
    const Vector3 normal = polygonNormal(corners, identityPoints);

    std::vector<int> ring(cornerCount);
    std::iota(ring.begin(), ring.end(), 0);

    // A zero area polygon has no usable plane, so fan it and bail.
    if(!normal.allFinite())
    {
        fanRing(ring);
        return triangles;
    }

    // Two in plane axes to project each corner down to 2D.
    Vector3 axisU = (std::abs(normal.x()) > 0.9) ? Vector3(0, 1, 0) : Vector3(1, 0, 0);
    axisU = (axisU - normal * axisU.dot(normal)).normalized();
    const Vector3 axisV = normal.cross(axisU);

    std::vector<double> projX(cornerCount);
    std::vector<double> projY(cornerCount);
    for(int cornerIndex=0; cornerIndex<cornerCount; ++cornerIndex)
    {
        projX[cornerIndex] = corners[cornerIndex].dot(axisU);
        projY[cornerIndex] = corners[cornerIndex].dot(axisV);
    }

    // Signed area gives the winding so the convex test below points inward.
    double signedArea = 0.0;
    for(int cornerIndex=0; cornerIndex<cornerCount; ++cornerIndex)
    {
        const int nextIndex = (cornerIndex+1)%cornerCount;
        signedArea += projX[cornerIndex]*projY[nextIndex] - projX[nextIndex]*projY[cornerIndex];
    }
    const double windingSign = (signedArea >= 0.0) ? 1.0 : -1.0;

    auto crossSign = [&](int a, int b, int c)
    {
        const double edge0x = projX[b]-projX[a];
        const double edge0y = projY[b]-projY[a];
        const double edge1x = projX[c]-projX[b];
        const double edge1y = projY[c]-projY[b];
        return (edge0x*edge1y - edge0y*edge1x) * windingSign;
    };
    auto pointInTriangle = [&](int point, int a, int b, int c)
    {
        const double side0 = crossSign(a, b, point);
        const double side1 = crossSign(b, c, point);
        const double side2 = crossSign(c, a, point);
        const bool hasNegative = side0 < 0 || side1 < 0 || side2 < 0;
        const bool hasPositive = side0 > 0 || side1 > 0 || side2 > 0;
        return !(hasNegative && hasPositive);
    };

    int guard = 0;
    const int guardLimit = cornerCount * cornerCount + 1;
    while(ring.size() > 3 && guard++ < guardLimit)
    {
        const int ringSize = static_cast<int>(ring.size());
        bool clipped = false;
        for(int slot=0; slot<ringSize; ++slot)
        {
            const int prevCorner = ring[(slot + ringSize - 1) % ringSize];
            const int earCorner  = ring[slot];
            const int nextCorner = ring[(slot + 1) % ringSize];

            // The ear tip has to be a convex corner.
            if(crossSign(prevCorner, earCorner, nextCorner) <= 0) continue;

            // No other corner may sit inside the candidate ear.
            bool containsOther = false;
            for(int other : ring)
            {
                if(other==prevCorner || other==earCorner || other==nextCorner) continue;
                if(pointInTriangle(other, prevCorner, earCorner, nextCorner)) { containsOther = true; break; }
            }
            if(containsOther) continue;

            triangles.push_back({prevCorner, earCorner, nextCorner});
            ring.erase(ring.begin() + slot);
            clipped = true;
            break;
        }
        if(!clipped) break;
    }

    // Whatever remains is the final triangle, or a fan fallback when clipping stalls.
    if(ring.size() == 3) triangles.push_back({ring[0], ring[1], ring[2]});
    else if(ring.size() > 3) fanRing(ring);

    return triangles;
}

} // namespace


TriangulatedMesh triangulateMesh(const geo::Mesh& src)
{
    TriangulatedMesh result;
    result.mesh = std::make_shared<geo::Mesh>(src.getPath());

    // Copy every point so output offsets match the source one to one.
    const Offset numPoints = src.getNumPoints();
    std::vector<Vector3> positions;
    positions.reserve(numPoints);
    for (Offset pointOffset = 0; pointOffset < numPoints; ++pointOffset)
    {
        positions.push_back(src.getPointPos(pointOffset));
    }
    result.mesh->addPoints(positions);

    // Fan triangulate each face from its first corner.
    std::vector<Offset> triPointsFlat;
    std::vector<Offset> triVertexCounts;
    const Offset numFaces = src.getNumFaces();
    for (Offset faceOffset = 0; faceOffset < numFaces; ++faceOffset)
    {
        const std::span<const intT> facePoints = src.getFacePoints(faceOffset);
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


Vector3 polygonNormal(std::span<const Vector3> positions,
                          std::span<const intT> polygonPoints)
{
    Vector3 normal(0, 0, 0);
    const size_t count = polygonPoints.size();
    for(size_t cornerIndex=0; cornerIndex<count; ++cornerIndex)
    {
        const Vector3& cur = positions[polygonPoints[cornerIndex]];
        const Vector3& nxt = positions[polygonPoints[(cornerIndex+1)%count]];
        normal.x() += (cur.y() - nxt.y()) * (cur.z() + nxt.z());
        normal.y() += (cur.z() - nxt.z()) * (cur.x() + nxt.x());
        normal.z() += (cur.x() - nxt.x()) * (cur.y() + nxt.y());
    }
    normal.normalize();
    return normal;
}


std::vector<std::array<Offset, 3>> earClipTriangleIndices(const geo::Mesh& mesh,
                                                              std::span<const Offset> faceOffsets)
{
    std::vector<std::array<Offset, 3>> triangles;
    std::vector<Vector3> corners;
    for(const Offset faceOffset : faceOffsets)
    {
        const unsigned int cornerCount = mesh.getFaceVertCount(faceOffset);
        if(cornerCount < 3) continue;

        // Each corner's position lives at its vertex offset on the face.
        const Offset faceStartVertex = mesh.getFaceStartVertex(faceOffset);
        corners.resize(cornerCount);
        for(unsigned int cornerIndex=0; cornerIndex<cornerCount; ++cornerIndex)
            corners[cornerIndex] = mesh.getPosFromVert(faceStartVertex + cornerIndex);

        // Lift the per face triple back onto the mesh's vertex offsets.
        for(const std::array<int, 3>& tri : earClipPolygon(corners))
            triangles.push_back({faceStartVertex + tri[0], faceStartVertex + tri[1], faceStartVertex + tri[2]});
    }
    return triangles;
}


std::vector<std::array<Offset, 3>> earClipTriangleIndices(const geo::Mesh& mesh)
{
    return earClipTriangleIndices(mesh, mesh.getFaces().toVector());
}

} // namespace enzo::utils
