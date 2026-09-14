#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/AttributeTransfer.h"
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

// Builds a three point mesh whose float weight is 0, 8 and 4 at its points.
geo::Mesh buildWeightedMesh()
{
    geo::Mesh mesh = buildThreePointMesh();
    auto weight = mesh.addAttribute<floatT>(attr::AttributeOwner::POINT, "weight");
    weight.setValue(0, 0);
    weight.setValue(1, 8);
    weight.setValue(2, 4);
    return mesh;
}

// Returns the float weight at a point.
floatT getWeight(const geo::Mesh& mesh, Offset pointOffset)
{
    const auto weight = mesh.getAttribByName(attr::AttributeOwner::POINT, "weight");
    return attr::AttributeHandleRO<floatT>(weight).getValue(pointOffset);
}

} // namespace

TEST_CASE("Copying a point carries its group membership")
{
    geo::Mesh mesh = buildThreePointMesh();
    mesh.addPointGroup("selected");
    mesh.addToPointGroup("selected", {1});

    utils::copyAttributeValues(
        mesh,
        mesh,
        attr::AttributeOwner::POINT,
        std::vector<Offset>{1},
        std::vector<Offset>{2}
    );

    auto selected = mesh.getGroupByName(attr::AttributeOwner::POINT, "selected");
    REQUIRE(attr::AttributeHandleRO<boolT>(selected).getValue(2) == true);
}

TEST_CASE("Copying several points writes each into its own destination")
{
    const geo::Mesh source = buildWeightedMesh();
    geo::Mesh dest = buildThreePointMesh();
    dest.addAttributesFrom(source, attr::AttributeOwner::POINT);

    utils::copyAttributeValues(
        source,
        dest,
        attr::AttributeOwner::POINT,
        std::vector<Offset>{1, 2},
        std::vector<Offset>{2, 0}
    );

    REQUIRE(getWeight(dest, 2) == Catch::Approx(8));
    REQUIRE(getWeight(dest, 0) == Catch::Approx(4));
}

TEST_CASE("Interpolating several points writes each blend into its own destination")
{
    const geo::Mesh source = buildWeightedMesh();
    geo::Mesh dest = buildThreePointMesh();
    dest.addAttributesFrom(source, attr::AttributeOwner::POINT);

    utils::interpolateAttributeValues(
        source,
        dest,
        attr::AttributeOwner::POINT,
        std::vector<utils::ElementBlend>{{0, 1, 0.25, 2}, {1, 2, 0.5, 0}}
    );

    REQUIRE(getWeight(dest, 2) == Catch::Approx(2));
    REQUIRE(getWeight(dest, 0) == Catch::Approx(6));
}

TEST_CASE("Interpolated group membership follows the nearer source point")
{
    geo::Mesh mesh = buildThreePointMesh();
    mesh.addPointGroup("selected");
    mesh.addToPointGroup("selected", {0});

    utils::interpolateAttributeValues(
        mesh,
        mesh,
        attr::AttributeOwner::POINT,
        std::vector<utils::ElementBlend>{{0, 1, 0.25, 2}}
    );

    auto selected = mesh.getGroupByName(attr::AttributeOwner::POINT, "selected");
    REQUIRE(attr::AttributeHandleRO<boolT>(selected).getValue(2) == true);
}

TEST_CASE("Interpolated float leans toward the nearer source point")
{
    geo::Mesh mesh = buildThreePointMesh();
    auto weight = mesh.addAttribute<floatT>(attr::AttributeOwner::POINT, "weight");
    weight.setValue(0, 0);
    weight.setValue(1, 8);

    utils::interpolateAttributeValues(
        mesh,
        mesh,
        attr::AttributeOwner::POINT,
        std::vector<utils::ElementBlend>{{0, 1, 0.25, 2}}
    );

    REQUIRE(weight.getValue(2) == Catch::Approx(2));
}

TEST_CASE("Interpolated integer rounds to the nearest whole number")
{
    geo::Mesh mesh = buildThreePointMesh();
    auto count = mesh.addAttribute<intT>(attr::AttributeOwner::POINT, "count");
    count.setValue(0, 0);
    count.setValue(1, 10);

    utils::interpolateAttributeValues(
        mesh,
        mesh,
        attr::AttributeOwner::POINT,
        std::vector<utils::ElementBlend>{{0, 1, 0.25, 2}}
    );

    REQUIRE(count.getValue(2) == 3);
}

TEST_CASE("Interpolated bool takes the value of the nearer source point")
{
    geo::Mesh mesh = buildThreePointMesh();
    auto flag = mesh.addAttribute<boolT>(attr::AttributeOwner::POINT, "flag");
    flag.setValue(0, true);
    flag.setValue(1, false);

    utils::interpolateAttributeValues(
        mesh,
        mesh,
        attr::AttributeOwner::POINT,
        std::vector<utils::ElementBlend>{{0, 1, 0.25, 2}}
    );

    REQUIRE(flag.getValue(2) == true);
}
