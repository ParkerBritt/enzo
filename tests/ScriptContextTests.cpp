#include "Engine/Script/ScriptContext.h"
#include "Engine/Network/NodePacket.h"
#include "Engine/Primitives/Mesh.h"
#include <catch2/catch_test_macros.hpp>

using namespace enzo;

namespace {
std::shared_ptr<geo::Mesh> buildMesh(int pointCount)
{
    auto mesh = std::make_shared<geo::Mesh>();
    for (int point = 0; point < pointCount; ++point) mesh->addPoint(Vector3::Zero());
    return mesh;
}
} // namespace

TEST_CASE("getInputMesh returns the mesh at each primitive index")
{
    NodePacket input;
    const auto first = buildMesh(2);
    const auto second = buildMesh(5);
    input.addPrimitive(first);
    input.addPrimitive(second);

    const script::ScriptContext context(0, input);

    REQUIRE(&context.getInputMesh(0) == first.get());
    REQUIRE(&context.getInputMesh(1) == second.get());
}

TEST_CASE("getAttributeReader reads the mesh at each primitive index")
{
    NodePacket input;
    input.addPrimitive(buildMesh(2));
    input.addPrimitive(buildMesh(5));

    const script::ScriptContext context(0, input);

    REQUIRE_FALSE(context.getAttributeReader(0).hasElement(attr::AttributeOwner::POINT, 4));
    REQUIRE(context.getAttributeReader(1).hasElement(attr::AttributeOwner::POINT, 4));
}
