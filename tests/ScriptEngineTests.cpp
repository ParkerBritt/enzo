#include "Engine/Expression/ExpressionContext.h"
#include "Engine/Expression/PointScript.h"
#include "Engine/Expression/ScriptEngine.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeLoader.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Primitives/Mesh.h"
#include <catch2/catch_test_macros.hpp>

using namespace enzo;

struct NMReset
{
    NMReset()
    {
        nt::NodeLoader::loadNodes();
        nt::nm()._reset();
    }
    ~NMReset() { nt::nm()._reset(); }
};

TEST_CASE("compile reuses the compiled script for the same code")
{
    String error;
    auto first = expr::ScriptEngine::instance().compile("@doubled = float(pt * 2l)", error);
    auto second = expr::ScriptEngine::instance().compile("@doubled = float(pt * 2l)", error);
    REQUIRE(first);
    REQUIRE(first == second);
}

TEST_CASE("compile reports code that does not compile")
{
    String error;
    REQUIRE_FALSE(expr::ScriptEngine::instance().compile("let broken = ", error));
    REQUIRE_FALSE(error.empty());
}

TEST_CASE_METHOD(NMReset, "A script run over many points reads a parameter once")
{
    auto& nm = nt::nm();
    nt::NodeId source = nm.createNode("enzo::transform");
    nm.getNode(source).getParameter("translate").lock()->setFloat(7.0f);

    String error;
    auto script = expr::ScriptEngine::instance().compile(R"(@offset = prm("transform1.translate"))", error);
    INFO(error);
    REQUIRE(script);

    geo::Mesh input;
    for (int point = 0; point < 100; ++point) input.addPoint(Vector3::Zero());
    geo::Mesh output = input;

    // Runs the whole point range against one context, as a single cook does
    auto instance = script->clone();
    expr::ExpressionContext context(source);
    REQUIRE(instance->addWrittenAttributes(output, error));
    REQUIRE(instance->run(input, output, 0, input.getNumPoints(), &context, error));

    auto offsets = output.getAttribByName(attr::AttributeOwner::POINT, "offset");
    REQUIRE(attr::AttributeHandleRO<floatT>(offsets).getValue(99) == 7.0f);
    REQUIRE(context.getExpressionDependencies().size() == 1);
    REQUIRE(context.getExpressionDependencies()[0] == nt::Unit{source});
}
