#include "Engine/Core/Types.h"
#include "Engine/Primitives/Camera.h"
#include "Engine/Primitives/Mesh.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace enzo;

namespace {

// Builds a mesh with three loose points.
geo::Mesh buildThreePointMesh()
{
    geo::Mesh mesh;
    const std::vector<Vector3> positions = {{0, 0, 0}, {1, 0, 0}, {0.25, 0, 0}};
    mesh.addPoints(positions);
    return mesh;
}

} // namespace

TEST_CASE("Adding attributes from a source also adds its groups")
{
    geo::Mesh source = buildThreePointMesh();
    source.addPointGroup("selected");
    geo::Mesh dest = buildThreePointMesh();

    dest.addAttributesFrom(source, attr::AttributeOwner::POINT);

    REQUIRE(dest.getGroupByName(attr::AttributeOwner::POINT, "selected") != nullptr);
}

TEST_CASE("A camera stores only primitive elements")
{
    geo::Camera camera;

    REQUIRE(camera.hasOwner(attr::AttributeOwner::PRIMITIVE));
    REQUIRE_FALSE(camera.hasOwner(attr::AttributeOwner::POINT));
    REQUIRE_FALSE(camera.hasOwner(attr::AttributeOwner::VERTEX));
    REQUIRE_FALSE(camera.hasOwner(attr::AttributeOwner::FACE));
}

TEST_CASE("A mesh stores every kind of element")
{
    geo::Mesh mesh;

    REQUIRE(mesh.hasOwner(attr::AttributeOwner::PRIMITIVE));
    REQUIRE(mesh.hasOwner(attr::AttributeOwner::POINT));
    REQUIRE(mesh.hasOwner(attr::AttributeOwner::VERTEX));
    REQUIRE(mesh.hasOwner(attr::AttributeOwner::FACE));
}
