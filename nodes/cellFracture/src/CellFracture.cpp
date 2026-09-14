#include "Engine/Attribute/Transform.h"
#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/AttributeTransfer.h"
#include "Engine/GeometryAlgorithms/PlaneCut.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Primitives/Mesh.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <tbb/parallel_for.h>
#include <tuple>
#include <vector>

namespace {

using namespace enzo;

constexpr Offset kNoPoint = std::numeric_limits<Offset>::max();

/// @brief Returns the distance from a position to the farthest point of the mesh.
float getFarthestPointDistance(const geo::Mesh& mesh, const Vector3& position)
{
    float farthestSquaredDistance = 0;
    for (const Vector3& pointPosition : mesh.pointPosSpan())
    {
        const float squaredDistance = (pointPosition - position).squaredNorm();
        farthestSquaredDistance = std::max(farthestSquaredDistance, squaredDistance);
    }
    return std::sqrt(farthestSquaredDistance);
}

/// @brief Returns the indices of every seed but one, nearest to that seed first.
std::vector<size_t>
getOtherSeedsByDistance(const std::vector<Vector3>& seedPositions, size_t seedIndex)
{
    const Vector3& seedPosition = seedPositions[seedIndex];

    std::vector<size_t> otherSeedIndices;
    for (size_t otherSeedIndex = 0; otherSeedIndex < seedPositions.size(); ++otherSeedIndex)
        if (otherSeedIndex != seedIndex) otherSeedIndices.push_back(otherSeedIndex);

    std::sort(otherSeedIndices.begin(), otherSeedIndices.end(), [&](size_t left, size_t right) {
        const float leftDistance = (seedPositions[left] - seedPosition).squaredNorm();
        const float rightDistance = (seedPositions[right] - seedPosition).squaredNorm();
        return std::tie(leftDistance, left) < std::tie(rightDistance, right);
    });
    return otherSeedIndices;
}

/// @brief Returns the part of the mesh closer to one seed than to any other, with each cut capped.
std::shared_ptr<geo::Mesh> cutPiece(
    const geo::Mesh& mesh,
    const std::vector<Vector3>& seedPositions,
    size_t seedIndex,
    const String& insideGroupName
)
{
    const Vector3& seedPosition = seedPositions[seedIndex];

    // Cut away everything past the halfway plane between this seed and each other seed.
    std::shared_ptr<geo::Mesh> piece;
    const geo::Mesh* uncut = &mesh;
    float pieceRadius = getFarthestPointDistance(mesh, seedPosition);
    for (const size_t otherSeedIndex : getOtherSeedsByDistance(seedPositions, seedIndex))
    {
        const Vector3& otherSeedPosition = seedPositions[otherSeedIndex];
        const Vector3 otherSeedToSeed = seedPosition - otherSeedPosition;
        const float seedDistance = otherSeedToSeed.norm();

        // Seeds in the same place give their shared space to the first of them.
        if (seedDistance == 0)
        {
            if (otherSeedIndex < seedIndex) return std::make_shared<geo::Mesh>(mesh.getPath());
            continue;
        }

        // Stops once the halfway plane lies past the piece, since every later seed is farther.
        if (seedDistance / 2 > pieceRadius) break;

        const Vector3 towardSeed = otherSeedToSeed / seedDistance;
        const Vector3 midpoint = (seedPosition + otherSeedPosition) / 2;
        const std::optional<utils::PlaneCut> cut =
            utils::cutMeshByPlane(*uncut, midpoint, towardSeed);
        if (!cut) continue;

        const std::vector<Offset> capFaces = utils::fillCutCaps(*cut->mesh, cut->cutEdges);
        if (!insideGroupName.empty())
        {
            cut->mesh->addFaceGroup(insideGroupName);
            cut->mesh->addToFaceGroup(insideGroupName, capFaces);
        }

        piece = cut->mesh;
        uncut = piece.get();
        if (piece->getNumFaces() == 0) break;
        pieceRadius = getFarthestPointDistance(*piece, seedPosition);
    }

    if (!piece) piece = std::make_shared<geo::Mesh>(mesh);
    return piece;
}

/// @brief Adds a piece's faces and the points they use to the fractured mesh.
void appendPiece(
    geo::Mesh& fractured,
    const geo::Mesh& piece,
    intT pieceNumber,
    const String& pieceAttributeName
)
{
    // Collect the valid faces and the points they use.
    std::vector<Offset> pieceFaces;
    std::vector<Offset> usedIndexByPiecePoint(piece.getNumPoints(), kNoPoint);
    std::vector<Offset> usedPiecePoints;
    std::vector<Vector3> usedPositions;
    for (const Offset faceOffset : piece.getFaces())
    {
        if (!piece.isValidFace(faceOffset)) continue;
        pieceFaces.push_back(faceOffset);
        for (const intT pointOffset : piece.getFacePoints(faceOffset))
        {
            if (usedIndexByPiecePoint[pointOffset] != kNoPoint) continue;
            usedIndexByPiecePoint[pointOffset] = usedPiecePoints.size();
            usedPiecePoints.push_back(pointOffset);
            usedPositions.push_back(piece.getPointPos(pointOffset));
        }
    }
    if (pieceFaces.empty()) return;

    const std::vector<Offset> fracturedPoints = fractured.addPoints(usedPositions);

    std::vector<Offset> cornerPoints;
    std::vector<Offset> cornerCounts;
    for (const Offset faceOffset : pieceFaces)
    {
        for (const intT pointOffset : piece.getFacePoints(faceOffset))
            cornerPoints.push_back(fracturedPoints[usedIndexByPiecePoint[pointOffset]]);
        cornerCounts.push_back(piece.getFaceVertCount(faceOffset));
    }
    const std::vector<Offset> fracturedFaces = fractured.addFaces(cornerPoints, cornerCounts);

    // Carry attributes across.
    utils::copyAttributeValues(
        piece,
        fractured,
        attr::AttributeOwner::POINT,
        usedPiecePoints,
        fracturedPoints
    );
    utils::copyAttributeValues(
        piece,
        fractured,
        attr::AttributeOwner::FACE,
        pieceFaces,
        fracturedFaces
    );

    std::vector<Offset> pieceVertices;
    std::vector<Offset> fracturedVertices;
    for (size_t faceIndex = 0; faceIndex < pieceFaces.size(); ++faceIndex)
    {
        const Offset pieceFace = pieceFaces[faceIndex];
        const Offset pieceStartVertex = piece.getFaceStartVertices()[pieceFace];
        const Offset fracturedStartVertex =
            fractured.getFaceStartVertices()[fracturedFaces[faceIndex]];
        for (Offset cornerIndex = 0; cornerIndex < piece.getFaceVertCount(pieceFace); ++cornerIndex)
        {
            pieceVertices.push_back(pieceStartVertex + cornerIndex);
            fracturedVertices.push_back(fracturedStartVertex + cornerIndex);
        }
    }
    utils::copyAttributeValues(
        piece,
        fractured,
        attr::AttributeOwner::VERTEX,
        pieceVertices,
        fracturedVertices
    );

    // Number the faces by the seed they formed around.
    if (pieceAttributeName.empty()) return;
    auto pieceAttribute =
        fractured.addAttribute<intT>(attr::AttributeOwner::FACE, pieceAttributeName);
    for (const Offset fracturedFace : fracturedFaces)
        pieceAttribute.setValue(fracturedFace, pieceNumber);
}

/// @brief Returns the mesh broken into one piece for each seed, merged into a single mesh.
std::shared_ptr<geo::Mesh> fractureMesh(
    const geo::Mesh& mesh,
    const std::vector<Vector3>& seedPositions,
    const String& pieceAttributeName,
    const String& insideGroupName
)
{
    std::vector<std::shared_ptr<geo::Mesh>> pieces(seedPositions.size());
    tbb::parallel_for(size_t{0}, seedPositions.size(), [&](size_t seedIndex) {
        pieces[seedIndex] = cutPiece(mesh, seedPositions, seedIndex, insideGroupName);
    });

    auto fractured = std::make_shared<geo::Mesh>(mesh.getPath());
    for (const attr::AttributeOwner owner :
         {attr::AttributeOwner::POINT, attr::AttributeOwner::VERTEX, attr::AttributeOwner::FACE})
        fractured->addAttributesFrom(mesh, owner);
    if (!pieceAttributeName.empty())
        fractured->addAttribute<intT>(attr::AttributeOwner::FACE, pieceAttributeName);
    if (!insideGroupName.empty()) fractured->addFaceGroup(insideGroupName);

    for (size_t seedIndex = 0; seedIndex < pieces.size(); ++seedIndex)
        appendPiece(*fractured, *pieces[seedIndex], seedIndex, pieceAttributeName);

    return fractured;
}

class CellFracture : public nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void CellFracture::cook()
{
    if (!outputRequested(0)) return;

    NodePacket packet = cloneInputPacket(0);

    NodePacket seedPacket = cloneInputPacket(1);
    std::vector<Vector3> seedPositions;
    for (const Transform& seedTransform : seedPacket.getTransforms(TransformClass::POINT_PRIORITY))
        seedPositions.push_back(seedTransform * Vector3::Zero());

    if (seedPositions.empty())
    {
        setOutputPacket(0, packet);
        return;
    }

    const String pieceAttributeName = evalParmString("pieceAttribute");
    const String insideGroupName = evalParmString("insideGroup");

    NodePacket output;
    for (const geo::PrimPtr& prim : packet.getPrimitives())
    {
        if (prim->getType() != geo::PrimType::MESH)
        {
            output.addPrimitive(prim);
            continue;
        }
        const auto mesh = std::static_pointer_cast<geo::Mesh>(prim);
        output.addPrimitive(
            fractureMesh(*mesh, seedPositions, pieceAttributeName, insideGroupName)
        );
    }

    setOutputPacket(0, output);
}

} // namespace

ENZO_REGISTER_NODE(cellFracture, CellFracture)
