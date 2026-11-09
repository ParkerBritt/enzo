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

    utils::copyAttributes(
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

    utils::copyAttributes(
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

    utils::interpolateAttributes(
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

    utils::interpolateAttributes(
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

    utils::interpolateAttributes(
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

    utils::interpolateAttributes(
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

    utils::interpolateAttributes(
        mesh,
        mesh,
        attr::AttributeOwner::POINT,
        std::vector<utils::ElementBlend>{{0, 1, 0.25, 2}}
    );

    REQUIRE(flag.getValue(2) == true);
}

TEST_CASE("Copying values repeatedly writes the source block once per copy")
{
    geo::Mesh source;
    source.addPoints(std::vector<Vector3>{{0, 0, 0}, {1, 0, 0}});
    auto sourceWeights = source.addAttribute<floatT>(attr::AttrOwner::POINT, "weight");
    sourceWeights.setValue(0, 1.0f);
    sourceWeights.setValue(1, 2.0f);
    source.addPointGroup("selected");
    source.addToPointGroup("selected", {1});

    geo::Mesh dest;
    dest.addPoints(std::vector<Vector3>(5, Vector3::Zero()));

    utils::copyAttributesRepeatedly(source, dest, attr::AttributeOwner::POINT, 2, 1, 2);

    // Check each copy lands in its own block after the start
    const attr::AttributeHandleRO<floatT> weights(
        dest.getAttribByName(attr::AttributeOwner::POINT, "weight")
    );
    REQUIRE(weights.getValue(0) == 0.0f);
    REQUIRE(weights.getValue(1) == 1.0f);
    REQUIRE(weights.getValue(2) == 2.0f);
    REQUIRE(weights.getValue(3) == 1.0f);
    REQUIRE(weights.getValue(4) == 2.0f);

    const attr::AttributeHandleRO<boolT> selected(
        dest.getGroupByName(attr::AttributeOwner::POINT, "selected")
    );
    REQUIRE_FALSE(selected.getValue(1));
    REQUIRE(selected.getValue(2));
    REQUIRE_FALSE(selected.getValue(3));
    REQUIRE(selected.getValue(4));
}

TEST_CASE("Copying values repeatedly skips an attribute of another type on the destination")
{
    geo::Mesh source;
    source.addPoints(std::vector<Vector3>{{0, 0, 0}});
    source.addAttribute<floatT>(attr::AttrOwner::POINT, "weight").setValue(0, 1.0f);

    geo::Mesh dest;
    dest.addPoints(std::vector<Vector3>(2, Vector3::Zero()));
    dest.addAttribute<intT>(attr::AttrOwner::POINT, "weight");

    utils::copyAttributesRepeatedly(source, dest, attr::AttributeOwner::POINT, 1, 1, 1);

    const attr::AttributeHandleRO<intT> weights(
        dest.getAttribByName(attr::AttributeOwner::POINT, "weight")
    );
    REQUIRE(weights.getValue(1) == 0);
}

TEST_CASE("Copying attributes adds the ones the destination lacks")
{
    geo::Mesh source;
    source.addPoints(std::vector<Vector3>{{0, 0, 0}});
    source.addAttribute<floatT>(attr::AttrOwner::POINT, "weight").setValue(0, 2.0f);
    source.addPointGroup("selected");

    geo::Mesh dest;
    dest.addPoints(std::vector<Vector3>{{0, 0, 0}});

    utils::copyAttributes(
        source,
        dest,
        attr::AttributeOwner::POINT,
        std::vector<Offset>{0},
        std::vector<Offset>{0}
    );

    const auto weight = dest.getAttribByName(attr::AttributeOwner::POINT, "weight");
    REQUIRE(weight != nullptr);
    REQUIRE(attr::AttributeHandleRO<floatT>(weight).getValue(0) == 2.0f);
    REQUIRE(dest.getGroupByName(attr::AttributeOwner::POINT, "selected") != nullptr);
}
