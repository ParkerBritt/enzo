#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/Primitives/Mesh.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace enzo;

namespace {

// Builds a mesh with three loose points, the last one lying between the first two.
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

TEST_CASE("Copying a point carries its group membership")
{
    geo::Mesh mesh = buildThreePointMesh();
    mesh.addPointGroup("selected");
    mesh.addToPointGroup("selected", {1});

    mesh.copyAttributeValuesFrom(mesh, attr::AttributeOwner::POINT, 1, 2);

    auto selected = mesh.getGroupByName(attr::AttributeOwner::POINT, "selected");
    REQUIRE(attr::AttributeHandleRO<boolT>(selected).getValue(2) == true);
}

TEST_CASE("Interpolated group membership follows the nearer source point")
{
    geo::Mesh mesh = buildThreePointMesh();
    mesh.addPointGroup("selected");
    mesh.addToPointGroup("selected", {0});

    mesh.interpolateAttributeValuesFrom(mesh, attr::AttributeOwner::POINT, 0, 1, 0.25, 2);

    auto selected = mesh.getGroupByName(attr::AttributeOwner::POINT, "selected");
    REQUIRE(attr::AttributeHandleRO<boolT>(selected).getValue(2) == true);
}

TEST_CASE("Interpolated float leans toward the nearer source point")
{
    geo::Mesh mesh = buildThreePointMesh();
    auto weight = mesh.addAttribute<floatT>(attr::AttributeOwner::POINT, "weight");
    weight.setValue(0, 0);
    weight.setValue(1, 8);

    mesh.interpolateAttributeValuesFrom(mesh, attr::AttributeOwner::POINT, 0, 1, 0.25, 2);

    REQUIRE(weight.getValue(2) == Catch::Approx(2));
}

TEST_CASE("Interpolated integer rounds to the nearest whole number")
{
    geo::Mesh mesh = buildThreePointMesh();
    auto count = mesh.addAttribute<intT>(attr::AttributeOwner::POINT, "count");
    count.setValue(0, 0);
    count.setValue(1, 10);

    mesh.interpolateAttributeValuesFrom(mesh, attr::AttributeOwner::POINT, 0, 1, 0.25, 2);

    REQUIRE(count.getValue(2) == 3);
}

TEST_CASE("Interpolated bool takes the value of the nearer source point")
{
    geo::Mesh mesh = buildThreePointMesh();
    auto flag = mesh.addAttribute<boolT>(attr::AttributeOwner::POINT, "flag");
    flag.setValue(0, true);
    flag.setValue(1, false);

    mesh.interpolateAttributeValuesFrom(mesh, attr::AttributeOwner::POINT, 0, 1, 0.25, 2);

    REQUIRE(flag.getValue(2) == true);
}
