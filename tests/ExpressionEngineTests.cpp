#include "Engine/Expression/DasRuntime.h"
#include "Engine/Expression/ExpressionContext.h"
#include "Engine/Expression/ExpressionEngine.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeLoader.h"
#include "Engine/Parameter/NodeParameter.h"
#include <catch2/catch_test_macros.hpp>

using namespace enzo;

TEST_CASE("evalFloat evaluates an arithmetic expression")
{
    expr::ExpressionEngine& engine = expr::ExpressionEngine::instance();

    floatT result = 0;
    String error;
    REQUIRE(engine.evalFloat("5 + 5", nullptr, result, error));
    REQUIRE(result == 10.0f);
}

TEST_CASE("evalInt evaluates an arithmetic expression")
{
    expr::ExpressionEngine& engine = expr::ExpressionEngine::instance();

    intT result = 0;
    String error;
    REQUIRE(engine.evalInt("2 * 3 + 1", nullptr, result, error));
    REQUIRE(result == 7);
}

TEST_CASE("evalFloat reports an error for a malformed expression")
{
    expr::ExpressionEngine& engine = expr::ExpressionEngine::instance();

    floatT result = 0;
    String error;
    REQUIRE_FALSE(engine.evalFloat("5 +", nullptr, result, error));
    REQUIRE_FALSE(error.empty());
}

struct NMReset
{
    NMReset()
    {
        nt::NodeLoader::loadNodes();
        nt::nm()._reset();
    }
    ~NMReset() { nt::nm()._reset(); }
};

TEST_CASE_METHOD(NMReset, "Prm reads another node's parameter by path")
{
    auto& nm = nt::nm();

    // The source node holds the value the expression should pull
    nt::NodeId source = nm.createNode("enzo::transform");
    nm.getNode(source).getParameter("translate").lock()->setFloat(7.0f);

    // A second node reads the source's translate through a path expression
    nt::NodeId reader = nm.createNode("enzo::transform");
    auto translate = nm.getNode(reader).getParameter("translate").lock();
    translate->setExpression("prm(\"transform1.translate\")");

    REQUIRE(translate->evalFloat() == 7.0f);
}

TEST_CASE_METHOD(NMReset, "Prm reads a chosen component of a vector parameter")
{
    auto& nm = nt::nm();

    // The source holds a distinct value in each component of its translate
    nt::NodeId source = nm.createNode("enzo::transform");
    auto sourceTranslate = nm.getNode(source).getParameter("translate").lock();
    sourceTranslate->setFloat(1.0f, 0);
    sourceTranslate->setFloat(2.0f, 1);
    sourceTranslate->setFloat(3.0f, 2);

    // The reader pulls the second and third components by index
    nt::NodeId reader = nm.createNode("enzo::transform");
    auto translate = nm.getNode(reader).getParameter("translate").lock();

    translate->setExpression("prm(\"transform1.translate\", 1)");
    REQUIRE(translate->evalFloat() == 2.0f);

    translate->setExpression("prm(\"transform1.translate\", 2)");
    REQUIRE(translate->evalFloat() == 3.0f);
}

TEST_CASE_METHOD(NMReset, "Prm without an index reads the first component")
{
    auto& nm = nt::nm();

    // The source holds different values across its translate components
    nt::NodeId source = nm.createNode("enzo::transform");
    auto sourceTranslate = nm.getNode(source).getParameter("translate").lock();
    sourceTranslate->setFloat(1.0f, 0);
    sourceTranslate->setFloat(2.0f, 1);

    // Omitting the index reads the same component as passing 0
    nt::NodeId reader = nm.createNode("enzo::transform");
    auto translate = nm.getNode(reader).getParameter("translate").lock();
    translate->setExpression("prm(\"transform1.translate\")");

    REQUIRE(translate->evalFloat() == 1.0f);
}

TEST_CASE_METHOD(NMReset, "Changing a parameter recooks nodes whose expressions read it")
{
    auto& nm = nt::nm();

    // The source node holds the value the reader pulls
    nt::NodeId source = nm.createNode("enzo::transform");
    nm.getNode(source).getParameter("translate").lock()->setFloat(7.0f);

    // The reader pulls the source's translate through an expression
    nt::NodeId reader = nm.createNode("enzo::transform");
    auto translate = nm.getNode(reader).getParameter("translate").lock();
    translate->setExpression("prm(\"transform1.translate\")");

    // Evaluating once records the captured dependency, then cooking clears the
    // reader so the later source change is what dirties it
    translate->evalFloat();
    nm.cook(reader);
    REQUIRE_FALSE(nm.getNode(reader).isDirty());

    // Changing the source marks the reader stale through the captured edge
    nm.getNode(source).getParameter("translate").lock()->setFloat(9.0f);
    REQUIRE(nm.getNode(reader).isDirty());
}

