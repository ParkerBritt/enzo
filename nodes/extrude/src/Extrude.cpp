#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Parameter/Style.h"
#include "Engine/Primitives/Mesh.h"
#include "Engine/Selection/Selection.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
using Edge = std::pair<enzo::Offset, enzo::Offset>;
struct EdgeHash
{
    size_t operator()(const Edge& edge) const noexcept
    {
        const size_t a = std::hash<enzo::Offset>{}(edge.first);
        const size_t b = std::hash<enzo::Offset>{}(edge.second);
        return a ^ (b + 0x9e3779b97f4a7c15ULL + (a << 6) + (a >> 2));
    }
};

/**
 * @brief Inward bisector offset at a polygon corner.
 *
 * Slides the corner along the bisector of its two incident edges so each
 * edge moves inward by @p distance. The two edges can come from different
 * faces (a connected boundary corner where two selected faces meet), so
 * each edge brings its own face normal to define what inward means.
 *
 * @return The offset to add to the corner, or zero if the two edges are
 * antiparallel and the bisector is undefined.
 */
enzo::Vector3 cornerInsetOffset(
    const enzo::Vector3& prevPos,
    const enzo::Vector3& cornerPos,
    const enzo::Vector3& nextPos,
    const enzo::Vector3& inEdgeFaceNormal,
    const enzo::Vector3& outEdgeFaceNormal,
    float distance
)
{
    const enzo::Vector3 inDir = (cornerPos - prevPos).normalized();
    const enzo::Vector3 outDir = (nextPos - cornerPos).normalized();
    const enzo::Vector3 inNormal = inEdgeFaceNormal.cross(inDir).normalized();
    const enzo::Vector3 outNormal = outEdgeFaceNormal.cross(outDir).normalized();
    const double denom = 1.0 + inNormal.dot(outNormal);
    if (std::abs(denom) < 1e-6) return enzo::Vector3::Zero();
    const enzo::Vector3 bisector = (inNormal + outNormal) / denom;
    return bisector * static_cast<double>(distance);
}
} // namespace

