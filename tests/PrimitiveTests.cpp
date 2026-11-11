#include "Engine/Core/Types.h"
#include "Engine/Primitives/Camera.h"
#include "Engine/Primitives/Mesh.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>
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

// Builds a mesh with three loose points, each with a density of 0.
geo::Mesh buildDensityMesh()
{
    geo::Mesh mesh = buildThreePointMesh();
    mesh.addAttribute<floatT>(attr::AttributeOwner::POINT, "Density");
    return mesh;
}

std::shared_ptr<const attr::Attribute> getDensity(const geo::Primitive& primitive)
{
    return primitive.getAttribByName(attr::AttributeOwner::POINT, "Density");
}

floatT getDensityValue(const std::shared_ptr<const attr::Attribute>& density, Offset pointOffset)
{
    return attr::AttributeHandleRO<floatT>(density).getValue(pointOffset);
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

TEST_CASE("Writing to a shared attribute leaves the other holder's values untouched")
{
    geo::Mesh mesh = buildDensityMesh();
    const std::shared_ptr<const attr::Attribute> upstreamDensity = getDensity(mesh);

    mesh.getWriteHandle<floatT>(attr::AttributeOwner::POINT, "Density")->setValue(0, 5);

    REQUIRE(getDensityValue(upstreamDensity, 0) == 0);
    REQUIRE(getDensityValue(getDensity(mesh), 0) == 5);
}

TEST_CASE("Writing to an unshared attribute writes in place")
{
    geo::Mesh mesh = buildDensityMesh();
    const attr::Attribute* densityBefore = getDensity(mesh).get();

    mesh.getWriteHandle<floatT>(attr::AttributeOwner::POINT, "Density")->setValue(0, 5);

    REQUIRE(getDensity(mesh).get() == densityBefore);
}

TEST_CASE("Writing to a shared attribute twice copies it once")
{
    geo::Mesh mesh = buildDensityMesh();
    const std::shared_ptr<const attr::Attribute> upstreamDensity = getDensity(mesh);

    mesh.getWriteHandle<floatT>(attr::AttributeOwner::POINT, "Density")->setValue(0, 5);
    const attr::Attribute* firstCopy = getDensity(mesh).get();
    mesh.getWriteHandle<floatT>(attr::AttributeOwner::POINT, "Density")->setValue(1, 5);

    REQUIRE(firstCopy != upstreamDensity.get());
    REQUIRE(getDensity(mesh).get() == firstCopy);
}
