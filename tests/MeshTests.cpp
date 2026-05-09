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
