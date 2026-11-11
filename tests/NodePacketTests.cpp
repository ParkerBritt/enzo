#include "Engine/Network/NodePacket.h"
#include "Engine/Primitives/Mesh.h"
#include "Engine/Primitives/PointLight.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <utility>

using namespace enzo;

namespace {

// Builds a packet holding one mesh at /mesh with three points, each with a density of 0.
NodePacket buildDensityPacket()
{
    auto mesh = std::make_shared<geo::Mesh>("/mesh");
    mesh->addPoints(std::vector<Vector3>{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}});
    mesh->addAttribute<floatT>(attr::AttributeOwner::POINT, "Density");
    NodePacket packet;
    packet.addPrimitive(mesh);
    return packet;
}

std::shared_ptr<const attr::Attribute> getDensity(const geo::Primitive& primitive)
{
    return primitive.getAttribByName(attr::AttributeOwner::POINT, "Density");
}

} // namespace

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

TEST_CASE("A copied packet holds its own primitives")
{
    const NodePacket original = buildDensityPacket();
    const NodePacket copy = original;

    REQUIRE(copy.getPrimitive(0) != original.getPrimitive(0));
}

TEST_CASE("A copied packet shares the attributes of the original")
{
    const NodePacket original = buildDensityPacket();
    const NodePacket copy = original;

    REQUIRE(getDensity(*copy.getPrimitive(0)) == getDensity(*original.getPrimitive(0)));
}

TEST_CASE("Writing to a copied packet leaves the original's values untouched")
{
    const NodePacket original = buildDensityPacket();
    NodePacket copy = original;

    copy.getPrimitive(0)
        ->getWriteHandle<floatT>(attr::AttributeOwner::POINT, "Density")
        ->setValue(0, 5);

    const attr::AttributeHandleRO<floatT> originalDensity(getDensity(*original.getPrimitive(0)));
    REQUIRE(originalDensity.getValue(0) == 0);
}

TEST_CASE("Adding an attribute to a copied packet leaves the original without it")
{
    const NodePacket original = buildDensityPacket();
    NodePacket copy = original;

    copy.getPrimitive(0)->addAttribute<floatT>(attr::AttributeOwner::POINT, "Mass");

    REQUIRE(
        original.getPrimitive(0)->getAttribByName(attr::AttributeOwner::POINT, "Mass") == nullptr
    );
}

TEST_CASE("Moving a primitive in a copied packet leaves the original's path untouched")
{
    const NodePacket original = buildDensityPacket();
    NodePacket copy = original;

    copy.movePrim("/mesh", "/moved");

    REQUIRE(original.getPrimitive(0)->getPath() == "/mesh");
}
