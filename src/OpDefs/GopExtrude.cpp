#include "OpDefs/GopExtrude.h"
#include "Engine/Operator/Mesh.h"
#include "Engine/Operator/Selection.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Types.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
using Edge = std::pair<enzo::ga::Offset, enzo::ga::Offset>;
struct EdgeHash
{
    size_t operator()(const Edge& edge) const noexcept
    {
        const size_t a = std::hash<enzo::ga::Offset>{}(edge.first);
        const size_t b = std::hash<enzo::ga::Offset>{}(edge.second);
        return a ^ (b + 0x9e3779b97f4a7c15ULL + (a << 6) + (a >> 2));
    }
};
}

// Adjacent selected faces share their top ring. Internal edges between them
// disappear and corner points move by the average of the incident face normals.
static void extrudeConnected(std::shared_ptr<enzo::geo::Mesh> mesh,
                             const std::vector<enzo::ga::Offset>& faces,
                             float distance,
                             const std::string& sideGroupName,
                             const std::string& topGroupName)
{
    auto faceNormals = mesh->getFaceNormal();

    // Record every directed edge and accumulate displacement per shared point
    std::unordered_set<Edge, EdgeHash> directedEdges;
    std::unordered_map<enzo::ga::Offset, enzo::bt::Vector3> displacementSum;
    std::unordered_map<enzo::ga::Offset, unsigned int> displacementCount;

    for (auto face : faces)
    {
        auto facePoints = mesh->getFacePoints(face);
        const enzo::bt::Vector3 displacement = faceNormals[face] * distance;
        const unsigned int facePointCount = mesh->getFacePointCount(face);

        for (size_t i = 0; i < facePointCount; ++i)
        {
            const enzo::ga::Offset pointOffset = facePoints[i];
            const enzo::ga::Offset nextPointOffset = facePoints[(i + 1) % facePointCount];
            directedEdges.insert({pointOffset, nextPointOffset});

            auto [it, inserted] = displacementSum.try_emplace(pointOffset, displacement);
            if (!inserted) it->second += displacement;
            displacementCount[pointOffset] += 1;
        }
    }

    // Duplicate each unique ring point along its averaged displacement
    std::vector<enzo::ga::Offset> uniqueOldPoints;
    std::vector<enzo::bt::Vector3> newPointPositions;
    uniqueOldPoints.reserve(displacementSum.size());
    newPointPositions.reserve(displacementSum.size());

    for (const auto& [oldOffset, dispSum] : displacementSum)
    {
        const float averageDivisor = static_cast<float>(displacementCount[oldOffset]);
        const enzo::bt::Vector3 averagedDisplacement = dispSum / averageDivisor;
        uniqueOldPoints.push_back(oldOffset);
        newPointPositions.push_back(mesh->getPointPos(oldOffset) + averagedDisplacement);
    }

    // Create new points and map old ring to new ring
    std::vector<enzo::ga::Offset> newPoints = mesh->addPoints(newPointPositions);
    std::unordered_map<enzo::ga::Offset, enzo::ga::Offset> oldToNew;
    oldToNew.reserve(uniqueOldPoints.size());
    for (size_t i = 0; i < uniqueOldPoints.size(); ++i)
    {
        oldToNew[uniqueOldPoints[i]] = newPoints[i];
    }

    // Build top caps and side quads
    std::vector<enzo::ga::Offset> sidePointOffsetsFlat;
    std::vector<enzo::ga::Offset> sideVertexCounts;
    std::vector<enzo::ga::Offset> topPointOffsetsFlat;
    std::vector<enzo::ga::Offset> topVertexCounts;

    for (auto face : faces)
    {
        auto facePoints = mesh->getFacePoints(face);
        const unsigned int facePointCount = mesh->getFacePointCount(face);

        // Top cap mirrors the original winding on the new ring
        for (size_t i = 0; i < facePointCount; ++i)
        {
            topPointOffsetsFlat.push_back(oldToNew[facePoints[i]]);
        }
        topVertexCounts.push_back(facePointCount);

        // Skip internal edges (their reverse appears in another selected face)
        for (size_t i = 0; i < facePointCount; ++i)
        {
            const enzo::ga::Offset bottomStart = facePoints[i];
            const enzo::ga::Offset bottomEnd = facePoints[(i + 1) % facePointCount];

            if (directedEdges.count({bottomEnd, bottomStart})) continue;

            const enzo::ga::Offset topStart = oldToNew[bottomStart];
            const enzo::ga::Offset topEnd = oldToNew[bottomEnd];

            sidePointOffsetsFlat.push_back(bottomStart);
            sidePointOffsetsFlat.push_back(bottomEnd);
            sidePointOffsetsFlat.push_back(topEnd);
            sidePointOffsetsFlat.push_back(topStart);
            sideVertexCounts.push_back(4);
        }
    }

    // Add the new faces and tag them
    std::vector<enzo::ga::Offset> sideOffsets = mesh->addFaces(sidePointOffsetsFlat, sideVertexCounts);
    std::vector<enzo::ga::Offset> topOffsets = mesh->addFaces(topPointOffsetsFlat, topVertexCounts);
    mesh->addToFaceGroup(sideGroupName, sideOffsets);
    mesh->addToFaceGroup(topGroupName, topOffsets);
}

