#include "Engine/Operator/NodePacket.h"
#include "Engine/Operator/Primitive.h"
#include <catch2/catch_test_macros.hpp>
#include <Engine/Operator/IndexSet.h>
#include <Engine/Operator/Selection.h>
#include <Engine/Operator/Mesh.h>
#include <memory>

using namespace enzo;
TEST_CASE("getPrims")
{
    // Setup packet
    NodePacket packet;
    geo::PrimPtr selectedPrim = std::make_shared<geo::Mesh>("/selected");
    geo::PrimPtr unselectedPrim = std::make_shared<geo::Mesh>("/unselected");
    packet.addPrimitive(unselectedPrim);
    packet.addPrimitive(selectedPrim);

    Selection selection("/selected");
    auto primitives = selection.getPrims(packet);
    REQUIRE(primitives.size() == 1);
    REQUIRE(primitives[0]->getPath() == "/selected");
}

TEST_CASE("containsPrim")
{
    // Setup packet
    NodePacket packet;
    geo::PrimPtr selectedPrim = std::make_shared<geo::Mesh>("/selected");
    geo::PrimPtr partiallySelectedPrim = std::make_shared<geo::Mesh>("/selected");
    geo::PrimPtr unselectedPrim = std::make_shared<geo::Mesh>("/unselected");
    packet.addPrimitive(unselectedPrim);
    packet.addPrimitive(selectedPrim);

    Selection selection("/selected f{10}");
    REQUIRE(selection.containsPrim(selectedPrim));
    REQUIRE(selection.containsPrim(partiallySelectedPrim));
    REQUIRE_FALSE(selection.containsPrim(unselectedPrim));
}

TEST_CASE("Selection containsFace for explicit face indices") {
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/selected");
    Selection selection("/selected f{10}");
    REQUIRE(selection.containsFace(prim, 10));
    REQUIRE_FALSE(selection.containsFace(prim, 11));
}

TEST_CASE("Selection getFaces explicit indices") {
    auto mesh = std::make_shared<geo::Mesh>("/selected");
    ga::Offset p0 = mesh->addPoint(bt::Vector3(0, 0, 0));
    ga::Offset p1 = mesh->addPoint(bt::Vector3(1, 0, 0));
    ga::Offset p2 = mesh->addPoint(bt::Vector3(0, 1, 0));
    for (int i = 0; i < 11; ++i) {
        mesh->addFace({p0, p1, p2});
    }
    Selection selection("/selected f{10}");
    auto faces = selection.getFaces(mesh);
    REQUIRE(faces == std::vector<ga::Index>{10});
}

TEST_CASE("Selection containsFace wildcard") {
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/selected");
    Selection selection("/selected f{*}");
    REQUIRE(selection.containsFace(prim, 0));
    REQUIRE(selection.containsFace(prim, 999));
}

TEST_CASE("Selection wildcard does not apply to unselected prims") {
    geo::PrimPtr other = std::make_shared<geo::Mesh>("/other");
    Selection selection("/selected f{*}");
    REQUIRE_FALSE(selection.containsFace(other, 0));
}

TEST_CASE("Selection getFaces wildcard") {
    auto mesh = std::make_shared<geo::Mesh>("/selected");
    ga::Offset p0 = mesh->addPoint(bt::Vector3(0, 0, 0));
    ga::Offset p1 = mesh->addPoint(bt::Vector3(1, 0, 0));
    ga::Offset p2 = mesh->addPoint(bt::Vector3(0, 1, 0));
    for (int i = 0; i < 5; ++i) {
        mesh->addFace({p0, p1, p2});
    }
    Selection selection("/selected f{*}");
    auto faces = selection.getFaces(mesh);
    REQUIRE(faces == std::vector<ga::Index>{0, 1, 2, 3, 4});
}
