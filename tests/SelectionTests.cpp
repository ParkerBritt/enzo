#include "Engine/Operator/NodePacket.h"
#include "Engine/Operator/Primitive.h"
#include <catch2/catch_test_macros.hpp>
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
    geo::PrimPtr unselectedPrim = std::make_shared<geo::Mesh>("/unselected");
    packet.addPrimitive(unselectedPrim);
    packet.addPrimitive(selectedPrim);

    Selection selection("/selected");
    REQUIRE(selection.containsPrim(selectedPrim));
    REQUIRE_FALSE(selection.containsPrim(unselectedPrim));
}
