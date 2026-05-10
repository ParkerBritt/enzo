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

TEST_CASE("Selection empty string selects no prims") {
    NodePacket packet;
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/foo");
    packet.addPrimitive(prim);

    Selection selection("");
    REQUIRE(selection.getPrims(packet).empty());
    REQUIRE_FALSE(selection.containsPrim(prim));
}

TEST_CASE("Selection without path getPrims returns all prims") {
    NodePacket packet;
    geo::PrimPtr a = std::make_shared<geo::Mesh>("/a");
    geo::PrimPtr b = std::make_shared<geo::Mesh>("/b");
    packet.addPrimitive(a);
    packet.addPrimitive(b);

    Selection selection("p{0}");
    auto prims = selection.getPrims(packet);
    REQUIRE(prims.size() == 2);
}

TEST_CASE("Selection without path containsPoint matches on any prim") {
    geo::PrimPtr a = std::make_shared<geo::Mesh>("/a");
    geo::PrimPtr b = std::make_shared<geo::Mesh>("/b");

    Selection selection("p{0}");
    REQUIRE(selection.containsPoint(a, 0));
    REQUIRE(selection.containsPoint(b, 0));
    REQUIRE_FALSE(selection.containsPoint(a, 1));
}

TEST_CASE("Selection without path containsPrim is partial") {
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/foo");

    Selection selection("p{0}");
    REQUIRE(selection.containsPrim(prim));
    REQUIRE_FALSE(selection.containsPrim(prim, true));
}

TEST_CASE("setInverted toggles the flag") {
    Selection selection("/a");
    REQUIRE_FALSE(selection.getInverted());
    selection.setInverted(true);
    REQUIRE(selection.getInverted());
}

TEST_CASE("containsPrim inverted") {
    geo::PrimPtr selectedPrim = std::make_shared<geo::Mesh>("/selected");
    geo::PrimPtr unselectedPrim = std::make_shared<geo::Mesh>("/unselected");

    // Whole-prim selection: inverted hides the named prim and exposes others
    Selection whole("/selected");
    whole.setInverted(true);
    REQUIRE_FALSE(whole.containsPrim(selectedPrim));
    REQUIRE(whole.containsPrim(unselectedPrim));
    REQUIRE(whole.containsPrim(unselectedPrim, true));

    // Partial selection: inverted still touches the named prim, but not as a whole
    Selection partial("/selected f{0}");
    partial.setInverted(true);
    REQUIRE(partial.containsPrim(selectedPrim));
    REQUIRE_FALSE(partial.containsPrim(selectedPrim, true));
    REQUIRE(partial.containsPrim(unselectedPrim, true));
}

TEST_CASE("containsFace inverted") {
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/selected");
    Selection selection("/selected f{10}");
    selection.setInverted(true);
    REQUIRE_FALSE(selection.containsFace(prim, 10));
    REQUIRE(selection.containsFace(prim, 11));
}

TEST_CASE("getPrims inverted includes unmentioned prims") {
    NodePacket packet;
    geo::PrimPtr selectedPrim = std::make_shared<geo::Mesh>("/selected");
    geo::PrimPtr unselectedPrim = std::make_shared<geo::Mesh>("/unselected");
    packet.addPrimitive(selectedPrim);
    packet.addPrimitive(unselectedPrim);

    Selection selection("/selected");
    selection.setInverted(true);
    auto prims = selection.getPrims(packet);
    REQUIRE(prims.size() == 1);
    REQUIRE(prims[0]->getPath() == "/unselected");
}

TEST_CASE("getFaces inverted returns complement") {
    auto mesh = std::make_shared<geo::Mesh>("/selected");
    ga::Offset p0 = mesh->addPoint(bt::Vector3(0, 0, 0));
    ga::Offset p1 = mesh->addPoint(bt::Vector3(1, 0, 0));
    ga::Offset p2 = mesh->addPoint(bt::Vector3(0, 1, 0));
    for (int i = 0; i < 5; ++i) {
        mesh->addFace({p0, p1, p2});
    }
    Selection selection("/selected f{2}");
    selection.setInverted(true);
    auto faces = selection.getFaces(mesh);
    REQUIRE(faces == std::vector<ga::Offset>{0, 1, 3, 4});
}
