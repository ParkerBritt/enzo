#include <Engine/Operator/Mesh.h>
#include <Engine/Types.h>
#include <catch2/catch_test_macros.hpp>

using namespace enzo;

TEST_CASE("deleteFaces") {
    geo::Mesh mesh;
    auto p0 = mesh.addPoint(bt::Vector3(0, 0, 0));
    auto p1 = mesh.addPoint(bt::Vector3(1, 0, 0));
    auto p2 = mesh.addPoint(bt::Vector3(0, 1, 0));
    mesh.addFace({p0, p1, p2});
    mesh.addFace({p0, p1, p2});
    mesh.addFace({p0, p1, p2});

    mesh.deleteFaces({1});

    REQUIRE(mesh.isValidFace(0));
    REQUIRE_FALSE(mesh.isValidFace(1));
    REQUIRE(mesh.isValidFace(2));

    REQUIRE(mesh.isValidVertex(2));
    REQUIRE_FALSE(mesh.isValidVertex(3));
    REQUIRE_FALSE(mesh.isValidVertex(4));
    REQUIRE_FALSE(mesh.isValidVertex(5));
    REQUIRE(mesh.isValidVertex(6));
}

TEST_CASE("deletePoints") {
    geo::Mesh mesh;

    // Create points
    auto p0 = mesh.addPoint(bt::Vector3(0, 0, 0));
    auto p1 = mesh.addPoint(bt::Vector3(1, 0, 0));
    auto p2 = mesh.addPoint(bt::Vector3(0, 1, 0));
    auto p3 = mesh.addPoint(bt::Vector3(1, 1, 0));

    // Create faces
    mesh.addFace({p0, p1, p2});
    mesh.addFace({p1, p2, p3});

    // Delete point 0
    mesh.deletePoints({p0});

    // Test point 0 is invalid, others still valid
    REQUIRE_FALSE(mesh.isValidPoint(0));
    REQUIRE(mesh.isValidPoint(1));
    REQUIRE(mesh.isValidPoint(2));
    REQUIRE(mesh.isValidPoint(3));

    // Test faces are still valid (point was just detached from face 0)
    REQUIRE(mesh.isValidFace(0));
    REQUIRE(mesh.isValidFace(1));

    // Test only the vertex that referenced p0 went invalid
    REQUIRE_FALSE(mesh.isValidVertex(0));
    REQUIRE(mesh.isValidVertex(1));
    REQUIRE(mesh.isValidVertex(2));
    REQUIRE(mesh.isValidVertex(3));
    REQUIRE(mesh.isValidVertex(4));
    REQUIRE(mesh.isValidVertex(5));
}

TEST_CASE("deletePointsAndFaces") {
    geo::Mesh mesh;

    // Create points
    auto p0 = mesh.addPoint(bt::Vector3(0, 0, 0));
    auto p1 = mesh.addPoint(bt::Vector3(1, 0, 0));
    auto p2 = mesh.addPoint(bt::Vector3(0, 1, 0));
    auto p3 = mesh.addPoint(bt::Vector3(1, 1, 0));

    // Create faces
    mesh.addFace({p0, p1, p2});
    mesh.addFace({p1, p2, p3});

    // Delete point 0 and any face that uses it
    mesh.deletePoints({p0}, true);

    // Test point 0 is invalid, others still valid
    REQUIRE_FALSE(mesh.isValidPoint(0));
    REQUIRE(mesh.isValidPoint(1));
    REQUIRE(mesh.isValidPoint(2));
    REQUIRE(mesh.isValidPoint(3));

    // Test face 0 went away with the point, face 1 is untouched
    REQUIRE_FALSE(mesh.isValidFace(0));
    REQUIRE(mesh.isValidFace(1));

    // Test all of face 0's vertices went invalid, face 1's didn't
    REQUIRE_FALSE(mesh.isValidVertex(0));
    REQUIRE_FALSE(mesh.isValidVertex(1));
    REQUIRE_FALSE(mesh.isValidVertex(2));
    REQUIRE(mesh.isValidVertex(3));
    REQUIRE(mesh.isValidVertex(4));
    REQUIRE(mesh.isValidVertex(5));
}
