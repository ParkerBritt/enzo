#include "Engine/Network/NodePacket.h"
#include "Engine/Primitives/Mesh.h"
#include "Engine/Primitives/PointLight.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>

using namespace enzo;

TEST_CASE("A packet numbers each primitive added at a taken path")
{
    NodePacket packet;
    for (int lightIndex = 0; lightIndex < 3; ++lightIndex)
        packet.addPrimitive(std::make_shared<geo::PointLight>("/pointLight"));

    REQUIRE(packet.getPrimitive(0)->getPath() == "/pointLight");
    REQUIRE(packet.getPrimitive(1)->getPath() == "/pointLight_1");
    REQUIRE(packet.getPrimitive(2)->getPath() == "/pointLight_2");
}

TEST_CASE("A packet numbers past paths that are already numbered")
{
    NodePacket packet;
    packet.addPrimitive(std::make_shared<geo::PointLight>("/pointLight_1"));
    packet.addPrimitive(std::make_shared<geo::PointLight>("/pointLight"));
    packet.addPrimitive(std::make_shared<geo::PointLight>("/pointLight"));

    REQUIRE(packet.getPrimitive(2)->getPath() == "/pointLight_2");
}

TEST_CASE("A packet merges a mesh into the mesh at its path")
{
    NodePacket packet;
    packet.attemptMerge(std::make_shared<geo::Mesh>("/mesh"));
    packet.attemptMerge(std::make_shared<geo::Mesh>("/mesh"));

    REQUIRE(packet.size() == 1);
}

TEST_CASE("A packet numbers a primitive that cannot merge into the one at its path")
{
    NodePacket packet;
    packet.attemptMerge(std::make_shared<geo::PointLight>("/pointLight"));
    packet.attemptMerge(std::make_shared<geo::PointLight>("/pointLight"));
    packet.attemptMerge(std::make_shared<geo::PointLight>("/pointLight"));

    REQUIRE(packet.getPrimitive(2)->getPath() == "/pointLight_2");
}

TEST_CASE("A packet numbers a primitive moved to a taken path")
{
    NodePacket packet;
    packet.addPrimitive(std::make_shared<geo::PointLight>("/first"));
    packet.addPrimitive(std::make_shared<geo::PointLight>("/second"));
    packet.movePrim("/second", "/first");

    REQUIRE(packet.getPrimitive(1)->getPath() == "/first_1");
}

TEST_CASE("A packet ignores moving a primitive it does not hold")
{
    NodePacket packet;
    packet.addPrimitive(std::make_shared<geo::PointLight>("/pointLight"));
    const auto outsider = std::make_shared<geo::PointLight>("/pointLight");
    packet.movePrim(outsider, "/moved");
    packet.addPrimitive(std::make_shared<geo::PointLight>("/pointLight"));

    REQUIRE(packet.getPrimitive(1)->getPath() == "/pointLight_1");
}

TEST_CASE("A packet frees the path of a removed primitive")
{
    NodePacket packet;
    packet.addPrimitive(std::make_shared<geo::PointLight>("/pointLight"));
    packet.removePrim("/pointLight");
    packet.addPrimitive(std::make_shared<geo::PointLight>("/pointLight"));

    REQUIRE(packet.getPrimitive(0)->getPath() == "/pointLight");
}
