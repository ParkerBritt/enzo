#include "Engine/GeometryAlgorithms/PlaneCut.h"
#include "Engine/GeometryAlgorithms/AttributeTransfer.h"
#include "Engine/Primitives/Mesh.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <unordered_map>

namespace enzo::utils {

namespace {

// Distances closer to the plane than this count as lying on it.
constexpr float kOnPlaneTolerance = 1e-6f;

constexpr Offset kNoPointIndex = std::numeric_limits<Offset>::max();

// Where an element of the cut mesh comes from, as a blend between two source elements.
// An element carried over whole names the same source element twice with a blend of 0.
struct BlendSource
{
    Offset sourceOffset0;
    Offset sourceOffset1;
    float blend;
};

// One corner of a cut face.
struct CutCorner
{
    BlendSource pointSource;
    BlendSource vertexSource;
    // Whether the edge arriving at this corner replaces corners that were removed.
    bool arrivesAlongCut;
    // Index into the cut mesh's point sources.
    Offset pointIndex = 0;
};

// The corners of a source face left on the kept side.
struct ClippedFace
{
    std::vector<CutCorner> corners;
    bool removedAnyCorner;
    bool liesOnPlane;
};

// One face of the cut mesh.
struct CutFace
{
    Offset sourceFace;
    std::vector<CutCorner> corners;
    bool liesOnPlane;
};

// Returns the position a blend between two source points lands on.
Vector3 getBlendedPosition(std::span<const Vector3> sourcePositions, const BlendSource& pointSource)
{
    const Vector3& position0 = sourcePositions[pointSource.sourceOffset0];
    const Vector3& position1 = sourcePositions[pointSource.sourceOffset1];
    return position0 + (position1 - position0) * pointSource.blend;
}

// Returns whether a cut point lies on the plane.
bool isOnPlane(const BlendSource& pointSource, const std::vector<float>& distances)
{
    const bool isCrossing = pointSource.sourceOffset0 != pointSource.sourceOffset1;
    return isCrossing || distances[pointSource.sourceOffset0] == 0;
}

// Returns the signed distance of every point, positive on the kept side and 0 on the plane.
std::vector<float> getSignedDistances(
    std::span<const Vector3> sourcePositions,
    const Vector3& planePoint,
    const Vector3& keptSideNormal
)
{
    std::vector<float> distances(sourcePositions.size());
    for (Offset pointOffset = 0; pointOffset < sourcePositions.size(); ++pointOffset)
    {
        const float distance = (sourcePositions[pointOffset] - planePoint).dot(keptSideNormal);
        distances[pointOffset] = std::abs(distance) < kOnPlaneTolerance ? 0.0f : distance;
    }
    return distances;
}

// Returns whether any corner of a face arrives along the cut.
bool hasCornerArrivingAlongCut(const std::vector<CutCorner>& faceCorners)
{
    return std::any_of(faceCorners.begin(), faceCorners.end(), [](const CutCorner& corner) {
        return corner.arrivesAlongCut;
    });
}

// Returns a face's corners on the kept side, with a corner added wherever an edge crosses the
// plane.
ClippedFace clipFace(
    const geo::Mesh& mesh,
    Offset faceOffset,
    const geo::FaceNormalHandle& faceNormals,
    const std::vector<float>& distances,
    const Vector3& keptSideNormal
)
{
    const std::span<const Vector3> sourcePositions = mesh.pointPosSpan();
    const std::span<const intT> facePoints = mesh.getFacePoints(faceOffset);
    const Offset faceStartVertex = mesh.getFaceStartVertices()[faceOffset];
    const Offset faceCornerCount = facePoints.size();

    ClippedFace clipped{{}, false, true};
    clipped.corners.reserve(faceCornerCount);
    bool removedSinceLastCorner = false;
    for (Offset cornerIndex = 0; cornerIndex < faceCornerCount; ++cornerIndex)
    {
        const Offset nextCornerIndex = (cornerIndex + 1) % faceCornerCount;
        const Offset point = facePoints[cornerIndex];
        const Offset nextPoint = facePoints[nextCornerIndex];
        const Offset vertex = faceStartVertex + cornerIndex;
        const Offset nextVertex = faceStartVertex + nextCornerIndex;
        const float distance = distances[point];
        const float nextDistance = distances[nextPoint];
        if (distance != 0) clipped.liesOnPlane = false;

        if (distance >= 0)
        {
            clipped.corners.push_back(
                {{point, point, 0}, {vertex, vertex, 0}, removedSinceLastCorner}
            );
            removedSinceLastCorner = false;
        }
        else
        {
            removedSinceLastCorner = true;
            clipped.removedAnyCorner = true;
        }

        // Treat an edge along the plane as cut when the face's inside lies on the removed side.
        const bool edgeLiesOnPlane = distance == 0 && nextDistance == 0;
        if (edgeLiesOnPlane)
        {
            const Vector3 edgeDirection = sourcePositions[nextPoint] - sourcePositions[point];
            const Vector3 faceNormal = faceNormals[faceOffset];
            const Vector3 towardInside = faceNormal.cross(edgeDirection).normalized();
            if (towardInside.dot(keptSideNormal) < -kOnPlaneTolerance)
                removedSinceLastCorner = true;
        }

        const bool edgeCrossesPlane =
            (distance > 0 && nextDistance < 0) || (distance < 0 && nextDistance > 0);
        if (edgeCrossesPlane)
        {
            const float blend = distance / (distance - nextDistance);
            clipped.corners.push_back(
                {{point, nextPoint, blend}, {vertex, nextVertex, blend}, removedSinceLastCorner}
            );
            removedSinceLastCorner = false;
        }
    }

    // Corners removed at the end of the walk sit before the first corner.
    if (removedSinceLastCorner && !clipped.corners.empty())
        clipped.corners.front().arrivesAlongCut = true;

    return clipped;
}

/**
 * @brief Returns the faces a cut face's corners make up, one for each separate piece left on the
 * kept side.
 *
 * @param cutDirection The direction the plane runs across the face.
 *
 * @note A face whose run ends do not alternate between start and finish along the cut comes back
 * whole.
 */
std::vector<std::vector<CutCorner>> splitAlongCut(
    std::vector<CutCorner> faceCorners,
    const Vector3& cutDirection,
    std::span<const Vector3> sourcePositions
)
{
    // Break the corners into runs, each starting at a corner that arrives along the cut.
    const auto firstArrival =
        std::find_if(faceCorners.begin(), faceCorners.end(), [](const CutCorner& corner) {
            return corner.arrivesAlongCut;
        });
    if (firstArrival == faceCorners.end()) return {faceCorners};
    std::rotate(faceCorners.begin(), firstArrival, faceCorners.end());

    std::vector<std::vector<CutCorner>> runs;
    for (const CutCorner& corner : faceCorners)
    {
        if (corner.arrivesAlongCut) runs.emplace_back();
        runs.back().push_back(corner);
    }
    if (runs.size() == 1) return {faceCorners};

    // Sort the first and last corner of every run by how far along the cut they sit.
    struct RunEnd
    {
        float distanceAlongCut;
        Offset runIndex;
        bool isRunStart;
    };
    auto getDistanceAlongCut = [&](const CutCorner& corner) {
        return getBlendedPosition(sourcePositions, corner.pointSource).dot(cutDirection);
    };
    std::vector<RunEnd> runEnds;
    for (Offset runIndex = 0; runIndex < runs.size(); ++runIndex)
    {
        runEnds.push_back({getDistanceAlongCut(runs[runIndex].front()), runIndex, true});
        runEnds.push_back({getDistanceAlongCut(runs[runIndex].back()), runIndex, false});
    }
    std::stable_sort(runEnds.begin(), runEnds.end(), [](const RunEnd& left, const RunEnd& right) {
        return left.distanceAlongCut < right.distanceAlongCut;
    });

    // Join the sorted ends in pairs, since the kept side covers the stretch between each pair.
    std::vector<Offset> nextRun(runs.size());
    for (Offset endIndex = 0; endIndex < runEnds.size(); endIndex += 2)
    {
        const RunEnd& end0 = runEnds[endIndex];
        const RunEnd& end1 = runEnds[endIndex + 1];
        if (end0.isRunStart == end1.isRunStart) return {faceCorners};
        const RunEnd& runStart = end0.isRunStart ? end0 : end1;
        const RunEnd& runFinish = end0.isRunStart ? end1 : end0;
        nextRun[runFinish.runIndex] = runStart.runIndex;
    }

    // Follow each run into the run it joins until the face closes.
    std::vector<std::vector<CutCorner>> splitFaces;
    std::vector<bool> isRunUsed(runs.size(), false);
    for (Offset firstRun = 0; firstRun < runs.size(); ++firstRun)
    {
        if (isRunUsed[firstRun]) continue;
        std::vector<CutCorner>& splitFace = splitFaces.emplace_back();
        for (Offset run = firstRun; !isRunUsed[run]; run = nextRun[run])
        {
            isRunUsed[run] = true;
            splitFace.insert(splitFace.end(), runs[run].begin(), runs[run].end());
        }
    }
    return splitFaces;
}

/**
 * @brief Returns the edges of the cut faces that lie on the plane and border the removed side.
 *
 * @note An edge that another cut face runs along in the opposite direction is left out. Faces lying
 * flat in the plane only leave out the edges they share and add none of their own.
 */
std::vector<std::pair<Offset, Offset>> getCutEdges(
    const std::vector<CutFace>& cutFaces,
    const std::vector<Offset>& cutPoints,
    const std::vector<float>& distances
)
{
    std::vector<std::pair<Offset, Offset>> candidateEdges;
    std::vector<std::pair<Offset, Offset>> planeEdges;
    for (const CutFace& cutFace : cutFaces)
    {
        const Offset faceCornerCount = cutFace.corners.size();
        for (Offset cornerIndex = 0; cornerIndex < faceCornerCount; ++cornerIndex)
        {
            const Offset previousCornerIndex =
                (cornerIndex + faceCornerCount - 1) % faceCornerCount;
            const CutCorner& previousCorner = cutFace.corners[previousCornerIndex];
            const CutCorner& corner = cutFace.corners[cornerIndex];
            const bool edgeLiesOnPlane = isOnPlane(previousCorner.pointSource, distances) &&
                                         isOnPlane(corner.pointSource, distances);
            if (!edgeLiesOnPlane) continue;
            if (previousCorner.pointIndex == corner.pointIndex) continue;

            const Offset startPoint = cutPoints[previousCorner.pointIndex];
            const Offset endPoint = cutPoints[corner.pointIndex];
            planeEdges.push_back({startPoint, endPoint});
            if (!cutFace.liesOnPlane) candidateEdges.push_back({startPoint, endPoint});
        }
    }
    std::sort(planeEdges.begin(), planeEdges.end());

    std::vector<std::pair<Offset, Offset>> cutEdges;
    for (const auto& [startPoint, endPoint] : candidateEdges)
    {
        const std::pair<Offset, Offset> reversedEdge{endPoint, startPoint};
        if (std::binary_search(planeEdges.begin(), planeEdges.end(), reversedEdge)) continue;
        cutEdges.push_back({startPoint, endPoint});
    }
    return cutEdges;
}

// Carries attribute values from the source mesh onto the cut mesh.
void copyCutAttributes(
    const geo::Mesh& mesh,
    geo::Mesh& cutMesh,
    const std::vector<BlendSource>& pointSources,
    const std::vector<Offset>& cutPoints,
    const std::vector<CutFace>& cutFaces,
    const std::vector<Offset>& cutFaceOffsets
)
{
    for (const attr::AttributeOwner owner :
         {attr::AttributeOwner::POINT, attr::AttributeOwner::VERTEX, attr::AttributeOwner::FACE})
        cutMesh.addAttributesFrom(mesh, owner);

    std::vector<ElementBlend> pointBlends;
    pointBlends.reserve(pointSources.size());
    for (Offset pointIndex = 0; pointIndex < pointSources.size(); ++pointIndex)
    {
        const BlendSource& pointSource = pointSources[pointIndex];
        pointBlends.push_back(
            {pointSource.sourceOffset0,
             pointSource.sourceOffset1,
             pointSource.blend,
             cutPoints[pointIndex]}
        );
    }
    interpolateAttributeValues(mesh, cutMesh, attr::AttributeOwner::POINT, pointBlends);

    std::vector<Offset> sourceFaces;
    std::vector<ElementBlend> vertexBlends;
    sourceFaces.reserve(cutFaces.size());
    for (Offset faceIndex = 0; faceIndex < cutFaces.size(); ++faceIndex)
    {
        const CutFace& cutFace = cutFaces[faceIndex];
        const Offset cutFaceStartVertex = cutMesh.getFaceStartVertices()[cutFaceOffsets[faceIndex]];
        sourceFaces.push_back(cutFace.sourceFace);

        for (Offset cornerIndex = 0; cornerIndex < cutFace.corners.size(); ++cornerIndex)
        {
            const BlendSource& vertexSource = cutFace.corners[cornerIndex].vertexSource;
            vertexBlends.push_back(
                {vertexSource.sourceOffset0,
                 vertexSource.sourceOffset1,
                 vertexSource.blend,
                 cutFaceStartVertex + cornerIndex}
            );
        }
    }
    copyAttributeValues(mesh, cutMesh, attr::AttributeOwner::FACE, sourceFaces, cutFaceOffsets);
    interpolateAttributeValues(mesh, cutMesh, attr::AttributeOwner::VERTEX, vertexBlends);
}

} // namespace

std::optional<PlaneCut>
cutMeshByPlane(const geo::Mesh& mesh, const Vector3& planePoint, const Vector3& keptSideNormal)
{
    const std::span<const Vector3> sourcePositions = mesh.pointPosSpan();
    const std::vector<float> distances =
        getSignedDistances(sourcePositions, planePoint, keptSideNormal);
    const bool hasPointPastPlane =
        std::any_of(distances.begin(), distances.end(), [](float distance) {
            return distance < 0;
        });
    if (!hasPointPastPlane) return std::nullopt;
    const geo::FaceNormalHandle faceNormals = mesh.getFaceNormal();

    // Clip every face, finding each point by the source point or source edge it sits on so
    // neighbouring faces share it.
    std::vector<BlendSource> pointSources;
    std::vector<Offset> pointIndexBySourcePoint(sourcePositions.size(), kNoPointIndex);
    std::map<std::pair<Offset, Offset>, Offset> pointIndexBySourceEdge;
    std::vector<CutFace> cutFaces;
    bool removedAnyCorner = false;
    for (const Offset faceOffset : mesh.getFaces())
    {
        if (!mesh.isValidFace(faceOffset)) continue;
        if (!mesh.isClosed(faceOffset)) continue;

        ClippedFace clipped = clipFace(mesh, faceOffset, faceNormals, distances, keptSideNormal);
        if (clipped.removedAnyCorner) removedAnyCorner = true;

        for (CutCorner& corner : clipped.corners)
        {
            const BlendSource& pointSource = corner.pointSource;
            const bool isSourcePoint = pointSource.sourceOffset0 == pointSource.sourceOffset1;
            const std::pair<Offset, Offset> sourceEdge =
                std::minmax(pointSource.sourceOffset0, pointSource.sourceOffset1);
            Offset& pointIndex =
                isSourcePoint
                    ? pointIndexBySourcePoint[pointSource.sourceOffset0]
                    : pointIndexBySourceEdge.try_emplace(sourceEdge, kNoPointIndex).first->second;
            if (pointIndex == kNoPointIndex)
            {
                pointIndex = pointSources.size();
                pointSources.push_back(pointSource);
            }
            corner.pointIndex = pointIndex;
            corner.pointSource = pointSources[pointIndex];
        }

        if (clipped.corners.size() < 3) continue;
        if (!hasCornerArrivingAlongCut(clipped.corners))
        {
            cutFaces.push_back({faceOffset, std::move(clipped.corners), clipped.liesOnPlane});
            continue;
        }

        const Vector3 cutDirection = keptSideNormal.cross(faceNormals[faceOffset]);
        for (std::vector<CutCorner>& splitCorners :
             splitAlongCut(std::move(clipped.corners), cutDirection, sourcePositions))
        {
            if (splitCorners.size() < 3) continue;
            cutFaces.push_back({faceOffset, std::move(splitCorners), clipped.liesOnPlane});
        }
    }

    // Build the cut mesh's points.
    auto cutMesh = std::make_shared<geo::Mesh>(mesh.getPath());
    std::vector<Vector3> cutPositions;
    cutPositions.reserve(pointSources.size());
    for (const BlendSource& pointSource : pointSources)
        cutPositions.push_back(getBlendedPosition(sourcePositions, pointSource));
    const std::vector<Offset> cutPoints = cutMesh->addPoints(cutPositions);

    // Build the cut mesh's faces.
    std::vector<Offset> cornerPoints;
    std::vector<Offset> cornerCounts;
    for (const CutFace& cutFace : cutFaces)
    {
        for (const CutCorner& corner : cutFace.corners)
            cornerPoints.push_back(cutPoints[corner.pointIndex]);
        cornerCounts.push_back(cutFace.corners.size());
    }
    const std::vector<Offset> cutFaceOffsets = cutMesh->addFaces(cornerPoints, cornerCounts);

    copyCutAttributes(mesh, *cutMesh, pointSources, cutPoints, cutFaces, cutFaceOffsets);

    PlaneCut cut{cutMesh, {}};
    if (removedAnyCorner) cut.cutEdges = getCutEdges(cutFaces, cutPoints, distances);
    return cut;
}

std::vector<Offset>
fillCutCaps(geo::Mesh& mesh, const std::vector<std::pair<Offset, Offset>>& cutEdges)
{
    // A cap runs along each cut edge backwards, from the edge's end point to its start point.
    std::unordered_map<Offset, std::vector<Offset>> nextCapPoints;
    for (const auto& [startPoint, endPoint] : cutEdges)
        nextCapPoints[endPoint].push_back(startPoint);

    // Walk the unused edges, closing a loop each time the walk comes back to a point already on it.
    std::vector<Offset> capPoints;
    std::vector<Offset> capCornerCounts;
    for (const auto& [startPoint, endPoint] : cutEdges)
    {
        std::vector<Offset> walk = {endPoint};
        std::unordered_map<Offset, size_t> walkIndexByPoint = {{endPoint, 0}};
        while (true)
        {
            auto nextPoints = nextCapPoints.find(walk.back());
            if (nextPoints == nextCapPoints.end() || nextPoints->second.empty()) break;
            const Offset nextPoint = nextPoints->second.back();
            nextPoints->second.pop_back();

            const auto repeat = walkIndexByPoint.find(nextPoint);
            if (repeat == walkIndexByPoint.end())
            {
                walkIndexByPoint[nextPoint] = walk.size();
                walk.push_back(nextPoint);
                continue;
            }

            const size_t loopStart = repeat->second;
            const size_t loopCornerCount = walk.size() - loopStart;
            if (loopCornerCount >= 3)
            {
                capPoints.insert(capPoints.end(), walk.begin() + loopStart, walk.end());
                capCornerCounts.push_back(loopCornerCount);
            }
            for (size_t walkIndex = loopStart + 1; walkIndex < walk.size(); ++walkIndex)
                walkIndexByPoint.erase(walk[walkIndex]);
            walk.resize(loopStart + 1);
        }
    }

    if (capCornerCounts.empty()) return {};
    return mesh.addFaces(capPoints, capCornerCounts);
}

} // namespace enzo::utils
