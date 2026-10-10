#include "Engine/Core/Types.h"
#include "Engine/Network/NodePacket.h"
#include "Engine/Primitives/Camera.h"
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

    DisplayGeometry geometry;
    buildDisplayGeometry(geometry, packet);

    REQUIRE(geometry.positions.size() == 4);
    REQUIRE(geometry.normals.size() == 4);
    REQUIRE(geometry.topology.triangleIndices.size() == 6);
    REQUIRE(geometry.topology.edgeIndices.size() == 8);
    REQUIRE(geometry.topology.lineIndices.empty());
}

TEST_CASE("Display geometry draws an open face as a line")
{
    auto mesh = std::make_shared<geo::Mesh>("/line");
    const std::vector<Vector3> positions = {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}};
    mesh->addFace(mesh->addPoints(positions), false);
    NodePacket packet;
    packet.addPrimitive(mesh);

    DisplayGeometry geometry;
    buildDisplayGeometry(geometry, packet);

    REQUIRE(geometry.topology.lineIndices == std::vector<std::uint32_t>{0, 1, 1, 2});
    REQUIRE(geometry.topology.triangleIndices.empty());
    REQUIRE(geometry.topology.edgeIndices.empty());
}

TEST_CASE("Display geometry numbers each mesh on from the vertices before it")
{
    NodePacket packet;
    packet.addPrimitive(buildSquareMesh("/first"));
    packet.addPrimitive(buildSquareMesh("/second"));

    DisplayGeometry geometry;
    buildDisplayGeometry(geometry, packet);

    REQUIRE(geometry.positions.size() == 8);
    const std::vector<std::uint32_t>& triangleIndices = geometry.topology.triangleIndices;
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

    DisplayGeometry geometry;
    buildDisplayGeometry(geometry, packet);

    REQUIRE(geometry.positions[2] == glm::vec3(1.f, 0.f, 1.f));
    REQUIRE(geometry.normals[0].y != 0.f);
}

TEST_CASE("Display geometry leaves out deleted faces")
{
    std::shared_ptr<geo::Mesh> mesh = buildSquareMesh("/squares");
    const std::vector<Vector3> positions = {{2, 0, 0}, {3, 0, 0}, {3, 0, 1}, {2, 0, 1}};
    mesh->addFace(mesh->addPoints(positions));
    mesh->deleteFaces({0}, false);
    NodePacket packet;
    packet.addPrimitive(mesh);

    DisplayGeometry geometry;
    buildDisplayGeometry(geometry, packet);

    REQUIRE(geometry.topology.triangleIndices.size() == 6);
    REQUIRE(geometry.topology.edgeIndices.size() == 8);
}

TEST_CASE("Display geometry keeps the points that belong to no face")
{
    std::shared_ptr<geo::Mesh> mesh = buildSquareMesh("/square");
    mesh->addPoint({5, 0, 0});
    NodePacket packet;
    packet.addPrimitive(mesh);

    DisplayGeometry geometry;
    buildDisplayGeometry(geometry, packet);

    REQUIRE(geometry.soloPointPositions == std::vector<glm::vec3>{{5.f, 0.f, 0.f}});
}

TEST_CASE("Display geometry places each camera by its transform")
{
    auto camera = std::make_shared<geo::Camera>("/camera");
    Matrix4 transform = Matrix4::Identity();
    transform(0, 3) = 2.f;
    camera->setTransform(transform);
    NodePacket packet;
    packet.addPrimitive(camera);

    DisplayGeometry geometry;
    buildDisplayGeometry(geometry, packet);

    REQUIRE(geometry.cameraTransforms.size() == 1);
    REQUIRE(geometry.cameraTransforms[0][3] == glm::vec4(2.f, 0.f, 0.f, 1.f));
}

TEST_CASE("Display geometry refilled from a smaller packet holds only the new contents")
{
    NodePacket twoSquares;
    twoSquares.addPrimitive(buildSquareMesh("/first"));
    twoSquares.addPrimitive(buildSquareMesh("/second"));
    NodePacket oneSquare;
    oneSquare.addPrimitive(buildSquareMesh("/square"));

    DisplayGeometry geometry;
    buildDisplayGeometry(geometry, twoSquares);
    buildDisplayGeometry(geometry, oneSquare);

    REQUIRE(geometry.positions.size() == 4);
    REQUIRE(geometry.topology.triangleIndices.size() == 6);
    REQUIRE(geometry.topology.edgeIndices.size() == 8);
    REQUIRE(geometry.pointPositions.size() == 4);
}
