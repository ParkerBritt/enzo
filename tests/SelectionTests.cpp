#include "Engine/Operator/NodePacket.h"
#include "Engine/Operator/Primitive.h"
#include <catch2/catch_test_macros.hpp>
#include <Engine/Operator/Selection.h>
#include <Engine/Operator/Mesh.h>
#include <memory>

TEST_CASE("goobus")
{
    using namespace enzo;

    // Setup packet
    NodePacket packet;
    geo::PrimPtr selectedPrim = std::make_shared<geo::Mesh>("/selected");
    geo::PrimPtr unselectedPrim = std::make_shared<geo::Mesh>("/unselected");
    packet.addPrimitive(unselectedPrim);
    packet.addPrimitive(selectedPrim);

    Selection selection("/selected");
    auto primitives = selection.getPrimitives(packet);
    REQUIRE(primitives.size() == 1);
    REQUIRE(primitives[0]->getPath() == "/selected");
}