TEST_CASE_METHOD(NMReset, "Prm reports an error when the path matches no parameter")
{
    auto& nm = nt::nm();

    // The reader points its expression at a node that does not exist
    nt::NodeId reader = nm.createNode("enzo::transform");
    auto translate = nm.getNode(reader).getParameter("translate").lock();
    translate->setExpression("prm(\"does_not_exist.translate\")");

    // A bad path falls back to zero and surfaces the failure as an error
    String error;
    REQUIRE(translate->evalFloat(0, error) == 0.0f);
    REQUIRE_FALSE(error.empty());
}

TEST_CASE_METHOD(NMReset, "PrmI reads another node's integer parameter")
{
    auto& nm = nt::nm();

    // The source node holds the integer the expression should pull
    nt::NodeId source = nm.createNode("enzo::grid");
    nm.getNode(source).getParameter("rows").lock()->setInt(5);

    // A second node reads the source's rows through a path expression
    nt::NodeId reader = nm.createNode("enzo::grid");
    auto rows = nm.getNode(reader).getParameter("rows").lock();
    rows->setExpression("prmI(\"grid1.rows\")");

    REQUIRE(rows->evalInt() == 5);
}

TEST_CASE_METHOD(NMReset, "PrmS reads another node's string parameter")
{
    auto& nm = nt::nm();

    // The source node holds the string the expression should pull
    nt::NodeId source = nm.createNode("enzo::path");
    nm.getNode(source).getParameter("path").lock()->setString("hello");

    // A second node reads the source's path through a path expression
    nt::NodeId reader = nm.createNode("enzo::path");
    auto path = nm.getNode(reader).getParameter("path").lock();
    path->setExpression("prmS(\"path1.path\")");

    REQUIRE(path->evalString() == "hello");
}

TEST_CASE_METHOD(NMReset, "Frame reads the frame the scene sits on")
{
    expr::ExpressionEngine& engine = expr::ExpressionEngine::instance();
    nt::nm().setFrame(48);

    floatT result = 0;
    String error;
    REQUIRE(engine.evalFloat("frame() * 2", nullptr, result, error));
    REQUIRE(result == 96.0f);
}

TEST_CASE_METHOD(NMReset, "Frame reads its new value after the scene moves")
{
    expr::ExpressionEngine& engine = expr::ExpressionEngine::instance();

    floatT result = 0;
    String error;
    nt::nm().setFrame(10);
    REQUIRE(engine.evalFloat("frame()", nullptr, result, error));
    REQUIRE(result == 10.0f);

    // Runs the same expression again, so a call folded to a constant would
    // return the first run's value.
    nt::nm().setFrame(20);
    REQUIRE(engine.evalFloat("frame()", nullptr, result, error));
    REQUIRE(result == 20.0f);
}

TEST_CASE_METHOD(NMReset, "Time reads the scene time in seconds")
{
    expr::ExpressionEngine& engine = expr::ExpressionEngine::instance();
    nt::nm().setFrame(25);

    floatT result = 0;
    String error;
    REQUIRE(engine.evalFloat("time()", nullptr, result, error));
    REQUIRE(result == 1.0f);
}

TEST_CASE_METHOD(NMReset, "Reading the frame makes the expression time dependent")
{
    expr::ExpressionEngine& engine = expr::ExpressionEngine::instance();
    expr::ExpressionContext context(nt::nullNode);

    floatT result = 0;
    String error;
    REQUIRE(engine.evalFloat("5 + 5", &context, result, error));
    REQUIRE_FALSE(context.dependsOnTime());

    REQUIRE(engine.evalFloat("frame()", &context, result, error));
    REQUIRE(context.dependsOnTime());
}

namespace {
floatT evalFloatExpression(const String& expression)
{
    floatT result = -1.0f;
    String error;
    const bool evaluated = expr::ExpressionEngine::instance().evalFloat(expression, nullptr, result, error);
    INFO(error);
    REQUIRE(evaluated);
    return result;
}
} // namespace

TEST_CASE("Rand returns the same value for the same seed")
{
    REQUIRE(evalFloatExpression("rand(7)") == evalFloatExpression("rand(7)"));
    REQUIRE(evalFloatExpression("rand(2.5)") == evalFloatExpression("rand(2.5)"));
}

TEST_CASE("Rand returns different values for neighbouring seeds")
{
    REQUIRE(evalFloatExpression("rand(1)") != evalFloatExpression("rand(2)"));
    REQUIRE(evalFloatExpression("rand(1.0)") != evalFloatExpression("rand(1.5)"));
    REQUIRE(evalFloatExpression("rand(float3(1.0, 2.0, 3.0))") != evalFloatExpression("rand(float3(1.0, 2.0, 4.0))"));
    REQUIRE(evalFloatExpression("rand(float3(1.0, 2.0, 3.0))") != evalFloatExpression("rand(float3(3.0, 2.0, 1.0))"));
}

TEST_CASE("Rand gives an int and an int64 seed the same value")
{
    REQUIRE(evalFloatExpression("rand(12)") == evalFloatExpression("rand(12l)"));
}

