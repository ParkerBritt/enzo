#include "Engine/GeometryAlgorithms/Normals.h"
#include "Engine/Primitives/Mesh.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace enzo::utils {

namespace {

// One face meeting at a point, with the angle the face turns through there.
struct Corner
{
    Offset face = 0;
    double angle = 0;
};

// Every corner of the mesh grouped by the point it sits on, stored as one
// array of corners plus the place each point's run begins.
struct CornersByPoint
{
    std::vector<Corner> corners;
    std::vector<Offset> pointStarts;

    std::span<const Corner> operator[](Offset pointOffset) const
    {
        const Offset start = pointStarts[pointOffset];
        return std::span(corners).subspan(start, pointStarts[pointOffset + 1] - start);
    }
};

// Returns the faces that enclose an area, the only ones with a normal.
std::vector<Offset> getShadeableFaces(const geo::Mesh& mesh)
{
    std::vector<Offset> shadeable;
    for (const Offset faceOffset : mesh.getFaces())
        if (mesh.isClosed(faceOffset) && mesh.getFaceVertCount(faceOffset) >= 3)
            shadeable.push_back(faceOffset);
    return shadeable;
}

// Returns the angle the face turns through at one of its corners, in radians.
double getCornerAngle(
    std::span<const Vector3> positions,
    std::span<const intT> facePoints,
    size_t cornerIndex
)
{
    const size_t cornerCount = facePoints.size();
    const Vector3& cornerPos = positions[facePoints[cornerIndex]];
    const Vector3& prevPos = positions[facePoints[(cornerIndex + cornerCount - 1) % cornerCount]];
    const Vector3& nextPos = positions[facePoints[(cornerIndex + 1) % cornerCount]];

    const Vector3 toPrev = prevPos - cornerPos;
    const Vector3 toNext = nextPos - cornerPos;
    const double lengths = toPrev.norm() * toNext.norm();
    if (lengths == 0) return 0;

    return std::acos(std::clamp(toPrev.dot(toNext) / lengths, -1.0, 1.0));
}

// Groups the corners of the given faces under the point each one sits on.
CornersByPoint gatherCornersByPoint(const geo::Mesh& mesh, std::span<const Offset> faces)
{
    const std::span<const Vector3> positions = mesh.pointPosSpan();
    const Offset numPoints = mesh.getNumPoints();

    // Count the corners on every point, keeping a point's count one place to
    // the right of the point.
    CornersByPoint grouped;
    grouped.pointStarts.assign(numPoints + 1, 0);
    for (const Offset faceOffset : faces)
        for (const intT pointOffset : mesh.getFacePoints(faceOffset))
            ++grouped.pointStarts[pointOffset + 1];

    // Adding each count to the one before it turns the counts into the place
    // where each point's run of corners begins.
    for (Offset pointOffset = 0; pointOffset < numPoints; ++pointOffset)
        grouped.pointStarts[pointOffset + 1] += grouped.pointStarts[pointOffset];

    // Walk the faces again, filling each point's run from its start.
    std::vector<Offset> nextFreeSlot = grouped.pointStarts;
    grouped.corners.resize(grouped.pointStarts.back());
    for (const Offset faceOffset : faces)
    {
        const std::span<const intT> facePoints = mesh.getFacePoints(faceOffset);
        for (size_t cornerIndex = 0; cornerIndex < facePoints.size(); ++cornerIndex)
        {
            const intT pointOffset = facePoints[cornerIndex];
            grouped.corners[nextFreeSlot[pointOffset]++] = {
                faceOffset,
                getCornerAngle(positions, facePoints, cornerIndex)
            };
        }
    }
    return grouped;
}

Vector3 normalizedOrZero(const Vector3& vector)
{
    const double length = vector.norm();
    return length > 0 ? Vector3(vector / length) : Vector3(0, 0, 0);
}

} // namespace

std::vector<Vector3> computePointNormals(const geo::Mesh& mesh)
{
    const geo::FaceNormalHandle faceNormals = mesh.getFaceNormal(true);
    const std::vector<Offset> shadeableFaces = getShadeableFaces(mesh);
    const CornersByPoint cornersByPoint = gatherCornersByPoint(mesh, shadeableFaces);
    const Offset numPoints = mesh.getNumPoints();

    std::vector<Vector3> pointNormals(numPoints, Vector3(0, 0, 0));
    for (Offset pointOffset = 0; pointOffset < numPoints; ++pointOffset)
    {
        Vector3 summed(0, 0, 0);
        for (const Corner& corner : cornersByPoint[pointOffset])
            summed += faceNormals[corner.face] * corner.angle;
        pointNormals[pointOffset] = normalizedOrZero(summed);
    }
    return pointNormals;
}

std::vector<Vector3> computeVertexNormals(const geo::Mesh& mesh, double cuspAngle)
{
    const geo::FaceNormalHandle faceNormals = mesh.getFaceNormal(true);
    const std::vector<Offset> shadeableFaces = getShadeableFaces(mesh);
    const CornersByPoint cornersByPoint = gatherCornersByPoint(mesh, shadeableFaces);
    const std::span<const intT> vertexPoints = mesh.vertexPointSpan();
    const std::span<const Offset> faceStarts = mesh.getFaceStartVertices();

    // Two faces count as smooth while the angle between them stays under the
    // cusp angle, so their normals meet at a dot product above this one.
    const double minSmoothDot = std::cos(cuspAngle * std::numbers::pi / 180);

    std::vector<Vector3> vertexNormals(mesh.getNumVerts(), Vector3(0, 0, 0));
    for (const Offset faceOffset : shadeableFaces)
    {
        const Vector3 ownNormal = faceNormals[faceOffset];
        const Offset faceStartVertex = faceStarts[faceOffset];
        const unsigned int cornerCount = mesh.getFaceVertCount(faceOffset);

        for (unsigned int cornerIndex = 0; cornerIndex < cornerCount; ++cornerIndex)
        {
            const Offset vertexOffset = faceStartVertex + cornerIndex;
            Vector3 summed(0, 0, 0);
            for (const Corner& corner : cornersByPoint[vertexPoints[vertexOffset]])
            {
                const Vector3 neighbourNormal = faceNormals[corner.face];
                if (ownNormal.dot(neighbourNormal) < minSmoothDot) continue;
                summed += neighbourNormal * corner.angle;
            }
            // Falls back to the face's own normal when the neighbours cancel out.
            vertexNormals[vertexOffset] = summed.isZero() ? ownNormal : normalizedOrZero(summed);
        }
    }
    return vertexNormals;
}

} // namespace enzo::utils