/// @brief Extrudes faces as a connected island, sharing corners between adjacent selected faces.
static void extrudeConnected(
    std::shared_ptr<enzo::geo::Mesh> mesh,
    const std::vector<enzo::Offset>& faces,
    float distance,
    float inset,
    const std::string& sideGroupName,
    const std::string& frontGroupName,
    bool emitFront,
    bool emitSide
)
{
    auto faceNormals = mesh->getFaceNormal();

    // Record directed edges and accumulate extrude displacement per shared point.
    std::unordered_map<Edge, size_t, EdgeHash> edgeToFaceIdx;
    std::unordered_map<enzo::Offset, enzo::Vector3> displacementSum;
    std::unordered_map<enzo::Offset, unsigned int> displacementCount;

    for (size_t faceIdx = 0; faceIdx < faces.size(); ++faceIdx)
    {
        const enzo::Offset face = faces[faceIdx];
        auto facePoints = mesh->getFacePoints(face);
        const enzo::Vector3 displacement = faceNormals[face] * distance;
        const unsigned int facePointCount = mesh->getFacePointCount(face);

        for (size_t i = 0; i < facePointCount; ++i)
        {
            const enzo::Offset pointOffset = facePoints[i];
            const enzo::Offset nextPointOffset = facePoints[(i + 1) % facePointCount];
            edgeToFaceIdx[{pointOffset, nextPointOffset}] = faceIdx;

            auto [it, inserted] = displacementSum.try_emplace(pointOffset, displacement);
            if (!inserted) it->second += displacement;
            displacementCount[pointOffset] += 1;
        }
    }

    // Outer boundary half-edges per point: those whose reverse is not in the
    // selection. Each entry records the neighbouring point along the outer
    // boundary and the face that owns the directed edge.
    struct OuterHalfEdge
    {
        enzo::Offset neighbour;
        size_t faceIdx;
    };
    std::unordered_map<enzo::Offset, OuterHalfEdge> outgoingOuter;
    std::unordered_map<enzo::Offset, OuterHalfEdge> incomingOuter;
    for (const auto& [edge, faceIdx] : edgeToFaceIdx)
    {
        if (edgeToFaceIdx.count({edge.second, edge.first})) continue;
        outgoingOuter[edge.first] = {edge.second, faceIdx};
        incomingOuter[edge.second] = {edge.first, faceIdx};
    }

    // Duplicate each unique ring point along its averaged displacement,
    // applying an inward-bisector inset on outer boundary points only.
    std::vector<enzo::Offset> uniqueOldPoints;
    std::vector<enzo::Vector3> newPointPositions;
    uniqueOldPoints.reserve(displacementSum.size());
    newPointPositions.reserve(displacementSum.size());

    for (const auto& [oldOffset, dispSum] : displacementSum)
    {
        auto inIt = incomingOuter.find(oldOffset);
        auto outIt = outgoingOuter.find(oldOffset);
        const bool onOuterBoundary = inIt != incomingOuter.end() || outIt != outgoingOuter.end();

        // Without a top cap, interior ring points are unreferenced and would orphan in the mesh.
        if (!emitFront && !onOuterBoundary) continue;

        const float averageDivisor = static_cast<float>(displacementCount[oldOffset]);
        const enzo::Vector3 averagedDisplacement = dispSum / averageDivisor;
        enzo::Vector3 newPos = mesh->getPointPos(oldOffset) + averagedDisplacement;

        if (inset != 0.0f)
        {
            if (inIt != incomingOuter.end() && outIt != outgoingOuter.end())
            {
                const enzo::Vector3 prevPos = mesh->getPointPos(inIt->second.neighbour);
                const enzo::Vector3 cornerPos = mesh->getPointPos(oldOffset);
                const enzo::Vector3 nextPos = mesh->getPointPos(outIt->second.neighbour);
                const enzo::Vector3 inFaceNormal = faceNormals[faces[inIt->second.faceIdx]];
                const enzo::Vector3 outFaceNormal = faceNormals[faces[outIt->second.faceIdx]];
                newPos += cornerInsetOffset(
                    prevPos,
                    cornerPos,
                    nextPos,
                    inFaceNormal,
                    outFaceNormal,
                    inset
                );
            }
        }

        uniqueOldPoints.push_back(oldOffset);
        newPointPositions.push_back(newPos);
    }

    // Create new points and map old ring to new ring
    std::vector<enzo::Offset> newPoints = mesh->addPoints(newPointPositions);
    std::unordered_map<enzo::Offset, enzo::Offset> oldToNew;
    oldToNew.reserve(uniqueOldPoints.size());
    for (size_t i = 0; i < uniqueOldPoints.size(); ++i)
    {
        oldToNew[uniqueOldPoints[i]] = newPoints[i];
    }

    // Build top caps and side quads
    std::vector<enzo::Offset> sidePointOffsetsFlat;
    std::vector<enzo::Offset> sideVertexCounts;
    std::vector<enzo::Offset> topPointOffsetsFlat;
    std::vector<enzo::Offset> topVertexCounts;

    for (auto face : faces)
    {
        auto facePoints = mesh->getFacePoints(face);
        const unsigned int facePointCount = mesh->getFacePointCount(face);

        if (emitFront)
        {
            // Top cap mirrors the original winding on the new ring
            for (size_t i = 0; i < facePointCount; ++i)
            {
                topPointOffsetsFlat.push_back(oldToNew[facePoints[i]]);
            }
            topVertexCounts.push_back(facePointCount);
        }

        if (emitSide)
        {
            // Skip internal edges whose reverse appears in another selected face
            for (size_t i = 0; i < facePointCount; ++i)
            {
                const enzo::Offset bottomStart = facePoints[i];
                const enzo::Offset bottomEnd = facePoints[(i + 1) % facePointCount];

                if (edgeToFaceIdx.count({bottomEnd, bottomStart})) continue;

                const enzo::Offset topStart = oldToNew[bottomStart];
                const enzo::Offset topEnd = oldToNew[bottomEnd];

                sidePointOffsetsFlat.push_back(bottomStart);
                sidePointOffsetsFlat.push_back(bottomEnd);
                sidePointOffsetsFlat.push_back(topEnd);
                sidePointOffsetsFlat.push_back(topStart);
                sideVertexCounts.push_back(4);
            }
        }
    }

    // Add the new faces and tag them
    if (emitSide)
    {
        std::vector<enzo::Offset> sideOffsets =
            mesh->addFaces(sidePointOffsetsFlat, sideVertexCounts);
        if (!sideGroupName.empty()) mesh->addToFaceGroup(sideGroupName, sideOffsets);
    }
    if (emitFront)
    {
        std::vector<enzo::Offset> topOffsets = mesh->addFaces(topPointOffsetsFlat, topVertexCounts);
        if (!frontGroupName.empty()) mesh->addToFaceGroup(frontGroupName, topOffsets);
    }
}