// Every selected face is lifted in isolation. Adjacent faces no longer share
// corner points so internal edges remain as side walls between them.
static void extrudeDisconnected(std::shared_ptr<enzo::geo::Mesh> mesh,
                                const std::vector<enzo::ga::Offset>& faces,
                                float distance,
                                const std::string& sideGroupName,
                                const std::string& topGroupName)
{
    auto faceNormals = mesh->getFaceNormal();

    std::vector<enzo::ga::Offset> sidePointOffsetsFlat;
    std::vector<enzo::ga::Offset> sideVertexCounts;
    std::vector<enzo::ga::Offset> topPointOffsetsFlat;
    std::vector<enzo::ga::Offset> topVertexCounts;

    for (auto face : faces)
    {
        auto facePoints = mesh->getFacePoints(face);
        const enzo::bt::Vector3 displacement = faceNormals[face] * distance;
        const unsigned int facePointCount = mesh->getFacePointCount(face);

        // Duplicate every ring point fresh for this face
        std::vector<enzo::bt::Vector3> newPositionsLocal;
        newPositionsLocal.reserve(facePointCount);
        for (size_t i = 0; i < facePointCount; ++i)
        {
            newPositionsLocal.push_back(mesh->getPointPos(facePoints[i]) + displacement);
        }
        std::vector<enzo::ga::Offset> faceNewPoints = mesh->addPoints(newPositionsLocal);

        // Top cap mirrors the original winding on the new ring
        for (size_t i = 0; i < facePointCount; ++i)
        {
            topPointOffsetsFlat.push_back(faceNewPoints[i]);
        }
        topVertexCounts.push_back(facePointCount);

        // Every edge gets its own side quad
        for (size_t i = 0; i < facePointCount; ++i)
        {
            const enzo::ga::Offset bottomStart = facePoints[i];
            const enzo::ga::Offset bottomEnd = facePoints[(i + 1) % facePointCount];
            const enzo::ga::Offset topStart = faceNewPoints[i];
            const enzo::ga::Offset topEnd = faceNewPoints[(i + 1) % facePointCount];

            sidePointOffsetsFlat.push_back(bottomStart);
            sidePointOffsetsFlat.push_back(bottomEnd);
            sidePointOffsetsFlat.push_back(topEnd);
            sidePointOffsetsFlat.push_back(topStart);
            sideVertexCounts.push_back(4);
        }
    }

    // Add the new faces and tag them
    std::vector<enzo::ga::Offset> sideOffsets = mesh->addFaces(sidePointOffsetsFlat, sideVertexCounts);
    std::vector<enzo::ga::Offset> topOffsets = mesh->addFaces(topPointOffsetsFlat, topVertexCounts);
    mesh->addToFaceGroup(sideGroupName, sideOffsets);
    mesh->addToFaceGroup(topGroupName, topOffsets);
}

void extrude(enzo::geo::PrimPtr prim, std::vector<enzo::ga::Offset> faces, float distance = 1, bool connected = true)
{
    std::shared_ptr<enzo::geo::Mesh> mesh = std::dynamic_pointer_cast<enzo::geo::Mesh>(prim);
    if (!mesh)
    {
        return;
    }

    // Group names for the three regions the extrude produces
    const std::string sideGroupName = "extrudeSide";
    const std::string topGroupName = "extrudeTop";
    const std::string bottomGroupName = "extrudeBottom";
    mesh->createFaceGroup(sideGroupName);
    mesh->createFaceGroup(topGroupName);
    mesh->createFaceGroup(bottomGroupName);

    // Originals stay as the bottom regardless of mode
    mesh->addToFaceGroup(bottomGroupName, faces);

    if (connected)
    {
        extrudeConnected(mesh, faces, distance, sideGroupName, topGroupName);
    }
    else
    {
        extrudeDisconnected(mesh, faces, distance, sideGroupName, topGroupName);
    }
}

GopExtrude::GopExtrude(enzo::nt::NetworkManager *network, enzo::op::OpInfo opInfo)
    : GeometryOpDef(network, opInfo) {}

void GopExtrude::cookOp(enzo::op::Context context) {
    using namespace enzo;

    if (outputRequested(0)) {
        NodePacket packet = context.cloneInputPacket(0);

        const bt::String selectionStr = context.evalStringParm("selection");
        const float distance = context.evalFloatParm("distance");
        const bool connected = context.evalBoolParm("connected");
        enzo::Selection selection(selectionStr);

        for (geo::PrimPtr prim : selection.getPrims(packet)) {
            extrude(prim, selection.getFaces(prim), distance, connected);
        }

        setOutputPacket(0, packet);
    }
}

enzo::prm::Template GopExtrude::parameterList[] = {
    enzo::prm::Template(enzo::prm::Type::STRING, enzo::prm::Name("selection", "Selection")),
    enzo::prm::Template(enzo::prm::Type::BOOL, enzo::prm::Name("connected", "Connected"), enzo::prm::Default(true)),
    enzo::prm::Template(enzo::prm::Type::FLOAT, enzo::prm::Name("distance", "Distance"), enzo::prm::Default(1), 1, enzo::prm::Range(-10, 10)),
    enzo::prm::Terminator};
