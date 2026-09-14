#include "Engine/Core/Types.h"
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
