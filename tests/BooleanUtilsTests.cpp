#include <Engine/Operator/AttributeHandle.h>
#include <Engine/Operator/Mesh.h>
#include <Engine/Types.h>
#include <Engine/Utils/BooleanUtils.h>
#include <Engine/Utils/MeshShapes.h>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <memory>

using namespace enzo;

namespace {

// Count valid faces with the given vertex count.
size_t countFacesWithVertexCount(const geo::Mesh& mesh, unsigned int expected)
{
    size_t hits = 0;
    const Offset total = mesh.getNumFaces();
    for (Offset faceOffset = 0; faceOffset < total; ++faceOffset)
    {
        if (!mesh.isValidFace(faceOffset)) continue;
        if (mesh.getFaceVertCount(faceOffset) == expected) ++hits;
    }
    return hits;
}

} // namespace


TEST_CASE("Union of two disjoint cubes preserves both as quads")
{
    // Build two cubes that do not touch.
    auto cubeA = utils::buildCube(Vector3(1, 1, 1), Vector3(0, 0, 0));
    auto cubeB = utils::buildCube(Vector3(1, 1, 1), Vector3(3, 0, 0));

    // Run union.
    auto result = utils::booleanMesh(*cubeA, *cubeB, utils::BooleanOp::UNION);
    REQUIRE(result != nullptr);

    // Disjoint inputs leave every quad intact, so the result is twelve quads with no triangles.
    REQUIRE(countFacesWithVertexCount(*result, 4) == 12);
    REQUIRE(countFacesWithVertexCount(*result, 3) == 0);
}


TEST_CASE("Union of overlapping cubes keeps un-cut quads as quads")
{
    // Offset along every axis so no pair of input face planes is coplanar.
    auto cubeA = utils::buildCube(Vector3(2, 2, 2), Vector3(0, 0, 0));
    auto cubeB = utils::buildCube(Vector3(2, 2, 2), Vector3(1.3, 0.4, 0.7));

    auto result = utils::booleanMesh(*cubeA, *cubeB, utils::BooleanOp::UNION);
    REQUIRE(result != nullptr);

    // Cube A's three negative-axis faces and cube B's three positive-axis faces lie entirely outside the other cube, so they should survive as quads.
    REQUIRE(countFacesWithVertexCount(*result, 4) >= 6);
}


TEST_CASE("Face attribute carries from source mesh to result face")
{
    // Build two disjoint cubes so every face in the result maps cleanly to one source face.
    auto cubeA = utils::buildCube(Vector3(1, 1, 1), Vector3(0, 0, 0));
    auto cubeB = utils::buildCube(Vector3(1, 1, 1), Vector3(3, 0, 0));

    // Tag every face of cube A with 11 and every face of cube B with 22.
    auto attrA = cubeA->addIntAttribute(attr::AttrOwner::FACE, "tag");
    for (Offset faceOffset = 0; faceOffset < cubeA->getNumFaces(); ++faceOffset)
        attrA.setValue(faceOffset, 11);
    auto attrB = cubeB->addIntAttribute(attr::AttrOwner::FACE, "tag");
    for (Offset faceOffset = 0; faceOffset < cubeB->getNumFaces(); ++faceOffset)
        attrB.setValue(faceOffset, 22);

    auto result = utils::booleanMesh(*cubeA, *cubeB, utils::BooleanOp::UNION);
    REQUIRE(result != nullptr);

    // Look up the tag attribute on the result.
    auto tagAttr = result->getAttribByName(attr::AttrOwner::FACE, "tag");
    REQUIRE(tagAttr != nullptr);
    attr::AttributeHandleInt tagHandle(std::const_pointer_cast<attr::Attribute>(tagAttr));

    // Tally how many result faces carry the A tag and how many carry the B tag.
    size_t fromA = 0;
    size_t fromB = 0;
    for (Offset faceOffset = 0; faceOffset < result->getNumFaces(); ++faceOffset)
    {
        if (!result->isValidFace(faceOffset)) continue;
        const auto value = tagHandle.getValue(faceOffset);
        if (value == 11) ++fromA;
        else if (value == 22) ++fromB;
    }
    REQUIRE(fromA == 6);
    REQUIRE(fromB == 6);
}


