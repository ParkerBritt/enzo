#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/MeshShapes.h"
#include "Engine/GeometryAlgorithms/PlaneCut.h"
#include "Engine/Primitives/Mesh.h"
#include <array>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <map>
#include <memory>
#include <set>
#include <vector>

using namespace enzo;

namespace {

// Builds a mesh from point positions and a flat list of quads, four point offsets each.
std::shared_ptr<geo::Mesh>
buildQuadMesh(const std::vector<Vector3>& positions, const std::vector<Offset>& quadPoints)
{
    auto mesh = std::make_shared<geo::Mesh>();
    mesh->addPoints(positions);
    const std::vector<Offset> vertexCounts(quadPoints.size() / 4, 4);
    mesh->addFaces(quadPoints, vertexCounts);
    return mesh;
}

// Builds a two by two quad spanning x and y from 0 to 2.
std::shared_ptr<geo::Mesh> buildSingleQuad()
{
    return buildQuadMesh({{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}}, {0, 1, 2, 3});
}

// Builds two unit quads side by side along x, sharing the edge at x equal to 1.
std::shared_ptr<geo::Mesh> buildTwoQuads()
{
    return buildQuadMesh(
        {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {0, 1, 0}, {1, 1, 0}, {2, 1, 0}},
        {0, 1, 4, 3, 1, 2, 5, 4}
    );
}

// Returns whether every edge is shared by exactly two faces running along it in opposite
// directions.
bool isClosedSurface(const geo::Mesh& mesh)
{
    std::set<std::pair<Offset, Offset>> directedEdges;
    for (const Offset faceOffset : mesh.getFaces())
    {
        const auto facePoints = mesh.getFacePoints(faceOffset);
        for (size_t cornerIndex = 0; cornerIndex < facePoints.size(); ++cornerIndex)
        {
            const Offset startPoint = facePoints[cornerIndex];
            const Offset endPoint = facePoints[(cornerIndex + 1) % facePoints.size()];
            if (!directedEdges.insert({startPoint, endPoint}).second) return false;
        }
    }
    for (const auto& [startPoint, endPoint] : directedEdges)
        if (!directedEdges.contains({endPoint, startPoint})) return false;
    return true;
}

// Adds a closed box between two corners, reusing points already in the mesh at the same position.
void addBox(
    geo::Mesh& mesh,
    std::map<std::array<float, 3>, Offset>& pointsByPosition,
    const Vector3& minCorner,
    const Vector3& maxCorner
)
{
    auto getPoint = [&](int xSide, int ySide, int zSide) {
        const std::array<float, 3> position = {
            xSide ? maxCorner.x() : minCorner.x(),
            ySide ? maxCorner.y() : minCorner.y(),
            zSide ? maxCorner.z() : minCorner.z(),
        };
        const auto existing = pointsByPosition.find(position);
        if (existing != pointsByPosition.end()) return existing->second;
        const Offset point = mesh.addPoint({position[0], position[1], position[2]});
        pointsByPosition[position] = point;
        return point;
    };

    mesh.addFace({getPoint(0, 0, 0), getPoint(0, 0, 1), getPoint(0, 1, 1), getPoint(0, 1, 0)});
    mesh.addFace({getPoint(1, 0, 0), getPoint(1, 1, 0), getPoint(1, 1, 1), getPoint(1, 0, 1)});
    mesh.addFace({getPoint(0, 0, 0), getPoint(1, 0, 0), getPoint(1, 0, 1), getPoint(0, 0, 1)});
    mesh.addFace({getPoint(0, 1, 0), getPoint(0, 1, 1), getPoint(1, 1, 1), getPoint(1, 1, 0)});
    mesh.addFace({getPoint(0, 0, 0), getPoint(0, 1, 0), getPoint(1, 1, 0), getPoint(1, 0, 0)});
    mesh.addFace({getPoint(0, 0, 1), getPoint(1, 0, 1), getPoint(1, 1, 1), getPoint(0, 1, 1)});
}

// Builds a closed square ring from z 0 to 1, three units across with a one unit hole through the
// middle.
std::shared_ptr<geo::Mesh> buildSquareRing()
{
    const float cornerDirections[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
    const float ringHalfWidths[2] = {1.5f, 0.5f};
    auto getPoint = [](Offset ring, Offset corner, Offset level) {
        return ring * 8 + corner * 2 + level;
    };

    std::vector<Vector3> positions;
    for (Offset ring = 0; ring < 2; ++ring)
        for (Offset corner = 0; corner < 4; ++corner)
            for (Offset level = 0; level < 2; ++level)
                positions.push_back(
                    {cornerDirections[corner][0] * ringHalfWidths[ring],
                     cornerDirections[corner][1] * ringHalfWidths[ring],
                     float(level)}
                );

    std::vector<Offset> quadPoints;
    for (Offset corner = 0; corner < 4; ++corner)
    {
        const Offset next = (corner + 1) % 4;
        const Offset outerTop = getPoint(0, corner, 1);
        const Offset outerNextTop = getPoint(0, next, 1);
        const Offset innerTop = getPoint(1, corner, 1);
        const Offset innerNextTop = getPoint(1, next, 1);
        const Offset outerBottom = getPoint(0, corner, 0);
        const Offset outerNextBottom = getPoint(0, next, 0);
        const Offset innerBottom = getPoint(1, corner, 0);
        const Offset innerNextBottom = getPoint(1, next, 0);

        const std::array<Offset, 4> cornerQuads[] = {
            {outerTop, outerNextTop, innerNextTop, innerTop},
            {outerBottom, innerBottom, innerNextBottom, outerNextBottom},
            {outerBottom, outerNextBottom, outerNextTop, outerTop},
            {innerBottom, innerTop, innerNextTop, innerNextBottom},
        };
        for (const std::array<Offset, 4>& quad : cornerQuads)
            quadPoints.insert(quadPoints.end(), quad.begin(), quad.end());
    }
    return buildQuadMesh(positions, quadPoints);
}

// Returns whether a point lies inside a face when both are seen looking down the z axis.
bool isInsideFaceAlongZ(const geo::Mesh& mesh, Offset faceOffset, float x, float y)
{
    const auto facePoints = mesh.getFacePoints(faceOffset);
    bool isInside = false;
    for (size_t cornerIndex = 0; cornerIndex < facePoints.size(); ++cornerIndex)
    {
        const size_t previousIndex = (cornerIndex + facePoints.size() - 1) % facePoints.size();
        const Vector3 corner = mesh.getPointPos(facePoints[cornerIndex]);
        const Vector3 previous = mesh.getPointPos(facePoints[previousIndex]);
        const bool edgeSpansY = (corner.y() > y) != (previous.y() > y);
        if (!edgeSpansY) continue;
        const float edgeXAtY = corner.x() + (previous.x() - corner.x()) * (y - corner.y()) /
                                                (previous.y() - corner.y());
        if (x < edgeXAtY) isInside = !isInside;
    }
    return isInside;
}

// Returns how many of the faces contain a point when seen looking down the z axis.
int countFacesCoveringAlongZ(
    const geo::Mesh& mesh,
    const std::vector<Offset>& faces,
    float x,
    float y
)
{
    int coveringCount = 0;
    for (const Offset faceOffset : faces)
        if (isInsideFaceAlongZ(mesh, faceOffset, x, y)) ++coveringCount;
    return coveringCount;
}

// Returns the part of a mesh nearer one seed than any other, capping after every cut.
std::shared_ptr<geo::Mesh>
cutAndCapPiece(const geo::Mesh& mesh, const std::vector<Vector3>& seedPositions, size_t seedIndex)
{
    auto piece = std::make_shared<geo::Mesh>(mesh);
    for (size_t otherSeedIndex = 0; otherSeedIndex < seedPositions.size(); ++otherSeedIndex)
    {
        if (otherSeedIndex == seedIndex) continue;
        const Vector3& seedPosition = seedPositions[seedIndex];
        const Vector3& otherSeedPosition = seedPositions[otherSeedIndex];
        const Vector3 towardSeed = (seedPosition - otherSeedPosition).normalized();
        const Vector3 midpoint = (seedPosition + otherSeedPosition) / 2;
        utils::PlaneCut cut = utils::cutMeshByPlane(*piece, midpoint, towardSeed);
        utils::fillCutCaps(*cut.mesh, cut.cutEdges);
        piece = cut.mesh;
    }
    return piece;
}

// Returns the largest x of any point in the mesh.
float getMaxX(const geo::Mesh& mesh)
{
    float maxX = -INFINITY;
    for (Offset pointOffset = 0; pointOffset < mesh.getNumPoints(); ++pointOffset)
        maxX = std::max(maxX, mesh.getPointPos(pointOffset).x());
    return maxX;
}

} // namespace

TEST_CASE("Plane through a quad keeps the side the normal points toward")
{
    auto quad = buildSingleQuad();

    const utils::PlaneCut cut = utils::cutMeshByPlane(*quad, {1, 0, 0}, {-1, 0, 0});

    REQUIRE(cut.mesh->getNumFaces() == 1);
    REQUIRE(cut.mesh->getFaceVertCount(0) == 4);
    REQUIRE(getMaxX(*cut.mesh) == Catch::Approx(1));
}

TEST_CASE("Quad entirely on the kept side comes through unchanged")
{
    auto quad = buildSingleQuad();

    const utils::PlaneCut cut = utils::cutMeshByPlane(*quad, {5, 0, 0}, {-1, 0, 0});

    REQUIRE(cut.mesh->getNumFaces() == 1);
    REQUIRE(cut.mesh->getNumPoints() == 4);
    REQUIRE(cut.cutEdges.empty());
}

TEST_CASE("Quad entirely on the removed side is dropped")
{
    auto quad = buildSingleQuad();

    const utils::PlaneCut cut = utils::cutMeshByPlane(*quad, {-5, 0, 0}, {-1, 0, 0});

    REQUIRE(cut.mesh->getNumFaces() == 0);
    REQUIRE(cut.mesh->getNumPoints() == 0);
    REQUIRE(cut.cutEdges.empty());
}

TEST_CASE("Two quads sharing an edge share the cut point on it")
{
    auto quads = buildTwoQuads();

    const utils::PlaneCut cut = utils::cutMeshByPlane(*quads, {0, 0.5, 0}, {0, -1, 0});

    // Three bottom points are kept and each of the three vertical edges gets one cut point.
    REQUIRE(cut.mesh->getNumFaces() == 2);
    REQUIRE(cut.mesh->getNumPoints() == 6);
}

TEST_CASE("Each cut face leaves one cut edge lying on the plane")
{
    auto quads = buildTwoQuads();

    const utils::PlaneCut cut = utils::cutMeshByPlane(*quads, {0, 0.5, 0}, {0, -1, 0});

    REQUIRE(cut.cutEdges.size() == 2);
    for (const auto& [startPoint, endPoint] : cut.cutEdges)
    {
        REQUIRE(cut.mesh->getPointPos(startPoint).y() == Catch::Approx(0.5));
        REQUIRE(cut.mesh->getPointPos(endPoint).y() == Catch::Approx(0.5));
        REQUIRE(startPoint != endPoint);
    }
}

TEST_CASE("Cut edge follows the winding of the face it came from")
{
    auto quad = buildSingleQuad();

    const utils::PlaneCut cut = utils::cutMeshByPlane(*quad, {1, 0, 0}, {-1, 0, 0});

    // The quad winds counterclockwise, so its edge along x equal to 1 runs from y 0 up to y 2.
    REQUIRE(cut.cutEdges.size() == 1);
    const auto [startPoint, endPoint] = cut.cutEdges[0];
    REQUIRE(cut.mesh->getPointPos(startPoint).y() == Catch::Approx(0));
    REQUIRE(cut.mesh->getPointPos(endPoint).y() == Catch::Approx(2));
}

TEST_CASE("Point attribute is interpolated at the cut")
{
    auto quad = buildSingleQuad();
    auto heat = quad->addAttribute<floatT>(attr::AttributeOwner::POINT, "heat");
    for (Offset pointOffset = 0; pointOffset < quad->getNumPoints(); ++pointOffset)
        heat.setValue(pointOffset, quad->getPointPos(pointOffset).x() * 10);

    const utils::PlaneCut cut = utils::cutMeshByPlane(*quad, {0.5, 0, 0}, {-1, 0, 0});

    // Heat is ten times x, so a cut point at x equal to 0.5 carries 5.
    auto cutHeatAttribute = cut.mesh->getAttribByName(attr::AttributeOwner::POINT, "heat");
    REQUIRE(cutHeatAttribute != nullptr);
    attr::AttributeHandleRO<floatT> cutHeat(cutHeatAttribute);
    REQUIRE(cut.mesh->getNumPoints() == 4);
    for (Offset pointOffset = 0; pointOffset < cut.mesh->getNumPoints(); ++pointOffset)
    {
        const float x = cut.mesh->getPointPos(pointOffset).x();
        REQUIRE(cutHeat.getValue(pointOffset) == Catch::Approx(x * 10));
    }
}

TEST_CASE("Vertex attribute is interpolated at the cut")
{
    auto quad = buildSingleQuad();
    auto heat = quad->addAttribute<floatT>(attr::AttributeOwner::VERTEX, "heat");
    for (Offset vertexOffset = 0; vertexOffset < quad->getNumVerts(); ++vertexOffset)
        heat.setValue(vertexOffset, quad->getPosFromVert(vertexOffset).x() * 10);

    const utils::PlaneCut cut = utils::cutMeshByPlane(*quad, {0.5, 0, 0}, {-1, 0, 0});

    // Heat is ten times x, so a cut corner at x equal to 0.5 carries 5.
    auto cutHeatAttribute = cut.mesh->getAttribByName(attr::AttributeOwner::VERTEX, "heat");
    REQUIRE(cutHeatAttribute != nullptr);
    attr::AttributeHandleRO<floatT> cutHeat(cutHeatAttribute);
    REQUIRE(cut.mesh->getNumVerts() == 4);
    for (Offset vertexOffset = 0; vertexOffset < cut.mesh->getNumVerts(); ++vertexOffset)
    {
        const float x = cut.mesh->getPosFromVert(vertexOffset).x();
        REQUIRE(cutHeat.getValue(vertexOffset) == Catch::Approx(x * 10));
    }
}

TEST_CASE("Corners on the plane are kept without adding cut points")
{
    // A diamond whose top and bottom corners sit on the plane at x equal to 1.
    auto diamond = buildQuadMesh({{1, 0, 0}, {2, 1, 0}, {1, 2, 0}, {0, 1, 0}}, {0, 1, 2, 3});

    const utils::PlaneCut cut = utils::cutMeshByPlane(*diamond, {1, 0, 0}, {-1, 0, 0});

    REQUIRE(cut.mesh->getNumFaces() == 1);
    REQUIRE(cut.mesh->getFaceVertCount(0) == 3);
    REQUIRE(cut.mesh->getNumPoints() == 3);
    REQUIRE(cut.cutEdges.size() == 1);
    const auto [startPoint, endPoint] = cut.cutEdges[0];
    REQUIRE(cut.mesh->getPointPos(startPoint).y() == Catch::Approx(0));
    REQUIRE(cut.mesh->getPointPos(endPoint).y() == Catch::Approx(2));
}

TEST_CASE("Concave face crossing the plane four times splits into two faces")
{
    // A U shape three wide and three tall with a notch from x 1 to 2 down to y equal to 1.
    auto uShape = std::make_shared<geo::Mesh>();
    uShape->addPoints(
        std::vector<Vector3>{
            {0, 0, 0},
            {3, 0, 0},
            {3, 3, 0},
            {2, 3, 0},
            {2, 1, 0},
            {1, 1, 0},
            {1, 3, 0},
            {0, 3, 0}
        }
    );
    uShape->addFace({0, 1, 2, 3, 4, 5, 6, 7});

    const utils::PlaneCut cut = utils::cutMeshByPlane(*uShape, {0, 2, 0}, {0, 1, 0});

    // Keeping everything above y equal to 2 leaves the two prongs of the U.
    REQUIRE(cut.mesh->getNumFaces() == 2);
    REQUIRE(cut.mesh->getFaceVertCount(0) == 4);
    REQUIRE(cut.mesh->getFaceVertCount(1) == 4);
    REQUIRE(cut.cutEdges.size() == 2);
    for (const auto& [startPoint, endPoint] : cut.cutEdges)
    {
        const Vector3 start = cut.mesh->getPointPos(startPoint);
        const Vector3 end = cut.mesh->getPointPos(endPoint);
        REQUIRE(start.y() == Catch::Approx(2));
        REQUIRE(end.y() == Catch::Approx(2));
        REQUIRE(std::abs(end.x() - start.x()) == Catch::Approx(1));
    }
}

TEST_CASE("Concave face whose kept part stays connected remains one face")
{
    auto uShape = std::make_shared<geo::Mesh>();
    uShape->addPoints(
        std::vector<Vector3>{
            {0, 0, 0},
            {3, 0, 0},
            {3, 3, 0},
            {2, 3, 0},
            {2, 1, 0},
            {1, 1, 0},
            {1, 3, 0},
            {0, 3, 0}
        }
    );
    uShape->addFace({0, 1, 2, 3, 4, 5, 6, 7});

    const utils::PlaneCut cut = utils::cutMeshByPlane(*uShape, {0, 2, 0}, {0, -1, 0});

    // Keeping everything below y equal to 2 leaves the base of the U with two short prongs.
    REQUIRE(cut.mesh->getNumFaces() == 1);
    REQUIRE(cut.mesh->getFaceVertCount(0) == 8);
    REQUIRE(cut.cutEdges.size() == 2);
}

TEST_CASE("Face group carries onto the cut face")
{
    auto quad = buildSingleQuad();
    quad->addFaceGroup("front");
    quad->addToFaceGroup("front", {0});

    const utils::PlaneCut cut = utils::cutMeshByPlane(*quad, {1, 0, 0}, {-1, 0, 0});

    auto cutGroup = cut.mesh->getGroupByName(attr::AttributeOwner::FACE, "front");
    REQUIRE(cutGroup != nullptr);
    REQUIRE(cut.mesh->getNumFaces() == 1);
    REQUIRE(attr::AttributeHandleRO<boolT>(cutGroup).getValue(0) == true);
}

TEST_CASE("Cutting a closed cube in half caps the opening")
{
    auto cube = utils::buildCube({2, 2, 2}, {0, 0, 0});
    utils::PlaneCut cut = utils::cutMeshByPlane(*cube, {0, 0, 0}, {-1, 0, 0});

    const std::vector<Offset> caps = utils::fillCutCaps(*cut.mesh, cut.cutEdges);

    REQUIRE(caps.size() == 1);
    REQUIRE(cut.mesh->getFaceVertCount(caps[0]) == 4);
    REQUIRE(isClosedSurface(*cut.mesh));
}

TEST_CASE("Cap faces away from the kept side")
{
    auto cube = utils::buildCube({2, 2, 2}, {0, 0, 0});
    utils::PlaneCut cut = utils::cutMeshByPlane(*cube, {0, 0, 0}, {-1, 0, 0});

    const std::vector<Offset> caps = utils::fillCutCaps(*cut.mesh, cut.cutEdges);

    REQUIRE(caps.size() == 1);
    REQUIRE(cut.mesh->getFaceNormal()[caps[0]].x() == Catch::Approx(1));
}

TEST_CASE("A cut along existing edges still caps the opening")
{
    // A closed box from x -1 to 1 whose long sides are split into two quads at x equal to 0.
    auto getPoint = [](Offset xIndex, Offset yIndex, Offset zIndex) {
        return xIndex * 4 + yIndex * 2 + zIndex;
    };
    std::vector<Vector3> positions;
    for (Offset xIndex = 0; xIndex < 3; ++xIndex)
        for (Offset yIndex = 0; yIndex < 2; ++yIndex)
            for (Offset zIndex = 0; zIndex < 2; ++zIndex)
                positions.push_back({xIndex - 1.0f, float(yIndex), float(zIndex)});

    const std::array<Offset, 4> endQuads[] = {
        {getPoint(0, 0, 0), getPoint(0, 0, 1), getPoint(0, 1, 1), getPoint(0, 1, 0)},
        {getPoint(2, 0, 0), getPoint(2, 1, 0), getPoint(2, 1, 1), getPoint(2, 0, 1)},
    };
    std::vector<Offset> quadPoints;
    for (const std::array<Offset, 4>& quad : endQuads)
        quadPoints.insert(quadPoints.end(), quad.begin(), quad.end());

    for (Offset xIndex = 0; xIndex < 2; ++xIndex)
    {
        const Offset startBottomFront = getPoint(xIndex, 0, 0);
        const Offset startBottomBack = getPoint(xIndex, 0, 1);
        const Offset startTopFront = getPoint(xIndex, 1, 0);
        const Offset startTopBack = getPoint(xIndex, 1, 1);
        const Offset endBottomFront = getPoint(xIndex + 1, 0, 0);
        const Offset endBottomBack = getPoint(xIndex + 1, 0, 1);
        const Offset endTopFront = getPoint(xIndex + 1, 1, 0);
        const Offset endTopBack = getPoint(xIndex + 1, 1, 1);

        const std::array<Offset, 4> sideQuads[] = {
            {startBottomFront, endBottomFront, endBottomBack, startBottomBack},
            {startTopFront, startTopBack, endTopBack, endTopFront},
            {startBottomFront, startTopFront, endTopFront, endBottomFront},
            {startBottomBack, endBottomBack, endTopBack, startTopBack},
        };
        for (const std::array<Offset, 4>& quad : sideQuads)
            quadPoints.insert(quadPoints.end(), quad.begin(), quad.end());
    }
    auto box = buildQuadMesh(positions, quadPoints);
    REQUIRE(isClosedSurface(*box));

    utils::PlaneCut cut = utils::cutMeshByPlane(*box, {0, 0, 0}, {-1, 0, 0});
    const std::vector<Offset> caps = utils::fillCutCaps(*cut.mesh, cut.cutEdges);

    REQUIRE(caps.size() == 1);
    REQUIRE(isClosedSurface(*cut.mesh));
}

TEST_CASE("Cutting an open surface adds no cap")
{
    auto quads = buildTwoQuads();
    utils::PlaneCut cut = utils::cutMeshByPlane(*quads, {0, 0.5, 0}, {0, -1, 0});

    const std::vector<Offset> caps = utils::fillCutCaps(*cut.mesh, cut.cutEdges);

    REQUIRE(caps.empty());
    REQUIRE(cut.mesh->getNumFaces() == 2);
}

TEST_CASE("Each closed loop of cut edges gets its own cap")
{
    auto cubes = utils::buildCube({2, 2, 2}, {0, 0, 0});
    cubes->merge(*utils::buildCube({2, 2, 2}, {0, 0, 5}));
    utils::PlaneCut cut = utils::cutMeshByPlane(*cubes, {0, 0, 0}, {-1, 0, 0});

    const std::vector<Offset> caps = utils::fillCutCaps(*cut.mesh, cut.cutEdges);

    REQUIRE(caps.size() == 2);
    REQUIRE(isClosedSurface(*cut.mesh));
}

TEST_CASE("Face attribute carries onto the cut face")
{
    auto quad = buildSingleQuad();
    auto tag = quad->addAttribute<intT>(attr::AttributeOwner::FACE, "tag");
    tag.setValue(0, 7);

    const utils::PlaneCut cut = utils::cutMeshByPlane(*quad, {1, 0, 0}, {-1, 0, 0});

    auto cutTagAttribute = cut.mesh->getAttribByName(attr::AttributeOwner::FACE, "tag");
    REQUIRE(cutTagAttribute != nullptr);
    REQUIRE(cut.mesh->getNumFaces() == 1);
    REQUIRE(attr::AttributeHandleRO<intT>(cutTagAttribute).getValue(0) == 7);
}

TEST_CASE("Cutting through a hole leaves the hole uncapped", "[!shouldfail]")
{
    auto ring = buildSquareRing();
    REQUIRE(isClosedSurface(*ring));

    utils::PlaneCut cut = utils::cutMeshByPlane(*ring, {0, 0, 0.5}, {0, 0, -1});
    const std::vector<Offset> caps = utils::fillCutCaps(*cut.mesh, cut.cutEdges);

    // The middle of the hole stays open and the solid part of the ring is covered exactly once.
    REQUIRE(isClosedSurface(*cut.mesh));
    REQUIRE(countFacesCoveringAlongZ(*cut.mesh, caps, 0, 0) == 0);
    REQUIRE(countFacesCoveringAlongZ(*cut.mesh, caps, 1, 0) == 1);
}

TEST_CASE("A flat surface lying in the plane gets no cap")
{
    auto quads = buildTwoQuads();

    utils::PlaneCut cut = utils::cutMeshByPlane(*quads, {0, 0, 0}, {0, 0, 1});
    const std::vector<Offset> caps = utils::fillCutCaps(*cut.mesh, cut.cutEdges);

    REQUIRE(caps.empty());
    REQUIRE(cut.mesh->getNumFaces() == 2);
}

TEST_CASE("A flat surface lying in the plane gets no cap while another part is cut")
{
    auto mesh = buildTwoQuads();
    mesh->merge(*utils::buildCube({2, 2, 2}, {10, 0, 0}));

    utils::PlaneCut cut = utils::cutMeshByPlane(*mesh, {0, 0, 0}, {0, 0, 1});
    const std::vector<Offset> caps = utils::fillCutCaps(*cut.mesh, cut.cutEdges);

    // Only the cube is opened by the cut.
    REQUIRE(caps.size() == 1);
}

TEST_CASE("Keeping only a face that lies on the plane leaves nothing", "[!shouldfail]")
{
    auto cube = utils::buildCube({2, 2, 2}, {0, 0, 0});

    // Everything past the cube's right face is kept, which holds no volume at all.
    utils::PlaneCut cut = utils::cutMeshByPlane(*cube, {1, 0, 0}, {1, 0, 0});
    utils::fillCutCaps(*cut.mesh, cut.cutEdges);

    REQUIRE(cut.mesh->getNumFaces() == 0);
}

TEST_CASE("Concave face touching the plane at its inner corners splits into two faces")
{
    // A U shape whose notch bottoms out exactly on the plane at y equal to 1.
    auto uShape = std::make_shared<geo::Mesh>();
    uShape->addPoints(
        std::vector<Vector3>{
            {0, 0, 0},
            {3, 0, 0},
            {3, 3, 0},
            {2, 3, 0},
            {2, 1, 0},
            {1, 1, 0},
            {1, 3, 0},
            {0, 3, 0}
        }
    );
    uShape->addFace({0, 1, 2, 3, 4, 5, 6, 7});

    const utils::PlaneCut cut = utils::cutMeshByPlane(*uShape, {0, 1, 0}, {0, 1, 0});

    REQUIRE(cut.mesh->getNumFaces() == 2);
}

TEST_CASE("Two boxes sharing an edge each get their own cap")
{
    geo::Mesh boxes;
    std::map<std::array<float, 3>, Offset> pointsByPosition;
    addBox(boxes, pointsByPosition, {0, 0, 0}, {1, 1, 1});
    addBox(boxes, pointsByPosition, {1, 1, 0}, {2, 2, 1});

    utils::PlaneCut cut = utils::cutMeshByPlane(boxes, {0, 0, 0.5}, {0, 0, -1});
    const std::vector<Offset> caps = utils::fillCutCaps(*cut.mesh, cut.cutEdges);

    REQUIRE(caps.size() == 2);
    REQUIRE(countFacesCoveringAlongZ(*cut.mesh, caps, 0.5, 0.5) == 1);
    REQUIRE(countFacesCoveringAlongZ(*cut.mesh, caps, 1.5, 1.5) == 1);
}

TEST_CASE("A plane through opposite edges of a cube caps the diagonal")
{
    auto cube = utils::buildCube({2, 2, 2}, {0, 0, 0});

    utils::PlaneCut cut = utils::cutMeshByPlane(*cube, {0, 0, 0}, Vector3(-1, 0, 1).normalized());
    const std::vector<Offset> caps = utils::fillCutCaps(*cut.mesh, cut.cutEdges);

    REQUIRE(caps.size() == 1);
    REQUIRE(cut.mesh->getFaceVertCount(caps[0]) == 4);
    REQUIRE(isClosedSurface(*cut.mesh));
}

TEST_CASE("Pieces cut far from the origin around uneven seed points stay closed")
{
    const Vector3 center(5000, 5000, 5000);
    auto cube = utils::buildCube({2000, 2000, 2000}, center);
    const std::vector<Vector3> seedPositions = {
        center + Vector3(-310.7f, 12.3f, -290.3f),
        center + Vector3(270.1f, -40.2f, -330.9f),
        center + Vector3(-250.2f, 33.3f, 300.4f),
        center + Vector3(320.8f, -5.5f, 280.6f),
        center + Vector3(10.3f, 20.7f, -20.1f),
    };

    for (size_t seedIndex = 0; seedIndex < seedPositions.size(); ++seedIndex)
    {
        CAPTURE(seedIndex);
        const auto piece = cutAndCapPiece(*cube, seedPositions, seedIndex);
        REQUIRE(piece->getNumFaces() > 0);
        REQUIRE(isClosedSurface(*piece));
    }
}