/// @brief Extrudes each selected face in isolation.
static void extrudeDisconnected(
    std::shared_ptr<enzo::geo::Mesh> mesh,
    const std::vector<enzo::Offset>& faces,
    float distance,
    float inset,
    const std::string& sideGroupName,
    const std::string& frontGroupName,
    bool emitFront,
    bool emitSide
)
{
    auto faceNormals = mesh->getFaceNormal();

    std::vector<enzo::Offset> sidePointOffsetsFlat;
    std::vector<enzo::Offset> sideVertexCounts;
    std::vector<enzo::Offset> topPointOffsetsFlat;
    std::vector<enzo::Offset> topVertexCounts;

    for (auto face : faces)
    {
        auto facePoints = mesh->getFacePoints(face);
        const enzo::Vector3 displacement = faceNormals[face] * distance;
        const enzo::Vector3 faceNormal = faceNormals[face];
        const unsigned int facePointCount = mesh->getFacePointCount(face);

        // Duplicate every ring point fresh for this face, applying an
        // inward-bisector inset on each corner.
        std::vector<enzo::Vector3> newPositionsLocal;
        newPositionsLocal.reserve(facePointCount);
        for (size_t i = 0; i < facePointCount; ++i)
        {
            enzo::Vector3 newPos = mesh->getPointPos(facePoints[i]) + displacement;
            if (inset != 0.0f)
            {
                const enzo::Offset prevOffset =
                    facePoints[(i + facePointCount - 1) % facePointCount];
                const enzo::Offset nextOffset = facePoints[(i + 1) % facePointCount];
                const enzo::Vector3 prevPos = mesh->getPointPos(prevOffset);
                const enzo::Vector3 cornerPos = mesh->getPointPos(facePoints[i]);
                const enzo::Vector3 nextPos = mesh->getPointPos(nextOffset);
                newPos +=
                    cornerInsetOffset(prevPos, cornerPos, nextPos, faceNormal, faceNormal, inset);
            }
            newPositionsLocal.push_back(newPos);
        }

        std::vector<enzo::Offset> faceNewPoints = mesh->addPoints(newPositionsLocal);

        if (emitFront)
        {
            // Top cap mirrors the original winding on the new ring
            for (size_t i = 0; i < facePointCount; ++i)
            {
                topPointOffsetsFlat.push_back(faceNewPoints[i]);
            }
            topVertexCounts.push_back(facePointCount);
        }

        if (emitSide)
        {
            // Every edge gets its own side quad
            for (size_t i = 0; i < facePointCount; ++i)
            {
                const enzo::Offset bottomStart = facePoints[i];
                const enzo::Offset bottomEnd = facePoints[(i + 1) % facePointCount];
                const enzo::Offset topStart = faceNewPoints[i];
                const enzo::Offset topEnd = faceNewPoints[(i + 1) % facePointCount];

                sidePointOffsetsFlat.push_back(bottomStart);
                sidePointOffsetsFlat.push_back(bottomEnd);
                sidePointOffsetsFlat.push_back(topEnd);
                sidePointOffsetsFlat.push_back(topStart);
                sideVertexCounts.push_back(4);
            }
        }
    }

    // Add the new faces and tag them
    if (emitSide)
    {
        std::vector<enzo::Offset> sideOffsets =
            mesh->addFaces(sidePointOffsetsFlat, sideVertexCounts);
        if (!sideGroupName.empty()) mesh->addToFaceGroup(sideGroupName, sideOffsets);
    }
    if (emitFront)
    {
        std::vector<enzo::Offset> topOffsets = mesh->addFaces(topPointOffsetsFlat, topVertexCounts);
        if (!frontGroupName.empty()) mesh->addToFaceGroup(frontGroupName, topOffsets);
    }
}

