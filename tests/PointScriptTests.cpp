#include "Engine/Expression/PointScript.h"
#include "Engine/Primitives/Mesh.h"
#include <catch2/catch_test_macros.hpp>

using namespace enzo;

namespace {
std::shared_ptr<expr::PointScript> compileScript(const String& code)
{
    String error;
    auto script = expr::PointScript::compile(code, error);
    INFO(error);
    REQUIRE(script);
    return script;
}

String getCompileError(const String& code)
{
    String error;
    REQUIRE_FALSE(expr::PointScript::compile(code, error));
    return error;
}

geo::Mesh buildThreePointMesh()
{
    geo::Mesh mesh;
    mesh.addPoint(Vector3(0, 0, 0));
    mesh.addPoint(Vector3(1, 0, 0));
    mesh.addPoint(Vector3(2, 0, 0));
    return mesh;
}

// Runs the script over every point of the input into a copy of it.
geo::Mesh runScript(expr::PointScript& script, const geo::Mesh& input)
{
    geo::Mesh output = input;
    String error;
    REQUIRE(script.addWrittenAttributes(output, error));
    INFO(error);
    REQUIRE(script.run(input, output, 0, input.getNumPoints(), nullptr, error));
    return output;
}

floatT getPointFloat(const geo::Mesh& mesh, const String& name, Offset point)
{
    auto attribute = mesh.getAttribByName(attr::AttributeOwner::POINT, name);
    REQUIRE(attribute);
    return attr::AttributeHandleRO<floatT>(attribute).getValue(point);
}

const expr::AttributeBinding& getBinding(const expr::PointScript& script, const String& name)
{
    for (const expr::AttributeBinding& binding : script.getBindings())
    {
        if (binding.name == name) return binding;
    }
    FAIL("no binding named " << name);
    return script.getBindings().front();
}
} // namespace

// Finding bindings

TEST_CASE("compile finds each binding with the type of its prefix")
{
    auto script = compileScript(R"(@plain = 1.0
f@weight = 1.0
i@id = pt
v@dir = float3(1.0)
b@selected = true
@Position.y = 1.0)");

    const auto& bindings = script->getBindings();
    REQUIRE(bindings.size() == 6);
    REQUIRE(bindings[0].name == "plain");
    REQUIRE(bindings[0].type == attr::AttributeType::floatT);
    REQUIRE(bindings[1].type == attr::AttributeType::floatT);
    REQUIRE(bindings[2].type == attr::AttributeType::intT);
    REQUIRE(bindings[3].type == attr::AttributeType::vectorT);
    REQUIRE(bindings[4].type == attr::AttributeType::boolT);
    REQUIRE(bindings[5].name == "Position");
    REQUIRE(bindings[5].type == attr::AttributeType::vectorT);
}

TEST_CASE("compile gives a binding without a prefix the type of a prefixed use")
{
    auto script = compileScript(R"(v@dir = float3(1.0)
@dir.x = 2.0)");
    REQUIRE(getBinding(*script, "dir").type == attr::AttributeType::vectorT);
}

TEST_CASE("compile reports a binding used with two types")
{
    const String error = getCompileError(R"(f@value = 1.0
i@value = 2l)");
    REQUIRE(error.find("@value") != String::npos);
}

TEST_CASE("compile reports a prefix that contradicts a fixed type")
{
    const String error = getCompileError("f@Position = 1.0");
    REQUIRE(error.find("@Position") != String::npos);
}

TEST_CASE("compile leaves @ inside strings, characters and comments alone")
{
    auto script = compileScript(R"(let text = "@inString"
let character = '@'
// @inLineComment
/* @inBlock /* @inNested */ @stillInBlock */
@real = 1.0)");

    REQUIRE(script->getBindings().size() == 1);
    REQUIRE(script->getBindings()[0].name == "real");
}

TEST_CASE("compile finds bindings inside string interpolation")
{
    auto script = compileScript(R"(let text = "{@first} and {@second + 1.0}")");
    REQUIRE(script->getBindings().size() == 2);
}

TEST_CASE("compile leaves daslang lambdas and function pointers alone")
{
    auto script = compileScript(R"(var offset = 1.0
var addOffset <- @ capture(:= offset) (value : float) : float { return value + offset; }
var addOne <- @(value : float) : float { return value + 1.0; }
@result = invoke(addOffset, invoke(addOne, 1.0)))");

    REQUIRE(script->getBindings().size() == 1);
    REQUIRE(script->getBindings()[0].name == "result");
}

// Which bindings are written

TEST_CASE("compile marks only the bindings the code assigns to")
{
    auto script = compileScript("@written = @readOnly");
    REQUIRE(getBinding(*script, "written").written);
    REQUIRE_FALSE(getBinding(*script, "readOnly").written);
}

TEST_CASE("compile marks a field assignment and a compound assignment as writes")
{
    auto script = compileScript(R"(@Position.y = 1.0
@count += 1.0)");
    REQUIRE(getBinding(*script, "Position").written);
    REQUIRE(getBinding(*script, "count").written);
}

