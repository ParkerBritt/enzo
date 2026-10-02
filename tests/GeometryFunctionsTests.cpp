#include "Engine/Expression/PointScript.h"
#include "Engine/Expression/ScriptContext.h"
#include "Engine/Network/NodePacket.h"
#include "Engine/Primitives/Mesh.h"
#include <catch2/catch_test_macros.hpp>

using namespace enzo;

namespace {
// Builds a quad with a height on each point, a value on each vertex and face,
// and a mass on the primitive.
geo::Mesh buildQuad()
{
    geo::Mesh mesh;
    mesh.addPoints(std::vector<Vector3>{
        Vector3(0, 0, 0), Vector3(1, 0, 0), Vector3(1, 1, 0), Vector3(0, 1, 0)
    });
    mesh.addFace({0, 1, 2, 3});

    auto height = mesh.addAttribute<floatT>(attr::AttributeOwner::POINT, "height");
    auto id = mesh.addAttribute<intT>(attr::AttributeOwner::POINT, "id");
    auto selected = mesh.addAttribute<boolT>(attr::AttributeOwner::POINT, "selected");
    for (Offset pointOffset = 0; pointOffset < 4; ++pointOffset)
    {
        height.setValue(pointOffset, floatT(pointOffset) * 10);
        id.setValue(pointOffset, pointOffset + 100);
        selected.setValue(pointOffset, pointOffset == 2);
    }

    auto corner = mesh.addAttribute<floatT>(attr::AttributeOwner::VERTEX, "corner");
    for (Offset vertexOffset = 0; vertexOffset < 4; ++vertexOffset)
        corner.setValue(vertexOffset, floatT(vertexOffset) + 0.5f);

    mesh.addAttribute<intT>(attr::AttributeOwner::FACE, "part").setValue(0, 7);
    mesh.addAttribute<floatT>(attr::AttributeOwner::PRIMITIVE, "mass").setValue(0, 2.5f);
    return mesh;
}

// Runs the script over every point of the input into a copy of it, filling the
// warnings it added.
geo::Mesh runScript(const String& code, const geo::Mesh& input, std::vector<String>& warnings)
{
    String error;
    auto script = expr::PointScript::compile(code, error);
    INFO(error);
    REQUIRE(script);

    NodePacket packet;
    packet.addPrimitive(std::make_shared<geo::Mesh>(input));
    const expr::ScriptContext context(0, packet);

    geo::Mesh output = input;
    REQUIRE(script->addWrittenAttributes(output, error));
    REQUIRE(script->run(context, 0, output, 0, input.getNumPoints(), error));
    warnings = context.getWarnings();
    return output;
}

geo::Mesh runScript(const String& code, const geo::Mesh& input)
{
    std::vector<String> warnings;
    return runScript(code, input, warnings);
}

template <typename Value>
Value getPointValue(const geo::Mesh& mesh, const String& name, Offset pointOffset)
{
    auto attribute = mesh.getAttribByName(attr::AttributeOwner::POINT, name, true);
    REQUIRE(attribute);
    return attr::AttributeHandleRO<Value>(attribute).getValue(pointOffset);
}
} // namespace

// Reading each type

TEST_CASE("pointAttr reads a float from another point")
{
    const geo::Mesh output = runScript(R"(@copy = pointAttr("height", 2))", buildQuad());

    REQUIRE(getPointValue<floatT>(output, "copy", 0) == 20.0f);
    REQUIRE(getPointValue<floatT>(output, "copy", 3) == 20.0f);
}

TEST_CASE("pointAttrInt reads an int")
{
    const geo::Mesh output = runScript(R"(i@copy = pointAttrInt("id", curPt()))", buildQuad());

    REQUIRE(getPointValue<intT>(output, "copy", 1) == 101);
}