struct GroupNames
{
    std::string side;
    std::string front;
    std::string back;
};

struct OutputFlags
{
    bool front;
    bool side;
    bool back;
};

void extrude(
    enzo::geo::PrimPtr prim,
    std::vector<enzo::Offset> faces,
    float distance,
    float inset,
    bool connected,
    const GroupNames& groupNames,
    const OutputFlags& outputs
)
{
    std::shared_ptr<enzo::geo::Mesh> mesh = std::dynamic_pointer_cast<enzo::geo::Mesh>(prim);
    if (!mesh)
    {
        return;
    }

    // Helpers ignore empty names, so disabled groups stay out of the mesh.
    const std::string sideGroup = outputs.side ? groupNames.side : std::string();
    const std::string frontGroup = outputs.front ? groupNames.front : std::string();

    if (outputs.side && !groupNames.side.empty()) mesh->createFaceGroup(groupNames.side);
    if (outputs.front && !groupNames.front.empty()) mesh->createFaceGroup(groupNames.front);
    if (outputs.back && !groupNames.back.empty())
    {
        mesh->createFaceGroup(groupNames.back);
        mesh->addToFaceGroup(groupNames.back, faces);
    }

    if (outputs.front || outputs.side)
    {
        if (connected)
        {
            extrudeConnected(
                mesh,
                faces,
                distance,
                inset,
                sideGroup,
                frontGroup,
                outputs.front,
                outputs.side
            );
        }
        else
        {
            extrudeDisconnected(
                mesh,
                faces,
                distance,
                inset,
                sideGroup,
                frontGroup,
                outputs.front,
                outputs.side
            );
        }
    }

    // Back output off removes the original selected faces, hollowing the extrude.
    if (!outputs.back)
    {
        mesh->deleteFaces(faces);
    }
}

namespace {

class Extrude : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void Extrude::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket packet = cloneInputPacket(0);

    const String selectionStr = evalParmString("selection");
    const float distance = evalParmFloat("distance");
    const float inset = evalParmFloat("inset");
    const bool connected = evalParmBool("connected");

    // An empty group name disables the group, so callers see no stray group.
    GroupNames groupNames;
    if (evalParmBool("frontGroupEnabled")) groupNames.front = evalParmString("frontGroupName");
    if (evalParmBool("sideGroupEnabled")) groupNames.side = evalParmString("sideGroupName");
    if (evalParmBool("backGroupEnabled")) groupNames.back = evalParmString("backGroupName");

    OutputFlags outputs;
    outputs.front = evalParmBool("frontOutput");
    outputs.side = evalParmBool("sideOutput");
    outputs.back = evalParmBool("backOutput");

    enzo::Selection selection(selectionStr);

    for (geo::PrimPtr prim : selection.getPrims(packet))
    {
        extrude(
            prim,
            selection.getFaces(prim),
            distance,
            inset,
            connected,
            groupNames,
            outputs
        );
    }

    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(extrude, Extrude)