TEST_CASE("Rand spreads many seeds evenly between zero and one")
{
    String error;
    auto script = expr::DasRuntime::instance().compile(
        "randSpreadTest",
        R"(options gen2
require enzo_expression
[export]
def sample(seed : int64; var result : float&) {
    result = rand(seed)
}
)",
        error
    );
    INFO(error);
    REQUIRE(script);

    // Buckets the values into tenths, so a clumped hash leaves one bucket short
    constexpr int sampleCount = 10000;
    constexpr int bucketCount = 10;
    int bucketSizes[bucketCount] = {};
    for (intT seed = 0; seed < sampleCount; ++seed)
    {
        floatT value = -1.0f;
        const expr::ScriptArgument arguments[] = {seed, &value};
        REQUIRE(script->run("sample", arguments, nullptr, 0, error));
        REQUIRE(value >= 0.0f);
        REQUIRE(value < 1.0f);
        ++bucketSizes[static_cast<int>(value * bucketCount)];
    }

    for (int bucketSize : bucketSizes)
    {
        REQUIRE(bucketSize > 900);
        REQUIRE(bucketSize < 1100);
    }
}

TEST_CASE("Rand keeps the values that saved scenes were built with")
{
    // Fails when the hash changes, since every scene using rand would change with it
    REQUIRE(evalFloatExpression("rand(0)") == 0.625266731f);
    REQUIRE(evalFloatExpression("rand(7)") == 0.484932601f);
    REQUIRE(evalFloatExpression("rand(-3)") == 0.0577041507f);
    REQUIRE(evalFloatExpression("rand(5000000000l)") == 0.299867272f);
    REQUIRE(evalFloatExpression("rand(0.0)") == 0.00776511431f);
    REQUIRE(evalFloatExpression("rand(-0.0)") == 0.00776511431f);
    REQUIRE(evalFloatExpression("rand(1.5)") == 0.0227157474f);
    REQUIRE(evalFloatExpression("rand(float3(1.0, 2.0, 3.0))") == 0.95123148f);
}

TEST_CASE("RandVector gives each component its own value")
{
    const floatT x = evalFloatExpression("randVector(7).x");
    const floatT y = evalFloatExpression("randVector(7).y");
    const floatT z = evalFloatExpression("randVector(7).z");
    REQUIRE(x != y);
    REQUIRE(y != z);
    REQUIRE(x != z);
}

TEST_CASE("RandVector gives an int and an int64 seed the same value")
{
    REQUIRE(evalFloatExpression("randVector(12).y") == evalFloatExpression("randVector(12l).y"));
}

TEST_CASE("RandVector spreads many seeds evenly between zero and one")
{
    String error;
    auto script = expr::DasRuntime::instance().compile(
        "randVectorSpreadTest",
        R"(options gen2
require enzo_expression
[export]
def sample(seed : int64; var result : float3&) {
    result = randVector(seed)
}
)",
        error
    );
    INFO(error);
    REQUIRE(script);

    // Buckets every component into tenths, so a clumped component leaves one bucket short
    constexpr int sampleCount = 10000;
    constexpr int bucketCount = 10;
    int bucketSizes[3][bucketCount] = {};
    for (intT seed = 0; seed < sampleCount; ++seed)
    {
        Vector3 value(-1.0f, -1.0f, -1.0f);
        const expr::ScriptArgument arguments[] = {seed, value.data()};
        REQUIRE(script->run("sample", arguments, nullptr, 0, error));
        for (int component = 0; component < 3; ++component)
        {
            REQUIRE(value[component] >= 0.0f);
            REQUIRE(value[component] < 1.0f);
            ++bucketSizes[component][static_cast<int>(value[component] * bucketCount)];
        }
    }

    for (const auto& componentBuckets : bucketSizes)
    {
        for (int bucketSize : componentBuckets)
        {
            REQUIRE(bucketSize > 900);
            REQUIRE(bucketSize < 1100);
        }
    }
}

TEST_CASE("RandVector keeps the values that saved scenes were built with")
{
    // Fails when the hash changes, since every scene using randVector would change with it
    REQUIRE(evalFloatExpression("randVector(7).x") == 0.219825983f);
    REQUIRE(evalFloatExpression("randVector(7).y") == 0.370548546f);
    REQUIRE(evalFloatExpression("randVector(7).z") == 0.597892702f);
    REQUIRE(evalFloatExpression("randVector(1.5).x") == 0.769698501f);
    REQUIRE(evalFloatExpression("randVector(1.5).y") == 0.570456207f);
    REQUIRE(evalFloatExpression("randVector(1.5).z") == 0.0621376634f);
    REQUIRE(evalFloatExpression("randVector(float3(1.0, 2.0, 3.0)).x") == 0.050245285f);
    REQUIRE(evalFloatExpression("randVector(float3(1.0, 2.0, 3.0)).y") == 0.272974133f);
    REQUIRE(evalFloatExpression("randVector(float3(1.0, 2.0, 3.0)).z") == 0.590052545f);
}
