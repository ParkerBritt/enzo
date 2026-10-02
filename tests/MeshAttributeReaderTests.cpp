#include "Engine/Script/MeshAttributeReader.h"
#include "Engine/Primitives/Mesh.h"
#include <catch2/catch_test_macros.hpp>

using namespace enzo;

namespace {
geo::Mesh buildQuad()
{
    geo::Mesh mesh;
    mesh.addPoints(std::vector<Vector3>{
        Vector3(0, 0, 0), Vector3(1, 0, 0), Vector3(1, 1, 0), Vector3(0, 1, 0)
    });
    mesh.addFace({0, 1, 2, 3});
    return mesh;
}
} // namespace

// Elements by index

TEST_CASE("hasElement finds every element of each owner")
{
    const geo::Mesh mesh = buildQuad();
    const script::MeshAttributeReader reader(mesh);

    REQUIRE(reader.hasElement(attr::AttributeOwner::POINT, 3));
    REQUIRE(reader.hasElement(attr::AttributeOwner::VERTEX, 2));
    REQUIRE(reader.hasElement(attr::AttributeOwner::FACE, 0));
    REQUIRE(reader.hasElement(attr::AttributeOwner::PRIMITIVE, 0));
}

TEST_CASE("hasElement rejects an index outside the elements")
{
    const geo::Mesh mesh = buildQuad();
    const script::MeshAttributeReader reader(mesh);

    REQUIRE_FALSE(reader.hasElement(attr::AttributeOwner::POINT, 4));
    REQUIRE_FALSE(reader.hasElement(attr::AttributeOwner::POINT, -1));
    REQUIRE_FALSE(reader.hasElement(attr::AttributeOwner::FACE, 1));
    REQUIRE_FALSE(reader.hasElement(attr::AttributeOwner::PRIMITIVE, 1));
}

// Attributes by name

TEST_CASE("getAttribute finds an attribute only on its own owner")
{
    geo::Mesh mesh = buildQuad();
    mesh.addAttribute<floatT>(attr::AttributeOwner::FACE, "area");
    const script::MeshAttributeReader reader(mesh);

    const script::ScriptAttribute* area = reader.getAttribute(attr::AttributeOwner::FACE, "area");
    REQUIRE(area);
    REQUIRE(area->type == attr::AttributeType::floatT);
    REQUIRE_FALSE(reader.getAttribute(attr::AttributeOwner::POINT, "area"));
}

TEST_CASE("getAttribute finds an intrinsic attribute")
{
    const geo::Mesh mesh = buildQuad();
    const script::MeshAttributeReader reader(mesh);

    const script::ScriptAttribute* position =
        reader.getAttribute(attr::AttributeOwner::POINT, "Position");
    REQUIRE(position);
    REQUIRE(position->type == attr::AttributeType::vectorT);
}