TEST_CASE("Point attribute survives on un-cut points")
{
    // Two disjoint cubes so every point survives.
    auto cubeA = utils::buildCube(Vector3(1, 1, 1), Vector3(0, 0, 0));
    auto cubeB = utils::buildCube(Vector3(1, 1, 1), Vector3(3, 0, 0));

    // Tag every point of A with 1 and every point of B with 2.
    auto attrA = cubeA->addIntAttribute(attr::AttrOwner::POINT, "src");
    for (Offset pointOffset = 0; pointOffset < cubeA->getNumPoints(); ++pointOffset)
        attrA.setValue(pointOffset, 1);
    auto attrB = cubeB->addIntAttribute(attr::AttrOwner::POINT, "src");
    for (Offset pointOffset = 0; pointOffset < cubeB->getNumPoints(); ++pointOffset)
        attrB.setValue(pointOffset, 2);

    auto result = utils::booleanMesh(*cubeA, *cubeB, utils::BooleanOp::UNION);
    REQUIRE(result != nullptr);

    auto srcAttr = result->getAttribByName(attr::AttrOwner::POINT, "src");
    REQUIRE(srcAttr != nullptr);
    attr::AttributeHandleInt srcHandle(std::const_pointer_cast<attr::Attribute>(srcAttr));

    // Tally how many points carry A's tag versus B's tag.
    size_t fromA = 0;
    size_t fromB = 0;
    for (Offset pointOffset = 0; pointOffset < result->getNumPoints(); ++pointOffset)
    {
        if (!result->isValidPoint(pointOffset)) continue;
        const auto value = srcHandle.getValue(pointOffset);
        if (value == 1) ++fromA;
        else if (value == 2) ++fromB;
    }
    REQUIRE(fromA == 8);
    REQUIRE(fromB == 8);
}


TEST_CASE("Cut point interpolates point attribute from edge endpoints")
{
    // Cube A spans x in [0, 2]. Cube B clips the right half away starting at x=1.
    auto cubeA = utils::buildCube(Vector3(2, 1, 1), Vector3(1, 0.5, 0.5));
    auto cubeB = utils::buildCube(Vector3(2, 3, 3), Vector3(2, 0.5, 0.5));

    // Tag each A point with ten times its X coordinate so a cut at x=1 should interpolate to 10.
    auto attrA = cubeA->addIntAttribute(attr::AttrOwner::POINT, "xCoord");
    for (Offset pointOffset = 0; pointOffset < cubeA->getNumPoints(); ++pointOffset)
    {
        const Vector3 position = cubeA->getPointPos(pointOffset);
        attrA.setValue(pointOffset, static_cast<intT>(std::lround(position.x() * 10)));
    }
    // Mark B points so they are easy to tell apart from interpolated values.
    auto attrB = cubeB->addIntAttribute(attr::AttrOwner::POINT, "xCoord");
    for (Offset pointOffset = 0; pointOffset < cubeB->getNumPoints(); ++pointOffset)
        attrB.setValue(pointOffset, -1);

    // Subtract B from A so the right half of A is cut away.
    auto result = utils::booleanMesh(*cubeA, *cubeB, utils::BooleanOp::SUBTRACT);
    REQUIRE(result != nullptr);

    auto attr = result->getAttribByName(attr::AttrOwner::POINT, "xCoord");
    REQUIRE(attr != nullptr);
    attr::AttributeHandleInt handle(std::const_pointer_cast<attr::Attribute>(attr));

    // Find points sitting on the x=1 cut plane and check their attribute interpolates to 10.
    size_t cutPointsAtTen = 0;
    for (Offset pointOffset = 0; pointOffset < result->getNumPoints(); ++pointOffset)
    {
        if (!result->isValidPoint(pointOffset)) continue;
        const Vector3 position = result->getPointPos(pointOffset);
        if (std::abs(position.x() - 1.0) > 1e-6) continue;
        REQUIRE(handle.getValue(pointOffset) == 10);
        ++cutPointsAtTen;
    }
    REQUIRE(cutPointsAtTen >= 1);
}