TEST_CASE("pointAttrVector reads a vector")
{
    const geo::Mesh output =
        runScript(R"(v@copy = pointAttrVector("Position", 2))", buildQuad());

    REQUIRE(getPointValue<Vector3>(output, "copy", 0) == Vector3(1, 1, 0));
}

TEST_CASE("pointAttrBool reads a bool")
{
    const geo::Mesh output =
        runScript(R"(b@copy = pointAttrBool("selected", curPt()))", buildQuad());

    REQUIRE_FALSE(getPointValue<boolT>(output, "copy", 1));
    REQUIRE(getPointValue<boolT>(output, "copy", 2));
}

// Reading each owner

TEST_CASE("vertexAttr reads a vertex by index")
{
    const geo::Mesh output = runScript(R"(@copy = vertexAttr("corner", 3))", buildQuad());

    REQUIRE(getPointValue<floatT>(output, "copy", 0) == 3.5f);
}

TEST_CASE("faceAttrInt reads a face by index")
{
    const geo::Mesh output = runScript(R"(i@copy = faceAttrInt("part", 0))", buildQuad());

    REQUIRE(getPointValue<intT>(output, "copy", 0) == 7);
}

TEST_CASE("primitiveAttr reads the primitive without an index")
{
    const geo::Mesh output = runScript(R"(@copy = primitiveAttr("mass"))", buildQuad());

    REQUIRE(getPointValue<floatT>(output, "copy", 0) == 2.5f);
}

TEST_CASE("pointAttr reads the input while the script writes the same attribute")
{
    const geo::Mesh output =
        runScript(R"(@height = pointAttr("height", (curPt() + 1l) % ptCount()))", buildQuad());

    REQUIRE(getPointValue<floatT>(output, "height", 0) == 10.0f);
    REQUIRE(getPointValue<floatT>(output, "height", 1) == 20.0f);
    REQUIRE(getPointValue<floatT>(output, "height", 3) == 0.0f);
}

// Checking for attributes

TEST_CASE("hasPointAttr finds only point attributes")
{
    const geo::Mesh output = runScript(R"(b@hasHeight = hasPointAttr("height")
b@hasCorner = hasPointAttr("corner")
b@hasMass = hasPrimitiveAttr("mass"))", buildQuad());

    REQUIRE(getPointValue<boolT>(output, "hasHeight", 0));
    REQUIRE_FALSE(getPointValue<boolT>(output, "hasCorner", 0));
    REQUIRE(getPointValue<boolT>(output, "hasMass", 0));
}

// Failed reads

TEST_CASE("A read of the wrong type returns zero and warns once")
{
    std::vector<String> warnings;
    const geo::Mesh output =
        runScript(R"(v@copy = pointAttrVector("height", curPt()))", buildQuad(), warnings);

    REQUIRE(getPointValue<Vector3>(output, "copy", 2) == Vector3::Zero());
    REQUIRE(
        warnings ==
        std::vector<String>{R"(pointAttrVector read "height" as vector but it is float)"}
    );
}

TEST_CASE("A read of a missing attribute returns zero and warns")
{
    std::vector<String> warnings;
    const geo::Mesh output =
        runScript(R"(i@copy = faceAttrInt("missing", 0))", buildQuad(), warnings);

    REQUIRE(getPointValue<intT>(output, "copy", 0) == 0);
    REQUIRE(
        warnings ==
        std::vector<String>{R"(faceAttrInt found no face attribute "missing")"}
    );
}

TEST_CASE("A read past the last element returns zero and warns")
{
    std::vector<String> warnings;
    const geo::Mesh output =
        runScript(R"(@copy = pointAttr("height", curPt() + 1l))", buildQuad(), warnings);

    REQUIRE(getPointValue<floatT>(output, "copy", 2) == 30.0f);
    REQUIRE(getPointValue<floatT>(output, "copy", 3) == 0.0f);
    REQUIRE(
        warnings ==
        std::vector<String>{"pointAttr read a point index that doesn't exist"}
    );
}