TEST_CASE("compile marks a binding written inside a branch that never runs")
{
    auto script = compileScript(R"(if (pt < 0l) {
    @never = 1.0
})");
    REQUIRE(getBinding(*script, "never").written);
}

TEST_CASE("compile marks a binding passed to a function that changes it")
{
    auto script = compileScript("swap(@first, @second)");
    REQUIRE(getBinding(*script, "first").written);
    REQUIRE(getBinding(*script, "second").written);
}

// Errors

TEST_CASE("compile reports errors on the line of the user's code")
{
    const String error = getCompileError(R"(let fine = 1
let broken = notDefined)");
    INFO(error);
    REQUIRE(error.find("line 2:") != String::npos);
}

TEST_CASE("compile shows bindings in errors as @name")
{
    const String error = getCompileError(R"(@value = "text")");
    INFO(error);
    REQUIRE(error.find("@value") != String::npos);
    REQUIRE(error.find("enzoAttrib_") == String::npos);
}

// Running over points

TEST_CASE("run writes a binding on every point")
{
    auto script = compileScript("@height = float(pt)");
    geo::Mesh output = runScript(*script, buildThreePointMesh());

    REQUIRE(getPointFloat(output, "height", 0) == 0.0f);
    REQUIRE(getPointFloat(output, "height", 1) == 1.0f);
    REQUIRE(getPointFloat(output, "height", 2) == 2.0f);
}

TEST_CASE("run reads the input and writes the output")
{
    auto script = compileScript("@Position.y = @Position.x + 1.0");
    const geo::Mesh input = buildThreePointMesh();
    geo::Mesh output = runScript(*script, input);

    REQUIRE(output.getPointPos(2) == Vector3(2, 3, 0));
    REQUIRE(input.getPointPos(2) == Vector3(2, 0, 0));
}

TEST_CASE("run creates an attribute written only in a branch that never runs")
{
    auto script = compileScript(R"(if (pt < 0l) {
    @never = 1.0
})");
    geo::Mesh output = runScript(*script, buildThreePointMesh());
    REQUIRE(getPointFloat(output, "never", 1) == 0.0f);
}

TEST_CASE("run leaves out an attribute the script only reads")
{
    auto script = compileScript("@copy = @missing + 1.0");
    geo::Mesh output = runScript(*script, buildThreePointMesh());

    REQUIRE(getPointFloat(output, "copy", 0) == 1.0f);
    REQUIRE_FALSE(output.getAttribByName(attr::AttributeOwner::POINT, "missing"));
}

TEST_CASE("run stores a binding after an early return")
{
    auto script = compileScript(R"(@value = 5.0
return)");
    geo::Mesh output = runScript(*script, buildThreePointMesh());
    REQUIRE(getPointFloat(output, "value", 0) == 5.0f);
}

TEST_CASE("run reports a panic on the line of the user's code")
{
    auto script = compileScript(R"(@value = 1.0
panic("failed on {pt}"))");
    const geo::Mesh input = buildThreePointMesh();
    geo::Mesh output = input;

    String error;
    REQUIRE(script->addWrittenAttributes(output, error));
    REQUIRE_FALSE(script->run(input, output, 0, input.getNumPoints(), nullptr, error));
    INFO(error);
    REQUIRE(error.find("failed on 0") != String::npos);
    REQUIRE(error.find("line 2:") != String::npos);
}

TEST_CASE("clone runs with the same bindings")
{
    auto script = compileScript("@height = float(pt)");
    auto clone = script->clone();
    REQUIRE(clone);

    geo::Mesh output = runScript(*clone, buildThreePointMesh());
    REQUIRE(getPointFloat(output, "height", 2) == 2.0f);
}

// Arithmetic between a vector and a single number

TEST_CASE("A vector binding scales by a whole number")
{
    auto script = compileScript("@Position *= 5");
    geo::Mesh output = runScript(*script, buildThreePointMesh());

    REQUIRE(output.getPointPos(2) == Vector3(10, 0, 0));
}

TEST_CASE("A vector binding adds one number to every component")
{
    auto script = compileScript("@Position += 2.5");
    geo::Mesh output = runScript(*script, buildThreePointMesh());

    REQUIRE(output.getPointPos(1) == Vector3(3.5f, 2.5f, 2.5f));
}

TEST_CASE("A vector reads either way round in a sum")
{
    auto script = compileScript("@Position = 1 + @Position - 1.0");
    geo::Mesh output = runScript(*script, buildThreePointMesh());

    REQUIRE(output.getPointPos(2) == Vector3(2, 0, 0));
}

TEST_CASE("A vector binding divides by a whole number")
{
    auto script = compileScript("@Position /= 2");
    geo::Mesh output = runScript(*script, buildThreePointMesh());

    REQUIRE(output.getPointPos(2) == Vector3(1, 0, 0));
}
