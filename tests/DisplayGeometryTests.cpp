#include "Engine/Core/Types.h"
#include "Engine/Network/NodePacket.h"
#include "Engine/Primitives/Mesh.h"
#include "Graphics/DisplayGeometry.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <vector>

using namespace enzo;
using namespace enzo::gfx;

namespace {

// Returns a mesh with one closed square face on the ground plane.
std::shared_ptr<geo::Mesh> buildSquareMesh(std::string_view path)
{
    auto mesh = std::make_shared<geo::Mesh>(path);
    const std::vector<Vector3> positions = {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}};
    const std::vector<Offset> pointOffsets = mesh->addPoints(positions);
    mesh->addFace(pointOffsets);
    return mesh;
}

} // namespace

TEST_CASE("Display geometry splits a closed face into triangles with an outline")
{
    NodePacket packet;
    packet.addPrimitive(buildSquareMesh("/square"));

    const std::shared_ptr<const DisplayGeometry> geometry = buildDisplayGeometry(packet);

    REQUIRE(geometry->positions.size() == 4);
    REQUIRE(geometry->normals.size() == 4);
    REQUIRE(geometry->topology.triangleIndices.size() == 6);
    REQUIRE(geometry->topology.edgeIndices.size() == 8);
    REQUIRE(geometry->topology.lineIndices.empty());
}

TEST_CASE("Display geometry draws an open face as a line")
{
    auto mesh = std::make_shared<geo::Mesh>("/line");
    const std::vector<Vector3> positions = {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}};
    mesh->addFace(mesh->addPoints(positions), false);
    NodePacket packet;
    packet.addPrimitive(mesh);

    const std::shared_ptr<const DisplayGeometry> geometry = buildDisplayGeometry(packet);

    REQUIRE(geometry->topology.lineIndices == std::vector<std::uint32_t>{0, 1, 1, 2});
    REQUIRE(geometry->topology.triangleIndices.empty());
    REQUIRE(geometry->topology.edgeIndices.empty());
}

TEST_CASE("Display geometry numbers each mesh on from the vertices before it")
{
    NodePacket packet;
    packet.addPrimitive(buildSquareMesh("/first"));
    packet.addPrimitive(buildSquareMesh("/second"));

    const std::shared_ptr<const DisplayGeometry> geometry = buildDisplayGeometry(packet);

    REQUIRE(geometry->positions.size() == 8);
    const std::vector<std::uint32_t>& triangleIndices = geometry->topology.triangleIndices;
    REQUIRE(triangleIndices.size() == 12);
    for (std::size_t corner = 0; corner < 6; ++corner)
    {
        REQUIRE(triangleIndices[corner] < 4);
        REQUIRE(triangleIndices[corner + 6] >= 4);
    }
}

TEST_CASE("Display geometry reads each vertex position from its point")
{
    NodePacket packet;
    packet.addPrimitive(buildSquareMesh("/square"));

    const std::shared_ptr<const DisplayGeometry> geometry = buildDisplayGeometry(packet);

    REQUIRE(geometry->positions[2] == glm::vec3(1.f, 0.f, 1.f));
    REQUIRE(geometry->normals[0].y != 0.f);
}

TEST_CASE("Display geometry leaves out deleted faces")
{
    std::shared_ptr<geo::Mesh> mesh = buildSquareMesh("/squares");
    const std::vector<Vector3> positions = {{2, 0, 0}, {3, 0, 0}, {3, 0, 1}, {2, 0, 1}};
    mesh->addFace(mesh->addPoints(positions));
    mesh->deleteFaces({0}, false);
    NodePacket packet;
    packet.addPrimitive(mesh);

    const std::shared_ptr<const DisplayGeometry> geometry = buildDisplayGeometry(packet);

    REQUIRE(geometry->topology.triangleIndices.size() == 6);
    REQUIRE(geometry->topology.edgeIndices.size() == 8);
}
